# virtual reflections e0
A 64-bit custom made operating system, using grub as bootloader

## TODO
- [ ] fat32
    - [x] read
    - [ ] write

## Setup build environment
### Windows host
```powershell
PS> .\docker_install.ps1
```

## Setup windows TAP network
Make sure to have openVPN for the tap interface.
1. Rename the tap interface to tap0
2. Change its ip to: `10.0.2.1`
3. Change its subnet mask to: `255.255.255.0`

## Building the ISO
### Windows host
```powershell
PS> .\docker_build.ps1
```

## Running QEMU
Start the QEMU environment with the required startup flags.

### Windows host
```powershell
PS> .\qemu_start.ps1
```

## Diagrams
![](docs/svg/kernel%20diagram.svg)
![](docs/svg/storage%20diagram.svg)

## Contributing
This project is not accepting contributions or pull requests.  
It is public for reference and educational purposes only.

## Resources
## General
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