#include "filesystem/initramfs.h"
#include "libstring.h"
#include "libmem.h"
#include "memory/generic.h"

static
initramfs_node_t* find_child(initramfs_node_t* dir, const char* name) {
    for (initramfs_node_t* n = dir->children; n; n = n->next_sibling) {
        if (streq(n->name, name))
            return n;
    }
    return nullptr;
}

static
vfs_inode_t* initramfs_find(vfs_inode_t* dir, const char* name) {
    if (!dir || dir->type != VFS_TYPE_DIR || !dir->private_data || !name)
        return nullptr;

    initramfs_node_t* child = find_child(dir->private_data, name);
    return child ? &child->inode : nullptr;
}

static
u64 initramfs_read(vfs_inode_t* inode, u64 offset, void* buffer, u64 size) {
    if (!inode || !inode->private_data || (!buffer && size != 0))
        return 0;

    if (inode->type == VFS_TYPE_DIR)
        return 0;

    if (inode->type != VFS_TYPE_FILE)
        return 0;

    if (size == 0 || offset >= inode->size)
        return 0;

    initramfs_node_t* node = inode->private_data;
    u64 count = inode->size - offset;
    if (count > size)
        count = size;

    memcpy(buffer, node->data + offset, count);
    return count;
}

static
bool initramfs_readdir(vfs_inode_t* dir, u64 index, vfs_dirent_t* out) {
    if (!dir || !dir->private_data || !out)
        return false;

    if (dir->type != VFS_TYPE_DIR)
        return false;

    initramfs_node_t* node = dir->private_data;
    initramfs_node_t* child = node->children;
    while (child && index != 0) {
        child = child->next_sibling;
        index--;
    }

    if (!child)
        return false;

    memzero(out, sizeof(vfs_dirent_t));

    u64 length = 0;
    while (child->name[length])
        length++;

    memcpy(out->name, child->name, length + 1);
    out->type = child->inode.type;
    out->ino = child->inode.ino;
    return true;
}

static const vfs_inode_vtable_t initramfs_vtable = {
    .find = initramfs_find,
    .read = initramfs_read,
    .write = nullptr,
    .readdir = initramfs_readdir
};

static
initramfs_node_t* create_node(vfs_superblock_t* sb, initramfs_node_t* parent, const char* name, vfs_type_t type) {
    initramfs_state_t* state = sb->private_data;
    initramfs_node_t* node = kalloc(sizeof(initramfs_node_t));
    if (!node)
        return nullptr;

    memzero(node, sizeof(initramfs_node_t));

    u64 length = 0;
    while (name[length])
        length++;

    memcpy(node->name, name, length + 1);

    node->inode.ino = state->next_ino++;
    node->inode.type = type;
    node->inode.sb = sb;
    node->inode.ops = &initramfs_vtable;
    node->inode.private_data = node;
    node->mode = type == VFS_TYPE_DIR ? (CPIO_MODE_DIR | 0755u) : (CPIO_MODE_FILE | 0644u);
    node->next_allocated = state->allocated;
    state->allocated = node;

    if (parent) {
        if (parent->last_child)
            parent->last_child->next_sibling = node;
        else
            parent->children = node;
        parent->last_child = node;
    }

    return node;
}

static
void discard_creation(vfs_superblock_t* sb) {
    initramfs_state_t* state = sb->private_data;
    if (state) {
        initramfs_node_t* node = state->allocated;
        while (node) {
            initramfs_node_t* next = node->next_allocated;
            kfree(node);
            node = next;
        }
        kfree(state);
    }
    kfree(sb);
}

static
bool parse_hex8(const u8* text, u32* out) {
    u32 value = 0;
    for (u32 i = 0; i < 8; i++) {
        const u8 c = text[i];
        u32 digit;
        if (c >= '0' && c <= '9')
            digit = c - '0';
        else if (c >= 'a' && c <= 'f')
            digit = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            digit = c - 'A' + 10;
        else
            return false;
        value = (value << 4) | digit;
    }

    *out = value;
    return true;
}

static
bool align_cursor(u64* cursor, u64 archive_size) {
    if (*cursor > archive_size)
        return false;

    const u64 padding = (4u - (*cursor & 3u)) & 3u;
    if (padding > archive_size - *cursor)
        return false;

    *cursor += padding;
    return true;
}

static
bool normalize_path(const char* name, u64 length, char* out) {
    if (length == 0 || name[0] == '/')
        return false;

    u64 input = 0;
    u64 output = 0;
    while (input < length) {
        while (input < length && name[input] == '/')
            input++;

        const u64 start = input;
        while (input < length && name[input] != '/')
            input++;

        const u64 count = input - start;
        if (count == 0 || (count == 1 && name[start] == '.'))
            continue;

        if (count == 2 && name[start] == '.' && name[start + 1] == '.')
            return false;

        if (count >= VFS_MAX_NAME)
            return false;

        if (output)
            out[output++] = '/';

        memcpy(out + output, name + start, count);
        output += count;
    }

    out[output] = '\0';
    return true;
}

