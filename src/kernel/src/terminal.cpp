#include "terminal.hpp"
#include "common.hpp"
#include "io.hpp"
#include "std/string.hpp"
#include "system_info.hpp"
#include "virtual_thread.hpp"

#include "time/clock.hpp"

#include "filesystems/vfs.hpp"

#include "drivers/pcie.hpp"
#include "drivers/keyboard.hpp"
#include "drivers/driver.hpp"
#include "drivers/ps2/ps2.hpp"
#include "network/nidm.hpp"
#include "storage/storage_manager.hpp"

#include "filesystems/vfs.hpp"
#include "drivers/vga.hpp"
#include "network/network_manager.hpp"
#include "process.hpp"

static std::dynamic_array<char> terminal_current_input {};
bool keep_terminal_alive = true;

extern void io_term_disable();

void terminal_execute(const std::string& path, const std::dynamic_array<std::string>& args) {
    switch (hash_fnv1a_64(path.c_str())) {
        case hash_fnv1a_64("memdump"): {
            auto heap = get_global_heap();
            for (size_t i = 0; i < heap->heap_block_array_size; i++) {
                heap_block_t* block = &heap->heap_block_array[i];
                if (block->used && block->size > 100 && !block->free) {
                    kprintf("%c %p-%p (%s)\n", block->free ? 'f' : 'u', block->start_real_addr, (void*)((u64)block->start_real_addr + block->size), size_format_to_string(block->size).c_str());
                }
            }
            break;
        }
        case hash_fnv1a_64("memstat"): {
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
        case hash_fnv1a_64("netstat"): {
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
        case hash_fnv1a_64("pcistat"): {
            for (auto& device : get_global_pcie_device_manager()->devices) {
                const char* cd = pci_get_class_description(&device);
                printf("[%u:%u.%u] %s:\n", device.bus, device.device, device.function, cd);
                printf("    Vendor ID: 0x%uh, Device ID: 0x%uh\n", device.vendor_device_id.vendor_id, device.vendor_device_id.device_id);
            }
            break;
        }
        case hash_fnv1a_64("ls"): {
            // TODO @since 11/10/2025 -- 01:09
            // check if is file
            std::dynamic_array<vfs_node_t> entries {};
            std::string arg_path = "";
            if (args.length() >= 1)
                arg_path = *args.get_at(0);

            bool result = vfs_list_directory(get_global_vfs(), arg_path.c_str(), &entries);
            if (!result) {
                printf("Directory not found");
                break;
            }

            for (auto& dir : entries)
                printf("%s\n", dir.name.c_str());

            break;
        }
        case hash_fnv1a_64("cat"): {
            // TODO @since 11/10/2025 -- 01:09
            // check if is directory
            std::dynamic_array<vfs_node_t> entries {};
            std::string arg_path = "";
            if (args.length() >= 1)
                arg_path = *args.get_at(0);

            file_descriptor_t result = vfs_open_file(get_global_vfs(), arg_path.c_str());
            if (result == FILE_DESCRIPTOR_INVALID) {
                printf("File not found");
                break;
            }

            u8* buffer;
            size_t size;
            if (vfs_read_file(get_global_vfs(), result, &buffer, &size)) {
                std::dynamic_array<u8> data {};
                data.assign(buffer, size);
                data.insert_back('\n');
                data.insert_back(0);
                printf((char*)data.get_data());
            } else {
                printf("Failed to read file");
            }
            break;
        }
        case hash_fnv1a_64("driverquery"): {
            if (args.length() >= 1) {
                auto arg0 = *args.get_at(0);
                if (arg0 == "list") {
                    printf("List of loaded drivers:\n");
                    for (const auto& driver : get_global_driver_manager()->loaded_drivers)
                        printf("    %s\n", driver.value->name.c_str());
                    
                    break;
                }

                if (args.length() >= 2) {
                    auto arg1 = *args.get_at(1);
                    system_driver_handle_t handle = driver_manager_get_driver_handle(get_global_driver_manager(), arg0.c_str());
                    if (handle == SYSTEM_DRIVER_HANDLE_INVALID) {
                        printf("Invalid driver name\n");
                        break;
                    }

                    printf("%s:\n", arg0.c_str());
                    u64 capability = driver_query_capability(get_global_driver_manager(), handle, arg1.c_str());
                    if (capability == MAX_UINT64) {
                        printf("    Capability: %s not supported", arg1.c_str());
                    } else if (capability == 0) {
                        printf("    Capability: %s not implemented", arg1.c_str());
                    } else {
                        printf("    Capability: %s version %u", arg1.c_str(), capability);
                    }
                }
            }
            break;
        }
        case hash_fnv1a_64("diskstat"): {
            if (args.length() >= 1) {
                auto arg0 = *args.get_at(0);

                vfs_storage_info_t storage_info {};
                if (!vfs_get_storage_info(get_global_vfs(), arg0.c_str(), &storage_info)) {
                    printf("Disk or drive not found\n");
                    break;
                }

                printf("%s:\n", storage_info.model.c_str());
                printf("    Serial: %s\n", storage_info.serial.c_str());
                printf("    Firmware: %s\n", storage_info.firmware.c_str());
                printf("    Disk size: %s\n", size_format_to_string(storage_info.capacity).c_str());
            }
            break;
        }
        case hash_fnv1a_64("systemstat"): {
            system_info_manager_t* sysinfo = get_global_system_info_manager();
            printf("Host:    %s %s %s %s\n", sysinfo->manufacturer.c_str(), sysinfo->product_name.c_str(), sysinfo->version.c_str(), sysinfo->serial_number.c_str());
            printf("CPU:     %s\n", sysinfo->cpu_name.c_str());
            printf("Threads: %ul\n", vthread_get_count());
            printf("Uptime:  %s\n", time_format_to_string(clock_get_time_since_boot()).c_str());
            break;
        }
        case hash_fnv1a_64("dns"): {
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
        case hash_fnv1a_64("start"): {
            if (args.length() < 1) {
                printf("No path given\n");
                break;
            }

            auto arg0 = *args.get_at(0);

            process_t* p = (process_t*)malloc(sizeof(process_t));
            memzero(p, sizeof(process_t));
            if (!create_process(p, arg0.c_str())) {
                printf("Failed to start process\n");
                free(p);
            }
            break;
        }
        case hash_fnv1a_64("help"): {
            printf("help                           Displays this help message\n");
            printf("memstat                        Memory info\n");
            printf("netstat                        Network card info\n");
            printf("pcistat                        PCI(e) info\n");
            printf("ls                             Lists files and directories\n");
            printf("cat                            Display file content\n");
            printf("systemstat                     Display system information\n");
            printf("diskstat                       Displays disk info\n");
            printf("    <path>                     Target disk path\n");
            printf("driverquery                    Query drivers for information\n");
            printf("    list                       List all drivers\n");
            printf("    <name> <feature>           List the capabiliy of a driver feature\n");
            printf("dns                            DNS information\n");
            printf("    server                     Get the dns server that is used\n");
            printf("    resolve <hostname>         Resolves the given hostname\n");
            break;
        }
        case hash_fnv1a_64("crash"): {
            vthread_terminate(2);
            break;
        }
        default:
            printf("Command not found");
            break;
    }
}

void terminal_keydown_callback(virtual_key_t vk) {
    switch (vk) {
        case VK_ENTER: {
            printf("\n");
            terminal_current_input.insert_back(0);

            auto parts = str_split(terminal_current_input.get_data(), ' ');
            std::dynamic_array<std::string> args {};
            if (parts.length() > 0)
                args.resize(parts.length() - 1);
            for (size_t i = 1; i < parts.length(); i++)
                args.insert_back(*parts.get_at(i));

            if (parts.length() == 0)
                terminal_execute("", {});
            else
                terminal_execute(*parts.get_at(0), args);

            terminal_current_input.clear();
            printf("\n> ");
            break;
        }
        case VK_BACKSPACE:
            if (terminal_current_input.length() > 0) {
                printf("%c", '\b');
                terminal_current_input.delete_at(terminal_current_input.length() - 1);
            }
            break;
        case VK_TAB:
            for (size_t i = 0; i < 4; i++) {
                printf(" ");
                terminal_current_input.insert_back(' ');
            }
            break;
        default:
            if (char ch = vk_to_ascii(vk, holding_shift(), holding_caps())) {
                printf("%c", ch);
                terminal_current_input.insert_back(ch);
            }
            break;
    }
}

int terminal_thread_main() {
    printf("\nVirtualReflectionsOS Interactive Terminal [v1:%s]\n", GIT_COMMIT_HASH);
    printf("Copyright (C) Blackline Technologies Ltd. System booted succesfully.\n");
    printf("Type 'help' for a list of commands.\n");

    // if (!ps2_port_test_device(ps2_device_type_t::KEYBOARD)) {
    //     printf("\nNo keyboard found, exiting ...\n");
    //     return 1;
    // }

    printf("\n> ");

    subscribe_on_key_down(terminal_keydown_callback);

    while (keep_terminal_alive) {
        // auto vk = wait_for_key();
        // terminal_keydown_callback(vk);
    }

    kprintf("terminal closed\n");
    printf("\n[terminal exited]\n");
    return 0;
}