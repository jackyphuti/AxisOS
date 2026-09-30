import React, { useState, useEffect } from 'react';
import {
  Monitor,
  Palette,
  Eye,
  Sliders,
  Image as ImageIcon,
  Wifi,
  WifiOff,
  Bluetooth,
  Volume2,
  VolumeX,
  HardDrive,
  Info,
  Battery,
  Search,
  Check,
  ChevronDown,
  ChevronRight,
  Sun,
  Moon,
  Laptop,
  Maximize2,
  Lock,
  RefreshCw,
  Radio,
  Headphones,
  Speaker,
  Shield,
  Key,
  Mouse,
  Keyboard,
  Printer,
  Globe,
  Network,
  Server,
  Terminal,
  Zap,
  Code,
  AlertTriangle,
  Layers,
  Cpu,
  FileCode,
  CheckCircle2,
  ExternalLink,
} from 'lucide-react';
import { useSystemState, WALLPAPERS } from '../../context/SystemStateContext';
import { useWindowManager } from '../../context/WindowManagerContext';
import { systemService, WifiNetwork, BluetoothDevice } from '../../services/systemService';

export type SettingsTab =
  // System & Hardware
  | 'displays'
  | 'power'
  | 'storage'
  // Devices & Peripherals
  | 'mouse-keyboard'
  | 'sound'
  | 'printers'
  // Network & Internet
  | 'wifi'
  | 'ethernet'
  | 'bluetooth'
  | 'proxy'
  // Personalization & Workspace
  | 'appearance'
  | 'wallpaper'
  | 'dock-workspace'
  // Apps & Execution
  | 'default-apps'
  | 'startup-apps'
  | 'env-variables'
  // Developer & Advanced
  | 'kernel-drivers'
  | 'dev-tools'
  | 'general';

interface CategoryGroup {
  groupName: string;
  items: {
    id: SettingsTab;
    label: string;
    icon: React.ReactNode;
    color: string;
    badge?: string;
  }[];
}

interface DisplayResolution {
  id: string;
  label: string;
  isDefault?: boolean;
}

const PRIMARY_RESOLUTIONS: DisplayResolution[] = [
  { id: '1080p', label: '1920 x 1080 (1080p)', isDefault: true },
  { id: '1600x900', label: '1600 x 900' },
  { id: '1344x756', label: '1344 x 756' },
];

const EXTENDED_RESOLUTIONS: DisplayResolution[] = [
  { id: '1080p', label: '1920 x 1080 (1080p)', isDefault: true },
  { id: '1600x900', label: '1600 x 900' },
  { id: '1366x768', label: '1366 x 768 (Native Panel)' },
  { id: '1344x756', label: '1344 x 756' },
  { id: '1280x720', label: '1280 x 720 (720p)' },
  { id: '1024x768', label: '1024 x 768' },
];

