#==========================================
## @file       vrtlkt.ps1
## @brief      toolkit for building & starting the os
## @copyright  Blackline Technologies Ltd.
#==========================================

# startup commands
param (
    [string]$Tool,
    [switch]$Debug,
    [switch]$NoWelcomeMessage
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
    $ENABLE_AHCI        = $config["qemu_enable_ahci"]
    $ENABLE_SERIAL_IO   = $config["qemu_enable_serial_io"]
    $ENABLE_VHD         = $config["qemu_enable_vhd"]
    $ENABLE_ISO         = $config["qemu_enable_iso"]
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
        "-m", "$($config["qemu_system_memory"])"
    )

    if ($config["qemu_enable_ahci"]) {
        $ARG_LIST += @("-device", "ahci,id=ahci")
    }

    if ($config["qemu_enable_serial_io"]) {
        $ARG_LIST += @("-serial", "stdio")
    }

    if ($config["qemu_enable_vhd"]) {
        $ARG_LIST += @(
            "-drive", "format=raw,file=`"$($config["qemu_path_vhd"])`",id=disk,if=none",
            "-device", "ide-hd,drive=disk,bus=ahci.0"
        )
    }

    if ($config["qemu_enable_iso"]) {
        $ARG_LIST += @(
            "-drive", "id=cdrom,if=none,media=cdrom,file=`"$($config["qemu_path_iso"])`"",
            "-device", "ide-cd,drive=cdrom,bus=ide.0"
        )
    }

    if ($config["qemu_enable_networking"]) {
        if ($config["qemu_enable_networking_type"] -eq "tap0") {
            $ARG_LIST += @(
                "-netdev", "tap,id=net0,ifname=tap0,script=no,downscript=no"
            )
        }
        
        if ($config["qemu_enable_networking_type"] -eq "nat") {
            $ARG_LIST += @(
                "-netdev", "user,id=net0"
            )
        }

        if ($config["qemu_enable_net_dump"]) {
            $ARG_LIST += @(
                "-object", "filter-dump,id=dump0,netdev=net0,file=`"$($config["qemu_network_dump_file_path"])`""
            )
        }

        $ARG_LIST += @(
            "-device", "$($config["qemu_network_card"]),netdev=net0,mac=$($config["qemu_mac"])"
        )
    }

    if ($Debug -or $config["qemu_force_debug"]) {
        $ARG_LIST += @(
            "-S",
            "-gdb", "tcp::1234",
            "-no-reboot"
        )
    }

    $label = if ($Debug -or $config["qemu_force_debug"]) { "DEBUG" } else { "DEFAULT" }
    Write-Host "Mode: $label"
    Write-Host "Args: $($ARG_LIST -join ' ')"
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
# $timestamp = Get-Date -Format "HH:mm:ss"

# welcome message
if (-not $NoWelcomeMessage) {
    Write-Host "VirtualReflectionsOS Toolkit [v1:$git_commit_hash]"
    Write-Host "Copyright (C) Blackline Technologies Ltd."
    Write-Host ""
}

switch ($Tool) {
    "install" {
        Write-Host "Installing docker build environment ..."
        Install-DockerEnvironment
    }

    "build" {
        Write-Host "Building kernel ..."
        Start-DockerEnvironment
    }

    "run" {
        Write-Host "Starting QEMU ..."
        Start-QEMU
    }

    default {
        Write-Host "Invalid tool."
        exit -1
    }
}

# program exit
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}