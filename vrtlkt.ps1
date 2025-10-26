#==========================================
## @file       vrtlkt.ps1
## @brief      toolkit for building & starting the os
## @copyright  Blackline Technologies Ltd.
#==========================================

# startup commands
param (
    [string]$Tool,
    [switch]$Debug
)

# functions
function Install-DockerEnvironment {
    docker build . -t virtual_reflections_os_buildenv
}

function Start-DockerEnvironment {
    docker run --name VirtualReflectionsOS --rm -v "${PWD}:/root/env" -e GIT_COMMIT_HASH=$git_commit_hash virtual_reflections_os_buildenv
}

function Start-QEMU {
    # features
    $ENABLE_AHCI        = $config["qemu_enable_ahci"]       # required
    $ENABLE_SERIAL_IO   = $config["qemu_enable_serial_io"]
    $ENABLE_VHD         = $config["qemu_enable_vhd"]
    $ENABLE_ISO         = $config["qemu_enable_iso"]        # required
    $ENABLE_NETWORKING  = $config["qemu_enable_networking"]
    $NETWORKING_TYPE    = $config["qemu_enable_networking_type"]
    $NETWORK_CARD       = $config["qemu_network_card"]
    $ENABLE_DEBUG       = $config["qemu_force_debug"]
    $ENABLE_NET_DUMP    = $config["qemu_enable_net_dump"]

    # system specs
    $SYSTEM_MEMORY      = $config["qemu_system_memory"]

    # files
    $ISO_PATH           = $config["qemu_path_iso"]
    $DISK_PATH          = $config["qemu_path_vhd"]

    $MAC                = $config["qemu_mac"]
    $NETWORK_DUMP_PATH  = $config["qemu_network_dump_file_path"]

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
            "-drive", "format=raw,file=`"$DISK_PATH`",id=disk,if=none",
            "-device", "ide-hd,drive=disk,bus=ahci.0"
        )
    }

    if ($ENABLE_ISO) {
        $ARG_LIST += @(
            "-drive", "id=cdrom,if=none,media=cdrom,file=`"$ISO_PATH`"",
            "-device", "ide-cd,drive=cdrom,bus=ide.0"
        )
    }

    if ($ENABLE_NETWORKING) {
        if ($NETWORKING_TYPE -eq "tap0") {
            $ARG_LIST += @(
                "-netdev", "tap,id=net0,ifname=tap0,script=no,downscript=no"
            )
        }
        
        if ($NETWORKING_TYPE -eq "nat") {
            $ARG_LIST += @(
                "-netdev", "user,id=net0"
            )
        }

        if ($ENABLE_NET_DUMP) {
            $ARG_LIST += @(
                "-object", "filter-dump,id=dump0,netdev=net0,file=`"$NETWORK_DUMP_PATH`""
            )
        }

        $ARG_LIST += @(
            "-device", "$NETWORK_CARD,netdev=net0,mac=$MAC"
        )
    }

    if ($Debug -or $ENABLE_DEBUG) {
        $ARG_LIST += @(
            "-S",
            "-gdb", "tcp::1234",
            "-no-reboot"
        )
    }

    $label = if ($Debug -or $ENABLE_DEBUG) { "DEBUG" } else { "DEFAULT" }
    $timestamp = Get-Date -Format "HH:mm:ss"
    Write-Host "[$timestamp] Starting QEMU ..."
    Write-Host "    Mode: $label"
    Write-Host "    Args: $($ARG_LIST -join ' ')"
    Write-Host ""

    qemu-system-x86_64.exe @ARG_LIST
}

function Read-ConfFile {
    $config = @{}

    Get-Content "vrtlkt.conf" | ForEach-Object {
        $line = $_.Trim()

        if ([string]::IsNullOrWhiteSpace($line)) { return }

        if ($line -match '^[#;]') { return }

        if ($line -match '^(.*?)\s*=\s*(.*)$') {
            $key = $matches[1].Trim()
            $value = $matches[2].Trim() -replace '^"(.*)"$', '$1'

            if ($value -match '^(true|false)$') {
                $value = [bool]::Parse($value)
            } elseif ($value -match '^\d+$') {
                $value = [int]$value
            }

            $config[$key] = $value
        }
    }

    return $config
}

# global variables
$git_commit_hash = git rev-parse --short HEAD
$config = Read-ConfFile

# welcome message
Write-Host "VirtualReflectionsOS Toolkit [v1:$git_commit_hash]"
Write-Host "Copyright (C) Blackline Technologies Ltd."
Write-Host ""

switch ($Tool) {
    "install" { Install-DockerEnvironment }
    "build"   { Start-DockerEnvironment }
    "run"     { Start-QEMU }
    default   { Write-Host "Invalid tool." }
}

# program exit
Write-Host ""
if ($LASTEXITCODE -ne 0) {
    Write-Host "An error occured in tool .."
    exit $LASTEXITCODE
} else {
    Write-Host "Tool finished successfully"
}