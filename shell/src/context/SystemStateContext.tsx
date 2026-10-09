import React, { createContext, useContext, useState, useEffect } from 'react';
import { AccentColor, SystemInfo, SystemTheme } from '../types/os';
import { systemService, SystemHardwareData } from '../services/systemService';

export interface WallpaperOption {
  id: string;
  name: string;
  gradient: string;
  previewColor: string;
}

export const WALLPAPERS: WallpaperOption[] = [
  {
    id: 'mint-aurora',
    name: 'Mint Aurora (Default)',
    gradient: 'radial-gradient(ellipse at 75% 20%, #87cf3e 0%, #356b1f 28%, #1f2b23 55%, #121815 80%, #0a0d0c 100%), radial-gradient(circle at 20% 80%, #5b9a28 0%, #1a251e 40%, transparent 70%)',
    previewColor: '#87cf3e',
  },
  {
    id: 'mint-slate',
    name: 'Mint Charcoal & Slate',
    gradient: 'linear-gradient(135deg, #1b241f 0%, #222d27 25%, #18201c 60%, #0d1210 100%), radial-gradient(circle at 80% 20%, #87cf3e28 0%, transparent 55%)',
    previewColor: '#2f343f',
  },
  {
    id: 'sonoma-horizon',
    name: 'Sonoma Horizon',
    gradient: 'radial-gradient(ellipse at top right, #1d4ed8 0%, #1e1b4b 35%, #0f172a 70%, #020617 100%), radial-gradient(ellipse at bottom left, #0284c7 0%, transparent 60%)',
    previewColor: '#1d4ed8',
  },
  {
    id: 'sequoia-sunrise',
    name: 'Sequoia Sunrise',
    gradient: 'linear-gradient(135deg, #431407 0%, #7c2d12 25%, #9a3412 50%, #1e1b4b 85%, #09090b 100%)',
    previewColor: '#9a3412',
  },
  {
    id: 'ventura-flow',
    name: 'Ventura Flow',
    gradient: 'radial-gradient(circle at 80% 20%, #ea580c 0%, #c2410c 30%, #431407 60%, #0f172a 100%)',
    previewColor: '#ea580c',
  },
  {
    id: 'monterey-purple',
    name: 'Monterey Purple',
    gradient: 'radial-gradient(circle at 20% 20%, #701a75 0%, #4c0519 40%, #09090b 80%)',
    previewColor: '#701a75',
  },
  {
    id: 'big-sur-cyan',
    name: 'Big Sur Waves',
    gradient: 'radial-gradient(ellipse at 50% 10%, #0369a1 0%, #0f172a 60%, #020617 100%)',
    previewColor: '#0369a1',
  },
];

export const ACCENT_COLOR_MAP: Record<AccentColor, { primary: string; bg: string; border: string; text: string; ring: string }> = {
  mint: {
    primary: 'bg-[#87cf3e] hover:bg-[#76bb33] text-black',
    bg: 'bg-[#87cf3e]/20',
    border: 'border-[#87cf3e]/40',
    text: 'text-[#87cf3e]',
    ring: 'focus:ring-[#87cf3e]',
  },
  blue: {
    primary: 'bg-[#2563eb] hover:bg-[#1d4ed8] text-white',
    bg: 'bg-[#2563eb]/20',
    border: 'border-[#2563eb]/50',
    text: 'text-[#60a5fa]',
    ring: 'focus:ring-[#2563eb]',
  },
  cyan: {
    primary: 'bg-cyan-500 hover:bg-cyan-600',
    bg: 'bg-cyan-500/20',
    border: 'border-cyan-500/40',
    text: 'text-cyan-400',
    ring: 'focus:ring-cyan-500',
  },
  purple: {
    primary: 'bg-purple-500 hover:bg-purple-600',
    bg: 'bg-purple-500/20',
    border: 'border-purple-500/40',
    text: 'text-purple-400',
    ring: 'focus:ring-purple-500',
  },
  emerald: {
    primary: 'bg-emerald-500 hover:bg-emerald-600',
    bg: 'bg-emerald-500/20',
    border: 'border-emerald-500/40',
    text: 'text-emerald-400',
    ring: 'focus:ring-emerald-500',
  },
  amber: {
    primary: 'bg-amber-500 hover:bg-amber-600',
    bg: 'bg-amber-500/20',
    border: 'border-amber-500/40',
    text: 'text-amber-400',
    ring: 'focus:ring-amber-500',
  },
  rose: {
    primary: 'bg-rose-500 hover:bg-rose-600',
    bg: 'bg-rose-500/20',
    border: 'border-rose-500/40',
    text: 'text-rose-400',
    ring: 'focus:ring-rose-500',
  },
};

