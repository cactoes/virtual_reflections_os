#include "kterminal.hpp"
#include "std/string.hpp"
#include "drivers/graphics/graphics_driver.hpp"
#include "virtual_thread.hpp"
#include "io.hpp"
#include "drivers/keyboard.hpp"
#include "process.hpp"
#include "network/nidm.hpp"
#include "network/network_manager.hpp"
#include "ansi.hpp"
#include "system_info.hpp"
#include "time/clock.hpp"

static kterminal_t* global_kterm_session = nullptr;

bool kterm_init(kterminal_t* kterm) {
    if (kterm->is_ready)
        return false;

    graphics_driver_t* gd = get_global_graphics_driver();
    if (!gd)
        return false;

    if (!graphics_driver_get_size(gd, (size_t*)&kterm->width, (size_t*)&kterm->height))
        return false;

    kterm->useable_height = kterm->height - (kterm->height % CHARACTER_HEIGHT);
    kterm->useable_width = kterm->width - (kterm->width % CHARACTER_WIDTH);

    kterm->y_rows = kterm->useable_height / CHARACTER_HEIGHT;
    kterm->x_rows = kterm->useable_width / CHARACTER_WIDTH;

    kterm->is_ready = true;
    kterm->keep_alive = true;

    return true;
}

bool kterm_handle_newline(kterminal_t* kterm) {
    if (!kterm)
        return false;

    graphics_driver_t* gd = get_global_graphics_driver();
    if (!gd)
        return false;

    graphics_driver_draw_square(gd, kterm->cursor.x * CHARACTER_WIDTH, kterm->cursor.y * CHARACTER_HEIGHT, CHARACTER_WIDTH, CHARACTER_HEIGHT, { 0, 0, 0 });

    kterm->cursor.x = 0;

    if (kterm->cursor.y + 1 < kterm->y_rows) {
        kterm->cursor.y++;
        return true;
    }

    graphics_driver_move_square(gd, 0, CHARACTER_HEIGHT, kterm->useable_width, kterm->useable_height - CHARACTER_HEIGHT, 0, 0);
    graphics_driver_draw_square(gd, 0, kterm->useable_height - CHARACTER_HEIGHT, kterm->useable_width, CHARACTER_HEIGHT, { 0, 0, 0 });

    return true;
}

bool kterm_write_char(kterminal_t* kterm, char ch) {
    if (!kterm)
        return false;

    graphics_driver_t* gd = get_global_graphics_driver();
    if (!gd)
        return false;

    if (kterm->cursor.x + 1 >= kterm->x_rows)
        kterm_handle_newline(kterm);

    switch (ch) {
        case '\n':
            kterm_handle_newline(kterm);
            break;
        case '\r':
            kterm->cursor.x = 0;
            break;
        case '\b':
            graphics_driver_draw_character(gd, kterm->cursor.x * CHARACTER_WIDTH, kterm->cursor.y * CHARACTER_HEIGHT, ' ', kterm->fg_color, { 0, 0, 0 });

            if (kterm->cursor.x >= 1)
                kterm->cursor.x--;

            break;
        case '\t':
            for (size_t i = 0; i < 4; i++)
                kterm_write_char(kterm, ' ');
            break;
        default:
            graphics_driver_draw_character(gd, kterm->cursor.x * CHARACTER_WIDTH, kterm->cursor.y * CHARACTER_HEIGHT, ch, kterm->fg_color, { 0, 0, 0 });
            kterm->cursor.x++;
            break;
    }

    graphics_driver_draw_character(gd, kterm->cursor.x * CHARACTER_WIDTH, kterm->cursor.y * CHARACTER_HEIGHT, '_', kterm->fg_color, { 0, 0, 0 });
    return true;
}

bool kterm_write_string_stream(kterminal_t* kterm, const char* buffer, u64 size) {
    for (u64 i = 0; i < size; i++)
        kterm_write_char(kterm, buffer[i]);

    return graphics_driver_render(get_global_graphics_driver());
}

