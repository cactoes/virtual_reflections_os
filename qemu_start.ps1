Write-Host "starting qemu"
qemu-system-x86_64.exe `
    -machine pc `
    -cdrom build/VirtualReflectionsOS.iso `
    -m 2G `
    -drive format=raw,file=test_disk.vhd,id=disk,if=none `
    -device ahci,id=ahci `
    -device ide-hd,drive=disk,bus=ahci.0