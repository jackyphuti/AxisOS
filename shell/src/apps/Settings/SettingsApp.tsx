import React, { useState, useEffect } from 'react';
import {
  Wifi,
  WifiOff,
  Network,
  Bluetooth,
  Palette,
  Image as ImageIcon,
  Monitor,
  Volume2,
  VolumeX,
  Battery,
  Mouse,
  Keyboard,
  Printer,
  Info,
  Search,
  Check,
  ChevronRight,
  Sun,
  Moon,
  Laptop,
  Lock,
  RefreshCw,
  Key,
  Speaker,
  Headphones,
  Sliders,
  Sparkles,
  ExternalLink,
  ShieldCheck,
  Zap,
} from 'lucide-react';
import { useSystemState, WALLPAPERS } from '../../context/SystemStateContext';
import { useWindowManager } from '../../context/WindowManagerContext';
import { systemService, WifiNetwork, BluetoothDevice } from '../../services/systemService';

export type SettingsTab =
  | 'wifi'
  | 'network'
  | 'bluetooth'
  | 'appearance'
  | 'background'
  | 'displays'
  | 'sound'
  | 'power'
  | 'mouse-touchpad'
  | 'keyboard'
  | 'printers'
  | 'about';

interface SidebarItem {
  id: SettingsTab;
  label: string;
  icon: React.ReactNode;
  iconBg: string;
}

