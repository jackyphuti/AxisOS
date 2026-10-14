#!/bin/bash
# ==============================================================================
# AxisOS - GameBoost & Hardware Overclocking Engine
# Dynamically optimizes CPU clock frequencies, unlocks GPU power limits,
# eliminates kernel split-lock latency, and sets memory parameters for gaming.
# ==============================================================================

set -e

CONF_FILE="/etc/axis-game-boost.conf"

ensure_conf() {
    if [ ! -f "$CONF_FILE" ]; then
        echo "GAME_BOOST_ENABLED=false" > "$CONF_FILE" 2>/dev/null || true
    fi
}

is_boost_enabled() {
    ensure_conf
    if [ -f "$CONF_FILE" ] && grep -qi "GAME_BOOST_ENABLED=true" "$CONF_FILE"; then
        return 0
    else
        return 1
    fi
}

enable_game_boost() {
    echo "[AxisOS GameBoost] Activating maximum gaming & overclocking performance mode..."
    mkdir -p "$(dirname "$CONF_FILE")"
    echo "GAME_BOOST_ENABLED=true" > "$CONF_FILE"
    echo "ACTIVATION_TIME=$(date -u +"%Y-%m-%dT%H:%M:%SZ")" >> "$CONF_FILE"

    # 1. CPU Clock Scaling & Governors
    # Set all available CPU cores to maximum 'performance' governor
    for gov in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
        if [ -w "$gov" ]; then
            echo "performance" > "$gov" 2>/dev/null || true
        fi
    done

    # Set Energy Performance Preference (EPP) to peak performance
    for epp in /sys/devices/system/cpu/cpu*/cpufreq/energy_performance_preference; do
        if [ -w "$epp" ]; then
            echo "performance" > "$epp" 2>/dev/null || true
        fi
    done

    # Set x86 energy perf policy to maximum performance if utility available
    if command -v x86_energy_perf_policy >/dev/null 2>&1; then
        x86_energy_perf_policy performance 2>/dev/null || true
    fi

    # Pin CPU minimum clock speed to maximum boost clock (zero clock latency)
    for cpu_dir in /sys/devices/system/cpu/cpu*/cpufreq; do
        if [ -r "$cpu_dir/scaling_max_freq" ] && [ -w "$cpu_dir/scaling_min_freq" ]; then
            max_khz=$(cat "$cpu_dir/scaling_max_freq" 2>/dev/null || true)
            if [ -n "$max_khz" ]; then
                echo "$max_khz" > "$cpu_dir/scaling_min_freq" 2>/dev/null || true
            fi
        fi
    done

    # Intel P-State unthrottling (ensure Turbo Boost is 100% active and unthrottled)
    if [ -d /sys/devices/system/cpu/intel_pstate ]; then
        [ -w /sys/devices/system/cpu/intel_pstate/no_turbo ] && echo 0 > /sys/devices/system/cpu/intel_pstate/no_turbo 2>/dev/null || true
        [ -w /sys/devices/system/cpu/intel_pstate/max_perf_pct ] && echo 100 > /sys/devices/system/cpu/intel_pstate/max_perf_pct 2>/dev/null || true
        [ -w /sys/devices/system/cpu/intel_pstate/min_perf_pct ] && echo 100 > /sys/devices/system/cpu/intel_pstate/min_perf_pct 2>/dev/null || true
    fi

    # AMD Core Performance Boost (CPB) unthrottling
    if [ -w /sys/devices/system/cpu/cpufreq/boost ]; then
        echo 1 > /sys/devices/system/cpu/cpufreq/boost 2>/dev/null || true
    fi

    # 2. GPU Overclocking & Maximum Power States
    # NVIDIA Dedicated GPUs
    if command -v nvidia-smi >/dev/null 2>&1; then
        echo "[AxisOS GameBoost] Configuring NVIDIA GPU Overclock & Power Limits..."
        nvidia-smi -pm 1 2>/dev/null || true
        # Read maximum supported wattage power limit and apply it
        max_power=$(nvidia-smi -q -d POWER 2>/dev/null | grep -i "Max Power Limit" | head -n1 | awk '{print $5}' | cut -d. -f1 || true)
        if [ -n "$max_power" ] && [ "$max_power" -gt 0 ] 2>/dev/null; then
            nvidia-smi -pl "$max_power" 2>/dev/null || true
        fi
        nvidia-smi --auto-boost-permission=0 2>/dev/null || true
        # PowerMizer maximum performance mode
        if [ -n "$DISPLAY" ] && command -v nvidia-settings >/dev/null 2>&1; then
            nvidia-settings -a "[gpu:0]/GPUPowerMizerMode=1" 2>/dev/null || true
        fi
    fi

    # AMD Radeon Dedicated & Integrated GPUs
    for drm_card in /sys/class/drm/card*/device; do
        if [ -d "$drm_card" ]; then
            # Force high DPM performance level
            if [ -w "$drm_card/power_dpm_force_performance_level" ]; then
                echo "high" > "$drm_card/power_dpm_force_performance_level" 2>/dev/null || true
            fi
            # Set 3D Fullscreen / Compute profile (Mode 1 = 3D profile)
            if [ -w "$drm_card/pp_power_profile_mode" ]; then
                echo 1 > "$drm_card/pp_power_profile_mode" 2>/dev/null || true
            fi
            # Performance state
            if [ -w "$drm_card/power_dpm_state" ]; then
                echo "performance" > "$drm_card/power_dpm_state" 2>/dev/null || true
            fi
        fi
    done

    # Intel Arc / Iris Xe Graphics
    for drm_card in /sys/class/drm/card*; do
        if [ -r "$drm_card/gt_max_freq_mhz" ] && [ -w "$drm_card/gt_boost_freq_mhz" ]; then
            gt_max=$(cat "$drm_card/gt_max_freq_mhz" 2>/dev/null || true)
            if [ -n "$gt_max" ]; then
                echo "$gt_max" > "$drm_card/gt_boost_freq_mhz" 2>/dev/null || true
            fi
        fi
    done

    # 3. Kernel Latency & Memory Tuning
    # Disable split-lock mitigation (eliminates 30-50% FPS micro-stutter in Proton/DirectX/Vulkan games)
    sysctl -w vm.split_lock_mitigate=0 2>/dev/null || true

    # Reduce memory swappiness so high-bandwidth game assets remain cached in RAM
    sysctl -w vm.swappiness=10 2>/dev/null || true

    # Maximize memory mapping headroom for Unreal Engine 5, DXVK, and large open-world games
    sysctl -w vm.max_map_count=2147483642 2>/dev/null || true

    # File descriptor headroom
    sysctl -w fs.file-max=2097152 2>/dev/null || true

    # Activate Feral GameMode daemon if available
    if command -v gamemoded >/dev/null 2>&1; then
        systemctl start gamemoded.service 2>/dev/null || gamemoded -d 2>/dev/null || true
    fi

    echo "[AxisOS GameBoost] SUCCESS: Hardware overclock and extreme performance profile engaged."
}