interface SystemStateContextType {
  theme: SystemTheme;
  setTheme: (theme: SystemTheme) => void;
  accentColor: AccentColor;
  setAccentColor: (accent: AccentColor) => void;
  wallpaper: WallpaperOption;
  setWallpaper: (wp: WallpaperOption) => void;
  volume: number;
  setVolume: (vol: number) => void;
  isMuted: boolean;
  setIsMuted: (muted: boolean) => void;
  brightness: number;
  setBrightness: (br: number) => void;
  toggleBrightness: () => void;
  isLocked: boolean;
  setIsLocked: (locked: boolean) => void;
  lockSession: () => void;
  wifiEnabled: boolean;
  setWifiEnabled: (en: boolean) => void;
  wifiConnected: boolean;
  setWifiConnected: (conn: boolean) => void;
  wifiSsid: string;
  setWifiSsid: (ssid: string) => void;
  bluetoothEnabled: boolean;
  setBluetoothEnabled: (en: boolean) => void;
  nightLight: boolean;
  setNightLight: (nl: boolean) => void;
  batteryLevel: number;
  isCharging: boolean;
  hasBattery: boolean;
  batteryStatus: string;
  isQuickSettingsOpen: boolean;
  setIsQuickSettingsOpen: (open: boolean) => void;
  isAppMenuOpen: boolean;
  setIsAppMenuOpen: (open: boolean) => void;
  isSpotlightOpen: boolean;
  setIsSpotlightOpen: (open: boolean) => void;
  powerModalOpen: boolean;
  setPowerModalOpen: (open: boolean) => void;
  systemInfo: SystemInfo;
  isLiveEnvironment: boolean;
  setIsLiveEnvironment: (live: boolean) => void;
  autoInstall: boolean;
  toggleWifi: (desired?: boolean) => Promise<void>;
  toggleBluetooth: () => Promise<void>;
  changeVolume: (vol: number) => Promise<void>;
  toggleMute: () => Promise<void>;
}

const initialSystemInfo: SystemInfo = {
  osName: 'AxisOS Linux 2.0',
  osVersion: 'Horizon',
  kernelVersion: '6.12.0-axisos-amd64',
  architecture: 'x86_64',
  compositor: 'Wayland (Cage / AxisShell)',
  initSystem: 'systemd 256',
  shellVersion: 'AxisShell v2.0.0',
  cpuModel: 'Intel/AMD 64-bit Processor',
  cpuCores: 8,
  gpuModel: 'Hardware Accelerated GPU',
  totalMemory: '16.0 GB Unified Memory',
  freeMemory: '12.4 GB Available',
  storageCapacity: 'NVMe Solid State Drive 256 GB',
  hostname: 'axis-pc',
  username: 'axis',
  uptime: 'up 1 hour',
  homeDir: '/home/axis',
};

const SystemStateContext = createContext<SystemStateContextType | null>(null);

