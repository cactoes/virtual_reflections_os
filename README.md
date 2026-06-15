# Virtual Reflections OS
A 64-bit custom-made operating system, using GRUB as bootloader, Which can play Minesweeper in usermode!

## VirtualReflectionsOS Toolkit
This is a simple helper powershell script for managing the build environment.

### Managing the build environment
```powershell
PS> .\vrtlkt.ps1 -Tool install
PS> .\vrtlkt.ps1 -Tool build
PS> .\vrtlkt.ps1 -Tool clean
```

### Starting the kernel
Adding the `-Debug` flag make qemu be able to connect to gdb on port `1234`
```powershell
PS> .\vrtlkt.ps1 -Tool run
```

## Diagrams for a quick overview
![](docs/svg/kernel%20diagram.svg)
![](docs/svg/storage%20diagram.svg)

## Current active tasks
- processes: process management
- window manager: desktop environment

## TODO
- [ ] redo subsystem manager
- [ ] (generic?) device drivers
- [ ] reworking vfs -> [storage_controller.md](storage_controller.md)
- [ ] rework keyboard controller -> move to io etc..
- [ ] drive / disk manager
    - [ ] interfaceable
    - [x] list of all disks
- [ ] work on vthreads
- [ ] general cleanup / restructuring
    - [ ] interrupt manager
- [ ] gui
- [ ] rtl8169 driver (maybe rtl8168)
    - [ ] fix DHCP packets not receiving
- [ ] (simple) usb drivers
    - [ ] keyboard
    - [ ] usb storage device

## Mapping driver symbols in debug mode
```
-exec add-symbol-file ./build/{driver} {address}
```

## Resources
### General
[OSDev - https://wiki.osdev.org/Expanded_Main_Page](https://wiki.osdev.org/Expanded_Main_Page) <br>
[stackoverflow - https://stackoverflow.com](https://stackoverflow.com) <br>
[DHCP Reference - https://datatracker.ietf.org/doc/html/rfc2131](https://datatracker.ietf.org/doc/html/rfc2131) <br>

### YouTube
[Write Your Own 64-bit Operating System Kernel - https://www.youtube.com/watch?v=wz9CZBeXR6U](https://www.youtube.com/watch?v=wz9CZBeXR6U) <br>

### GitHub
[MonkOS - https://github.com/beevik/MonkOS](https://github.com/beevik/MonkOS) <br>
[os64 - https://github.com/luke8086/os64](https://github.com/luke8086/os64) <br>
[wyoos - https://github.com/AlgorithMan-de/wyoos](https://github.com/AlgorithMan-de/wyoos) <br>
[osakaOS - https://github.com/pac-ac/osakaOS](https://github.com/pac-ac/osakaOS) <br>
[uefi-os - https://github.com/sansoune/uefi-os](https://github.com/sansoune/uefi-os)<br>
[MmdOS - https://github.com/Rostamborn/MmdOS](https://github.com/Rostamborn/MmdOS)<br>
[cavOS - https://github.com/malwarepad/cavOS](https://github.com/malwarepad/cavOS)<br>
[RedactedOS - https://github.com/differrari/RedactedOS](https://github.com/differrari/RedactedOS)<br>
[KeblaOS - https://github.com/baponkar/KeblaOS/tree/main](https://github.com/baponkar/KeblaOS/tree/main)<br>

## Contributing
This project is not accepting contributions or pull requests.  
It is public for reference and educational purposes only.
