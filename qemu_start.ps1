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
    # kernel
    "-cdrom", $iso_path,
    
    # machine settings
    "-machine", "pc",
    "-m", "4G",
    
    # serial i(o)
    "-serial", "stdio",
    
    # virtual hard disk
    "-drive", "format=raw,file=$disk_path,id=disk,if=none",
    
    # ahci adapter
    "-device", "ahci,id=ahci",
    "-device", "ide-hd,drive=disk,bus=ahci.0",
    
    # network card
    "-netdev", "user,id=net0",
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