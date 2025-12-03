#include "terminal.hpp"
#include "common.hpp"
#include "io.hpp"
#include "std/string.hpp"
#include "system_info.hpp"
#include "virtual_thread.hpp"

#include "gui/desktop.hpp"
#include "time/clock.hpp"
#include "arch/generic.hpp"
#include "filesystems/vfs.hpp"

#include "drivers/pcie.hpp"
#include "drivers/keyboard.hpp"
#include "drivers/driver.hpp"
#include "drivers/ps2/ps2.hpp"
#include "drivers/network/nidm.hpp"
#include "drivers/network/dns.hpp"

#include "subsystem_interface.hpp"
#include "subsystems/dns/interface.hpp"

static std::dynamic_array<char> terminal_current_input {};
bool keep_terminal_alive = true;

void terminal_execute(const std::string& path, const std::dynamic_array<std::string>& args) {
    switch (hash_fnv1a_64(path.c_str())) {
        case hash_fnv1a_64("memdump"): {
            auto heap = get_global_heap();
            for (size_t i = 0; i < heap->heap_block_array_size; i++) {
                heap_block_t* block = &heap->heap_block_array[i];
                if (block->used && block->size > 100 && !block->free) {
                    kprintf("%c %p-%p (%s)\n", block->free ? 'f' : 'u', block->start_real_addr, (void*)((uint64_t)block->start_real_addr + block->size), size_format_to_string(block->size).c_str());
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
            for (auto& device : get_global_nidm()->devices) {
                printf("%s:\n", device->name.c_str());
                printf("    MAC:            %uh:%uh:%uh:%uh:%uh:%uh\n", device->mac[0], device->mac[1], device->mac[2], device->mac[3], device->mac[4], device->mac[5]);
                printf("    IPv4:           %u.%u.%u.%u\n", device->ip.byte3, device->ip.byte2, device->ip.byte1, device->ip.byte0);
                printf("    Gateway:        %u.%u.%u.%u\n", device->gateway.byte3, device->gateway.byte2, device->gateway.byte1, device->gateway.byte0);
                printf("    Subnet mask:    %u.%u.%u.%u\n", device->subnet_mask.byte3, device->subnet_mask.byte2, device->subnet_mask.byte1, device->subnet_mask.byte0);
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
            std::dynamic_array<vfs_node_t*> entries {};
            std::string arg_path = "";
            if (args.length() >= 1)
                arg_path = *args.get_at(0);

            bool result = vfs_list_directory(get_global_vfs(), arg_path, &entries);
            if (!result) {
                printf("Directory not found");
                break;
            }

            for (auto& dir : entries)
                printf("%s\n", dir->meta.name.c_str());

            break;
        }
        case hash_fnv1a_64("cat"): {
            // TODO @since 11/10/2025 -- 01:09
            // check if is directory
            std::dynamic_array<vfs_node_t*> entries {};
            std::string arg_path = "";
            if (args.length() >= 1)
                arg_path = *args.get_at(0);

            file_descriptor_t result = vfs_open_file(get_global_vfs(), arg_path);
            if (result == FILE_DESCRIPTOR_INVALID) {
                printf("File not found");
                break;
            }

            std::dynamic_array<uint8_t> data {};
            if (vfs_read_file(get_global_vfs(), result, &data)) {
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
                    uint64_t capability = driver_query_capability(get_global_driver_manager(), handle, arg1.c_str());
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
                if (!vfs_get_disk_info(get_global_vfs(), arg0.c_str(), &storage_info)) {
                    printf("Disk or drive not found\n");
                    break;
                }
    
                printf("%s:\n", storage_info.model.c_str());
                printf("    Serial: %s\n", storage_info.serial.c_str());
                printf("    Firmware: %s\n", storage_info.firmare.c_str());
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
        case hash_fnv1a_64("gui"): {
            printf("Starting graphical environment ...\n");
            if (vthread_create(desktop_init, get_pml4()) == VTHREAD_HANDLE_INVALID)
                printf("Failed start graphical environment\n");

            // TODO @since 29/10/2025 -- 00:10
            // terminal keyboard unsubscribe
            // else the terminal keeps running during gui ...
            break;
        }
        case hash_fnv1a_64("dns"): {
            if (args.length() >= 1) {
                auto arg0 = *args.get_at(0);
                auto si_dns_client = subsystem_interface_get<subsystem_interface_dns_client_t>(ISUBSYSTEM_DNS_CLIENT);

                if (arg0 == "resolve") {
                    if (args.length() >= 2) {
                        auto arg1 = *args.get_at(1);
                        auto ip = si_dns_client->resolve(arg1.c_str());
                        if (ip != MAX_UINT32 && ip != 0)
                            printf("Hostname: %s\nIP:       %u.%u.%u.%u\n", arg1.c_str(), FROM_IP(ip));
                        else
                            printf("Unable to resolve hostname\n");
                        break;
                    }

                    printf("No host name given\n");
                    break;
                }

                if (arg0 == "server") {
                    printf("IP:       %u.%u.%u.%u\n", FROM_IP(si_dns_client->get_dns_server()));
                    break;
                }
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
    printf("VirtualReflectionsOS Interacive Terminal [v1:%s]\n", GIT_COMMIT_HASH);
    printf("Copyright (C) Blackline Technologies Ltd. System booted succesfully.\n");
    printf("Type 'help' for a list of commands.\n");

    if (!ps2_port_test_device(ps2_device_type_t::KEYBOARD)) {
        printf("\nNo keyboard found, exiting ...\n");
        return 1;
    }

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