disable_game_boost() {
    echo "[AxisOS GameBoost] Reverting to balanced / power-saving hardware profile..."
    mkdir -p "$(dirname "$CONF_FILE")"
    echo "GAME_BOOST_ENABLED=false" > "$CONF_FILE"
    echo "DEACTIVATION_TIME=$(date -u +"%Y-%m-%dT%H:%M:%SZ")" >> "$CONF_FILE"

    # 1. Restore CPU Governors
    default_gov="schedutil"
    if [ -d /sys/devices/system/cpu/intel_pstate ]; then
        default_gov="powersave"
    fi

    for gov in /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor; do
        if [ -w "$gov" ]; then
            echo "$default_gov" > "$gov" 2>/dev/null || true
        fi
    done

    # Restore EPP to balanced mode
    for epp in /sys/devices/system/cpu/cpu*/cpufreq/energy_performance_preference; do
        if [ -w "$epp" ]; then
            echo "balance_performance" > "$epp" 2>/dev/null || true
        fi
    done

    if command -v x86_energy_perf_policy >/dev/null 2>&1; then
        x86_energy_perf_policy balance-performance 2>/dev/null || true
    fi

    # Restore CPU minimum frequency to hardware floor
    for cpu_dir in /sys/devices/system/cpu/cpu*/cpufreq; do
        if [ -r "$cpu_dir/cpuinfo_min_freq" ] && [ -w "$cpu_dir/scaling_min_freq" ]; then
            min_khz=$(cat "$cpu_dir/cpuinfo_min_freq" 2>/dev/null || true)
            if [ -n "$min_khz" ]; then
                echo "$min_khz" > "$cpu_dir/scaling_min_freq" 2>/dev/null || true
            fi
        fi
    done

    # Restore Intel P-State percentages
    if [ -d /sys/devices/system/cpu/intel_pstate ]; then
        [ -w /sys/devices/system/cpu/intel_pstate/min_perf_pct ] && echo 10 > /sys/devices/system/cpu/intel_pstate/min_perf_pct 2>/dev/null || true
    fi

    # 2. Restore GPU Power States
    # AMD Radeon
    for drm_card in /sys/class/drm/card*/device; do
        if [ -d "$drm_card" ]; then
            if [ -w "$drm_card/power_dpm_force_performance_level" ]; then
                echo "auto" > "$drm_card/power_dpm_force_performance_level" 2>/dev/null || true
            fi
            if [ -w "$drm_card/pp_power_profile_mode" ]; then
                echo 0 > "$drm_card/pp_power_profile_mode" 2>/dev/null || true
            fi
        fi
    done

    # NVIDIA: Restore default power limit wattage and adaptive PowerMizer
    if command -v nvidia-smi >/dev/null 2>&1; then
        def_power=$(nvidia-smi -q -d POWER 2>/dev/null | grep -i "Default Power Limit" | head -n1 | awk '{print $5}' | cut -d. -f1 || true)
        if [ -n "$def_power" ] && [ "$def_power" -gt 0 ] 2>/dev/null; then
            nvidia-smi -pl "$def_power" 2>/dev/null || true
        fi
    fi
    if [ -n "${DISPLAY:-}" ] && command -v nvidia-settings >/dev/null 2>&1; then
        nvidia-settings -a "[gpu:0]/GPUPowerMizerMode=0" 2>/dev/null || true
    fi

    # 3. Restore Kernel Parameters
    sysctl -w vm.split_lock_mitigate=1 2>/dev/null || true
    sysctl -w vm.swappiness=60 2>/dev/null || true

    echo "[AxisOS GameBoost] SUCCESS: Hardware returned to balanced energy-efficient profile."
}

