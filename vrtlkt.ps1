#==========================================
## @file       vrtlkt.ps1
## @brief      toolkit for building & starting the os
## @copyright  Blackline Technologies Ltd.
#==========================================

# startup commands
param (
    [ValidateSet("install", "build", "run", "clean")]
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
    $argument_list = @(
        "-boot", "d",
        "-machine", "pc",
        "-m", "$($config["qemu_system_memory"])"
    )

    if ($config["qemu_enable_ahci"]) {
        $argument_list += @("-device", "ahci,id=ahci")
    }

    if ($config["qemu_enable_serial_io"]) {
        $argument_list += @("-serial", "stdio")
    }

    if ($config["qemu_enable_vhd"]) {
        $argument_list += @(
            "-drive", "format=raw,file=""$($config["qemu_path_vhd"])"",id=disk,if=none",
            "-device", "ide-hd,drive=disk,bus=ahci.0"
        )
    }

    if ($config["qemu_enable_iso"]) {
        $argument_list += @(
            "-drive", "id=cdrom,if=none,media=cdrom,file=""$($config["qemu_path_iso"])""",
            "-device", "ide-cd,drive=cdrom,bus=ide.0"
        )
    }

    if ($config["qemu_enable_networking"]) {
        if ($config["qemu_enable_networking_type"] -eq "tap") {
            $argument_list += @(
                "-netdev", "tap,id=net0,ifname=tap0,script=no,downscript=no"
            )
        }
        
        if ($config["qemu_enable_networking_type"] -eq "nat") {
            $argument_list += @(
                "-netdev", "user,id=net0"
            )
        }

        if ($config["qemu_enable_net_dump"]) {
            $argument_list += @(
                "-object", "filter-dump,id=dump0,netdev=net0,file=""$($config["qemu_network_dump_file_path"])"""
            )
        }

        $argument_list += @(
            "-device", "$($config["qemu_network_card"]),netdev=net0,mac=$($config["qemu_mac"])"
        )
    }

    if ($Debug -or $config["qemu_force_debug"]) {
        $argument_list += @(
            "-S",
            "-gdb", "tcp::1234",
            "-no-reboot"
        )
    }

    $label = if ($Debug -or $config["qemu_force_debug"]) { "DEBUG" } else { "DEFAULT" }
    Write-Host "Mode: $label"
    Write-Host "Args: $($argument_list -join ' ')"
    Write-Host ""

    qemu-system-x86_64.exe @argument_list
}

function Start-CleanBuildEnvironment {
    Remove-Item -Path "build" -Recurse -Force
    New-Item -Path "build" -ItemType Directory | Out-Null
}

function Read-ConfFile {
    $config = @{}

    $path = "vrtlkt.conf"
    if (-not (Test-Path $path)) {
        Write-Error "Config not found."
        exit 1
    }

    Get-Content $path  | ForEach-Object {
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
        Write-Host "Building project ..."
        Start-DockerEnvironment
    }

    "run" {
        Write-Host "Starting QEMU ..."
        Start-QEMU
    }

    "clean" {
        Write-Host "Cleaning build environment ..."
        Start-CleanBuildEnvironment
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

exit 0