#==========================================
## @file       qemu_start.ps1
## @brief      file for managing qemu startup
#==========================================

param (
    [string]$mode = ""
)

$iso_path = "build/VirtualReflectionsOS.iso"
$disk_path = "test_disk.vhd"

$qemuArgs = @(
    # machine settings
    "-boot", "d",
    "-machine", "pc",
    "-m", "4G",

    # devices
    "-device", "ahci,id=ahci",
    
    # serial (i)o
    "-serial", "stdio",
    
    # virtual hard disk
    "-drive", "format=raw,file=$disk_path,id=disk,if=none",
    "-device", "ide-hd,drive=disk,bus=ahci.0",
    
    # drive with our iso / is our iso
    "-drive", "id=cdrom,if=none,media=cdrom,file=$iso_path",
    "-device", "ide-cd,drive=cdrom,bus=ide.0",
    
    # network card
    "-netdev", "tap,id=net0,ifname=tap0,script=no,downscript=no",
    "-device", "e1000,netdev=net0"
)

if ($mode -eq "debug") {
    Write-Host "==== starting qemu (debug) ===="
    $qemuArgs += "-S"
    $qemuArgs += "-gdb"
    $qemuArgs += "tcp::1234"
    $qemuArgs += "-no-reboot"
} else {
    Write-Host "==== starting qemu ===="
}

qemu-system-x86_64.exe $qemuArgs