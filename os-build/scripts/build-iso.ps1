# ==============================================================================
# AxisOS Bootable ISO Builder for Windows / PowerShell
# ==============================================================================
param (
    [switch]$SkipShellBuild
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Split-Path -Parent (Split-Path -Parent $ScriptDir)
$ShellDir = Join-Path $RootDir "shell"

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "       AxisOS Windows ISO Build Orchestrator" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan

# 1. Build Shell
if (-not $SkipShellBuild) {
    Write-Host "[1/3] Building AxisOS Shell bundle..." -ForegroundColor Yellow
    Push-Location $ShellDir
    try {
        npm run build
    } finally {
        Pop-Location
    }
} else {
    Write-Host "[1/3] Skipping Shell build as requested." -ForegroundColor DarkGray
}

# 2. Check for Docker, Podman, or WSL
$ContainerTool = $null
if (Get-Command podman -ErrorAction SilentlyContinue) {
    $ContainerTool = "podman"
} elseif (Get-Command docker -ErrorAction SilentlyContinue) {
    try {
        & docker info >$null 2>&1
        if ($LASTEXITCODE -eq 0) {
            $ContainerTool = "docker"
        }
    } catch {}
}

if ($ContainerTool) {
    Write-Host "[2/3] Using $ContainerTool for Linux ISO generation..." -ForegroundColor Green
    $BuildDir = Join-Path $RootDir "os-build\work"
    New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null

    $ContainerfileContent = @"
FROM debian:bookworm-slim
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    live-build debootstrap squashfs-tools xorriso isolinux syslinux-common syslinux-efi \
    grub-pc-bin grub-efi-amd64-bin mtools dosfstools ca-certificates curl rsync python3 \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /build
CMD ["bash"]
"@
    Set-Content -Path (Join-Path $BuildDir "Containerfile") -Value $ContainerfileContent

    Write-Host "Building builder container image..." -ForegroundColor Yellow
    & $ContainerTool build -t axisos-builder -f (Join-Path $BuildDir "Containerfile") $BuildDir

    Write-Host "[3/3] Executing build inside container..." -ForegroundColor Yellow
    & $ContainerTool run --privileged --rm -v "${RootDir}:/workspace" -e "WORKSPACE_DIR=/workspace" axisos-builder python3 /workspace/os-build/scripts/build-hybrid-uefi-iso.py
} else {
    Write-Host "[2/3] Docker daemon not active or not installed." -ForegroundColor Yellow
    Write-Host "Checking for WSL..." -ForegroundColor Yellow

    if (Get-Command wsl -ErrorAction SilentlyContinue) {
        Write-Host "WSL detected! Executing ISO build inside WSL Ubuntu (root)..." -ForegroundColor Green
        
        $Full = [System.IO.Path]::GetFullPath($RootDir)
        $Drive = $Full.Substring(0, 1).ToLower()
        $Rest = $Full.Substring(2).Replace('\', '/')
        $WslRootDir = "/mnt/$Drive$Rest"

        wsl -d Ubuntu-26.04 -u root bash -c "cd '$WslRootDir' && python3 ./os-build/scripts/build-hybrid-uefi-iso.py"
    } else {
        Write-Host "Neither Docker, Podman, nor WSL found. Please install Docker Desktop or WSL." -ForegroundColor Red
        exit 1
    }
}

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host " AxisOS shell is fully built and ready in shell/dist!" -ForegroundColor Green
Write-Host " You can launch local preview with: npm run start" -ForegroundColor Green
Write-Host "==================================================" -ForegroundColor Cyan