bool kterm_write_ansi_stream(kterminal_t* kterm, const char* buffer, u64 size) {
    bool is_ansi_sequence = false;
    bool is_ansi_color_sequence = false;
    bool is_ansi_color_sequence_fg = true;
    char ansi_num_buf[4] = {0};
    int  ansi_num_len = 0;
    color_t color_map = kterm->fg_color;

    for (u64 i = 0; i < size; i++) {
        char ch = buffer[i];

        if (ch == '\033') {
            is_ansi_sequence = true;
            continue;
        }

        if (ch == '[' && is_ansi_sequence) {
            is_ansi_color_sequence = true;
            ansi_num_len = 0;
            continue;
        }

        if (is_ansi_color_sequence && char_is_num(ch)) {
            if (ansi_num_len < 3)
                ansi_num_buf[ansi_num_len++] = ch;
            continue;
        }

        if (is_ansi_color_sequence && (ch == ';' || ch == 'm')) {
            ansi_num_buf[ansi_num_len] = '\0';
            int code = atoi(ansi_num_buf);
            ansi_num_len = 0;

            if (code == 0) {
                color_map = { 255, 255, 255 };
            } else if (code >= 30 && code <= 37) {
                color_map = ansi_to_rgb((ansi_color_t)(code - 30));
            } else if (code >= 90 && code <= 97) {
                color_map = ansi_to_rgb((ansi_color_t)(code - 90 + 8));
            } else if (code >= 40 && code <= 47) {
                color_map = ansi_to_rgb((ansi_color_t)(code - 40));
            } else if (code >= 100 && code <= 107) {
                color_map = ansi_to_rgb((ansi_color_t)(code - 100 + 8));
            }

            if (ch == 'm') {
                kterm->fg_color = color_map;
                is_ansi_sequence = false;
                is_ansi_color_sequence = false;
                is_ansi_color_sequence_fg = true;
            }

            continue;
        }

        kterm_write_char(kterm, ch);
    }

    return graphics_driver_render(get_global_graphics_driver());
}

bool kterm_write_stream(kterminal_t* kterm, const char* buffer, u64 size) {
    if (!kterm || !kterm->is_ready || !buffer)
        return false;

    if (strff(buffer, '\033') >= 0)
        return kterm_write_ansi_stream(kterm, buffer, size);

    return kterm_write_string_stream(kterm, buffer, size);
}

bool kterm_write_stream(kterminal_t* kterm, const char* str) {
    return kterm_write_stream(kterm, str, strlen(str));
}

#include "arch/amd64/cpu.hpp"
#include "drivers/ps2/keyboard.hpp"
#include "drivers/ps2/ps2.hpp"

bool kterm_wait_for_input_device() {
    printf("waiting for input device...\n");

    i32 retry_counter = 0;

    while (!ps2_port_test_device(ps2_device_type_t::KEYBOARD) && retry_counter < 10) {
        retry_counter++;
        vthread_sleep(10);
    }

    return retry_counter < 10;
}