export const SystemStateProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [theme, setTheme] = useState<SystemTheme>('dark');
  const [accentColor, setAccentColor] = useState<AccentColor>('blue');
  const [wallpaper, setWallpaper] = useState<WallpaperOption>(WALLPAPERS[0]);
  const [volume, setVolume] = useState<number>(75);
  const [isMuted, setIsMuted] = useState<boolean>(false);
  const [brightness, setBrightnessState] = useState<number>(() => {
    try {
      const saved = localStorage.getItem('axisos_screen_brightness');
      return saved ? Math.max(10, Math.min(100, Number(saved))) : 85;
    } catch {
      return 85;
    }
  });

  const setBrightness = (val: number) => {
    const clamped = Math.max(10, Math.min(100, Math.round(val)));
    setBrightnessState(clamped);
    try {
      localStorage.setItem('axisos_screen_brightness', String(clamped));
    } catch {}
    systemService.setHardwareBrightness(clamped).catch(() => {});
  };

  const toggleBrightness = () => {
    if (brightness > 55) {
      setBrightness(30);
    } else {
      setBrightness(95);
    }
  };

  const [isLocked, setIsLocked] = useState<boolean>(false);
  const [wifiEnabled, setWifiEnabled] = useState<boolean>(true);
  const [wifiConnected, setWifiConnected] = useState<boolean>(false);
  const [wifiSsid, setWifiSsid] = useState<string>('Not Connected');
  const [bluetoothEnabled, setBluetoothEnabled] = useState<boolean>(true);
  const [nightLight, setNightLight] = useState<boolean>(false);
  const [batteryLevel, setBatteryLevel] = useState<number>(100);
  const [isCharging, setIsCharging] = useState<boolean>(true);
  const [hasBattery, setHasBattery] = useState<boolean>(false);
  const [batteryStatus, setBatteryStatus] = useState<string>('AC Connected');
  const [isQuickSettingsOpen, setIsQuickSettingsOpen] = useState<boolean>(false);
  const [isAppMenuOpen, setIsAppMenuOpen] = useState<boolean>(false);
  const [isSpotlightOpen, setIsSpotlightOpen] = useState<boolean>(false);
  const [powerModalOpen, setPowerModalOpen] = useState<boolean>(false);
  const [isLiveEnvironment, setIsLiveEnvironment] = useState<boolean>(false);
  const [autoInstall, setAutoInstall] = useState<boolean>(false);
  const [systemInfo, setSystemInfo] = useState<SystemInfo>(initialSystemInfo);

  // Sync theme class to document element for Tailwind dark variants
  useEffect(() => {
    if (theme === 'dark') {
      document.documentElement.classList.add('dark');
      document.documentElement.classList.remove('light');
    } else {
      document.documentElement.classList.remove('dark');
      document.documentElement.classList.add('light');
    }
  }, [theme]);

  // Load real host hardware dynamically
  useEffect(() => {
    let isMounted = true;
    systemService.getSystemInfo().then((real) => {
      if (!isMounted) return;
      if (typeof real.isLiveEnvironment === 'boolean') {
        setIsLiveEnvironment(real.isLiveEnvironment);
      }
      if (typeof real.autoInstall === 'boolean') {
        setAutoInstall(real.autoInstall);
      }
      setSystemInfo({
        osName: real.osName || 'AxisOS Linux 1.0',
        osVersion: real.osVersion || 'Horizon',
        kernelVersion: real.kernelVersion || '6.12.0-axisos-amd64',
        architecture: real.architecture || 'x86_64',
        compositor: 'Wayland (Cage / AxisShell)',
        initSystem: 'systemd 256',
        shellVersion: 'AxisShell v1.0.0',
        cpuModel: real.cpuModel || initialSystemInfo.cpuModel,
        cpuCores: real.cpuCores || 8,
        gpuModel: real.gpuModel || initialSystemInfo.gpuModel,
        totalMemory: real.totalMemory || initialSystemInfo.totalMemory,
        freeMemory: real.freeMemory || initialSystemInfo.freeMemory,
        storageCapacity: real.storageDevices?.[0]?.name || initialSystemInfo.storageCapacity,
        hostname: real.hostname || initialSystemInfo.hostname,
        username: real.username || initialSystemInfo.username,
        uptime: real.uptime || initialSystemInfo.uptime,
        homeDir: real.homeDir || `/home/${real.username || 'axis'}`,
      });
    });
    return () => {
      isMounted = false;
    };
  }, []);

  // Hardware state synchronization on boot
  useEffect(() => {
    let isMounted = true;
    systemService.getWifiStatus().then((w) => {
      if (!isMounted) return;
      setWifiConnected(w.connected);
      if (w.currentSsid) setWifiSsid(w.currentSsid);
    });
    systemService.getBluetoothStatus().then((b) => {
      if (!isMounted) return;
      setBluetoothEnabled(b.enabled);
    });
    systemService.getAudioStatus().then((a) => {
      if (!isMounted) return;
      setVolume(a.volume);
      setIsMuted(a.isMuted);
    });
    return () => {
      isMounted = false;
    };
  }, []);

  // Real host hardware battery polling
  useEffect(() => {
    let isMounted = true;
    const fetchBattery = async () => {
      try {
        const bat = await systemService.getBatteryStatus();
        if (!isMounted) return;
        setBatteryLevel(bat.level);
        setIsCharging(bat.charging);
        setHasBattery(bat.hasBattery);
        setBatteryStatus(bat.status);
      } catch {}
    };

    fetchBattery();
    const interval = setInterval(fetchBattery, 10000);
    return () => {
      isMounted = false;
      clearInterval(interval);
    };
  }, []);

  const lockSession = () => {
    setIsLocked(true);
    setIsAppMenuOpen(false);
    setIsQuickSettingsOpen(false);
    setIsSpotlightOpen(false);
    setPowerModalOpen(false);
  };

  // Hardware control functions
  const toggleWifi = async (desired?: boolean) => {
    const next = typeof desired === 'boolean' ? desired : !wifiEnabled;
    setWifiEnabled(next);
    if (!next) {
      setWifiConnected(false);
      setWifiSsid('Wi-Fi Disabled');
    }
    await systemService.toggleWifi(next);
    const updated = await systemService.getWifiStatus();
    setWifiEnabled(updated.enabled);
    setWifiConnected(updated.connected);
    if (updated.currentSsid) {
      setWifiSsid(updated.currentSsid);
    } else if (!updated.enabled) {
      setWifiSsid('Wi-Fi Disabled');
    } else {
      setWifiSsid('Not Connected');
    }
  };

  const toggleBluetooth = async () => {
    const next = !bluetoothEnabled;
    setBluetoothEnabled(next);
    await systemService.toggleBluetooth(next);
  };

  const changeVolume = async (newVol: number) => {
    setVolume(newVol);
    if (newVol > 0 && isMuted) setIsMuted(false);
    await systemService.setAudioVolume(newVol);
  };

  const toggleMute = async () => {
    const next = !isMuted;
    setIsMuted(next);
    await systemService.toggleAudioMute();
  };

  // Keyboard shortcut: Windows button launches App Menu, Meta+L locks screen, Cmd+Space / Ctrl+Space for Spotlight
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      // Lock screen shortcut: Meta + L or Ctrl + Alt + L
      if ((e.metaKey && e.key.toLowerCase() === 'l') || (e.ctrlKey && e.altKey && e.key.toLowerCase() === 'l')) {
        e.preventDefault();
        lockSession();
        return;
      }

      // Windows Button (Meta / Super / OS key standalone) launches App Menu
      if (
        (e.key === 'Meta' || e.key === 'OS' || e.code === 'MetaLeft' || e.code === 'MetaRight' || e.keyCode === 91 || e.keyCode === 92) &&
        !e.ctrlKey &&
        !e.altKey &&
        !e.shiftKey
      ) {
        e.preventDefault();
        setIsAppMenuOpen((prev) => !prev);
        return;
      }

      // Escape key closes menus
      if (e.key === 'Escape') {
        setIsAppMenuOpen(false);
        setIsSpotlightOpen(false);
        setIsQuickSettingsOpen(false);
        return;
      }

      // Spotlight Shortcut
      if ((e.metaKey || e.ctrlKey) && e.code === 'Space') {
        e.preventDefault();
        setIsSpotlightOpen((prev) => !prev);
      }
    };
    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, []);

  // Close menus when clicking outside
  useEffect(() => {
    const handleGlobalClick = (e: MouseEvent) => {
      const target = e.target as HTMLElement;
      if (!target.closest('#quick-settings-panel') && !target.closest('#quick-settings-trigger')) {
        setIsQuickSettingsOpen(false);
      }
      if (!target.closest('#app-menu-panel') && !target.closest('#app-menu-trigger')) {
        setIsAppMenuOpen(false);
      }
    };
    window.addEventListener('mousedown', handleGlobalClick);
    return () => window.removeEventListener('mousedown', handleGlobalClick);
  }, []);

  return (
    <SystemStateContext.Provider
      value={{
        theme,
        setTheme,
        accentColor,
        setAccentColor,
        wallpaper,
        setWallpaper,
        volume,
        setVolume,
        isMuted,
        setIsMuted,
        isLocked,
        setIsLocked,
        lockSession,
        brightness,
        setBrightness,
        toggleBrightness,
        wifiEnabled,
        setWifiEnabled,
        wifiConnected,
        setWifiConnected,
        wifiSsid,
        setWifiSsid,
        bluetoothEnabled,
        setBluetoothEnabled,
        nightLight,
        setNightLight,
        batteryLevel,
        isCharging,
        hasBattery,
        batteryStatus,
        isQuickSettingsOpen,
        setIsQuickSettingsOpen,
        isAppMenuOpen,
        setIsAppMenuOpen,
        isSpotlightOpen,
        setIsSpotlightOpen,
        powerModalOpen,
        setPowerModalOpen,
        systemInfo,
        isLiveEnvironment,
        setIsLiveEnvironment,
        autoInstall,
        toggleWifi,
        toggleBluetooth,
        changeVolume,
        toggleMute,
      }}
    >
      {children}
    </SystemStateContext.Provider>
  );
};

export const useSystemState = () => {
  const ctx = useContext(SystemStateContext);
  if (!ctx) throw new Error('useSystemState must be used within SystemStateProvider');
  return ctx;
};
