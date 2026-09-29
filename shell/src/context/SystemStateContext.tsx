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
    id: 'sonoma-horizon',
    name: 'Sonoma Horizon (Default)',
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
  blue: {
    primary: 'bg-blue-500 hover:bg-blue-600',
    bg: 'bg-blue-500/20',
    border: 'border-blue-500/40',
    text: 'text-blue-400',
    ring: 'focus:ring-blue-500',
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
}

const initialSystemInfo: SystemInfo = {
  osName: 'AxisOS Linux 1.0',
  osVersion: 'Horizon',
  kernelVersion: '6.12.0-axisos-amd64',
  architecture: 'x86_64',
  compositor: 'Wayland (Cage / AxisShell)',
  initSystem: 'systemd 256',
  shellVersion: 'AxisShell v1.0.0',
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
  const [brightness, setBrightness] = useState<number>(85);
  const [wifiConnected, setWifiConnected] = useState<boolean>(true);
  const [wifiSsid, setWifiSsid] = useState<string>('Axis-Fiber-5G');
  const [bluetoothEnabled, setBluetoothEnabled] = useState<boolean>(true);
  const [nightLight, setNightLight] = useState<boolean>(false);
  const [batteryLevel] = useState<number>(92);
  const [isCharging] = useState<boolean>(true);
  const [isQuickSettingsOpen, setIsQuickSettingsOpen] = useState<boolean>(false);
  const [isAppMenuOpen, setIsAppMenuOpen] = useState<boolean>(false);
  const [isSpotlightOpen, setIsSpotlightOpen] = useState<boolean>(false);
  const [powerModalOpen, setPowerModalOpen] = useState<boolean>(false);
  const [isLiveEnvironment, setIsLiveEnvironment] = useState<boolean>(false);
  const [systemInfo, setSystemInfo] = useState<SystemInfo>(initialSystemInfo);

  // Load real host hardware dynamically
  useEffect(() => {
    let isMounted = true;
    systemService.getSystemInfo().then((real) => {
      if (!isMounted) return;
      if (typeof real.isLiveEnvironment === 'boolean') {
        setIsLiveEnvironment(real.isLiveEnvironment);
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

  // Keyboard shortcut: Cmd+Space or Ctrl+Space for Spotlight
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
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
        brightness,
        setBrightness,
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
