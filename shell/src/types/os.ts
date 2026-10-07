export type AppId =
  | 'installer'
  | 'settings'
  | 'terminal'
  | 'file-manager'
  | 'text-editor'
  | 'system-monitor'
  | 'browser'
  | 'calculator'
  | 'about'
  | 'music'
  | 'photos'
  | 'notes'
  | 'software'
  | 'clock'
  | 'weather'
  | 'camera'
  | 'steam';

export interface AppDefinition {
  id: AppId;
  title: string;
  category: 'system' | 'utilities' | 'accessories';
  description: string;
  defaultWidth: number;
  defaultHeight: number;
  isPinned: boolean;
}

export interface WindowState {
  id: string;
  appId: AppId;
  title: string;
  x: number;
  y: number;
  width: number;
  height: number;
  isMinimized: boolean;
  isMaximized: boolean;
  zIndex: number;
  animating?: 'closing' | 'minimizing' | 'restoring';
  params?: Record<string, any>;
}

export type SystemTheme = 'dark' | 'light';

export type AccentColor = 'mint' | 'blue' | 'cyan' | 'purple' | 'emerald' | 'amber' | 'rose';

export interface SystemInfo {
  osName: string;
  osVersion: string;
  kernelVersion: string;
  architecture: string;
  compositor: string;
  initSystem: string;
  shellVersion: string;
  cpuModel: string;
  cpuCores: number;
  gpuModel: string;
  totalMemory: string;
  freeMemory: string;
  storageCapacity: string;
  hostname: string;
  username: string;
  uptime: string;
  homeDir?: string;
}

export interface InstallerData {
  language: string;
  location: string;
  keyboardLayout: string;
  targetDisk: string;
  eraseDisk: boolean;
  userFullName: string;
  username: string;
  computerName: string;
  password: string;
  autoLogin: boolean;
  locale?: string;
  timezone?: string;
  keymap?: string;
}