void kterm_execute_command(const std::string& command, const std::dynamic_array<std::string>& args) {
    if (str_starts_with(command.c_str(), "^")) {
        // fake commands / executables
        switch (hash_fnv1a_64(command.c_str())) {
            case hash_fnv1a_64("^netstat"): {
                for (auto& interface : get_global_nic()->interfaces) {
                    printf("%s:\n", interface->device_name);
                    printf("    MAC:            %uh:%uh:%uh:%uh:%uh:%uh\n", interface->mac[0], interface->mac[1], interface->mac[2], interface->mac[3], interface->mac[4], interface->mac[5]);
                    printf("    IPv4:           %u.%u.%u.%u\n", interface->ip.byte3, interface->ip.byte2, interface->ip.byte1, interface->ip.byte0);
                    printf("    Gateway:        %u.%u.%u.%u\n", interface->gateway.byte3, interface->gateway.byte2, interface->gateway.byte1, interface->gateway.byte0);
                    printf("    Subnet mask:    %u.%u.%u.%u\n", interface->subnet_mask.byte3, interface->subnet_mask.byte2, interface->subnet_mask.byte1, interface->subnet_mask.byte0);
                    printf("    Prefered:       %s\n", interface->is_prefered ? "yes" : "no");
                    printf("    Active:         %s\n", interface->is_active ? "yes" : "no");
                    printf("    Configured:     %s\n", interface->is_configured ? "yes" : "no");
                }
                break;
            }
            case hash_fnv1a_64("^memdump"): {
                auto heap = get_global_heap();
                u64 minsize = 256;
                if (args.length() >= 1) {
                    const auto str = *args.get_at(0);
                    minsize = (u64)atoll(str.c_str());
                }
                for (size_t i = 0; i < heap->heap_block_array_size; i++) {
                    heap_block_t* block = &heap->heap_block_array[i];
                    if (block->used && block->size >= minsize && !block->free) {
                        if (((u32*)block->start_real_addr)[0] == TAG_MAGIC)
                            kprintf("[%s] ", (u32*)block->start_real_addr + 1);
                        else
                            kprintf("[?] ");
                        kprintf("%c %p-%p (%s)\n", block->free ? 'f' : 'u', block->start_real_addr, (void*)((u64)block->start_real_addr + block->size), size_format_to_string(block->size).c_str());
                    }
                }
                break;
            }
            case hash_fnv1a_64("^dns"): {
                if (args.length() >= 1) {
                    auto arg0 = *args.get_at(0);
                    if (arg0 == "resolve") {
                        if (args.length() >= 2) {
                            auto arg1 = *args.get_at(1);
                            auto ip = network_manager_dns_query(get_global_network_manager(), arg1.c_str());
                            if (ip != 0)
                                printf("Hostname: %s\nIP:       %u.%u.%u.%u\n", arg1.c_str(), FROM_IP(ip));
                            else
                                printf("Unable to resolve hostname\n");
                            break;
                        }

                        printf("No host name given\n");
                        break;
                    }

                    if (arg0 == "server") {
                        printf("IP:       %u.%u.%u.%u\n", FROM_IP(DEFAULT_DNS_SERVER));
                        break;
                    }
                }

                break;
            }
            case hash_fnv1a_64("^diskstat"): {
                disk_manager_t* dm = get_global_disk_manager();
                vfs_t* vfs = get_global_vfs();

                for (auto& disk : dm->disks) {
                    const char* model    = disk.interface->get_model    ? disk.interface->get_model(disk.disk_data)    : "unknown";
                    const char* firmware = disk.interface->get_firmware ? disk.interface->get_firmware(disk.disk_data) : "unknown";
                    const char* serial = disk.interface->get_serial ? disk.interface->get_serial(disk.disk_data) : "unknown";
                    u64 capacity = disk.interface->get_capacity(disk.disk_data);

                    printf("%s - %s %s (%s) [%s]\n", disk.name, model, firmware, size_format_to_string(capacity).c_str(), serial);

                    char prefix[40];
                    sprintf(prefix, sizeof(prefix), "%sp", disk.name);
                    size_t disk_name_len = strlen(disk.name);

                    for (auto& mpstr : vfs->mount_points) {
                        const std::string& mount_name = mpstr.key;
                        bool is_partition  = str_starts_with(mount_name.c_str(), prefix);
                        bool is_whole_disk = mount_name == disk.name;

                        if (!is_partition && !is_whole_disk)
                            continue;

                        const vfs_mount_point_t& mp = mpstr.value;
                        const block_device_t* bd = mp.interface->get_block_device(mp.filesystem_data);
                        if (!bd)
                            continue;

                        u64 size = (bd->end_lba - bd->start_lba) * bd->block_size;

                        if (!is_whole_disk)
                            printf("    %s  %s\n", mount_name.c_str() + disk_name_len, size_format_to_string(size).c_str());
                    }
                }
                break;
            }
            case hash_fnv1a_64("^memstat"): {
                auto heap = get_global_heap();
                size_t used_mem = 0;
                for (size_t i = 0; i < heap->heap_block_array_size; i++) {
                    if (!heap->heap_block_array[i].free)
                        used_mem += heap->heap_block_array[i].size;
                }

                for (auto& h : get_global_dma_heap_manager()->heaps) {
                    for (size_t i = 0; i < h.heap_block_array_size; i++) {
                        if (!h.heap_block_array[i].free)
                            used_mem += h.heap_block_array[i].size;
                    }
                }

                size_t available_mem = heap->size + get_global_dma_heap_manager()->size;

                printf("Memory allocated:   %s/%s (%i%)\n", size_format_to_string(used_mem).c_str(), size_format_to_string(available_mem).c_str(), (int)(((double)used_mem / (double)available_mem) * 100));
                printf("Total available:    %s\n", size_format_to_string(get_global_system_info_manager()->memory_size).c_str());
                printf("Total comitted:     %f%\n", (double)(((double)available_mem / (double)get_global_system_info_manager()->memory_size) * 100));
                break;
            }
            case hash_fnv1a_64("^pcistat"): {
                for (auto& device : get_global_pcie_device_manager()->devices) {
                    const char* cd = pci_get_class_description(&device);
                    printf("[%u:%u.%u] %s:\n", device.bus, device.device, device.function, cd);
                    printf("    Vendor ID: 0x%uh, Device ID: 0x%uh\n", device.vendor_device_id.vendor_id, device.vendor_device_id.device_id);
                }
                break;
            }
            case hash_fnv1a_64("^systemstat"): {
                system_info_manager_t* sysinfo = get_global_system_info_manager();
                printf("Host:    %s %s %s %s\n", sysinfo->manufacturer.c_str(), sysinfo->product_name.c_str(), sysinfo->version.c_str(), sysinfo->serial_number.c_str());
                printf("CPU:     %s\n", sysinfo->cpu_name.c_str());
                printf("Threads: %ul\n", vthread_get_count());
                printf("Uptime:  %s\n", time_format_to_string(clock_get_time_since_boot()).c_str());
                break;
            }
            case hash_fnv1a_64("^help"): {
                printf("[all of the listed command need the \"^\" prefix]\n");
                printf("help                           Displays this help message\n");
                printf("memstat                        Memory info\n");
                printf("memstat                        Dump memory object to COM1\n");
                printf("    <size>                     Minimum byte size for object to be shown, default is 256");
                printf("netstat                        Network card info\n");
                printf("pcistat                        PCI(e) info\n");
                printf("systemstat                     Display system information\n");
                printf("diskstat                       Displays disk info\n");
                printf("    <path>                     Target disk path\n");
                printf("dns                            DNS information\n");
                printf("    server                     Get the dns server that is used\n");
                printf("    resolve <hostname>         Resolves the given hostname\n");
                break;
            }
            default:
                printf("Failed to find command, run ^help for a list of commands.\n");
                break;
        }

        return;
    }

    file_descriptor_t fd = vfs_open_file(get_global_vfs(), command.c_str());
    if (fd == FILE_DESCRIPTOR_INVALID) {
        printf("Failed to find executable\n");
        return;
    }
    vfs_close_file(get_global_vfs(), fd);

    process_t* p = (process_t*)malloc(sizeof(process_t));
    memzero(p, sizeof(process_t));
    if (!create_process(p, command.c_str())) {
        printf("Failed to start process\n");
        free(p);
    }
}