export const SettingsApp: React.FC<{ params?: Record<string, any> }> = ({ params }) => {
  const {
    theme,
    setTheme,
    wallpaper,
    setWallpaper,
    brightness,
    setBrightness,
    toggleBrightness,
    nightLight,
    setNightLight,
    systemInfo,
    wifiEnabled,
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

  const { closeWindow, minimizeWindow, toggleMaximizeWindow, windows, openApp } = useWindowManager();
  const currentWindow = windows.find((w) => w.appId === 'settings');

  // Normalize legacy tab parameter mapping
  const resolveInitialTab = (tabParam?: string): SettingsTab => {
    if (!tabParam) return 'wifi';
    if (tabParam === 'wallpaper') return 'background';
    if (tabParam === 'ethernet') return 'network';
    if (tabParam === 'mouse-keyboard') return 'mouse-touchpad';
    if (tabParam === 'general') return 'about';
    const validTabs: SettingsTab[] = [
      'wifi',
      'network',
      'bluetooth',
      'appearance',
      'background',
      'displays',
      'sound',
      'power',
      'mouse-touchpad',
      'keyboard',
      'printers',
      'about',
    ];
    return validTabs.includes(tabParam as SettingsTab) ? (tabParam as SettingsTab) : 'wifi';
  };

  const [activeTab, setActiveTab] = useState<SettingsTab>(() =>
    resolveInitialTab(params?.tab || currentWindow?.params?.tab)
  );
  const [searchQuery, setSearchQuery] = useState('');

  // Handle deep-linking navigation
  useEffect(() => {
    const target = params?.tab || currentWindow?.params?.tab;
    if (target) {
      setActiveTab(resolveInitialTab(target));
    }
  }, [params?.tab, currentWindow?.params?.tab]);

  // Wi-Fi state
  const [wifiNetworks, setWifiNetworks] = useState<WifiNetwork[]>([]);
  const [isScanningWifi, setIsScanningWifi] = useState(false);
  const [connectingSsid, setConnectingSsid] = useState<string | null>(null);
  const [wifiPasswordPrompt, setWifiPasswordPrompt] = useState<string | null>(null);
  const [wifiPasswordInput, setWifiPasswordInput] = useState('');

  // Bluetooth state
  const [bluetoothDevices, setBluetoothDevices] = useState<BluetoothDevice[]>([]);
  const [isScanningBt, setIsScanningBt] = useState(false);
  const [btConnectingMac, setBtConnectingMac] = useState<string | null>(null);

  // Displays state
  const [selectedResolution, setSelectedResolution] = useState('1920 x 1080 (16:9)');
  const [refreshRate, setRefreshRate] = useState('60 Hz');
  const [uiScaling, setUiScaling] = useState('100%');
  const [vrrMode, setVrrMode] = useState<'adaptive' | 'disabled'>('adaptive');
  const [nightLightWarmth, setNightLightWarmth] = useState(65);

  // Audio state
  const [selectedAudioOutput, setSelectedAudioOutput] = useState('Built-in Analog Stereo (Speakers)');
  const [testSoundPlaying, setTestSoundPlaying] = useState(false);

  // Power state
  const [powerPlan, setPowerPlan] = useState<'balanced' | 'performance' | 'saver'>('balanced');
  const [screenBlankTime, setScreenBlankTime] = useState('10 minutes');
  const [lidCloseAction, setLidCloseAction] = useState('Suspend');
  const [realBatteryData, setRealBatteryData] = useState<{ capacity: string; status: string } | null>(null);

  // Mouse & Touchpad state
  const [mouseSpeed, setMouseSpeed] = useState(5);
  const [naturalScrolling, setNaturalScrolling] = useState(false);
  const [mouseAcceleration, setMouseAcceleration] = useState(true);

  // Keyboard state
  const [keyboardLayout, setKeyboardLayout] = useState('English (US)');

  // Appearance state
  const [dockPosition, setDockPosition] = useState<'bottom' | 'left' | 'right'>('bottom');
  const [dockAutoHide, setDockAutoHide] = useState(false);

  // Load hardware telemetry
  useEffect(() => {
    let isMounted = true;
    systemService.getBatteryStatus().then((bat) => {
      if (!isMounted) return;
      setRealBatteryData({
        capacity: String(bat.level),
        status: bat.hasBattery ? (bat.charging ? 'Charging' : 'Discharging') : 'Fully Charged (AC Power)',
      });
    }).catch(() => {});
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
        if (res.captivePortal) {
          openApp('browser');
        }
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

  // Ubuntu GNOME Control Center categories
  const sidebarItems: SidebarItem[] = [
    { id: 'wifi', label: 'Wi-Fi', icon: <Wifi className="w-4 h-4" />, iconBg: 'bg-blue-500' },
    { id: 'network', label: 'Network', icon: <Network className="w-4 h-4" />, iconBg: 'bg-teal-500' },
    { id: 'bluetooth', label: 'Bluetooth', icon: <Bluetooth className="w-4 h-4" />, iconBg: 'bg-indigo-500' },
    { id: 'appearance', label: 'Appearance', icon: <Palette className="w-4 h-4" />, iconBg: 'bg-pink-500' },
    { id: 'background', label: 'Background', icon: <ImageIcon className="w-4 h-4" />, iconBg: 'bg-emerald-500' },
    { id: 'displays', label: 'Displays', icon: <Monitor className="w-4 h-4" />, iconBg: 'bg-amber-500' },
    { id: 'sound', label: 'Sound', icon: <Volume2 className="w-4 h-4" />, iconBg: 'bg-rose-500' },
    { id: 'power', label: 'Power', icon: <Battery className="w-4 h-4" />, iconBg: 'bg-emerald-600' },
    { id: 'mouse-touchpad', label: 'Mouse & Touchpad', icon: <Mouse className="w-4 h-4" />, iconBg: 'bg-cyan-600' },
    { id: 'keyboard', label: 'Keyboard', icon: <Keyboard className="w-4 h-4" />, iconBg: 'bg-purple-500' },
    { id: 'printers', label: 'Printers', icon: <Printer className="w-4 h-4" />, iconBg: 'bg-slate-500' },
    { id: 'about', label: 'About', icon: <Info className="w-4 h-4" />, iconBg: 'bg-slate-600' },
  ];

  const filteredSidebar = sidebarItems.filter((item) =>
    item.label.toLowerCase().includes(searchQuery.toLowerCase())
  );

  return (
    <div className="flex h-full w-full bg-[#f6f8f6] dark:bg-[#141b16] text-slate-800 dark:text-slate-100 select-none overflow-hidden font-sans">
      {/* ======================================================== */}
      {/* LEFT: Ubuntu Style Sidebar                              */}
      {/* ======================================================== */}
      <div
        data-window-drag
        className="w-64 bg-[#edf2ee] dark:bg-[#19221b] border-r border-black/5 dark:border-[#87cf3e]/20 p-3.5 flex flex-col gap-3 shrink-0"
      >
        {/* Header & Window Controls */}
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
          <span className="text-xs font-bold text-slate-600 dark:text-slate-300">Settings</span>
        </div>

        {/* Ubuntu Search Pill */}
        <div className="relative">
          <Search className="w-3.5 h-3.5 absolute left-3 top-2 text-slate-400" />
          <input
            type="text"
            placeholder="Search..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="w-full pl-8 pr-3 py-1 bg-white/70 dark:bg-white/5 border border-black/10 dark:border-white/10 rounded-full text-xs text-slate-800 dark:text-slate-100 placeholder-slate-400 outline-none focus:ring-2 focus:ring-[#87cf3e]/40 transition-all"
          />
        </div>

        {/* Ubuntu Sidebar List */}
        <div className="flex-1 overflow-y-auto pr-1 flex flex-col gap-1 text-xs">
          {filteredSidebar.map((item) => {
            const isActive = activeTab === item.id;
            return (
              <button
                key={item.id}
                onClick={() => setActiveTab(item.id)}
                className={`flex items-center space-x-3 px-3 py-2 rounded-xl transition-all text-left cursor-pointer ${
                  isActive
                    ? 'bg-[#87cf3e] text-black font-semibold shadow-xs'
                    : 'text-slate-700 dark:text-slate-300 hover:bg-black/5 dark:hover:bg-white/5'
                }`}
              >
                <div
                  className={`w-6 h-6 rounded-lg flex items-center justify-center text-white shrink-0 ${
                    isActive ? 'bg-black/20 text-black' : item.iconBg
                  }`}
                >
                  {item.icon}
                </div>
                <span className="truncate">{item.label}</span>
              </button>
            );
          })}
        </div>

        {/* Host Machine Badge */}
        <div className="pt-2 border-t border-black/5 dark:border-[#87cf3e]/15 flex items-center space-x-2.5 px-1">
          <div className="w-7 h-7 rounded-full bg-gradient-to-tr from-[#76bb33] to-[#87cf3e] flex items-center justify-center text-black font-bold text-[10px]">
            AX
          </div>
          <div className="overflow-hidden">
            <div className="text-[11px] font-semibold truncate text-slate-800 dark:text-slate-200">
              {systemInfo.hostname}
            </div>
            <div className="text-[9px] text-slate-500 dark:text-slate-400">AxisOS 2.0 • Linux 6.12</div>
          </div>
        </div>
      </div>

      {/* ======================================================== */}
      {/* RIGHT: Ubuntu Style Settings Content Viewport           */}
      {/* ======================================================== */}
      <div
        data-window-drag
        className="flex-1 flex flex-col overflow-y-auto p-6 text-slate-800 dark:text-slate-200"
      >
        {/* ==================== 1. WI-FI TAB ==================== */}
        {activeTab === 'wifi' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div className="flex items-center justify-between">
              <div>
                <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Wi-Fi</h1>
                <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                  Connect to wireless networks, manage Wi-Fi passwords, and captive portal login.
                </p>
              </div>
              <button
                onClick={handleScanWifi}
                disabled={isScanningWifi || !wifiEnabled}
                className="p-1.5 rounded-lg bg-black/5 dark:bg-white/5 hover:bg-black/10 dark:hover:bg-white/10 text-slate-600 dark:text-slate-300 transition-colors flex items-center gap-1.5 text-xs font-medium cursor-pointer"
              >
                <RefreshCw className={`w-3.5 h-3.5 ${isScanningWifi ? 'animate-spin text-[#87cf3e]' : ''}`} />
                <span>Scan</span>
              </button>
            </div>

            {/* Wi-Fi Switch Card */}
            <div className="bg-white dark:bg-[#1a231c] rounded-xl shadow-xs border border-black/5 dark:border-[#87cf3e]/20 p-4 flex items-center justify-between">
              <div className="flex items-center space-x-3">
                <div className={`w-9 h-9 rounded-xl flex items-center justify-center text-white ${wifiEnabled ? 'bg-[#87cf3e] text-black font-bold' : 'bg-slate-400'}`}>
                  {wifiEnabled ? <Wifi className="w-5 h-5" /> : <WifiOff className="w-5 h-5 text-white" />}
                </div>
                <div>
                  <div className="text-sm font-semibold text-slate-900 dark:text-white">Wi-Fi</div>
                  <div className="text-xs text-slate-500 dark:text-slate-400">
                    {!wifiEnabled ? 'Turned Off' : wifiConnected ? `Connected to ${wifiSsid}` : 'Scanning available networks...'}
                  </div>
                </div>
              </div>

              {/* Ubuntu-styled toggle switch */}
              <button
                onClick={() => toggleWifi(!wifiEnabled)}
                className={`w-12 h-6.5 rounded-full transition-colors relative p-0.5 cursor-pointer ${
                  wifiEnabled ? 'bg-[#87cf3e]' : 'bg-slate-300 dark:bg-slate-700'
                }`}
              >
                <div
                  className={`w-5.5 h-5.5 rounded-full bg-white shadow-md transition-transform ${
                    wifiEnabled ? 'translate-x-5.5' : 'translate-x-0'
                  }`}
                />
              </button>
            </div>

            {/* Password Modal */}
            {wifiPasswordPrompt && (
              <div className="bg-[#1e2821] border border-[#87cf3e]/30 rounded-xl p-4 flex flex-col gap-3 shadow-xl">
                <div className="flex items-center space-x-2 text-xs font-semibold text-slate-100">
                  <Key className="w-4 h-4 text-[#87cf3e]" />
                  <span>Enter Wi-Fi Password for "{wifiPasswordPrompt}"</span>
                </div>
                <div className="flex gap-2">
                  <input
                    type="password"
                    placeholder="Password"
                    value={wifiPasswordInput}
                    onChange={(e) => setWifiPasswordInput(e.target.value)}
                    autoFocus
                    className="flex-1 px-3 py-1.5 bg-[#141c16] border border-[#87cf3e]/30 rounded-lg text-xs text-slate-100 placeholder-slate-400 outline-none focus:ring-2 focus:ring-[#87cf3e]"
                  />
                  <button
                    onClick={() => handleConnectWifi(wifiPasswordPrompt, wifiPasswordInput)}
                    disabled={connectingSsid === wifiPasswordPrompt}
                    className="px-4 py-1.5 bg-[#87cf3e] text-black rounded-lg text-xs font-bold hover:bg-[#76bb33] transition-colors cursor-pointer"
                  >
                    {connectingSsid === wifiPasswordPrompt ? 'Connecting...' : 'Connect'}
                  </button>
                  <button
                    onClick={() => {
                      setWifiPasswordPrompt(null);
                      setWifiPasswordInput('');
                    }}
                    className="px-3 py-1.5 bg-white/5 hover:bg-white/10 rounded-lg text-xs text-slate-300 transition-colors cursor-pointer"
                  >
                    Cancel
                  </button>
                </div>
              </div>
            )}

            {/* Networks List */}
            {wifiEnabled && (
              <div className="bg-white dark:bg-[#1a231c] rounded-xl shadow-xs border border-black/5 dark:border-[#87cf3e]/20 p-4 flex flex-col gap-3">
                <div className="text-xs font-semibold text-[#87cf3e] uppercase tracking-wider">
                  Visible Wireless Networks
                </div>
                <div className="flex flex-col divide-y divide-black/5 dark:divide-white/5">
                  {wifiNetworks.map((net) => {
                    const isCurrent = net.ssid === wifiSsid;
                    const isOpen = net.security === 'Open';
                    return (
                      <div
                        key={net.ssid}
                        className="py-2.5 flex items-center justify-between text-xs hover:bg-black/5 dark:hover:bg-white/5 px-2 rounded-lg transition-colors"
                      >
                        <div className="flex items-center space-x-2.5">
                          <Wifi className={`w-4 h-4 ${isCurrent ? 'text-[#87cf3e]' : 'text-slate-400'}`} />
                          <div className="flex flex-col">
                            <span className="font-semibold text-slate-900 dark:text-slate-100">{net.ssid}</span>
                            <span className="text-[10px] text-slate-500 dark:text-slate-400">
                              {isOpen ? 'Open Network (Captive Portal Web Login)' : net.security} • Signal: {net.signal}%
                            </span>
                          </div>
                        </div>

                        <div className="flex items-center space-x-2">
                          {!isOpen && <Lock className="w-3.5 h-3.5 text-slate-400" />}
                          {isCurrent ? (
                            <span className="flex items-center space-x-1 text-[11px] font-semibold text-[#87cf3e]">
                              <Check className="w-3.5 h-3.5" />
                              <span>Connected</span>
                            </span>
                          ) : (
                            <button
                              onClick={() => {
                                if (isOpen) {
                                  handleConnectWifi(net.ssid);
                                } else {
                                  setWifiPasswordPrompt(net.ssid);
                                }
                              }}
                              disabled={connectingSsid === net.ssid}
                              className="px-3 py-1 bg-[#87cf3e]/15 hover:bg-[#87cf3e]/25 text-[#87cf3e] border border-[#87cf3e]/30 rounded-lg text-[11px] font-semibold transition-colors cursor-pointer"
                            >
                              {connectingSsid === net.ssid ? 'Connecting...' : isOpen ? 'Connect' : 'Connect'}
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

        {/* ==================== 2. NETWORK (ETHERNET) TAB ==================== */}
        {activeTab === 'network' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Network</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Wired Ethernet connection, IP configuration, and network details.
              </p>
            </div>

            <div className="bg-white dark:bg-[#1a231c] rounded-xl shadow-xs border border-black/5 dark:border-[#87cf3e]/20 p-5 flex flex-col gap-4">
              <div className="flex items-center justify-between pb-3 border-b border-black/5 dark:border-white/5">
                <div className="flex items-center space-x-3">
                  <div className="w-9 h-9 rounded-xl bg-teal-500 text-white flex items-center justify-center font-bold">
                    <Network className="w-5 h-5" />
                  </div>
                  <div>
                    <div className="text-sm font-semibold text-slate-900 dark:text-white">Wired Connection (eth0)</div>
                    <div className="text-xs text-slate-500 dark:text-slate-400">1000 Mb/s • Full Duplex</div>
                  </div>
                </div>
                <span className="px-2.5 py-1 rounded-full bg-emerald-500/10 text-[#87cf3e] text-xs font-semibold">
                  Connected
                </span>
              </div>

              <div className="grid grid-cols-2 gap-4 text-xs">
                <div>
                  <span className="text-slate-400">IPv4 Address</span>
                  <div className="font-mono font-semibold mt-0.5">192.168.1.145</div>
                </div>
                <div>
                  <span className="text-slate-400">Subnet Mask</span>
                  <div className="font-mono font-semibold mt-0.5">255.255.255.0</div>
                </div>
                <div>
                  <span className="text-slate-400">Default Gateway</span>
                  <div className="font-mono font-semibold mt-0.5">192.168.1.1</div>
                </div>
                <div>
                  <span className="text-slate-400">DNS Servers</span>
                  <div className="font-mono font-semibold mt-0.5">1.1.1.1, 8.8.8.8</div>
                </div>
              </div>
            </div>
          </div>
        )}

        {/* ==================== 3. BLUETOOTH TAB ==================== */}
        {activeTab === 'bluetooth' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div className="flex items-center justify-between">
              <div>
                <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Bluetooth</h1>
                <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                  Pair and connect wireless mice, keyboards, and headphones.
                </p>
              </div>
              <button
                onClick={handleScanBluetooth}
                disabled={isScanningBt || !bluetoothEnabled}
                className="p-1.5 rounded-lg bg-black/5 dark:bg-white/5 hover:bg-black/10 dark:hover:bg-white/10 text-slate-600 dark:text-slate-300 transition-colors flex items-center gap-1.5 text-xs font-medium cursor-pointer"
              >
                <RefreshCw className={`w-3.5 h-3.5 ${isScanningBt ? 'animate-spin text-[#87cf3e]' : ''}`} />
                <span>Scan</span>
              </button>
            </div>

            {/* Bluetooth Switch */}
            <div className="bg-white dark:bg-[#1a231c] rounded-xl shadow-xs border border-black/5 dark:border-[#87cf3e]/20 p-4 flex items-center justify-between">
              <div className="flex items-center space-x-3">
                <div className={`w-9 h-9 rounded-xl flex items-center justify-center text-white ${bluetoothEnabled ? 'bg-indigo-500' : 'bg-slate-400'}`}>
                  <Bluetooth className="w-5 h-5" />
                </div>
                <div>
                  <div className="text-sm font-semibold text-slate-900 dark:text-white">Bluetooth</div>
                  <div className="text-xs text-slate-500 dark:text-slate-400">
                    {bluetoothEnabled ? `Visible as "${systemInfo.hostname}"` : 'Turned Off'}
                  </div>
                </div>
              </div>

              <button
                onClick={toggleBluetooth}
                className={`w-12 h-6.5 rounded-full transition-colors relative p-0.5 cursor-pointer ${
                  bluetoothEnabled ? 'bg-[#87cf3e]' : 'bg-slate-300 dark:bg-slate-700'
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
              <div className="bg-white dark:bg-[#1a231c] rounded-xl shadow-xs border border-black/5 dark:border-[#87cf3e]/20 p-4 flex flex-col gap-3">
                <div className="text-xs font-semibold text-[#87cf3e] uppercase tracking-wider">
                  Discovered & Paired Devices
                </div>
                <div className="flex flex-col divide-y divide-black/5 dark:divide-white/5">
                  {bluetoothDevices.map((dev) => (
                    <div
                      key={dev.mac}
                      className="py-2.5 flex items-center justify-between text-xs hover:bg-black/5 dark:hover:bg-white/5 px-2 rounded-lg transition-colors"
                    >
                      <div className="flex items-center space-x-2.5">
                        <div className="w-7 h-7 rounded-lg bg-black/5 dark:bg-white/5 flex items-center justify-center text-slate-600 dark:text-slate-300">
                          {dev.name.toLowerCase().includes('head') || dev.name.toLowerCase().includes('audio') ? (
                            <Headphones className="w-4 h-4 text-indigo-400" />
                          ) : (
                            <Bluetooth className="w-4 h-4 text-[#87cf3e]" />
                          )}
                        </div>
                        <div>
                          <div className="font-semibold text-slate-900 dark:text-slate-100">{dev.name}</div>
                          <div className="text-[10px] text-slate-400">{dev.mac}</div>
                        </div>
                      </div>

                      <div className="flex items-center space-x-2">
                        {dev.connected ? (
                          <span className="text-[11px] font-semibold text-[#87cf3e]">Connected</span>
                        ) : (
                          <span className="text-[11px] text-slate-400">Not connected</span>
                        )}
                        <button
                          onClick={() => handleConnectBluetooth(dev.mac)}
                          disabled={btConnectingMac === dev.mac}
                          className="px-2.5 py-1 bg-black/5 dark:bg-white/10 hover:bg-black/10 dark:hover:bg-white/15 rounded-md text-[11px] font-medium transition-colors cursor-pointer"
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

        {/* ==================== 4. APPEARANCE TAB ==================== */}
        {activeTab === 'appearance' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Appearance</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Customize desktop style, Linux Mint accent colors, and dock layout.
              </p>
            </div>

            {/* Dark / Light Mode Switch */}
            <div className="bg-white dark:bg-[#1a231c] rounded-xl shadow-xs border border-black/5 dark:border-[#87cf3e]/20 p-5 flex flex-col gap-4">
              <span className="text-xs font-semibold">Style</span>
              <div className="grid grid-cols-2 gap-3">
                <button
                  onClick={() => setTheme('light')}
                  className={`p-3.5 rounded-xl border flex items-center justify-between transition-all cursor-pointer ${
                    theme === 'light' ? 'border-[#87cf3e] ring-2 ring-[#87cf3e]/30 bg-[#87cf3e]/10' : 'border-slate-200 dark:border-slate-700 hover:bg-black/5 dark:hover:bg-white/5'
                  }`}
                >
                  <div className="flex items-center space-x-2.5">
                    <Sun className="w-5 h-5 text-amber-500" />
                    <span className="text-xs font-semibold">Light</span>
                  </div>
                  {theme === 'light' && <Check className="w-4 h-4 text-[#87cf3e]" />}
                </button>

                <button
                  onClick={() => setTheme('dark')}
                  className={`p-3.5 rounded-xl border flex items-center justify-between transition-all cursor-pointer ${
                    theme === 'dark' ? 'border-[#87cf3e] ring-2 ring-[#87cf3e]/30 bg-[#87cf3e]/10' : 'border-slate-200 dark:border-slate-700 hover:bg-black/5 dark:hover:bg-white/5'
                  }`}
                >
                  <div className="flex items-center space-x-2.5">
                    <Moon className="w-5 h-5 text-[#87cf3e]" />
                    <span className="text-xs font-semibold">Dark (Charcoal Mint)</span>
                  </div>
                  {theme === 'dark' && <Check className="w-4 h-4 text-[#87cf3e]" />}
                </button>
              </div>

              {/* Accent Color Palette */}
              <div className="pt-3 border-t border-black/5 dark:border-white/5 flex flex-col gap-2.5">
                <span className="text-xs font-semibold">Accent Color</span>
                <div className="flex items-center gap-3">
                  {(['mint', 'blue', 'cyan', 'purple', 'emerald', 'amber', 'rose'] as const).map((col) => (
                    <button
                      key={col}
                      onClick={() => setAccentColor(col)}
                      className={`w-7 h-7 rounded-full flex items-center justify-center transition-transform hover:scale-110 cursor-pointer ${
                        col === 'mint' ? 'bg-[#87cf3e]' :
                        col === 'blue' ? 'bg-blue-500' :
                        col === 'cyan' ? 'bg-cyan-500' :
                        col === 'purple' ? 'bg-purple-500' :
                        col === 'emerald' ? 'bg-emerald-500' :
                        col === 'amber' ? 'bg-amber-500' : 'bg-rose-500'
                      }`}
                    >
                      {accentColor === col && <Check className={`w-3.5 h-3.5 ${col === 'mint' ? 'text-black font-bold' : 'text-white'}`} />}
                    </button>
                  ))}
                </div>
              </div>
            </div>

            {/* Ubuntu Dock Settings */}
            <div className="bg-white dark:bg-[#1a231c] rounded-xl shadow-xs border border-black/5 dark:border-[#87cf3e]/20 p-5 flex flex-col gap-4">
              <span className="text-xs font-semibold">Dock</span>
              <div className="flex items-center justify-between text-xs pb-3 border-b border-black/5 dark:border-white/5">
                <span>Position on screen</span>
                <div className="flex gap-1 bg-black/5 dark:bg-white/10 p-0.5 rounded-lg">
                  {(['bottom', 'left', 'right'] as const).map((pos) => (
                    <button
                      key={pos}
                      onClick={() => setDockPosition(pos)}
                      className={`px-3 py-1 rounded-md capitalize transition-colors cursor-pointer ${
                        dockPosition === pos ? 'bg-[#87cf3e] text-black font-semibold' : 'text-slate-500 hover:text-white'
                      }`}
                    >
                      {pos}
                    </button>
                  ))}
                </div>
              </div>

              <div className="flex items-center justify-between text-xs">
                <div>
                  <div className="font-medium">Auto-hide the Dock</div>
                  <div className="text-[10px] text-slate-400">The dock hides when windows overlap it</div>
                </div>
                <input
                  type="checkbox"
                  checked={dockAutoHide}
                  onChange={(e) => setDockAutoHide(e.target.checked)}
                  className="w-4 h-4 accent-[#87cf3e] cursor-pointer"
                />
              </div>
            </div>
          </div>
        )}

        {/* ==================== 5. BACKGROUND (WALLPAPER) TAB ==================== */}
        {activeTab === 'background' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Background</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Select your desktop wallpaper or personal picture.
              </p>
            </div>

            <div className="grid grid-cols-2 sm:grid-cols-3 gap-3.5">
              {WALLPAPERS.map((wp) => (
                <button
                  key={wp.id}
                  onClick={() => setWallpaper(wp)}
                  className={`h-28 rounded-2xl border p-3 flex flex-col justify-end text-left relative overflow-hidden transition-all shadow-md cursor-pointer ${
                    wallpaper.id === wp.id ? 'border-[#87cf3e] ring-2 ring-[#87cf3e]/60 scale-[1.02]' : 'border-black/10 hover:border-[#87cf3e]/40'
                  }`}
                  style={{ background: wp.gradient }}
                >
                  <span className="text-xs font-bold text-white drop-shadow">{wp.name}</span>
                  {wallpaper.id === wp.id && (
                    <div className="absolute top-2 right-2 w-5 h-5 rounded-full bg-[#87cf3e] flex items-center justify-center text-black font-bold shadow">
                      <Check className="w-3 h-3" />
                    </div>
                  )}
                </button>
              ))}
            </div>
          </div>
        )}

        {/* ==================== 6. DISPLAYS TAB (WITH WORKING BRIGHTNESS TOGGLE) ==================== */}
        {activeTab === 'displays' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Displays</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Resolution, screen brightness, Night Light warmth, and scaling.
              </p>
            </div>

            {/* Display Visual Box */}
            <div className="bg-[#18231c] rounded-2xl p-6 border border-[#87cf3e]/25 flex flex-col items-center justify-center min-h-[140px] relative shadow-inner">
              <div className="w-40 h-24 rounded-xl border-2 border-[#87cf3e] bg-[#87cf3e]/15 flex flex-col items-center justify-center gap-1 text-white shadow-lg">
                <Monitor className="w-6 h-6 text-[#87cf3e]" />
                <span className="text-xs font-bold">1920 × 1080</span>
                <span className="text-[10px] text-slate-300 font-mono">16:9 • 60 Hz</span>
              </div>
            </div>

            {/* Display Brightness Card (Fixed & Working!) */}
            <div className="bg-white dark:bg-[#1a231c] rounded-xl shadow-xs border border-black/5 dark:border-[#87cf3e]/20 p-5 flex flex-col gap-3">
              <div className="flex items-center justify-between">
                <div>
                  <div className="text-xs font-semibold text-slate-900 dark:text-white">Display Brightness</div>
                  <div className="text-[10px] text-slate-500 dark:text-slate-400">
                    Adjusts display luminance in real time
                  </div>
                </div>
                <button
                  onClick={toggleBrightness}
                  className="px-3 py-1 rounded-lg bg-[#87cf3e]/15 hover:bg-[#87cf3e]/25 text-[#87cf3e] text-xs font-semibold border border-[#87cf3e]/30 transition-colors cursor-pointer"
                  title="Toggle between bright and dimmed"
                >
                  {brightness > 55 ? 'Dim Display (30%)' : 'Brighten (95%)'}
                </button>
              </div>

              <div className="flex items-center gap-3 pt-1">
                <button
                  onClick={toggleBrightness}
                  className="p-1 rounded-md hover:bg-black/10 dark:hover:bg-white/10 text-slate-400 hover:text-[#87cf3e] transition-colors cursor-pointer"
                  title="Click to toggle brightness level"
                >
                  <Sun className={`w-4 h-4 ${brightness > 55 ? 'text-[#87cf3e]' : 'text-slate-400'}`} />
                </button>
                <input
                  type="range"
                  min="10"
                  max="100"
                  value={brightness}
                  onChange={(e) => setBrightness(Number(e.target.value))}
                  className="flex-1 h-2 bg-slate-200 dark:bg-slate-700 rounded-full appearance-none cursor-pointer accent-[#87cf3e]"
                />
                <span className="text-xs font-mono font-bold w-10 text-right text-[#87cf3e]">
                  {brightness}%
                </span>
              </div>
            </div>

            {/* Night Light Card */}
            <div className="bg-white dark:bg-[#1a231c] rounded-xl shadow-xs border border-black/5 dark:border-[#87cf3e]/20 p-5 flex flex-col gap-4">
              <div className="flex items-center justify-between">
                <div>
                  <div className="text-xs font-semibold text-slate-900 dark:text-white">Night Light</div>
                  <div className="text-[10px] text-slate-500 dark:text-slate-400">
                    Makes screen color warmer to help prevent eye strain and aid sleep
                  </div>
                </div>
                <button
                  onClick={() => setNightLight(!nightLight)}
                  className={`w-12 h-6.5 rounded-full transition-colors relative p-0.5 cursor-pointer ${
                    nightLight ? 'bg-[#87cf3e]' : 'bg-slate-300 dark:bg-slate-700'
                  }`}
                >
                  <div
                    className={`w-5.5 h-5.5 rounded-full bg-white shadow-md transition-transform ${
                      nightLight ? 'translate-x-5.5' : 'translate-x-0'
                    }`}
                  />
                </button>
              </div>

              {nightLight && (
                <div className="flex items-center gap-3 pt-2 border-t border-black/5 dark:border-white/5">
                  <span className="text-xs text-slate-400">Color Temperature:</span>
                  <input
                    type="range"
                    min="20"
                    max="100"
                    value={nightLightWarmth}
                    onChange={(e) => setNightLightWarmth(Number(e.target.value))}
                    className="flex-1 h-2 bg-slate-200 dark:bg-slate-700 rounded-full appearance-none cursor-pointer accent-amber-500"
                  />
                  <span className="text-xs font-mono text-amber-400">Warm</span>
                </div>
              )}
            </div>

            {/* Resolution & Refresh Rate */}
            <div className="bg-white dark:bg-[#1a231c] rounded-xl shadow-xs border border-black/5 dark:border-[#87cf3e]/20 p-5 flex flex-col gap-3.5 text-xs">
              <div className="flex items-center justify-between pb-3 border-b border-black/5 dark:border-white/5">
                <span>Resolution</span>
                <select
                  value={selectedResolution}
                  onChange={(e) => setSelectedResolution(e.target.value)}
                  className="bg-black/5 dark:bg-white/10 border border-black/10 dark:border-white/10 rounded-lg px-2.5 py-1 text-xs outline-none"
                >
                  <option>1920 x 1080 (16:9)</option>
                  <option>1600 x 900 (16:9)</option>
                  <option>1366 x 768 (16:9)</option>
                  <option>1280 x 720 (16:9)</option>
                </select>
              </div>

              <div className="flex items-center justify-between pb-3 border-b border-black/5 dark:border-white/5">
                <span>Refresh Rate</span>
                <select
                  value={refreshRate}
                  onChange={(e) => setRefreshRate(e.target.value)}
                  className="bg-black/5 dark:bg-white/10 border border-black/10 dark:border-white/10 rounded-lg px-2.5 py-1 text-xs outline-none"
                >
                  <option>60 Hz</option>
                  <option>120 Hz</option>
                  <option>144 Hz</option>
                </select>
              </div>

              <div className="flex items-center justify-between">
                <span>Scale</span>
                <div className="flex gap-1 bg-black/5 dark:bg-white/10 p-0.5 rounded-lg">
                  {['100%', '125%', '150%', '200%'].map((scale) => (
                    <button
                      key={scale}
                      onClick={() => setUiScaling(scale)}
                      className={`px-2.5 py-1 rounded-md transition-colors cursor-pointer ${
                        uiScaling === scale ? 'bg-[#87cf3e] text-black font-bold' : 'text-slate-500 hover:text-white'
                      }`}
                    >
                      {scale}
                    </button>
                  ))}
                </div>
              </div>
            </div>
          </div>
        )}

        {/* ==================== 7. SOUND TAB ==================== */}
        {activeTab === 'sound' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Sound</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Output volume, balance, and audio device management.
              </p>
            </div>

            <div className="bg-white dark:bg-[#1a231c] rounded-xl shadow-xs border border-black/5 dark:border-[#87cf3e]/20 p-5 flex flex-col gap-4">
              <div className="flex items-center justify-between">
                <div>
                  <div className="text-sm font-semibold text-slate-900 dark:text-white">Output Volume</div>
                  <div className="text-xs text-slate-400">Master system audio</div>
                </div>
                <button
                  onClick={playSoundChime}
                  className="px-3 py-1 bg-[#87cf3e]/15 hover:bg-[#87cf3e]/25 text-[#87cf3e] rounded-lg text-xs font-semibold border border-[#87cf3e]/30 transition-colors flex items-center space-x-1 cursor-pointer"
                >
                  <Speaker className={`w-3.5 h-3.5 ${testSoundPlaying ? 'animate-bounce' : ''}`} />
                  <span>{testSoundPlaying ? 'Playing...' : 'Test Sound'}</span>
                </button>
              </div>

              <div className="flex items-center gap-3">
                <button onClick={toggleMute} className="p-1 rounded text-slate-400 hover:text-white cursor-pointer">
                  {isMuted || volume === 0 ? <VolumeX className="w-5 h-5 text-rose-500" /> : <Volume2 className="w-5 h-5 text-[#87cf3e]" />}
                </button>
                <input
                  type="range"
                  min="0"
                  max="100"
                  value={isMuted ? 0 : volume}
                  onChange={(e) => changeVolume(Number(e.target.value))}
                  className="flex-1 h-2 bg-slate-200 dark:bg-slate-700 rounded-full appearance-none cursor-pointer accent-[#87cf3e]"
                />
                <span className="font-mono text-xs w-8 text-right font-bold text-[#87cf3e]">
                  {isMuted ? '0%' : `${volume}%`}
                </span>
              </div>

              {/* Output Sink Selection */}
              <div className="pt-3 border-t border-black/5 dark:border-white/5 flex flex-col gap-2">
                <span className="text-xs font-semibold text-slate-400 uppercase tracking-wider text-[11px]">Output Device</span>
                {['Built-in Analog Stereo (Speakers)', 'Headphones (3.5mm Jack)', 'HDMI / DisplayPort Audio'].map((sink) => (
                  <button
                    key={sink}
                    onClick={() => setSelectedAudioOutput(sink)}
                    className={`p-2.5 rounded-lg border text-left flex items-center justify-between text-xs transition-colors cursor-pointer ${
                      selectedAudioOutput === sink
                        ? 'border-[#87cf3e] bg-[#87cf3e]/10 font-semibold text-slate-100'
                        : 'border-black/5 dark:border-white/5 hover:bg-black/5 dark:hover:bg-white/5'
                    }`}
                  >
                    <div className="flex items-center space-x-2">
                      <Speaker className="w-4 h-4 text-[#87cf3e]" />
                      <span>{sink}</span>
                    </div>
                    {selectedAudioOutput === sink && <Check className="w-4 h-4 text-[#87cf3e]" />}
                  </button>
                ))}
              </div>
            </div>
          </div>
        )}

        {/* ==================== 8. POWER TAB ==================== */}
        {activeTab === 'power' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Power</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Power mode, battery charging status, and screen blanking.
              </p>
            </div>

            {/* Battery Indicator Card */}
            <div className="bg-white dark:bg-[#1a231c] rounded-xl shadow-xs border border-black/5 dark:border-[#87cf3e]/20 p-5 flex flex-col gap-4">
              <div className="flex items-center justify-between">
                <div>
                  <div className="text-3xl font-black text-slate-900 dark:text-white">
                    {realBatteryData?.capacity || '100'}%
                  </div>
                  <div className="text-xs text-slate-400 mt-0.5">
                    {realBatteryData?.status || 'Fully Charged (Connected to AC Power)'}
                  </div>
                </div>
                <div className="w-12 h-12 rounded-2xl bg-[#87cf3e]/20 text-[#87cf3e] flex items-center justify-center">
                  <Battery className="w-6 h-6" />
                </div>
              </div>

              <div className="w-full bg-slate-200 dark:bg-slate-700 h-2 rounded-full overflow-hidden">
                <div
                  className="bg-[#87cf3e] h-full rounded-full transition-all"
                  style={{ width: `${realBatteryData?.capacity || 100}%` }}
                />
              </div>
            </div>

            {/* Power Mode Card */}
            <div className="bg-white dark:bg-[#1a231c] rounded-xl shadow-xs border border-black/5 dark:border-[#87cf3e]/20 p-5 flex flex-col gap-4">
              <span className="text-xs font-semibold">Power Mode</span>
              <div className="grid grid-cols-3 gap-3">
                {[
                  { id: 'saver', title: 'Power Saver', desc: 'Reduces performance to save energy' },
                  { id: 'balanced', title: 'Balanced', desc: 'Standard performance and power usage' },
                  { id: 'performance', title: 'Performance', desc: 'High performance for intensive tasks' },
                ].map((mode) => (
                  <button
                    key={mode.id}
                    onClick={() => setPowerPlan(mode.id as any)}
                    className={`p-3 rounded-xl border text-left flex flex-col justify-between transition-all cursor-pointer ${
                      powerPlan === mode.id
                        ? 'border-[#87cf3e] bg-[#87cf3e]/10 ring-1 ring-[#87cf3e]'
                        : 'border-black/5 dark:border-white/10 hover:bg-black/5 dark:hover:bg-white/5'
                    }`}
                  >
                    <div>
                      <div className="text-xs font-bold text-slate-900 dark:text-white">{mode.title}</div>
                      <div className="text-[10px] text-slate-400 mt-1 leading-snug">{mode.desc}</div>
                    </div>
                    {powerPlan === mode.id && <Check className="w-4 h-4 text-[#87cf3e] mt-2 self-end" />}
                  </button>
                ))}
              </div>

              <div className="pt-3 border-t border-black/5 dark:border-white/5 flex flex-col gap-3 text-xs">
                <div className="flex items-center justify-between">
                  <span>Screen Blank Timeout</span>
                  <select
                    value={screenBlankTime}
                    onChange={(e) => setScreenBlankTime(e.target.value)}
                    className="bg-black/5 dark:bg-white/10 border border-black/10 dark:border-white/10 rounded-lg px-2.5 py-1 text-xs outline-none"
                  >
                    <option>5 minutes</option>
                    <option>10 minutes</option>
                    <option>15 minutes</option>
                    <option>Never</option>
                  </select>
                </div>

                <div className="flex items-center justify-between">
                  <span>Automatic Suspend</span>
                  <select
                    value={lidCloseAction}
                    onChange={(e) => setLidCloseAction(e.target.value)}
                    className="bg-black/5 dark:bg-white/10 border border-black/10 dark:border-white/10 rounded-lg px-2.5 py-1 text-xs outline-none"
                  >
                    <option>Suspend</option>
                    <option>Do nothing</option>
                  </select>
                </div>
              </div>
            </div>
          </div>
        )}

        {/* ==================== 9. MOUSE & TOUCHPAD TAB ==================== */}
        {activeTab === 'mouse-touchpad' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Mouse & Touchpad</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Pointer speed, natural scrolling, and acceleration.
              </p>
            </div>

            <div className="bg-white dark:bg-[#1a231c] rounded-xl shadow-xs border border-black/5 dark:border-[#87cf3e]/20 p-5 flex flex-col gap-4 text-xs">
              <div className="flex items-center justify-between">
                <span>Mouse Speed</span>
                <input
                  type="range"
                  min="1"
                  max="10"
                  value={mouseSpeed}
                  onChange={(e) => setMouseSpeed(Number(e.target.value))}
                  className="w-48 h-2 bg-slate-200 dark:bg-slate-700 rounded-full appearance-none cursor-pointer accent-[#87cf3e]"
                />
              </div>

              <div className="flex items-center justify-between pt-3 border-t border-black/5 dark:border-white/5">
                <div>
                  <div className="font-semibold text-slate-900 dark:text-white">Natural Scrolling</div>
                  <div className="text-[10px] text-slate-400">Content moves in the direction of fingers</div>
                </div>
                <input
                  type="checkbox"
                  checked={naturalScrolling}
                  onChange={(e) => setNaturalScrolling(e.target.checked)}
                  className="w-4 h-4 accent-[#87cf3e] cursor-pointer"
                />
              </div>

              <div className="flex items-center justify-between pt-3 border-t border-black/5 dark:border-white/5">
                <div>
                  <div className="font-semibold text-slate-900 dark:text-white">Pointer Acceleration</div>
                  <div className="text-[10px] text-slate-400">Dynamically scales cursor velocity</div>
                </div>
                <input
                  type="checkbox"
                  checked={mouseAcceleration}
                  onChange={(e) => setMouseAcceleration(e.target.checked)}
                  className="w-4 h-4 accent-[#87cf3e] cursor-pointer"
                />
              </div>
            </div>
          </div>
        )}

        {/* ==================== 10. KEYBOARD TAB ==================== */}
        {activeTab === 'keyboard' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Keyboard</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Input sources, layouts, and keyboard shortcuts.
              </p>
            </div>

            <div className="bg-white dark:bg-[#1a231c] rounded-xl shadow-xs border border-black/5 dark:border-[#87cf3e]/20 p-5 flex flex-col gap-4 text-xs">
              <div className="flex items-center justify-between">
                <span>Input Source / Layout</span>
                <select
                  value={keyboardLayout}
                  onChange={(e) => setKeyboardLayout(e.target.value)}
                  className="bg-black/5 dark:bg-white/10 border border-black/10 dark:border-white/10 rounded-lg px-2.5 py-1 text-xs outline-none"
                >
                  <option>English (US)</option>
                  <option>English (UK)</option>
                  <option>French</option>
                  <option>German</option>
                  <option>Spanish</option>
                </select>
              </div>

              <div className="pt-3 border-t border-black/5 dark:border-white/5 flex flex-col gap-2">
                <span className="font-semibold text-slate-400 uppercase tracking-wider text-[11px]">Key Shortcuts</span>
                <div className="flex justify-between py-1">
                  <span>Show Applications Overview</span>
                  <kbd className="px-2 py-0.5 rounded bg-black/10 dark:bg-white/10 font-mono text-[10px]">Super (Windows Key)</kbd>
                </div>
                <div className="flex justify-between py-1">
                  <span>Lock Screen</span>
                  <kbd className="px-2 py-0.5 rounded bg-black/10 dark:bg-white/10 font-mono text-[10px]">Super + L</kbd>
                </div>
                <div className="flex justify-between py-1">
                  <span>Launch Terminal</span>
                  <kbd className="px-2 py-0.5 rounded bg-black/10 dark:bg-white/10 font-mono text-[10px]">Ctrl + Alt + T</kbd>
                </div>
              </div>
            </div>
          </div>
        )}

        {/* ==================== 11. PRINTERS TAB ==================== */}
        {activeTab === 'printers' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">Printers</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Configured local and network CUPS printers.
              </p>
            </div>

            <div className="bg-white dark:bg-[#1a231c] rounded-xl shadow-xs border border-black/5 dark:border-[#87cf3e]/20 p-5 flex flex-col items-center justify-center py-12 gap-3 text-center">
              <Printer className="w-10 h-10 text-slate-400" />
              <div className="text-sm font-semibold text-slate-800 dark:text-slate-200">No Printers Detected</div>
              <p className="text-xs text-slate-400 max-w-sm">
                Connect a USB printer or ensure network printers with IPP/CUPS are powered on and on the same Wi-Fi.
              </p>
            </div>
          </div>
        )}

        {/* ==================== 12. ABOUT TAB ==================== */}
        {activeTab === 'about' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            <div>
              <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">About System</h1>
              <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                Hardware specifications and operating system details.
              </p>
            </div>

            {/* System Info Card */}
            <div className="bg-white dark:bg-[#1a231c] rounded-xl shadow-xs border border-black/5 dark:border-[#87cf3e]/20 p-5 flex flex-col gap-3 text-xs">
              <div className="flex justify-between py-1.5 border-b border-black/5 dark:border-white/5">
                <span className="text-slate-400">Device Name</span>
                <span className="font-semibold text-slate-900 dark:text-white">{systemInfo.hostname}</span>
              </div>
              <div className="flex justify-between py-1.5 border-b border-black/5 dark:border-white/5">
                <span className="text-slate-400">Memory</span>
                <span className="font-semibold text-slate-900 dark:text-white">{systemInfo.totalMemory}</span>
              </div>
              <div className="flex justify-between py-1.5 border-b border-black/5 dark:border-white/5">
                <span className="text-slate-400">Processor</span>
                <span className="font-semibold text-slate-900 dark:text-white">{systemInfo.cpuModel}</span>
              </div>
              <div className="flex justify-between py-1.5 border-b border-black/5 dark:border-white/5">
                <span className="text-slate-400">Graphics</span>
                <span className="font-semibold text-slate-900 dark:text-white">{systemInfo.gpuModel}</span>
              </div>
              <div className="flex justify-between py-1.5 border-b border-black/5 dark:border-white/5">
                <span className="text-slate-400">Disk Capacity</span>
                <span className="font-semibold text-slate-900 dark:text-white">{systemInfo.storageCapacity}</span>
              </div>
              <div className="flex justify-between py-1.5 border-b border-black/5 dark:border-white/5">
                <span className="text-slate-400">OS Name</span>
                <span className="font-semibold text-slate-900 dark:text-white">AxisOS Linux 2.0 (Horizon)</span>
              </div>
              <div className="flex justify-between py-1.5 border-b border-black/5 dark:border-white/5">
                <span className="text-slate-400">OS Type</span>
                <span className="font-semibold text-slate-900 dark:text-white">64-bit (Debian Core)</span>
              </div>
              <div className="flex justify-between py-1.5">
                <span className="text-slate-400">Kernel Version</span>
                <span className="font-mono font-semibold text-[#87cf3e]">{systemInfo.kernelVersion}</span>
              </div>
            </div>

            {/* Check for Updates Action */}
            <div className="bg-white dark:bg-[#1a231c] rounded-xl shadow-xs border border-black/5 dark:border-[#87cf3e]/20 p-4 flex items-center justify-between">
              <div>
                <div className="text-xs font-semibold text-slate-900 dark:text-white">Software Updates</div>
                <div className="text-[10px] text-slate-400">Official Debian Bookworm and Flathub repositories</div>
              </div>
              <button
                onClick={() => openApp('software')}
                className="px-3 py-1.5 rounded-lg bg-[#87cf3e] text-black font-semibold text-xs hover:bg-[#76bb33] transition-colors cursor-pointer"
              >
                Open Software Center
              </button>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};
