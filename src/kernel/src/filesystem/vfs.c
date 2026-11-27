#include "filesystem/vfs.h"
#include "memory/generic.h"
#include "libstring.h"
#include "libmem.h"

static vfs_dentry_t* global_root;

static
vfs_dentry_t* follow_mounts(vfs_dentry_t* dentry) {
    while (dentry->mounted)
        dentry = dentry->mounted->root;

    return dentry;
}

static
vfs_dentry_t* dentry_lookup(vfs_dentry_t* dir, const char* name) {
    for (vfs_dentry_t* child = dir->children; child; child = child->next_sibling) {
        if (streq(child->name, name))
            return child;
    }

    const vfs_inode_vtable_t* ops = dir->inode->ops;
    if (!ops || !ops->find)
        return nullptr;

    vfs_inode_t* inode = ops->find(dir->inode, name);
    if (!inode)
        return nullptr;

    return vfs_dentry_create(dir, name, inode);
}

vfs_dentry_t* vfs_resolve(const char* path) {
    if (!global_root || !path || path[0] != '/')
        return nullptr;

    vfs_dentry_t* current = follow_mounts(global_root);
    const char* p = path;
    char name[VFS_MAX_NAME];

    while (*p) {
        while (*p == '/')
            p++;

        if (!*p)
            break;

        u64 length = 0;
        while (p[length] && p[length] != '/')
            length++;

        if (length >= VFS_MAX_NAME)
            return nullptr;

        memcpy(name, p, length);
        name[length] = '\0';
        p += length;

        if (streq(name, "."))
            continue;

        if (streq(name, "..")) {
            while (current == current->inode->sb->root && current->inode->sb->mountpoint)
                current = current->inode->sb->mountpoint;

            if (current->parent)
                current = current->parent;

            continue;
        }

        if (current->inode->type != VFS_TYPE_DIR)
            return nullptr;

        current = dentry_lookup(current, name);
        if (!current)
            return nullptr;

        current = follow_mounts(current);
    }

    return current;
}

bool vfs_mount_root(vfs_superblock_t* sb) {
    if (global_root)
        return false;

    sb->mountpoint = nullptr;
    global_root = sb->root;
    return true;
}

vfs_dentry_t* vfs_dentry_create(vfs_dentry_t* parent, const char* name, vfs_inode_t* inode) {
    if (!name || !inode)
        return nullptr;

    const u64 length = strlen(name);
    if (length >= VFS_MAX_NAME)
        return nullptr;

    vfs_dentry_t* dentry = kalloc(sizeof(vfs_dentry_t));
    if (!dentry)
        return nullptr;

    memzero(dentry, sizeof(vfs_dentry_t));
    memcpy(dentry->name, name, length + 1);
    dentry->parent = parent;
    dentry->inode = inode;

    if (parent) {
        dentry->next_sibling = parent->children;
        parent->children = dentry;
    }

    return dentry;
}

bool vfs_mount(const char* path, vfs_superblock_t* sb) {
    if (!sb || !sb->root)
        return false;

    vfs_dentry_t* mountpoint = vfs_resolve(path);
    if (!mountpoint)
        return false;

    if (mountpoint->inode->type != VFS_TYPE_DIR)
        return false;

    if (mountpoint->mounted)
        return false;

    mountpoint->mounted = sb;
    sb->mountpoint = mountpoint;
    return true;
}


bool vfs_open(const char* path, u32 flags, vfs_file_t** out) {
    if (!out)
        return false;

    vfs_dentry_t* dentry = vfs_resolve(path);
    if (!dentry)
        return false;

    vfs_file_t* file = kalloc(sizeof(vfs_file_t));
    if (!file)
        return false;

    memzero(file, sizeof(vfs_file_t));
    file->dentry = dentry;
    file->flags = flags;
    file->offset = 0;

    *out = file;
    return true;
}

u64 vfs_read(vfs_file_t* file, void* buffer, u64 size) {
    if (!file || (!buffer && size != 0))
        return 0;

    if (!(file->flags & VFS_O_READ))
        return 0;

    vfs_inode_t* inode = file->dentry->inode;
    if (inode->type == VFS_TYPE_DIR)
        return 0;

    if (!inode->ops || !inode->ops->read)
        return 0;

    const u64 count = inode->ops->read(inode, file->offset, buffer, size);
    if (count > 0)
        file->offset += (u64)count;

    return count;
}

u64 vfs_write(vfs_file_t* file, const void* buffer, u64 size) {
    if (!file || (!buffer && size != 0))
        return 0;

    if (!(file->flags & VFS_O_WRITE))
        return 0;

    vfs_inode_t* inode = file->dentry->inode;
    if (inode->type == VFS_TYPE_DIR)
        return 0;

    if (!inode->ops || !inode->ops->write)
        return 0;

    const u64 count = inode->ops->write(inode, file->offset, buffer, size);
    if (count > 0)
        file->offset += (u64)count;

    return count;
}

void vfs_close(vfs_file_t* file) {
    if (!file)
        return;

    kfree(file);
}