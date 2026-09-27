# ==============================================================================
# AxisOS Virtual Machine Launcher for Windows (QEMU / KVM)
# ==============================================================================
param (
    [string]$IsoPath = "",
    [string]$DiskPath = "",
    [int]$RamMB = 4096,
    [int]$Cores = 4,
    [switch]$BuildIfMissing
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RootDir = Split-Path -Parent (Split-Path -Parent $ScriptDir)

if (-not $IsoPath) {
    $DefaultIso = Join-Path $RootDir "axisos-live-amd64.iso"
    if (Test-Path $DefaultIso) {
        $IsoPath = $DefaultIso
    } else {
        $OsBuildIso = Join-Path $RootDir "os-build\axisos-live-amd64.iso"
        if (Test-Path $OsBuildIso) {
            $IsoPath = $OsBuildIso
        } else {
            $IsoPath = $DefaultIso
        }
    }
}

if (-not $DiskPath) {
    $DiskPath = Join-Path $RootDir "axisos-disk.qcow2"
}

Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "         AxisOS QEMU Virtual Machine" -ForegroundColor Cyan
Write-Host "==================================================" -ForegroundColor Cyan
Write-Host "  RAM:     $RamMB MB" -ForegroundColor White
Write-Host "  Cores:   $Cores CPUs" -ForegroundColor White
Write-Host "  ISO:     $IsoPath" -ForegroundColor White
Write-Host "  Disk:    $DiskPath" -ForegroundColor White

# Convert Windows paths to WSL paths
function ConvertTo-WslPath([string]$winPath) {
    $full = [System.IO.Path]::GetFullPath($winPath)
    $drive = $full.Substring(0, 1).ToLower()
    $rest = $full.Substring(2).Replace('\', '/')
    return "/mnt/$drive$rest"
}

$WslIsoPath = ConvertTo-WslPath $IsoPath
$WslDiskPath = ConvertTo-WslPath $DiskPath
$WslRootDir = ConvertTo-WslPath $RootDir

# Check if ISO exists
if (-not (Test-Path $IsoPath)) {
    Write-Host ""
    Write-Host "[!] Live ISO not found at: $IsoPath" -ForegroundColor Yellow
    Write-Host "    To generate the bootable ISO, run:" -ForegroundColor Yellow
    Write-Host "    wsl -d Ubuntu-26.04 -u root bash -c 'cd $WslRootDir && ./os-build/scripts/inner-build.sh'" -ForegroundColor Cyan
    Write-Host ""
    if (-not $BuildIfMissing) {
        $reply = Read-Host "Would you like to build the ISO right now? (Y/n)"
        if ($reply -eq "" -or $reply -match "^[yY]") {
            $BuildIfMissing = $true
        } else {
            Write-Host "Exiting. Run with an existing ISO or build it first." -ForegroundColor Red
            exit 1
        }
    }

    if ($BuildIfMissing) {
        Write-Host "Starting AxisOS Live ISO build inside WSL (Ubuntu)..." -ForegroundColor Green
        wsl -d Ubuntu-26.04 -u root bash -c "cd $WslRootDir && ./os-build/scripts/inner-build.sh"
        if (-not (Test-Path $IsoPath)) {
            Write-Host "ISO build completed, but $IsoPath not found." -ForegroundColor Red
            exit 1
        }
    }
}

# Ensure KVM permissions and virtual disk exist
wsl -d Ubuntu-26.04 -u root bash -c "chmod 666 /dev/kvm 2>/dev/null || true"
wsl -d Ubuntu-26.04 bash -c "if [ ! -f '$WslDiskPath' ]; then qemu-img create -f qcow2 '$WslDiskPath' 20G; fi"

Write-Host ""
Write-Host "Launching QEMU Virtual Machine..." -ForegroundColor Green
Write-Host "A GUI window will appear shortly." -ForegroundColor Cyan
Write-Host ""

# Check KVM availability
$HasKvm = (wsl -d Ubuntu-26.04 bash -c "test -r /dev/kvm -a -w /dev/kvm && echo 'yes'") -eq "yes"
$AccelFlag = if ($HasKvm) { "-enable-kvm -cpu host" } else { "-cpu max" }

$QemuArgs = "$AccelFlag -m $RamMB -smp $Cores " +
    "-drive file='$WslDiskPath',if=virtio,format=qcow2 " +
    "-cdrom '$WslIsoPath' -boot d " +
    "-vga virtio -display gtk " +
    "-device virtio-net-pci,netdev=net0 -netdev user,id=net0 " +
    "-audiodev pa,id=snd0,server=unix:/mnt/wslg/PulseServer -device intel-hda -device hda-duplex,audiodev=snd0 " +
    "-usb -device usb-tablet -serial file:/tmp/axisos-serial.log"

wsl -d Ubuntu-26.04 bash -c "export DISPLAY=:0; export PULSE_SERVER=unix:/mnt/wslg/PulseServer; qemu-system-x86_64 $QemuArgs || qemu-system-x86_64 -cpu max -m $RamMB -smp $Cores -drive file='$WslDiskPath',if=virtio,format=qcow2 -cdrom '$WslIsoPath' -boot d -vga virtio -display sdl -device virtio-net-pci,netdev=net0 -netdev user,id=net0 -audiodev pa,id=snd0,server=unix:/mnt/wslg/PulseServer -device intel-hda -device hda-duplex,audiodev=snd0 -usb -device usb-tablet -serial file:/tmp/axisos-serial.log"