export const SettingsApp: React.FC<{ params?: Record<string, any> }> = ({ params }) => {
  const {
    theme,
    setTheme,
    wallpaper,
    setWallpaper,
    brightness,
    setBrightness,
    nightLight,
    setNightLight,
    systemInfo,
    wifiConnected,
    wifiSsid,
    setWifiSsid,
    setWifiConnected,
    bluetoothEnabled,
    volume,
    isMuted,
    toggleWifi,
    toggleBluetooth,
    changeVolume,
    toggleMute,
    accentColor,
    setAccentColor,
  } = useSystemState();

  const { closeWindow, minimizeWindow, toggleMaximizeWindow, windows } = useWindowManager();
  const currentWindow = windows.find((w) => w.appId === 'settings');

  const [activeTab, setActiveTab] = useState<SettingsTab>(() => (params?.tab as SettingsTab) || 'displays');
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedMonitor, setSelectedMonitor] = useState<'built-in' | 'external'>('external');
  const [selectedResolution, setSelectedResolution] = useState<string>('1080p');
  const [showAllResolutions, setShowAllResolutions] = useState<boolean>(false);
  const [refreshRate, setRefreshRate] = useState<string>('144Hz');
  const [uiScaling, setUiScaling] = useState<string>('100%');
  const [useAsMode, setUseAsMode] = useState<string>('Extended display');
  const [isArranging, setIsArranging] = useState<boolean>(false);
  const [trueTone, setTrueTone] = useState<boolean>(true);
  const [realBatteryData, setRealBatteryData] = useState<{ capacity: string; status: string } | null>(null);

  // Deep linking: Automatically jump to tab when window parameters change
  useEffect(() => {
    const targetTab = params?.tab || currentWindow?.params?.tab;
    if (targetTab) {
      setActiveTab(targetTab as SettingsTab);
    }
  }, [params?.tab, currentWindow?.params?.tab]);

  // Hardware states for Wi-Fi, Bluetooth, Audio
  const [wifiNetworks, setWifiNetworks] = useState<WifiNetwork[]>([]);
  const [isScanningWifi, setIsScanningWifi] = useState(false);
  const [connectingSsid, setConnectingSsid] = useState<string | null>(null);
  const [wifiPasswordPrompt, setWifiPasswordPrompt] = useState<string | null>(null);
  const [wifiPasswordInput, setWifiPasswordInput] = useState('');
  const [bluetoothDevices, setBluetoothDevices] = useState<BluetoothDevice[]>([]);
  const [isScanningBt, setIsScanningBt] = useState(false);
  const [btConnectingMac, setBtConnectingMac] = useState<string | null>(null);
  const [selectedAudioOutput, setSelectedAudioOutput] = useState('Built-in Audio Analog Stereo (PipeWire)');
  const [audioSampleRate, setAudioSampleRate] = useState('48,000 Hz (Standard)');
  const [audioBufferSize, setAudioBufferSize] = useState('128 samples (2.6 ms)');
  const [testSoundPlaying, setTestSoundPlaying] = useState(false);

  // Progressive disclosure: Advanced settings accordion toggles
  const [showAdvancedDriverSettings, setShowAdvancedDriverSettings] = useState(false);
  const [showAdvancedEthernetSettings, setShowAdvancedEthernetSettings] = useState(false);
  const [showAdvancedAudioSettings, setShowAdvancedAudioSettings] = useState(false);
  const [showAdvancedPowerSettings, setShowAdvancedPowerSettings] = useState(false);

  // Devices & Power state
  const [powerPlan, setPowerPlan] = useState<'balanced' | 'performance' | 'saver'>('balanced');
  const [lidCloseAction, setLidCloseAction] = useState('Sleep');
  const [powerButtonAction, setPowerButtonAction] = useState('Shut down');
  const [mouseSpeed, setMouseSpeed] = useState(6);
  const [mouseAcceleration, setMouseAcceleration] = useState(true);
  const [scrollLines, setScrollLines] = useState(3);
  const [keyboardLayout, setKeyboardLayout] = useState('English (US)');

  // Network state
  const [ethernetConnected, setEthernetConnected] = useState(true);
  const [ipv6Enabled, setIpv6Enabled] = useState(true);
  const [macSpoofing, setMacSpoofing] = useState(false);
  const [proxyMode, setProxyMode] = useState<'off' | 'auto' | 'manual'>('off');

  // Personalization state
  const [dockPosition, setDockPosition] = useState<'bottom' | 'left' | 'right'>('bottom');
  const [dockAutoHide, setDockAutoHide] = useState(false);
  const [windowSnapping, setWindowSnapping] = useState(true);

  // Apps state
  const [defaultBrowser, setDefaultBrowser] = useState('Axis Browser (Chromium)');
  const [defaultEditor, setDefaultEditor] = useState('TextEdit');
  const [envVars, setEnvVars] = useState<[string, string][]>([
    ['PATH', '/usr/local/bin:/usr/bin:/bin:/usr/games'],
    ['WAYLAND_DISPLAY', 'wayland-0'],
    ['XDG_CURRENT_DESKTOP', 'AxisOS:Cage'],
    ['EDITOR', 'nano'],
  ]);

  // Read real Linux battery and hardware via IPC
  useEffect(() => {
    let isMounted = true;
    systemService.executeCommand('cat /sys/class/power_supply/BAT0/capacity 2>/dev/null && cat /sys/class/power_supply/BAT0/status 2>/dev/null')
      .then((res) => {
        if (!isMounted) return;
        if (res.stdout) {
          const lines = res.stdout.trim().split('\n');
          setRealBatteryData({
            capacity: lines[0] || '95',
            status: lines[1] || 'Normal',
          });
        }
      })
      .catch(() => {});
    return () => {
      isMounted = false;
    };
  }, []);

  const handleScanWifi = async () => {
    setIsScanningWifi(true);
    try {
      const list = await systemService.scanWifi();
      setWifiNetworks(list);
    } catch {}
    setIsScanningWifi(false);
  };

  const handleConnectWifi = async (ssid: string, password?: string) => {
    setConnectingSsid(ssid);
    try {
      const res = await systemService.connectWifi(ssid, password);
      if (res.success) {
        setWifiSsid(ssid);
        setWifiConnected(true);
        setWifiPasswordPrompt(null);
        setWifiPasswordInput('');
        handleScanWifi();
      }
    } catch {}
    setConnectingSsid(null);
  };

  const handleScanBluetooth = async () => {
    setIsScanningBt(true);
    try {
      const list = await systemService.getBluetoothDevices();
      setBluetoothDevices(list);
    } catch {}
    setIsScanningBt(false);
  };

  const handleConnectBluetooth = async (mac: string) => {
    setBtConnectingMac(mac);
    try {
      await systemService.connectBluetooth(mac);
      setBluetoothDevices((prev) =>
        prev.map((d) => (d.mac === mac ? { ...d, connected: !d.connected } : d))
      );
    } catch {}
    setBtConnectingMac(null);
  };

  const playSoundChime = () => {
    try {
      const AudioCtx = window.AudioContext || (window as any).webkitAudioContext;
      if (!AudioCtx) return;
      const ctx = new AudioCtx();
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();
      osc.type = 'sine';
      osc.frequency.setValueAtTime(587.33, ctx.currentTime);
      osc.frequency.setValueAtTime(880, ctx.currentTime + 0.12);
      gain.gain.setValueAtTime(0.25, ctx.currentTime);
      gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.5);
      osc.connect(gain);
      gain.connect(ctx.destination);
      osc.start();
      osc.stop(ctx.currentTime + 0.5);
      setTestSoundPlaying(true);
      setTimeout(() => setTestSoundPlaying(false), 600);
    } catch {}
  };

  useEffect(() => {
    if (activeTab === 'wifi') {
      handleScanWifi();
    } else if (activeTab === 'bluetooth') {
      handleScanBluetooth();
    }
  }, [activeTab]);

  // Grouped Navigation Layout for Desktop Ergonomics
  const categoryGroups: CategoryGroup[] = [
    {
      groupName: 'SYSTEM & HARDWARE',
      items: [
        { id: 'displays', label: 'Displays', icon: <Monitor className="w-3.5 h-3.5" />, color: 'bg-[#007AFF]' },
        { id: 'power', label: 'Power & Battery', icon: <Battery className="w-3.5 h-3.5" />, color: 'bg-emerald-500' },
        { id: 'storage', label: 'Storage & Disks', icon: <HardDrive className="w-3.5 h-3.5" />, color: 'bg-purple-500' },
      ],
    },
    {
      groupName: 'DEVICES & PERIPHERALS',
      items: [
        { id: 'mouse-keyboard', label: 'Mouse & Keyboard', icon: <Mouse className="w-3.5 h-3.5" />, color: 'bg-indigo-500' },
        { id: 'sound', label: 'Sound & Audio', icon: <Volume2 className="w-3.5 h-3.5" />, color: 'bg-rose-500' },
        { id: 'printers', label: 'Printers & Scanners', icon: <Printer className="w-3.5 h-3.5" />, color: 'bg-slate-500' },
      ],
    },
    {
      groupName: 'NETWORK & INTERNET',
      items: [
        { id: 'wifi', label: 'Wi-Fi', icon: <Wifi className="w-3.5 h-3.5" />, color: 'bg-[#007AFF]' },
        { id: 'ethernet', label: 'Ethernet', icon: <Network className="w-3.5 h-3.5" />, color: 'bg-teal-500' },
        { id: 'bluetooth', label: 'Bluetooth', icon: <Bluetooth className="w-3.5 h-3.5" />, color: 'bg-blue-500' },
        { id: 'proxy', label: 'Proxy & VPN', icon: <Globe className="w-3.5 h-3.5" />, color: 'bg-cyan-600' },
      ],
    },
    {
      groupName: 'PERSONALIZATION',
      items: [
        { id: 'appearance', label: 'Appearance', icon: <Palette className="w-3.5 h-3.5" />, color: 'bg-pink-500' },
        { id: 'wallpaper', label: 'Wallpaper', icon: <ImageIcon className="w-3.5 h-3.5" />, color: 'bg-cyan-500' },
        { id: 'dock-workspace', label: 'Dock & Snapping', icon: <Layers className="w-3.5 h-3.5" />, color: 'bg-violet-600' },
      ],
    },
    {
      groupName: 'APPS & EXECUTION',
      items: [
        { id: 'default-apps', label: 'Default Apps', icon: <CheckCircle2 className="w-3.5 h-3.5" />, color: 'bg-amber-500' },
        { id: 'startup-apps', label: 'Startup Programs', icon: <Zap className="w-3.5 h-3.5" />, color: 'bg-orange-500' },
        { id: 'env-variables', label: 'Environment Vars', icon: <Terminal className="w-3.5 h-3.5" />, color: 'bg-slate-600', badge: 'Restart' },
      ],
    },
    {
      groupName: 'DEVELOPER & ADVANCED',
      items: [
        { id: 'kernel-drivers', label: 'Kernel & Hardware', icon: <Cpu className="w-3.5 h-3.5" />, color: 'bg-red-600' },
        { id: 'dev-tools', label: 'Developer Subsystem', icon: <Code className="w-3.5 h-3.5" />, color: 'bg-sky-600' },
        { id: 'general', label: 'General / About', icon: <Info className="w-3.5 h-3.5" />, color: 'bg-slate-400' },
      ],
    },
  ];

  const visibleResolutions = showAllResolutions ? EXTENDED_RESOLUTIONS : PRIMARY_RESOLUTIONS;

  return (
    <div className="flex h-full w-full bg-[#F5F5F7] dark:bg-[#1E1E1E] text-slate-900 dark:text-slate-100 select-none overflow-hidden font-sans">
      {/* ======================================================== */}
      {/* SIDEBAR (MASTER): Persistent Navigation                 */}
      {/* ======================================================== */}
      <div
        data-window-drag
        className="w-64 bg-white/70 dark:bg-slate-900/70 backdrop-blur-md border-r border-black/5 dark:border-white/10 p-3.5 flex flex-col gap-3 shrink-0"
      >
        {/* Top Header: Traffic Lights */}
        <div className="flex items-center justify-between pb-1">
          <div className="flex items-center space-x-2">
            <button
              onClick={() => currentWindow && closeWindow(currentWindow.id)}
              className="w-3 h-3 rounded-full bg-[#ff5f56] border border-[#e0443e] flex items-center justify-center group cursor-pointer transition-transform active:scale-90"
              title="Close"
            >
              <span className="text-[8px] font-black text-rose-950 opacity-0 group-hover:opacity-100 leading-none">
                ×
              </span>
            </button>
            <button
              onClick={() => currentWindow && minimizeWindow(currentWindow.id)}
              className="w-3 h-3 rounded-full bg-[#ffbd2e] border border-[#dea123] flex items-center justify-center group cursor-pointer transition-transform active:scale-90"
              title="Minimize"
            >
              <span className="text-[9px] font-black text-amber-950 opacity-0 group-hover:opacity-100 leading-none -translate-y-0.5">
                –
              </span>
            </button>
            <button
              onClick={() => currentWindow && toggleMaximizeWindow(currentWindow.id)}
              className="w-3 h-3 rounded-full bg-[#27c93f] border border-[#1aab29] flex items-center justify-center group cursor-pointer transition-transform active:scale-90"
              title="Zoom"
            >
              <span className="text-[7px] font-black text-emerald-950 opacity-0 group-hover:opacity-100 leading-none">
                +
              </span>
            </button>
          </div>
          <span className="text-[11px] font-semibold text-slate-400 dark:text-slate-500">Settings</span>
        </div>

        {/* Search Bar */}
        <div className="relative">
          <Search className="w-3.5 h-3.5 absolute left-2.5 top-2 text-slate-400" />
          <input
            type="text"
            placeholder="Search settings, hardware, or drivers..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="w-full pl-8 pr-3 py-1 bg-black/5 dark:bg-white/5 border border-black/5 dark:border-white/10 rounded-lg text-xs text-slate-800 dark:text-slate-200 placeholder-slate-400 outline-none focus:ring-1 focus:ring-[#007AFF] transition-all"
          />
        </div>

        {/* Grouped Sidebar Menu */}
        <div className="flex-1 overflow-y-auto pr-1 flex flex-col gap-3 text-xs">
          {categoryGroups.map((group) => {
            const visibleItems = group.items.filter((item) =>
              item.label.toLowerCase().includes(searchQuery.toLowerCase())
            );
            if (visibleItems.length === 0) return null;

            return (
              <div key={group.groupName} className="flex flex-col gap-0.5">
                <span className="px-2.5 py-1 text-[10px] font-bold text-slate-400 uppercase tracking-wider">
                  {group.groupName}
                </span>
                {visibleItems.map((item) => {
                  const isActive = activeTab === item.id;
                  return (
                    <button
                      key={item.id}
                      onClick={() => setActiveTab(item.id)}
                      className={`relative flex items-center justify-between px-2.5 py-1.5 rounded-lg transition-colors text-left ${
                        isActive
                          ? 'bg-[#007AFF] text-white font-medium shadow-xs'
                          : 'text-slate-700 dark:text-slate-300 hover:bg-black/5 dark:hover:bg-white/5'
                      }`}
                    >
                      <div className="flex items-center space-x-2.5 truncate">
                        <div
                          className={`w-5 h-5 rounded-md flex items-center justify-center text-white shrink-0 ${
                            isActive ? 'bg-white/20' : item.color
                          }`}
                        >
                          {item.icon}
                        </div>
                        <span className="truncate">{item.label}</span>
                      </div>
                      {item.badge && (
                        <span className={`text-[9px] px-1.5 py-0.2 rounded font-semibold ${
                          isActive ? 'bg-white/20 text-white' : 'bg-amber-500/10 text-amber-500 border border-amber-500/20'
                        }`}>
                          {item.badge}
                        </span>
                      )}
                    </button>
                  );
                })}
              </div>
            );
          })}
        </div>

        {/* Host Machine Identifier */}
        <div className="pt-2 border-t border-black/5 dark:border-white/10 flex items-center space-x-2.5 px-1">
          <div className="w-7 h-7 rounded-full bg-gradient-to-tr from-[#007AFF] to-blue-600 flex items-center justify-center text-white font-bold text-[10px]">
            AX
          </div>
          <div className="overflow-hidden">
            <div className="text-[11px] font-semibold truncate text-slate-800 dark:text-slate-200">
              {systemInfo.hostname}
            </div>
            <div className="text-[9px] text-slate-400">Linux 6.12 Kernel • udev active</div>
          </div>
        </div>
      </div>

      {/* ======================================================== */}
      {/* MAIN CONTENT AREA (DETAIL): Deep Configuration           */}
      {/* ======================================================== */}
      <div
        data-window-drag
        className="flex-1 flex flex-col overflow-y-auto bg-[#F5F5F7] dark:bg-[#1E1E1E] p-6 text-slate-800 dark:text-slate-200"
      >
        {/* ==================== DISPLAYS TAB ==================== */}
        {activeTab === 'displays' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Displays</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Configure monitor topology, hardware refresh rates, and Wayland desktop scaling.
              </p>
            </div>

            {/* Display Virtual Canvas Arrangement */}
            <div className="bg-slate-900 rounded-2xl p-6 border border-white/10 flex flex-col items-center justify-center min-h-[190px] relative shadow-inner">
              <div className="flex items-end gap-3 z-10">
                <button
                  onClick={() => setSelectedMonitor('built-in')}
                  className={`w-28 h-20 rounded-xl border flex flex-col items-center justify-center gap-1.5 transition-all ${
                    selectedMonitor === 'built-in'
                      ? 'border-[#007AFF] bg-blue-500/20 ring-2 ring-[#007AFF]/40 text-white'
                      : 'border-white/20 bg-white/5 text-slate-400 hover:text-white'
                  }`}
                >
                  <Laptop className="w-5 h-5" />
                  <span className="text-[10px] font-semibold">eDP-1 (Built-in)</span>
                </button>

                <button
                  onClick={() => setSelectedMonitor('external')}
                  className={`w-36 h-24 rounded-xl border flex flex-col items-center justify-center gap-1.5 transition-all ${
                    selectedMonitor === 'external'
                      ? 'border-[#007AFF] bg-blue-500/20 ring-2 ring-[#007AFF]/40 text-white'
                      : 'border-white/20 bg-white/5 text-slate-400 hover:text-white'
                  }`}
                >
                  <Monitor className="w-6 h-6" />
                  <span className="text-[11px] font-semibold">DP-1 (Primary 144Hz)</span>
                </button>
              </div>
              <div className="text-[10px] text-slate-400 mt-3 font-mono">
                DRM KMS Connector: /dev/dri/card0 • Wayland Output Active
              </div>
            </div>

            {/* Display Properties Card */}
            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-4">
              <div className="flex items-center justify-between pb-3 border-b border-black/5 dark:border-white/5">
                <span className="text-xs font-semibold">Display Arrangement Mode</span>
                <select
                  value={useAsMode}
                  onChange={(e) => setUseAsMode(e.target.value)}
                  className="bg-black/5 dark:bg-white/10 border border-black/10 dark:border-white/10 rounded-lg px-2.5 py-1 text-xs outline-none"
                >
                  <option>Extended display</option>
                  <option>Mirror for DP-1</option>
                  <option>Main display only</option>
                </select>
              </div>

              {/* Resolution Picker */}
              <div className="flex items-center justify-between pb-3 border-b border-black/5 dark:border-white/5">
                <div>
                  <div className="text-xs font-semibold">Resolution</div>
                  <div className="text-[10px] text-slate-400">Native panel timing configuration</div>
                </div>
                <select
                  value={selectedResolution}
                  onChange={(e) => setSelectedResolution(e.target.value)}
                  className="bg-black/5 dark:bg-white/10 border border-black/10 dark:border-white/10 rounded-lg px-2.5 py-1 text-xs outline-none"
                >
                  {visibleResolutions.map((res) => (
                    <option key={res.id} value={res.id}>{res.label}</option>
                  ))}
                </select>
              </div>

              {/* Refresh Rate */}
              <div className="flex items-center justify-between pb-3 border-b border-black/5 dark:border-white/5">
                <div>
                  <div className="text-xs font-semibold">Refresh Rate</div>
                  <div className="text-[10px] text-slate-400">Variable rate refresh & Adaptive Sync (VRR)</div>
                </div>
                <select
                  value={refreshRate}
                  onChange={(e) => setRefreshRate(e.target.value)}
                  className="bg-black/5 dark:bg-white/10 border border-black/10 dark:border-white/10 rounded-lg px-2.5 py-1 text-xs outline-none"
                >
                  <option value="60Hz">60.00 Hz (Standard)</option>
                  <option value="120Hz">120.00 Hz (ProMotion)</option>
                  <option value="144Hz">144.00 Hz (High Precision)</option>
                  <option value="240Hz">240.00 Hz (Ultra High Speed)</option>
                </select>
              </div>

              {/* UI Scaling with Restart Warning */}
              <div className="flex items-center justify-between">
                <div>
                  <div className="text-xs font-semibold flex items-center gap-1.5">
                    <span>UI Scaling</span>
                    <span className="text-[9px] px-1.5 py-0.5 rounded bg-amber-500/10 text-amber-500 font-semibold border border-amber-500/20 flex items-center gap-1">
                      <AlertTriangle className="w-2.5 h-2.5" />
                      Requires Session Restart
                    </span>
                  </div>
                  <div className="text-[10px] text-slate-400">DPI scaling for Wayland compositor</div>
                </div>
                <div className="flex items-center gap-1 bg-black/5 dark:bg-white/10 p-0.5 rounded-lg text-xs">
                  {['100%', '125%', '150%', '200%'].map((scale) => (
                    <button
                      key={scale}
                      onClick={() => setUiScaling(scale)}
                      className={`px-2 py-0.5 rounded-md transition-all ${
                        uiScaling === scale ? 'bg-[#007AFF] text-white font-semibold' : 'text-slate-500 hover:text-white'
                      }`}
                    >
                      {scale}
                    </button>
                  ))}
                </div>
              </div>
            </div>

            {/* Brightness Card */}
            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-3">
              <div className="text-xs font-semibold">Display Brightness</div>
              <div className="flex items-center gap-3">
                <Sun className="w-4 h-4 text-slate-400" />
                <input
                  type="range"
                  min="10"
                  max="100"
                  value={brightness}
                  onChange={(e) => setBrightness(Number(e.target.value))}
                  className="flex-1 h-2 bg-slate-200 dark:bg-slate-700 rounded-full appearance-none cursor-pointer accent-[#007AFF]"
                />
                <span className="text-xs font-mono w-8 text-right">{brightness}%</span>
              </div>
            </div>
          </div>
        )}

        {/* ==================== POWER & BATTERY TAB ==================== */}
        {activeTab === 'power' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Power & Battery</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Hardware ACPI power profiles, lid close behavior, and battery conservation.
              </p>
            </div>

            {/* Battery Status Platter */}
            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-4">
              <div className="flex items-center justify-between">
                <div>
                  <div className="text-3xl font-black text-slate-900 dark:text-white">
                    {realBatteryData?.capacity || '95'}%
                  </div>
                  <div className="text-xs text-slate-400 mt-0.5">
                    Kernel sysfs: /sys/class/power_supply/BAT0 • Status: {realBatteryData?.status || 'Good'}
                  </div>
                </div>
                <div className="w-12 h-12 rounded-2xl bg-emerald-500/20 text-emerald-500 flex items-center justify-center">
                  <Battery className="w-6 h-6" />
                </div>
              </div>

              <div className="w-full bg-slate-200 dark:bg-slate-700 h-2.5 rounded-full overflow-hidden">
                <div
                  className="bg-emerald-500 h-full rounded-full transition-all"
                  style={{ width: `${realBatteryData?.capacity || 95}%` }}
                />
              </div>
            </div>

            {/* Power Mode Selector */}
            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-4">
              <span className="text-xs font-semibold">Energy & CPU Frequency Governor</span>
              <div className="grid grid-cols-3 gap-3">
                {[
                  { id: 'saver', title: 'Power Saver', desc: 'Downclock CPU for maximum battery life' },
                  { id: 'balanced', title: 'Balanced (Standard)', desc: 'Dynamic CPU scaling via Intel/AMD driver' },
                  { id: 'performance', title: 'High Performance', desc: 'Disable CPU throttling for raw speed' },
                ].map((plan) => (
                  <button
                    key={plan.id}
                    onClick={() => setPowerPlan(plan.id as any)}
                    className={`p-3 rounded-xl border text-left flex flex-col justify-between transition-all ${
                      powerPlan === plan.id
                        ? 'border-[#007AFF] bg-blue-50/40 dark:bg-blue-950/20 ring-1 ring-[#007AFF]'
                        : 'border-black/5 dark:border-white/10 hover:bg-black/5 dark:hover:bg-white/5'
                    }`}
                  >
                    <div>
                      <div className="text-xs font-bold">{plan.title}</div>
                      <div className="text-[10px] text-slate-400 mt-1">{plan.desc}</div>
                    </div>
                    {powerPlan === plan.id && <Check className="w-4 h-4 text-[#007AFF] mt-2 self-end" />}
                  </button>
                ))}
              </div>

              {/* Hardware Actions */}
              <div className="pt-3 border-t border-black/5 dark:border-white/5 flex flex-col gap-3">
                <div className="flex items-center justify-between text-xs">
                  <span>When Laptop Lid Closes</span>
                  <select
                    value={lidCloseAction}
                    onChange={(e) => setLidCloseAction(e.target.value)}
                    className="bg-black/5 dark:bg-white/10 border border-black/10 dark:border-white/10 rounded-lg px-2.5 py-1 text-xs outline-none"
                  >
                    <option>Sleep</option>
                    <option>Turn off display</option>
                    <option>Do nothing</option>
                  </select>
                </div>

                <div className="flex items-center justify-between text-xs">
                  <span>When Physical Power Button Pressed</span>
                  <select
                    value={powerButtonAction}
                    onChange={(e) => setPowerButtonAction(e.target.value)}
                    className="bg-black/5 dark:bg-white/10 border border-black/10 dark:border-white/10 rounded-lg px-2.5 py-1 text-xs outline-none"
                  >
                    <option>Shut down</option>
                    <option>Sleep</option>
                    <option>Prompt confirmation modal</option>
                  </select>
                </div>
              </div>
            </div>
          </div>
        )}

        {/* ==================== STORAGE & DISKS TAB ==================== */}
        {activeTab === 'storage' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Storage & Disks</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Physical NVMe, SATA SSDs, Btrfs subvolumes, and Linux mount points.
              </p>
            </div>

            {/* Drives List */}
            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-4">
              <span className="text-xs font-semibold uppercase tracking-wider text-slate-400">
                Connected Block Devices
              </span>
              <div className="flex flex-col divide-y divide-black/5 dark:divide-white/5">
                {[
                  { name: 'NVMe Solid State Drive', id: '/dev/nvme0n1', size: '256 GB', free: '210 GB free', type: 'Btrfs Root (/)', status: 'Healthy' },
                  { name: 'SATA Secondary Storage', id: '/dev/sda', size: '1000 GB', free: '750 GB free', type: 'ext4 Data (/mnt/data)', status: 'Mounted' },
                ].map((disk) => (
                  <div key={disk.id} className="py-3 flex items-center justify-between text-xs">
                    <div className="flex items-center space-x-3">
                      <div className="w-9 h-9 rounded-xl bg-purple-500/10 text-purple-400 flex items-center justify-center">
                        <HardDrive className="w-5 h-5" />
                      </div>
                      <div>
                        <div className="font-semibold text-slate-900 dark:text-white">{disk.name}</div>
                        <div className="text-[10px] text-slate-400 font-mono">
                          {disk.id} • {disk.type} • {disk.free}
                        </div>
                      </div>
                    </div>
                    <span className="px-2 py-0.5 rounded-full bg-emerald-500/10 text-emerald-400 text-[10px] font-semibold">
                      {disk.status}
                    </span>
                  </div>
                ))}
              </div>
            </div>

            {/* Progressive Disclosure: Advanced TRIM & Filesystem parameters */}
            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-4">
              <button
                onClick={() => setShowAdvancedDriverSettings(!showAdvancedDriverSettings)}
                className="w-full flex items-center justify-between text-xs font-semibold text-slate-700 dark:text-slate-300"
              >
                <span>Advanced Storage Maintenance & TRIM</span>
                <ChevronRight className={`w-4 h-4 transition-transform ${showAdvancedDriverSettings ? 'rotate-90' : ''}`} />
              </button>

              {showAdvancedDriverSettings && (
                <div className="pt-3 mt-3 border-t border-black/5 dark:border-white/5 flex flex-col gap-2.5 text-xs text-slate-500">
                  <div className="flex justify-between items-center">
                    <span>Periodic SSD TRIM (fstrim.timer)</span>
                    <span className="font-semibold text-emerald-500">Enabled (Weekly)</span>
                  </div>
                  <div className="flex justify-between items-center">
                    <span>Btrfs Transparent Compression</span>
                    <span className="font-mono text-slate-700 dark:text-slate-300">zstd:1</span>
                  </div>
                </div>
              )}
            </div>
          </div>
        )}

        {/* ==================== MOUSE & KEYBOARD TAB ==================== */}
        {activeTab === 'mouse-keyboard' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Mouse & Keyboard</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Precision tracking, hardware acceleration curves, and input keymaps.
              </p>
            </div>

            {/* Mouse Tracking */}
            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-4">
              <span className="text-xs font-semibold">Pointer & Scrolling</span>
              <div className="flex items-center justify-between text-xs">
                <span>Tracking Speed</span>
                <input
                  type="range"
                  min="1"
                  max="10"
                  value={mouseSpeed}
                  onChange={(e) => setMouseSpeed(Number(e.target.value))}
                  className="w-48 h-2 bg-slate-200 dark:bg-slate-700 rounded-full appearance-none cursor-pointer accent-[#007AFF]"
                />
              </div>

              <div className="flex items-center justify-between text-xs pt-2 border-t border-black/5 dark:border-white/5">
                <div>
                  <div className="font-medium">Pointer Acceleration (Libinput Profile)</div>
                  <div className="text-[10px] text-slate-400">Adapts pointer movement based on physical velocity</div>
                </div>
                <input
                  type="checkbox"
                  checked={mouseAcceleration}
                  onChange={(e) => setMouseAcceleration(e.target.checked)}
                  className="w-4 h-4 accent-[#007AFF]"
                />
              </div>

              <div className="flex items-center justify-between text-xs pt-2 border-t border-black/5 dark:border-white/5">
                <span>Scroll Wheel Lines</span>
                <select
                  value={scrollLines}
                  onChange={(e) => setScrollLines(Number(e.target.value))}
                  className="bg-black/5 dark:bg-white/10 border border-black/10 dark:border-white/10 rounded-lg px-2.5 py-1 text-xs outline-none"
                >
                  <option value={1}>1 line</option>
                  <option value={3}>3 lines (Standard)</option>
                  <option value={6}>6 lines</option>
                  <option value={10}>10 lines</option>
                </select>
              </div>
            </div>

            {/* Keyboard Layout */}
            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-4">
              <span className="text-xs font-semibold">Keyboard Layout & Input Subsystem</span>
              <div className="flex items-center justify-between text-xs">
                <span>Active Keymap</span>
                <select
                  value={keyboardLayout}
                  onChange={(e) => setKeyboardLayout(e.target.value)}
                  className="bg-black/5 dark:bg-white/10 border border-black/10 dark:border-white/10 rounded-lg px-2.5 py-1 text-xs outline-none"
                >
                  <option>English (US)</option>
                  <option>English (UK)</option>
                  <option>German (QWERTZ)</option>
                  <option>French (AZERTY)</option>
                </select>
              </div>
            </div>
          </div>
        )}

        {/* ==================== SOUND TAB ==================== */}
        {activeTab === 'sound' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Sound & Audio</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Output, input, and PipeWire / WirePlumber professional DAC routing.
              </p>
            </div>

            {/* Volume Master Slider */}
            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-4">
              <div className="flex items-center justify-between">
                <div>
                  <div className="text-sm font-semibold text-slate-900 dark:text-white">Output Volume</div>
                  <div className="text-xs text-slate-400">PipeWire 1.2 / ALSA High Definition Audio</div>
                </div>
                <button
                  onClick={playSoundChime}
                  className="px-3 py-1 bg-[#007AFF]/10 hover:bg-[#007AFF]/20 text-[#007AFF] dark:text-blue-400 rounded-lg text-xs font-semibold transition-colors flex items-center space-x-1"
                >
                  <Speaker className={`w-3.5 h-3.5 ${testSoundPlaying ? 'animate-bounce' : ''}`} />
                  <span>{testSoundPlaying ? 'Playing Chime...' : 'Test Sound'}</span>
                </button>
              </div>

              <div className="flex items-center gap-3">
                <button onClick={toggleMute} className="p-1 rounded text-slate-500 hover:text-white">
                  {isMuted || volume === 0 ? <VolumeX className="w-5 h-5 text-rose-500" /> : <Volume2 className="w-5 h-5 text-[#007AFF]" />}
                </button>
                <input
                  type="range"
                  min="0"
                  max="100"
                  value={isMuted ? 0 : volume}
                  onChange={(e) => changeVolume(Number(e.target.value))}
                  className="flex-1 h-2 bg-slate-200 dark:bg-slate-700 rounded-full appearance-none cursor-pointer accent-[#007AFF]"
                />
                <span className="font-mono text-xs w-8 text-right font-medium">
                  {isMuted ? '0%' : `${volume}%`}
                </span>
              </div>

              {/* Output Device Selector */}
              <div className="pt-3 border-t border-black/5 dark:border-white/5 flex flex-col gap-2">
                <span className="text-xs font-semibold text-slate-500 uppercase tracking-wider text-[11px]">Output Audio Sink</span>
                {['Built-in Audio Analog Stereo (Speakers)', 'Headphones (3.5mm Analog Audio Out)', 'HDMI / DisplayPort Digital Stereo'].map((dev) => (
                  <button
                    key={dev}
                    onClick={() => setSelectedAudioOutput(dev)}
                    className={`p-2.5 rounded-lg border text-left flex items-center justify-between text-xs transition-colors ${
                      selectedAudioOutput === dev
                        ? 'border-[#007AFF] bg-blue-50/50 dark:bg-blue-950/20 font-medium'
                        : 'border-black/5 dark:border-white/5 hover:bg-black/5 dark:hover:bg-white/5'
                    }`}
                  >
                    <div className="flex items-center space-x-2">
                      <Speaker className="w-4 h-4 text-slate-500" />
                      <span>{dev}</span>
                    </div>
                    {selectedAudioOutput === dev && <Check className="w-4 h-4 text-[#007AFF]" />}
                  </button>
                ))}
              </div>
            </div>

            {/* Progressive Disclosure: Professional Audio DAC settings */}
            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-4">
              <button
                onClick={() => setShowAdvancedAudioSettings(!showAdvancedAudioSettings)}
                className="w-full flex items-center justify-between text-xs font-semibold text-slate-700 dark:text-slate-300"
              >
                <span>Professional Audio & Latency Buffer (PipeWire)</span>
                <ChevronRight className={`w-4 h-4 transition-transform ${showAdvancedAudioSettings ? 'rotate-90' : ''}`} />
              </button>

              {showAdvancedAudioSettings && (
                <div className="pt-3 mt-3 border-t border-black/5 dark:border-white/5 flex flex-col gap-3 text-xs">
                  <div className="flex justify-between items-center">
                    <span>Sample Rate</span>
                    <select
                      value={audioSampleRate}
                      onChange={(e) => setAudioSampleRate(e.target.value)}
                      className="bg-black/5 dark:bg-white/10 border border-black/10 dark:border-white/10 rounded-lg px-2 py-0.5 outline-none"
                    >
                      <option>44,100 Hz (CD Quality)</option>
                      <option>48,000 Hz (Standard)</option>
                      <option>96,000 Hz (High Resolution Studio)</option>
                    </select>
                  </div>
                  <div className="flex justify-between items-center">
                    <span>Quantum / Buffer Size</span>
                    <select
                      value={audioBufferSize}
                      onChange={(e) => setAudioBufferSize(e.target.value)}
                      className="bg-black/5 dark:bg-white/10 border border-black/10 dark:border-white/10 rounded-lg px-2 py-0.5 outline-none"
                    >
                      <option>64 samples (1.3 ms Low Latency)</option>
                      <option>128 samples (2.6 ms)</option>
                      <option>256 samples (5.3 ms)</option>
                      <option>512 samples (10.6 ms Stable)</option>
                    </select>
                  </div>
                </div>
              )}
            </div>
          </div>
        )}

        {/* ==================== WI-FI TAB ==================== */}
        {activeTab === 'wifi' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div className="flex items-center justify-between">
              <div>
                <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Wi-Fi</h1>
                <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                  Manage wireless interfaces, connection profiles, and network security.
                </p>
              </div>
              <button
                onClick={handleScanWifi}
                disabled={isScanningWifi || !wifiConnected}
                className="p-1.5 rounded-lg bg-black/5 dark:bg-white/5 hover:bg-black/10 dark:hover:bg-white/10 text-slate-600 dark:text-slate-300 transition-colors flex items-center gap-1.5 text-xs font-medium"
              >
                <RefreshCw className={`w-3.5 h-3.5 ${isScanningWifi ? 'animate-spin text-[#007AFF]' : ''}`} />
                <span>Scan</span>
              </button>
            </div>

            {/* Toggle Card */}
            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-4 flex items-center justify-between">
              <div className="flex items-center space-x-3">
                <div className={`w-9 h-9 rounded-xl flex items-center justify-center text-white ${wifiConnected ? 'bg-[#007AFF]' : 'bg-slate-400'}`}>
                  {wifiConnected ? <Wifi className="w-5 h-5" /> : <WifiOff className="w-5 h-5" />}
                </div>
                <div>
                  <div className="text-sm font-semibold text-slate-900 dark:text-white">Wi-Fi</div>
                  <div className="text-xs text-slate-500 dark:text-slate-400">
                    {wifiConnected ? `Connected to ${wifiSsid}` : 'Disconnected'}
                  </div>
                </div>
              </div>

              <button
                onClick={toggleWifi}
                className={`w-12 h-6.5 rounded-full transition-colors relative p-0.5 ${
                  wifiConnected ? 'bg-[#34C759]' : 'bg-slate-300 dark:bg-slate-600'
                }`}
              >
                <div
                  className={`w-5.5 h-5.5 rounded-full bg-white shadow-md transition-transform ${
                    wifiConnected ? 'translate-x-5.5' : 'translate-x-0'
                  }`}
                />
              </button>
            </div>

            {/* Password Modal */}
            {wifiPasswordPrompt && (
              <div className="bg-blue-50/80 dark:bg-blue-950/40 border border-blue-200 dark:border-blue-800 rounded-xl p-4 flex flex-col gap-3">
                <div className="flex items-center space-x-2 text-xs font-semibold text-blue-900 dark:text-blue-200">
                  <Key className="w-4 h-4 text-[#007AFF]" />
                  <span>Enter password for "{wifiPasswordPrompt}"</span>
                </div>
                <div className="flex gap-2">
                  <input
                    type="password"
                    placeholder="WPA2/WPA3 Pre-Shared Key"
                    value={wifiPasswordInput}
                    onChange={(e) => setWifiPasswordInput(e.target.value)}
                    className="flex-1 px-3 py-1.5 bg-white dark:bg-[#1E1E1E] border border-blue-300 dark:border-blue-700 rounded-lg text-xs outline-none focus:ring-1 focus:ring-[#007AFF]"
                  />
                  <button
                    onClick={() => handleConnectWifi(wifiPasswordPrompt, wifiPasswordInput)}
                    disabled={connectingSsid === wifiPasswordPrompt}
                    className="px-3 py-1.5 bg-[#007AFF] text-white rounded-lg text-xs font-semibold hover:bg-blue-600 transition-colors"
                  >
                    {connectingSsid === wifiPasswordPrompt ? 'Connecting...' : 'Join'}
                  </button>
                  <button
                    onClick={() => setWifiPasswordPrompt(null)}
                    className="px-3 py-1.5 bg-black/5 dark:bg-white/10 rounded-lg text-xs text-slate-600 dark:text-slate-300"
                  >
                    Cancel
                  </button>
                </div>
              </div>
            )}

            {/* Network List */}
            {wifiConnected && (
              <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-4 flex flex-col gap-3">
                <div className="text-xs font-semibold text-slate-500 uppercase tracking-wider">
                  Available Networks
                </div>
                <div className="flex flex-col divide-y divide-black/5 dark:divide-white/5">
                  {wifiNetworks.map((net) => {
                    const isCurrent = net.ssid === wifiSsid;
                    return (
                      <div
                        key={net.ssid}
                        className="py-2.5 flex items-center justify-between text-xs hover:bg-black/5 dark:hover:bg-white/5 px-2 rounded-lg transition-colors"
                      >
                        <div className="flex items-center space-x-2.5">
                          <Wifi className={`w-4 h-4 ${isCurrent ? 'text-[#007AFF]' : 'text-slate-400'}`} />
                          <div className="flex flex-col">
                            <span className="font-semibold text-slate-800 dark:text-slate-200">{net.ssid}</span>
                            <span className="text-[10px] text-slate-400">{net.security} • Signal: {net.signal}%</span>
                          </div>
                        </div>

                        <div className="flex items-center space-x-2">
                          {net.security !== 'Open' && <Lock className="w-3.5 h-3.5 text-slate-400" />}
                          {isCurrent ? (
                            <span className="flex items-center space-x-1 text-[11px] font-semibold text-[#34C759]">
                              <Check className="w-3.5 h-3.5" />
                              <span>Connected</span>
                            </span>
                          ) : (
                            <button
                              onClick={() => {
                                if (net.security === 'Open') {
                                  handleConnectWifi(net.ssid);
                                } else {
                                  setWifiPasswordPrompt(net.ssid);
                                }
                              }}
                              disabled={connectingSsid === net.ssid}
                              className="px-2.5 py-1 bg-[#007AFF]/10 hover:bg-[#007AFF]/20 text-[#007AFF] dark:text-blue-400 rounded-md text-[11px] font-medium transition-colors"
                            >
                              {connectingSsid === net.ssid ? 'Connecting...' : 'Connect'}
                            </button>
                          )}
                        </div>
                      </div>
                    );
                  })}
                </div>
              </div>
            )}
          </div>
        )}

        {/* ==================== ETHERNET TAB ==================== */}
        {activeTab === 'ethernet' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Ethernet</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Gigabit wired network interfaces, IPv4/IPv6 address assignments, and MTU.
              </p>
            </div>

            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-4">
              <div className="flex items-center justify-between pb-3 border-b border-black/5 dark:border-white/5">
                <div className="flex items-center space-x-3">
                  <div className="w-9 h-9 rounded-xl bg-teal-500 text-white flex items-center justify-center">
                    <Network className="w-5 h-5" />
                  </div>
                  <div>
                    <div className="text-sm font-semibold">eth0 (PCI Realtek Gigabit)</div>
                    <div className="text-xs text-slate-400">Connected • 1000 Mbps Full Duplex</div>
                  </div>
                </div>
                <span className="px-2.5 py-1 rounded-full bg-emerald-500/10 text-emerald-400 text-xs font-semibold">
                  Active (Carrier)
                </span>
              </div>

              <div className="grid grid-cols-2 gap-4 text-xs">
                <div>
                  <span className="text-slate-400">IP Address (DHCP)</span>
                  <div className="font-mono font-semibold mt-0.5">192.168.1.145</div>
                </div>
                <div>
                  <span className="text-slate-400">Subnet Mask</span>
                  <div className="font-mono font-semibold mt-0.5">255.255.255.0 (/24)</div>
                </div>
                <div>
                  <span className="text-slate-400">Gateway</span>
                  <div className="font-mono font-semibold mt-0.5">192.168.1.1</div>
                </div>
                <div>
                  <span className="text-slate-400">Hardware MAC</span>
                  <div className="font-mono font-semibold mt-0.5">00:1A:2B:3C:4D:5E</div>
                </div>
              </div>
            </div>

            {/* Progressive Disclosure: MAC Randomization & IPv6 */}
            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-4">
              <button
                onClick={() => setShowAdvancedEthernetSettings(!showAdvancedEthernetSettings)}
                className="w-full flex items-center justify-between text-xs font-semibold text-slate-700 dark:text-slate-300"
              >
                <span>Advanced Protocol & Privacy Settings</span>
                <ChevronRight className={`w-4 h-4 transition-transform ${showAdvancedEthernetSettings ? 'rotate-90' : ''}`} />
              </button>

              {showAdvancedEthernetSettings && (
                <div className="pt-3 mt-3 border-t border-black/5 dark:border-white/5 flex flex-col gap-3 text-xs">
                  <div className="flex justify-between items-center">
                    <div>
                      <div className="font-medium">MAC Address Randomization (Privacy)</div>
                      <div className="text-[10px] text-slate-400">Spoofs a random MAC address upon each connection</div>
                    </div>
                    <input
                      type="checkbox"
                      checked={macSpoofing}
                      onChange={(e) => setMacSpoofing(e.target.checked)}
                      className="w-4 h-4 accent-[#007AFF]"
                    />
                  </div>
                  <div className="flex justify-between items-center">
                    <span>IPv6 Networking Protocol</span>
                    <input
                      type="checkbox"
                      checked={ipv6Enabled}
                      onChange={(e) => setIpv6Enabled(e.target.checked)}
                      className="w-4 h-4 accent-[#007AFF]"
                    />
                  </div>
                </div>
              )}
            </div>
          </div>
        )}

        {/* ==================== BLUETOOTH TAB ==================== */}
        {activeTab === 'bluetooth' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div className="flex items-center justify-between">
              <div>
                <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Bluetooth</h1>
                <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                  Pair accessories, keyboards, trackpads, and wireless audio devices.
                </p>
              </div>
              <button
                onClick={handleScanBluetooth}
                disabled={isScanningBt || !bluetoothEnabled}
                className="p-1.5 rounded-lg bg-black/5 dark:bg-white/5 hover:bg-black/10 dark:hover:bg-white/10 text-slate-600 dark:text-slate-300 transition-colors flex items-center gap-1.5 text-xs font-medium"
              >
                <RefreshCw className={`w-3.5 h-3.5 ${isScanningBt ? 'animate-spin text-[#007AFF]' : ''}`} />
                <span>Scan</span>
              </button>
            </div>

            {/* Toggle Card */}
            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-4 flex items-center justify-between">
              <div className="flex items-center space-x-3">
                <div className={`w-9 h-9 rounded-xl flex items-center justify-center text-white ${bluetoothEnabled ? 'bg-[#007AFF]' : 'bg-slate-400'}`}>
                  <Bluetooth className="w-5 h-5" />
                </div>
                <div>
                  <div className="text-sm font-semibold text-slate-900 dark:text-white">Bluetooth</div>
                  <div className="text-xs text-slate-500 dark:text-slate-400">
                    {bluetoothEnabled ? `Discoverable as ${systemInfo.hostname}` : 'Off'}
                  </div>
                </div>
              </div>

              <button
                onClick={toggleBluetooth}
                className={`w-12 h-6.5 rounded-full transition-colors relative p-0.5 ${
                  bluetoothEnabled ? 'bg-[#34C759]' : 'bg-slate-300 dark:bg-slate-600'
                }`}
              >
                <div
                  className={`w-5.5 h-5.5 rounded-full bg-white shadow-md transition-transform ${
                    bluetoothEnabled ? 'translate-x-5.5' : 'translate-x-0'
                  }`}
                />
              </button>
            </div>

            {/* Devices List */}
            {bluetoothEnabled && (
              <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-4 flex flex-col gap-3">
                <div className="text-xs font-semibold text-slate-500 uppercase tracking-wider">
                  Paired & Discovered Devices
                </div>
                <div className="flex flex-col divide-y divide-black/5 dark:divide-white/5">
                  {bluetoothDevices.map((dev) => (
                    <div
                      key={dev.mac}
                      className="py-2.5 flex items-center justify-between text-xs hover:bg-black/5 dark:hover:bg-white/5 px-2 rounded-lg transition-colors"
                    >
                      <div className="flex items-center space-x-2.5">
                        <div className="w-7 h-7 rounded-lg bg-black/5 dark:bg-white/5 flex items-center justify-center text-slate-600 dark:text-slate-300">
                          {dev.name.toLowerCase().includes('pod') || dev.name.toLowerCase().includes('head') ? (
                            <Headphones className="w-4 h-4 text-[#007AFF]" />
                          ) : (
                            <Radio className="w-4 h-4 text-slate-400" />
                          )}
                        </div>
                        <div className="flex flex-col">
                          <span className="font-semibold text-slate-800 dark:text-slate-200">{dev.name}</span>
                          <span className="text-[10px] text-slate-400 font-mono">{dev.mac}</span>
                        </div>
                      </div>

                      <div className="flex items-center space-x-2">
                        {dev.connected ? (
                          <span className="text-[11px] font-semibold text-[#34C759]">Connected</span>
                        ) : (
                          <span className="text-[11px] text-slate-400">Not Connected</span>
                        )}
                        <button
                          onClick={() => handleConnectBluetooth(dev.mac)}
                          disabled={btConnectingMac === dev.mac}
                          className="px-2.5 py-1 bg-black/5 dark:bg-white/10 hover:bg-black/10 dark:hover:bg-white/15 rounded-md text-[11px] font-medium transition-colors"
                        >
                          {btConnectingMac === dev.mac ? '...' : (dev.connected ? 'Disconnect' : 'Connect')}
                        </button>
                      </div>
                    </div>
                  ))}
                </div>
              </div>
            )}
          </div>
        )}

        {/* ==================== APPEARANCE TAB ==================== */}
        {activeTab === 'appearance' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Appearance</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Desktop themes, accent palettes, and frosted glass window effects.
              </p>
            </div>

            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-4">
              <span className="text-xs font-semibold">Theme (Instant Application)</span>
              <div className="grid grid-cols-2 gap-3">
                <button
                  onClick={() => setTheme('light')}
                  className={`p-3 rounded-xl border flex items-center justify-between transition-all ${
                    theme === 'light' ? 'border-[#007AFF] ring-2 ring-[#007AFF]/30 bg-blue-50/50' : 'border-slate-200 dark:border-slate-700'
                  }`}
                >
                  <div className="flex items-center space-x-2.5">
                    <Sun className="w-5 h-5 text-amber-500" />
                    <span className="text-xs font-semibold">Light Mode</span>
                  </div>
                  {theme === 'light' && <Check className="w-4 h-4 text-[#007AFF]" />}
                </button>

                <button
                  onClick={() => setTheme('dark')}
                  className={`p-3 rounded-xl border flex items-center justify-between transition-all ${
                    theme === 'dark' ? 'border-[#007AFF] ring-2 ring-[#007AFF]/30 bg-blue-50/10' : 'border-slate-200 dark:border-slate-700'
                  }`}
                >
                  <div className="flex items-center space-x-2.5">
                    <Moon className="w-5 h-5 text-blue-400" />
                    <span className="text-xs font-semibold">Dark Mode</span>
                  </div>
                  {theme === 'dark' && <Check className="w-4 h-4 text-[#007AFF]" />}
                </button>
              </div>

              {/* Accent Color Palette */}
              <div className="pt-3 border-t border-black/5 dark:border-white/5 flex flex-col gap-2">
                <span className="text-xs font-semibold">Accent Color</span>
                <div className="flex items-center gap-3">
                  {(['blue', 'cyan', 'purple', 'emerald', 'amber', 'rose'] as const).map((col) => (
                    <button
                      key={col}
                      onClick={() => setAccentColor(col)}
                      className={`w-7 h-7 rounded-full flex items-center justify-center transition-transform hover:scale-110 ${
                        col === 'blue' ? 'bg-[#007AFF]' :
                        col === 'cyan' ? 'bg-cyan-500' :
                        col === 'purple' ? 'bg-purple-500' :
                        col === 'emerald' ? 'bg-emerald-500' :
                        col === 'amber' ? 'bg-amber-500' : 'bg-rose-500'
                      }`}
                    >
                      {accentColor === col && <Check className="w-3.5 h-3.5 text-white" />}
                    </button>
                  ))}
                </div>
              </div>
            </div>
          </div>
        )}

        {/* ==================== WALLPAPER TAB ==================== */}
        {activeTab === 'wallpaper' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Wallpaper</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Dynamic desktop gradients, high-resolution photography, and multi-monitor spanning.
              </p>
            </div>

            <div className="grid grid-cols-2 sm:grid-cols-3 gap-3.5">
              {WALLPAPERS.map((wp) => (
                <button
                  key={wp.id}
                  onClick={() => setWallpaper(wp)}
                  className={`h-28 rounded-2xl border p-3 flex flex-col justify-end text-left relative overflow-hidden transition-all shadow-md ${
                    wallpaper.id === wp.id ? 'border-[#007AFF] ring-2 ring-[#007AFF]/60 scale-[1.02]' : 'border-black/10'
                  }`}
                  style={{ background: wp.gradient }}
                >
                  <span className="text-xs font-bold text-white drop-shadow">{wp.name}</span>
                  {wallpaper.id === wp.id && (
                    <div className="absolute top-2 right-2 w-5 h-5 rounded-full bg-[#007AFF] flex items-center justify-center text-white shadow">
                      <Check className="w-3 h-3" />
                    </div>
                  )}
                </button>
              ))}
            </div>
          </div>
        )}

        {/* ==================== DOCK & WORKSPACE TAB ==================== */}
        {activeTab === 'dock-workspace' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Dock & Workspace</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Dock positioning, magnification, window snapping, and virtual workspace boundaries.
              </p>
            </div>

            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-4">
              <div className="flex items-center justify-between pb-3 border-b border-black/5 dark:border-white/5 text-xs">
                <span>Dock Screen Position</span>
                <div className="flex gap-1 bg-black/5 dark:bg-white/10 p-0.5 rounded-lg">
                  {(['bottom', 'left', 'right'] as const).map((pos) => (
                    <button
                      key={pos}
                      onClick={() => setDockPosition(pos)}
                      className={`px-3 py-1 rounded-md capitalize transition-colors ${
                        dockPosition === pos ? 'bg-[#007AFF] text-white font-semibold' : 'text-slate-500 hover:text-white'
                      }`}
                    >
                      {pos}
                    </button>
                  ))}
                </div>
              </div>

              <div className="flex items-center justify-between pb-3 border-b border-black/5 dark:border-white/5 text-xs">
                <div>
                  <div className="font-semibold">Automatically Hide and Show Dock</div>
                  <div className="text-[10px] text-slate-400">Reveals the dock when hovering at screen edge</div>
                </div>
                <input
                  type="checkbox"
                  checked={dockAutoHide}
                  onChange={(e) => setDockAutoHide(e.target.checked)}
                  className="w-4 h-4 accent-[#007AFF]"
                />
              </div>

              <div className="flex items-center justify-between text-xs">
                <div>
                  <div className="font-semibold">Aero & Tiling Window Snapping</div>
                  <div className="text-[10px] text-slate-400">Snap windows to halves or quadrants when dragged to edge</div>
                </div>
                <input
                  type="checkbox"
                  checked={windowSnapping}
                  onChange={(e) => setWindowSnapping(e.target.checked)}
                  className="w-4 h-4 accent-[#007AFF]"
                />
              </div>
            </div>
          </div>
        )}

        {/* ==================== DEFAULT APPS TAB ==================== */}
        {activeTab === 'default-apps' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Default Applications</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Map file types, protocol handlers, and core desktop activities to native apps.
              </p>
            </div>

            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-3.5 text-xs">
              <div className="flex items-center justify-between">
                <span>Web Browser (.html, .htm, http/https)</span>
                <select
                  value={defaultBrowser}
                  onChange={(e) => setDefaultBrowser(e.target.value)}
                  className="bg-black/5 dark:bg-white/10 border border-black/10 dark:border-white/10 rounded-lg px-2.5 py-1 text-xs outline-none"
                >
                  <option>Axis Browser (Chromium)</option>
                  <option>VSCodium Integrated Browser</option>
                </select>
              </div>

              <div className="flex items-center justify-between">
                <span>Text & Source Editor (.txt, .md, .cpp, .py)</span>
                <select
                  value={defaultEditor}
                  onChange={(e) => setDefaultEditor(e.target.value)}
                  className="bg-black/5 dark:bg-white/10 border border-black/10 dark:border-white/10 rounded-lg px-2.5 py-1 text-xs outline-none"
                >
                  <option>TextEdit</option>
                  <option>VSCodium / Code</option>
                  <option>Neovim (Terminal)</option>
                </select>
              </div>

              <div className="flex items-center justify-between">
                <span>Media Player (.mp3, .wav, .mp4, .mkv)</span>
                <select className="bg-black/5 dark:bg-white/10 border border-black/10 dark:border-white/10 rounded-lg px-2.5 py-1 text-xs outline-none">
                  <option>Music Player</option>
                  <option>VLC Media Player</option>
                </select>
              </div>
            </div>
          </div>
        )}

        {/* ==================== ENVIRONMENT VARIABLES TAB ==================== */}
        {activeTab === 'env-variables' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <div className="flex items-center gap-2">
                <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Environment Variables</h1>
                <span className="px-2 py-0.5 rounded-full bg-amber-500/10 text-amber-500 text-[10px] font-bold border border-amber-500/20">
                  Requires Session Logout
                </span>
              </div>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                POSIX system and session environment variables configured in /etc/environment and ~/.profile.
              </p>
            </div>

            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-3 font-mono text-xs">
              {envVars.map(([k, v], i) => (
                <div key={k} className="flex items-center gap-3">
                  <span className="w-40 font-bold text-cyan-500 truncate">{k}</span>
                  <input
                    type="text"
                    value={v}
                    onChange={(e) => {
                      const copy = [...envVars];
                      copy[i][1] = e.target.value;
                      setEnvVars(copy);
                    }}
                    className="flex-1 px-2.5 py-1 bg-black/5 dark:bg-white/5 border border-black/10 dark:border-white/10 rounded-lg text-slate-800 dark:text-slate-200 outline-none"
                  />
                </div>
              ))}
            </div>
          </div>
        )}

        {/* ==================== KERNEL & HARDWARE DRIVERS TAB ==================== */}
        {activeTab === 'kernel-drivers' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Linux Kernel & Device Model</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Kernel module abstractions, sysfs hardware tree, and PCIe/ACPI device drivers.
              </p>
            </div>

            {/* Kernel Summary Card */}
            <div className="bg-slate-900 text-white rounded-2xl p-5 border border-white/10 flex items-center justify-between">
              <div>
                <div className="text-sm font-bold font-mono text-cyan-400">Linux 6.12.0-axisos-amd64</div>
                <div className="text-xs text-slate-400 mt-0.5">
                  Device Tree & ACPI 6.5 Tables Active • Threaded IRQs • Preempt-RT
                </div>
              </div>
              <Cpu className="w-8 h-8 text-cyan-400" />
            </div>

            {/* Loaded Drivers (lsmod) */}
            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-3">
              <span className="text-xs font-semibold uppercase tracking-wider text-slate-400">
                Active Kernel Device Drivers (udev / Linux Device Model)
              </span>
              <div className="flex flex-col divide-y divide-black/5 dark:divide-white/5 font-mono text-xs">
                {[
                  { module: 'axis_hw_driver', type: 'PCIe Accelerator / DMA', state: 'Loaded (Live)' },
                  { module: 'iwlwifi', type: 'Intel Wireless 802.11ax', state: 'Loaded (Live)' },
                  { module: 'snd_sof_pci', type: 'Intel Sound Open Firmware', state: 'Loaded (Live)' },
                  { module: 'btusb / bluez', type: 'Bluetooth HCI Controller', state: 'Loaded (Live)' },
                  { module: 'i915 / amdgpu', type: 'Direct Rendering Manager (KMS)', state: 'Loaded (Live)' },
                ].map((mod) => (
                  <div key={mod.module} className="py-2.5 flex items-center justify-between">
                    <div>
                      <span className="font-bold text-slate-800 dark:text-slate-200">{mod.module}</span>
                      <div className="text-[10px] text-slate-400 font-sans">{mod.type}</div>
                    </div>
                    <span className="px-2 py-0.5 rounded bg-emerald-500/10 text-emerald-400 text-[10px] font-semibold">
                      {mod.state}
                    </span>
                  </div>
                ))}
              </div>
            </div>
          </div>
        )}

        {/* ==================== GENERAL / ABOUT TAB ==================== */}
        {activeTab === 'general' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">About This System</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Hardware specifications, processor topology, and software licensing.
              </p>
            </div>

            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-2.5 text-xs">
              <div className="flex justify-between py-1 border-b border-black/5 dark:border-white/5">
                <span className="text-slate-400">Processor</span>
                <span className="font-semibold">{systemInfo.cpuModel} ({systemInfo.cpuCores} Cores)</span>
              </div>
              <div className="flex justify-between py-1 border-b border-black/5 dark:border-white/5">
                <span className="text-slate-400">Graphics Architecture</span>
                <span className="font-semibold">{systemInfo.gpuModel}</span>
              </div>
              <div className="flex justify-between py-1 border-b border-black/5 dark:border-white/5">
                <span className="text-slate-400">System Memory</span>
                <span className="font-semibold">{systemInfo.totalMemory}</span>
              </div>
              <div className="flex justify-between py-1 border-b border-black/5 dark:border-white/5">
                <span className="text-slate-400">Operating System</span>
                <span className="font-semibold">{systemInfo.osName} ({systemInfo.osVersion})</span>
              </div>
              <div className="flex justify-between py-1 border-b border-black/5 dark:border-white/5">
                <span className="text-slate-400">Kernel Version</span>
                <span className="font-semibold font-mono">{systemInfo.kernelVersion}</span>
              </div>
              <div className="flex justify-between py-1">
                <span className="text-slate-400">Primary Storage</span>
                <span className="font-semibold">{systemInfo.storageCapacity}</span>
              </div>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};
