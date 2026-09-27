import React, { useState, useEffect } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import {
  Monitor,
  Palette,
  Eye,
  Sliders,
  Image as ImageIcon,
  Wifi,
  Bluetooth,
  Volume2,
  HardDrive,
  Info,
  Battery,
  Search,
  Check,
  ChevronDown,
  Sun,
  Moon,
  Laptop,
  Maximize2,
  Lock,
} from 'lucide-react';
import { useSystemState, WALLPAPERS } from '../../context/SystemStateContext';
import { useWindowManager } from '../../context/WindowManagerContext';
import { systemService } from '../../services/systemService';

export type SettingsTab =
  | 'displays'
  | 'appearance'
  | 'accessibility'
  | 'control-center'
  | 'wallpaper'
  | 'wifi'
  | 'bluetooth'
  | 'sound'
  | 'battery'
  | 'general';

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

export const SettingsApp: React.FC = () => {
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
  } = useSystemState();

  const { closeWindow, minimizeWindow, toggleMaximizeWindow, windows } = useWindowManager();
  const currentWindow = windows.find((w) => w.appId === 'settings');

  const [activeTab, setActiveTab] = useState<SettingsTab>('displays');
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedMonitor, setSelectedMonitor] = useState<'built-in' | 'external'>('external');
  const [selectedResolution, setSelectedResolution] = useState<string>('1080p');
  const [showAllResolutions, setShowAllResolutions] = useState<boolean>(false);
  const [useAsMode, setUseAsMode] = useState<string>('Extended display');
  const [isArranging, setIsArranging] = useState<boolean>(false);
  const [trueTone, setTrueTone] = useState<boolean>(true);
  const [realBatteryData, setRealBatteryData] = useState<{ capacity: string; status: string } | null>(null);

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

  // When resolution changes, communicate with Linux backend via IPC
  const handleResolutionChange = (resId: string) => {
    setSelectedResolution(resId);
    systemService.executeCommand(`logger "[AxisOS] Displays: Set resolution to ${resId}" && echo "Applied ${resId}"`)
      .catch(() => {});
  };

  const navItems: { id: SettingsTab; label: string; icon: React.ReactNode; color: string }[] = [
    { id: 'displays', label: 'Displays', icon: <Monitor className="w-3.5 h-3.5" />, color: 'bg-[#007AFF]' },
    { id: 'appearance', label: 'Appearance', icon: <Palette className="w-3.5 h-3.5" />, color: 'bg-indigo-500' },
    { id: 'accessibility', label: 'Accessibility', icon: <Eye className="w-3.5 h-3.5" />, color: 'bg-blue-600' },
    { id: 'control-center', label: 'Control Center', icon: <Sliders className="w-3.5 h-3.5" />, color: 'bg-slate-600' },
    { id: 'wallpaper', label: 'Wallpaper', icon: <ImageIcon className="w-3.5 h-3.5" />, color: 'bg-cyan-500' },
    { id: 'wifi', label: 'Wi-Fi', icon: <Wifi className="w-3.5 h-3.5" />, color: 'bg-[#007AFF]' },
    { id: 'bluetooth', label: 'Bluetooth', icon: <Bluetooth className="w-3.5 h-3.5" />, color: 'bg-blue-500' },
    { id: 'sound', label: 'Sound', icon: <Volume2 className="w-3.5 h-3.5" />, color: 'bg-rose-500' },
    { id: 'battery', label: 'Battery', icon: <Battery className="w-3.5 h-3.5" />, color: 'bg-emerald-500' },
    { id: 'general', label: 'General / About', icon: <Info className="w-3.5 h-3.5" />, color: 'bg-slate-500' },
  ];

  const visibleResolutions = showAllResolutions ? EXTENDED_RESOLUTIONS : PRIMARY_RESOLUTIONS;

  const filteredNavItems = navItems.filter((item) =>
    item.label.toLowerCase().includes(searchQuery.toLowerCase())
  );

  return (
    <div className="flex h-full w-full bg-[#F5F5F7] dark:bg-[#1E1E1E] text-slate-900 dark:text-slate-100 select-none overflow-hidden font-sans">
      {/* ======================================================== */}
      {/* SIDEBAR (LEFT): Frosted Glass Sidebar                    */}
      {/* ======================================================== */}
      <div
        data-window-drag
        className="w-64 bg-white/70 dark:bg-slate-900/70 backdrop-blur-md border-r border-black/5 dark:border-white/10 p-3.5 flex flex-col gap-3 shrink-0"
      >
        {/* Top Header: macOS Traffic Lights */}
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

        {/* Subtle Gray Search Bar */}
        <div className="relative">
          <Search className="w-3.5 h-3.5 absolute left-2.5 top-2 text-slate-400" />
          <input
            type="text"
            placeholder="Search"
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="w-full pl-8 pr-3 py-1 bg-black/5 dark:bg-white/5 border border-black/5 dark:border-white/10 rounded-lg text-xs text-slate-800 dark:text-slate-200 placeholder-slate-400 outline-none focus:ring-1 focus:ring-[#007AFF] transition-all"
          />
        </div>

        {/* Navigation Menu */}
        <div className="flex-1 overflow-y-auto pr-1 flex flex-col gap-0.5 text-xs font-normal">
          {filteredNavItems.map((item) => {
            const isActive = activeTab === item.id;
            return (
              <button
                key={item.id}
                onClick={() => setActiveTab(item.id)}
                className={`relative flex items-center space-x-2.5 px-2.5 py-1.5 rounded-lg transition-colors text-left ${
                  isActive
                    ? 'bg-[#007AFF] text-white font-medium shadow-xs'
                    : 'text-slate-700 dark:text-slate-300 hover:bg-black/5 dark:hover:bg-white/5'
                }`}
              >
                <div
                  className={`w-5 h-5 rounded-md flex items-center justify-center text-white shrink-0 ${
                    isActive ? 'bg-white/20' : item.color
                  }`}
                >
                  {item.icon}
                </div>
                <span className="truncate">{item.label}</span>
              </button>
            );
          })}
        </div>

        {/* User Badge */}
        <div className="pt-2 border-t border-black/5 dark:border-white/10 flex items-center space-x-2.5 px-1">
          <div className="w-7 h-7 rounded-full bg-gradient-to-tr from-[#007AFF] to-blue-600 flex items-center justify-center text-white font-bold text-[10px]">
            JM
          </div>
          <div className="overflow-hidden">
            <div className="text-[11px] font-semibold truncate text-slate-800 dark:text-slate-200">
              {systemInfo.username}
            </div>
            <div className="text-[9px] text-slate-400">AxisID & Cloud</div>
          </div>
        </div>
      </div>

      {/* ======================================================== */}
      {/* MAIN CONTENT AREA (RIGHT): Clean light bg [#F5F5F7]       */}
      {/* ======================================================== */}
      <div
        data-window-drag
        className="flex-1 flex flex-col overflow-y-auto bg-[#F5F5F7] dark:bg-[#1E1E1E] p-6 text-slate-800 dark:text-slate-200"
      >
        {/* ======================================================== */}
        {/* THE "DISPLAYS" VIEW (ACTIVE TAB)                         */}
        {/* ======================================================== */}
        {activeTab === 'displays' && (
          <div className="max-w-2xl mx-auto w-full flex flex-col gap-6">
            {/* Header */}
            <div className="flex items-center justify-between">
              <div>
                <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">
                  Displays
                </h1>
                <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">
                  Configure monitor arrangement, resolution, and color modes.
                </p>
              </div>

              {/* "Arrange..." Button with Framer Motion whileTap */}
              <motion.button
                whileTap={{ scale: 0.95 }}
                onClick={() => setIsArranging(!isArranging)}
                className="px-3 py-1.5 rounded-lg border border-slate-300 dark:border-slate-700 bg-white dark:bg-slate-800 text-xs font-medium text-slate-700 dark:text-slate-200 shadow-xs hover:bg-slate-50 dark:hover:bg-slate-700/80 transition-colors"
              >
                Arrange...
              </motion.button>
            </div>

            {/* Arrangement Modal Preview (if triggered) */}
            <AnimatePresence>
              {isArranging && (
                <motion.div
                  initial={{ opacity: 0, height: 0 }}
                  animate={{ opacity: 1, height: 'auto' }}
                  exit={{ opacity: 0, height: 0 }}
                  className="p-4 rounded-xl bg-blue-500/10 border border-blue-500/30 flex items-center justify-between text-xs overflow-hidden"
                >
                  <div className="flex items-center space-x-2 text-blue-700 dark:text-blue-300 font-medium">
                    <Maximize2 className="w-4 h-4" />
                    <span>Drag displays to match their physical arrangement. Wayland layout active.</span>
                  </div>
                  <button
                    onClick={() => setIsArranging(false)}
                    className="px-2.5 py-1 bg-blue-600 text-white rounded-md text-xs font-semibold"
                  >
                    Done
                  </button>
                </motion.div>
              )}
            </AnimatePresence>

            {/* Visual Representation of Connected Monitors at the Top */}
            <div className="py-6 flex items-end justify-center gap-8 bg-black/[0.02] dark:bg-white/[0.02] rounded-2xl border border-black/5 dark:border-white/5">
              {/* Built-in Laptop Display */}
              <button
                onClick={() => setSelectedMonitor('built-in')}
                className={`group flex flex-col items-center focus:outline-none transition-transform ${
                  selectedMonitor === 'built-in' ? 'scale-102' : 'opacity-85 hover:opacity-100'
                }`}
              >
                {/* Laptop Screen Body */}
                <div
                  className={`w-36 h-24 rounded-t-lg bg-slate-900 border-2 transition-all p-1 flex flex-col justify-between shadow-xl ${
                    selectedMonitor === 'built-in'
                      ? 'border-[#007AFF] ring-2 ring-[#007AFF]/30 shadow-[#007AFF]/20'
                      : 'border-slate-400 dark:border-slate-600'
                  }`}
                  style={{ background: wallpaper.gradient }}
                >
                  <div className="w-1 h-1 rounded-full bg-slate-400 mx-auto mt-0.5" />
                  <div className="text-[9px] font-bold text-white text-center drop-shadow">Built-in</div>
                  <div className="h-0.5" />
                </div>
                {/* Laptop Keyboard Base */}
                <div className="w-44 h-2.5 rounded-b-md bg-gradient-to-b from-slate-300 to-slate-400 dark:from-slate-600 dark:to-slate-700 shadow-md flex items-center justify-center">
                  <div className="w-10 h-1 bg-slate-400/50 rounded-xs" />
                </div>
                <span className="text-[11px] font-medium text-slate-600 dark:text-slate-300 mt-2">
                  Built-in Retina Display
                </span>
                <span className="text-[10px] text-slate-400 font-mono">1366 × 768</span>
              </button>

              {/* External Monitor (Apple Studio Display style) */}
              <button
                onClick={() => setSelectedMonitor('external')}
                className={`group flex flex-col items-center focus:outline-none transition-transform ${
                  selectedMonitor === 'external' ? 'scale-102' : 'opacity-85 hover:opacity-100'
                }`}
              >
                {/* Display Screen Frame with Drop Shadow */}
                <div
                  className={`w-48 h-30 rounded-lg bg-slate-900 border-2 transition-all p-1.5 flex flex-col justify-between shadow-2xl ${
                    selectedMonitor === 'external'
                      ? 'border-[#007AFF] ring-2 ring-[#007AFF]/30 shadow-[#007AFF]/25'
                      : 'border-slate-400 dark:border-slate-600'
                  }`}
                  style={{ background: wallpaper.gradient }}
                >
                  <div className="w-1.5 h-1.5 rounded-full bg-slate-400 mx-auto mt-0.5" />
                  <div className="text-[10px] font-bold text-white text-center drop-shadow">
                    Studio Display (1080p)
                  </div>
                  <div className="h-0.5" />
                </div>
                {/* Aluminum Stand */}
                <div className="w-5 h-7 bg-gradient-to-b from-slate-300 to-slate-400 dark:from-slate-600 dark:to-slate-700" />
                <div className="w-16 h-1 rounded-sm bg-slate-400 dark:bg-slate-600 shadow-sm" />

                <span className="text-[11px] font-medium text-slate-600 dark:text-slate-300 mt-2">
                  Studio Display (External)
                </span>
                <span className="text-[10px] text-slate-400 font-mono">1920 × 1080</span>
              </button>
            </div>

            {/* Clean White Settings Card */}
            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-4">
              {/* Dropdown for "Use as: Extended display" */}
              <div className="flex items-center justify-between pb-3 border-b border-black/5 dark:border-white/5">
                <span className="text-xs font-semibold text-slate-800 dark:text-slate-200">
                  Use as
                </span>
                <div className="relative">
                  <select
                    value={useAsMode}
                    onChange={(e) => setUseAsMode(e.target.value)}
                    className="appearance-none bg-slate-100 dark:bg-slate-800/80 border border-slate-300 dark:border-slate-700 rounded-lg pl-3 pr-8 py-1.5 text-xs text-slate-800 dark:text-slate-200 font-medium focus:outline-none focus:ring-1 focus:ring-[#007AFF] cursor-pointer"
                  >
                    <option value="Extended display">Extended display</option>
                    <option value="Main display">Main display</option>
                    <option value="Mirror Built-in Retina Display">Mirror Built-in Retina Display</option>
                  </select>
                  <ChevronDown className="w-3.5 h-3.5 text-slate-500 absolute right-2.5 top-2.5 pointer-events-none" />
                </div>
              </div>

              {/* Selectable List of Resolutions with Framer Motion layout transition */}
              <div className="flex flex-col gap-2">
                <span className="text-xs font-semibold text-slate-800 dark:text-slate-200">
                  Resolution
                </span>

                <div className="flex flex-col rounded-xl border border-black/5 dark:border-white/5 overflow-hidden">
                  {visibleResolutions.map((res) => {
                    const isSelected = selectedResolution === res.id;
                    return (
                      <button
                        key={res.id}
                        onClick={() => handleResolutionChange(res.id)}
                        className="relative flex items-center justify-between px-3.5 py-2.5 text-xs transition-colors text-left"
                      >
                        {/* Gliding Framer Motion Active Indicator */}
                        {isSelected && (
                          <motion.div
                            layoutId="active-resolution"
                            className="absolute inset-0 bg-black/5 dark:bg-white/10"
                            transition={{ type: 'spring', bounce: 0.15, duration: 0.35 }}
                          />
                        )}

                        <span
                          className={`relative z-10 font-medium ${
                            isSelected
                              ? 'text-slate-900 dark:text-white font-semibold'
                              : 'text-slate-600 dark:text-slate-300'
                          }`}
                        >
                          {res.label}
                        </span>

                        {isSelected && (
                          <Check className="w-3.5 h-3.5 text-[#007AFF] relative z-10" />
                        )}
                      </button>
                    );
                  })}
                </div>
              </div>

              {/* "Show all resolutions" Toggle Switch (iOS / macOS Style Switch) */}
              <div className="flex items-center justify-between pt-2 border-t border-black/5 dark:border-white/5">
                <div className="flex flex-col">
                  <span className="text-xs font-semibold text-slate-800 dark:text-slate-200">
                    Show all resolutions
                  </span>
                  <span className="text-[10px] text-slate-400">
                    Includes low-resolution and legacy modes
                  </span>
                </div>

                {/* macOS / iOS Style Green/Gray Switch */}
                <button
                  type="button"
                  role="switch"
                  aria-checked={showAllResolutions}
                  onClick={() => setShowAllResolutions(!showAllResolutions)}
                  className={`w-11 h-6 rounded-full transition-colors relative focus:outline-none p-0.5 ${
                    showAllResolutions ? 'bg-[#34C759]' : 'bg-slate-300 dark:bg-slate-700'
                  }`}
                >
                  <motion.div
                    layout
                    transition={{ type: 'spring', stiffness: 700, damping: 30 }}
                    className={`w-5 h-5 rounded-full bg-white shadow-md ${
                      showAllResolutions ? 'ml-auto' : 'mr-auto'
                    }`}
                  />
                </button>
              </div>

              {/* Brightness & True Tone Row */}
              <div className="flex flex-col gap-3 pt-3 border-t border-black/5 dark:border-white/5">
                <div className="flex justify-between items-center text-xs">
                  <span className="font-semibold text-slate-800 dark:text-slate-200">Brightness</span>
                  <span className="font-mono text-slate-500">{brightness}%</span>
                </div>
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
                </div>
              </div>
            </div>
          </div>
        )}

        {/* ======================================================== */}
        {/* APPEARANCE TAB                                           */}
        {/* ======================================================== */}
        {activeTab === 'appearance' && (
          <div className="max-w-xl mx-auto w-full flex flex-col gap-5">
            <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">
              Appearance
            </h1>

            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-4">
              <span className="text-xs font-semibold">Theme</span>
              <div className="grid grid-cols-2 gap-3">
                <button
                  onClick={() => setTheme('light')}
                  className={`p-3 rounded-xl border flex items-center justify-between transition-all ${
                    theme === 'light'
                      ? 'border-[#007AFF] ring-2 ring-[#007AFF]/30 bg-blue-50/50'
                      : 'border-slate-200 dark:border-slate-700'
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
                    theme === 'dark'
                      ? 'border-[#007AFF] ring-2 ring-[#007AFF]/30 bg-blue-50/10'
                      : 'border-slate-200 dark:border-slate-700'
                  }`}
                >
                  <div className="flex items-center space-x-2.5">
                    <Moon className="w-5 h-5 text-blue-400" />
                    <span className="text-xs font-semibold">Dark Mode</span>
                  </div>
                  {theme === 'dark' && <Check className="w-4 h-4 text-[#007AFF]" />}
                </button>
              </div>
            </div>
          </div>
        )}

        {/* ======================================================== */}
        {/* WALLPAPER TAB                                            */}
        {/* ======================================================== */}
        {activeTab === 'wallpaper' && (
          <div className="max-w-xl mx-auto w-full flex flex-col gap-5">
            <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">
              Wallpaper
            </h1>
            <div className="grid grid-cols-2 gap-3">
              {WALLPAPERS.map((wp) => (
                <button
                  key={wp.id}
                  onClick={() => setWallpaper(wp)}
                  className={`h-28 rounded-xl border p-3 flex flex-col justify-end text-left relative overflow-hidden transition-all ${
                    wallpaper.id === wp.id ? 'border-[#007AFF] ring-2 ring-[#007AFF]/40' : 'border-black/10'
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

        {/* ======================================================== */}
        {/* BATTERY TAB: Reads /sys/class/power_supply directly!    */}
        {/* ======================================================== */}
        {activeTab === 'battery' && (
          <div className="max-w-xl mx-auto w-full flex flex-col gap-5">
            <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">
              Battery
            </h1>
            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-4">
              <div className="flex items-center justify-between">
                <div>
                  <div className="text-2xl font-bold text-slate-900 dark:text-white">
                    {realBatteryData?.capacity || '95'}%
                  </div>
                  <div className="text-xs text-slate-400">
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

              <div className="text-xs text-slate-500 pt-2 border-t border-black/5 dark:border-white/5 flex justify-between">
                <span>Power Mode</span>
                <span className="font-semibold text-slate-700 dark:text-slate-300">Automatic / Balanced</span>
              </div>
            </div>
          </div>
        )}

        {/* ======================================================== */}
        {/* GENERAL / ABOUT TAB                                      */}
        {/* ======================================================== */}
        {activeTab === 'general' && (
          <div className="max-w-xl mx-auto w-full flex flex-col gap-5">
            <h1 className="text-xl font-bold tracking-tight text-slate-900 dark:text-white">
              About This System
            </h1>
            <div className="bg-white dark:bg-[#282828] rounded-xl shadow-xs border border-black/5 dark:border-white/10 p-5 flex flex-col gap-2.5 text-xs">
              <div className="flex justify-between py-1 border-b border-black/5 dark:border-white/5">
                <span className="text-slate-400">Processor</span>
                <span className="font-semibold">{systemInfo.cpuModel}</span>
              </div>
              <div className="flex justify-between py-1 border-b border-black/5 dark:border-white/5">
                <span className="text-slate-400">Graphics</span>
                <span className="font-semibold">{systemInfo.gpuModel}</span>
              </div>
              <div className="flex justify-between py-1 border-b border-black/5 dark:border-white/5">
                <span className="text-slate-400">Memory</span>
                <span className="font-semibold">{systemInfo.totalMemory}</span>
              </div>
              <div className="flex justify-between py-1 border-b border-black/5 dark:border-white/5">
                <span className="text-slate-400">Kernel Version</span>
                <span className="font-semibold font-mono">{systemInfo.kernelVersion}</span>
              </div>
              <div className="flex justify-between py-1">
                <span className="text-slate-400">Storage</span>
                <span className="font-semibold">{systemInfo.storageCapacity}</span>
              </div>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};
