#==========================================
## @file       qemu_start.ps1
## @brief      file for managing qemu startup
#==========================================

param (
    [string]$mode = ""
)

# features
$ENABLE_AHCI        = $true # required
$ENABLE_SERIAL_IO   = $true
$ENABLE_VHD         = $true
$ENABLE_ISO         = $true # required
$ENABLE_NETWORKING  = $true

# system specs
$SYSTEM_MEMORY      = "4G"

# files
$ISO_PATH           = "build/VirtualReflectionsOS.iso"
$DISK_PATH          = "test_disk.vhd"

# base
$ARG_LIST = @(
    "-boot", "d",
    "-machine", "pc",
    "-m", $SYSTEM_MEMORY
)

if ($ENABLE_AHCI) {
    $ARG_LIST += @("-device", "ahci,id=ahci")
}

if ($ENABLE_SERIAL_IO) {
    $ARG_LIST += @("-serial", "stdio")
}

if ($ENABLE_VHD) {
    $ARG_LIST += @(
        "-drive", "format=raw,file=$DISK_PATH,id=disk,if=none",
        "-device", "ide-hd,drive=disk,bus=ahci.0"
    )
}

if ($ENABLE_ISO) {
    $ARG_LIST += @(
        "-drive", "id=cdrom,if=none,media=cdrom,file=$ISO_PATH",
        "-device", "ide-cd,drive=cdrom,bus=ide.0"
    )
}

if ($ENABLE_NETWORKING) {
    $ARG_LIST += @(
        "-netdev", "tap,id=net0,ifname=tap0,script=no,downscript=no",
        "-device", "e1000,netdev=net0",
        "-object", "filter-dump,id=dump0,netdev=net0,file=netdump.pcap"
    )
}

if ($mode -eq "debug") {
    $ARG_LIST += @(
        "-S",
        "-gdb", "tcp::1234",
        "-no-reboot"
    )
}

$label = if ($mode -eq "debug") { "DEBUG" } else { "DEFAULT" }
$timestamp = Get-Date -Format "HH:mm:ss"
Write-Host "[$timestamp] starting qemu"
Write-Host "    mode: $label"
Write-Host "    args: $($ARG_LIST -join ' ')"
Write-Host ""

qemu-system-x86_64.exe @ARG_LIST