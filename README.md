# virtual reflections e0
A 64-bit custom made operating system, using grub as bootloader

## TODO
- [ ] network driver (intel e1000)
- [ ] shell
- [ ] buffers / streams (lockable / mutex)
- [ ] filesystem
    - [x] ISO9660
        - [x] read
    - [ ] fat32
        - [x] read
        - [ ] write

## Goal
Bootable desktop that can play a minesweeper game

## Setup build environment
### Windows host
```powershell
PS> docker build . -t virtual_reflections_os_buildenv
```

### Usage
When inside the build env. run the `build.sh` script to automatically `make clean && make build`.
```bash
$ ./build.sh
```

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

## Diagram
![](docs/svg/kernel%20diagram.svg)

## Resources
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

+-----------------------------------------+
| harware layer (cpu) -> x86_64, ...      |
| harware layer (other) -> SATA, VGA, ... |
+-----------------------------------------+
| kernel layer -> mm, interupts, ...      |
+-----------------------------------------+
| user layer -> webbrowser, ...           |
+-----------------------------------------+

[ what exe file is this ]
[ known start of program ]
[ syscalls ]
[ sections: .code, .text, ... ]