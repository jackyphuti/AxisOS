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

# 2. Check for Docker or Podman on Windows
$ContainerTool = $null
if (Get-Command podman -ErrorAction SilentlyContinue) {
    $ContainerTool = "podman"
} elseif (Get-Command docker -ErrorAction SilentlyContinue) {
    $ContainerTool = "docker"
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
    grub-pc-bin grub-efi-amd64-bin mtools dosfstools ca-certificates curl rsync \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /build
CMD ["bash"]
"@
    Set-Content -Path (Join-Path $BuildDir "Containerfile") -Value $ContainerfileContent

    Write-Host "Building builder container image..." -ForegroundColor Yellow
    & $ContainerTool build -t axisos-builder -f (Join-Path $BuildDir "Containerfile") $BuildDir

    Write-Host "[3/3] Executing live-build inside container..." -ForegroundColor Yellow
    & $ContainerTool run --privileged --rm -v "${RootDir}:/workspace" axisos-builder /workspace/os-build/scripts/inner-build.sh
} else {
    Write-Host "[2/3] Docker/Podman not found on Windows host." -ForegroundColor Yellow
    Write-Host "Checking for WSL..." -ForegroundColor Yellow

    if (Get-Command wsl -ErrorAction SilentlyContinue) {
        Write-Host "WSL detected. To build the ISO using WSL or Docker Desktop:" -ForegroundColor Cyan
        Write-Host "  1. Install Docker Desktop or Podman Desktop on Windows." -ForegroundColor White
        Write-Host "  OR" -ForegroundColor White
        Write-Host "  2. Inside WSL: sudo apt-get install -y live-build debootstrap xorriso" -ForegroundColor White
        Write-Host "     cd /mnt/c/Users/.../AxisOS && sudo ./os-build/scripts/inner-build.sh" -ForegroundColor White
    }
}

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host " AxisOS shell is fully built and ready in shell/dist!" -ForegroundColor Green
Write-Host " You can launch local preview with: npm run start" -ForegroundColor Green
Write-Host "==================================================" -ForegroundColor Cyan