void kterm_handle_command(kterminal_t* kterm) {
    if (!kterm)
        return;

    const char* current_input = kterm->current_input.get_data();
    std::dynamic_array<std::string> parts = str_split(std::string(current_input), ' ');
    std::dynamic_array<std::string> args {};

    if (parts.length() == 0)
        return;

    args.resize(parts.length() - 1);
    
    for (u64 i = 1; i < parts.length(); i++)
        args.insert_back(*parts.get_at(i));

    kterm_execute_command(*parts.get_at(0), args);
}

void kterm_keyboard_handler(virtual_key_t vk) {
    kterminal_t* kterm = get_kterm_session();
    if (!kterm)
        return;

    switch (vk) {
        case VK_ENTER: {
            kterm_write_stream(kterm, "\n");
            kterm->current_input.insert_back(0);

            kterm_handle_command(kterm);

            kterm->current_input.clear();
            kterm_write_stream(kterm, "\n> ");

            break;
        }
        case VK_BACKSPACE: {
            if (kterm->current_input.length() > 0) {
                kterm_write_stream(kterm, "\b");
                kterm->current_input.delete_at(kterm->current_input.length() - 1);
            }
            break;
        }
        case VK_TAB: {
            kterm_write_stream(kterm, "    ");
            kterm->current_input.insert_back(' ');
            kterm->current_input.insert_back(' ');
            kterm->current_input.insert_back(' ');
            kterm->current_input.insert_back(' ');
            break;
        }
        default: {
            if (char ch = vk_to_ascii(vk, holding_shift(), holding_caps())) {
                kterm_write_string_stream(kterm, &ch, 1);
                kterm->current_input.insert_back(ch);
            }
            break;
        }
    }
}

int kterm_main_loop() {
    printf("\nVirtualReflectionsOS Interactive Terminal [v1:%s]\n", GIT_COMMIT_HASH);
    printf("Copyright (C) Blackline Technologies Ltd. System booted succesfully.\n");
    printf("Type 'help' for a list of commands.\n");

    if (!kterm_wait_for_input_device()) {
        printf("\nNo keyboard found, exiting ...\n");
        return 1;
    }

    printf("\n> ");

    subscribe_on_key_down(kterm_keyboard_handler);

    if (auto kterm = get_kterm_session()) {
        while (kterm->keep_alive)
            vthread_yield();
    }

    kprintf("~~ terminal closed ~~\n");
    printf("[kterm] closed\n");
    return 0;
}

bool kterm_start(kterminal_t* kterm) {
    if (!kterm || !kterm->is_ready)
        return false;

    return vthread_create(kterm_main_loop, "_ZN7kthread4ktermEv") != VTHREAD_HANDLE_INVALID;
}

void set_kterm_session(kterminal_t* kterm) {
    global_kterm_session = kterm;
}

kterminal_t* get_kterm_session() {
    return global_kterm_session;
}