static
bool insert_entry(vfs_superblock_t* sb, const char* name, u64 name_length, u32 mode, const u8* data, u32 size) {
    char path[CPIO_PATH_MAX + 1];
    if (!normalize_path(name, name_length, path))
        return false;

    const vfs_type_t type = (mode & CPIO_MODE_TYPE) == CPIO_MODE_DIR ? VFS_TYPE_DIR : VFS_TYPE_FILE;
    if (type == VFS_TYPE_FILE && name[name_length - 1] == '/')
        return false;

    initramfs_state_t* state = sb->private_data;
    initramfs_node_t* current = state->root;
    if (!path[0]) {
        if (type != VFS_TYPE_DIR || current->explicit_entry)
            return false;

        current->explicit_entry = true;
        current->mode = mode;
        return true;
    }

    char* component = path;
    while (*component) {
        char* end = component;
        while (*end && *end != '/')
            end++;

        const bool last = *end == '\0';
        *end = '\0';

        initramfs_node_t* child = find_child(current, component);
        if (!last) {
            if (child && child->inode.type != VFS_TYPE_DIR)
                return false;

            if (!child)
                child = create_node(sb, current, component, VFS_TYPE_DIR);

            if (!child)
                return false;

            current = child;
            component = end + 1;
            continue;
        }

        if (child) {
            if (type != VFS_TYPE_DIR || child->inode.type != VFS_TYPE_DIR || child->explicit_entry)
                return false;
        } else {
            child = create_node(sb, current, component, type);
            if (!child)
                return false;
        }

        child->explicit_entry = true;
        child->mode = mode;
        child->inode.size = size;
        child->data = type == VFS_TYPE_FILE ? data : nullptr;
        return true;
    }

    return false;
}

static
bool parse_archive(vfs_superblock_t* sb) {
    initramfs_state_t* state = sb->private_data;
    const u8* archive = state->archive;
    const u64 archive_size = state->archive_size;
    u64 cursor = 0;

    for (;;) {
        if (cursor > archive_size || archive_size - cursor < CPIO_HEADER_SIZE)
            return false;

        const u8* header = archive + cursor;
        if (memeq(header, "070702", 6))
            return false;

        if (!memeq(header, "070701", 6))
            return false;

        u32 fields[13];
        for (u32 i = 0; i < 13; i++) {
            if (!parse_hex8(header + 6 + i * 8, &fields[i]))
                return false;
        }

        const u32 mode = fields[1];
        const u32 nlink = fields[4];
        const u32 size = fields[6];
        const u32 name_size = fields[11];
        if (fields[12] != 0)
            return false;

        cursor += CPIO_HEADER_SIZE;
        if (name_size == 0 || name_size > CPIO_PATH_MAX + 1 ||
            name_size > archive_size - cursor)
            return false;

        const char* name = (const char*)(archive + cursor);
        const u64 name_length = (u64)name_size - 1;
        if (name[name_length] != '\0')
            return false;

        for (u64 i = 0; i < name_length; i++) {
            if (name[i] == '\0')
                return false;
        }

        cursor += name_size;
        if (!align_cursor(&cursor, archive_size) || size > archive_size - cursor)
            return false;

        const u8* data = archive + cursor;
        cursor += size;
        if (!align_cursor(&cursor, archive_size))
            return false;

        if (name_length == 10 && memeq(name, "TRAILER!!!", 10)) {
            if (size != 0)
                return false;

            while (cursor < archive_size) {
                if (archive[cursor++] != 0)
                    return false;
            }

            return true;
        }

        const u32 type = mode & CPIO_MODE_TYPE;
        if (type != CPIO_MODE_DIR && type != CPIO_MODE_FILE)
            return false;

        if (nlink == 0 || (type == CPIO_MODE_DIR && size != 0))
            return false;

        if (type == CPIO_MODE_FILE && nlink != 1)
            return false;

        if (!insert_entry(sb, name, name_length, mode, data, size))
            return false;
    }
}

bool initramfs_create(const void* archive, u64 archive_size, vfs_superblock_t** out) {
    *out = nullptr;
    if (archive_size < CPIO_HEADER_SIZE)
        return false;

    vfs_superblock_t* sb = kalloc(sizeof(*sb));
    if (!sb)
        return false;

    memzero(sb, sizeof(*sb));

    initramfs_state_t* state = kalloc(sizeof(*state));
    if (!state) {
        kfree(sb);
        return false;
    }

    memzero(state, sizeof(*state));
    sb->private_data = state;
    state->archive = archive;
    state->archive_size = archive_size;
    state->next_ino = 1;
    state->root = create_node(sb, nullptr, "/", VFS_TYPE_DIR);
    if (!state->root) {
        discard_creation(sb);
        return false;
    }

    if (!parse_archive(sb)) {
        discard_creation(sb);
        return false;
    }

    sb->root = vfs_dentry_create(nullptr, "/", &state->root->inode);
    if (!sb->root) {
        discard_creation(sb);
        return false;
    }

    *out = sb;
    return true;
}