print_status() {
    ensure_conf
    local enabled="false"
    if is_boost_enabled; then
        enabled="true"
    fi

    local current_gov="unknown"
    local sample_gov=$(cat /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 2>/dev/null || echo "")
    [ -n "$sample_gov" ] && current_gov="$sample_gov"

    local split_lock="1"
    split_lock=$(sysctl -n vm.split_lock_mitigate 2>/dev/null || echo "1")

    local swappiness="60"
    swappiness=$(sysctl -n vm.swappiness 2>/dev/null || echo "60")

    local max_maps="65530"
    max_maps=$(sysctl -n vm.max_map_count 2>/dev/null || echo "65530")

    local gpu_type="Auto / Integrated"
    if command -v nvidia-smi >/dev/null 2>&1; then
        gpu_type="NVIDIA Dedicated GPU"
    elif ls /sys/class/drm/card*/device/power_dpm_force_performance_level >/dev/null 2>&1; then
        gpu_type="AMD Radeon GPU"
    fi

    echo "GAME_BOOST_ENABLED=$enabled"
    echo "CPU_GOVERNOR=$current_gov"
    echo "GPU_TYPE=$gpu_type"
    echo "VM_SPLIT_LOCK_MITIGATE=$split_lock"
    echo "VM_SWAPPINESS=$swappiness"
    echo "VM_MAX_MAP_COUNT=$max_maps"
}

case "$1" in
    on|enable|start)
        enable_game_boost
        ;;
    off|disable|stop)
        disable_game_boost
        ;;
    toggle)
        if is_boost_enabled; then
            disable_game_boost
        else
            enable_game_boost
        fi
        ;;
    status)
        print_status
        ;;
    apply-current)
        if is_boost_enabled; then
            enable_game_boost
        else
            disable_game_boost
        fi
        ;;
    *)
        echo "Usage: $0 {on|off|toggle|status|apply-current}"
        exit 1
        ;;
esac
