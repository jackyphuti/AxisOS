import React, { useState, useEffect } from 'react';
import {
  Wifi,
  WifiOff,
  Battery,
  BatteryCharging,
  Search,
  Sliders,
  Sparkles,
} from 'lucide-react';
import { useSystemState } from '../context/SystemStateContext';
import { useWindowManager, APP_REGISTRY } from '../context/WindowManagerContext';

export const TopBar: React.FC = () => {
  const {
    isQuickSettingsOpen,
    setIsQuickSettingsOpen,
    isSpotlightOpen,
    setIsSpotlightOpen,
    isAppMenuOpen,
    setIsAppMenuOpen,
    setPowerModalOpen,
    wifiConnected,
    batteryLevel,
    isCharging,
    isLiveEnvironment,
  } = useSystemState();

  const { openApp, activeAppId, activeWindowId, closeWindow } = useWindowManager();
  const [currentTime, setCurrentTime] = useState<string>('');
  const [currentDate, setCurrentDate] = useState<string>('');
  const [appleMenuOpen, setAppleMenuOpen] = useState(false);
  const [activeMenuDropdown, setActiveMenuDropdown] = useState<string | null>(null);

  const activeApp = activeAppId ? APP_REGISTRY[activeAppId] : null;
  const activeAppName = activeApp ? activeApp.title : 'Finder';

  useEffect(() => {
    const updateDateTime = () => {
      const now = new Date();
      setCurrentTime(
        now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })
      );
      setCurrentDate(
        now.toLocaleDateString([], { weekday: 'short', month: 'short', day: 'numeric' })
      );
    };

    updateDateTime();
    const interval = setInterval(updateDateTime, 1000);
    return () => clearInterval(interval);
  }, []);

  // Global click outside for menus
  useEffect(() => {
    const handleClick = (e: MouseEvent) => {
      const target = e.target as HTMLElement;
      if (!target.closest('#apple-menu') && !target.closest('#app-menus-container')) {
        setAppleMenuOpen(false);
        setActiveMenuDropdown(null);
      }
    };
    window.addEventListener('mousedown', handleClick);
    return () => window.removeEventListener('mousedown', handleClick);
  }, []);

  return (
    <header className="h-7 w-full bg-slate-950/40 backdrop-blur-2xl border-b border-white/10 px-3 flex items-center justify-between z-40 select-none text-[12px] font-normal text-slate-200">
      {/* Left: Apple / Axis Menu & Active App Menus */}
      <div className="flex items-center space-x-1" id="app-menus-container">
        {/* Apple/Axis Brand Menu */}
        <div className="relative" id="apple-menu">
          <button
            onClick={() => setAppleMenuOpen(!appleMenuOpen)}
            className={`px-2 py-0.5 rounded transition-colors flex items-center justify-center font-black ${
              appleMenuOpen ? 'bg-white/20 text-white' : 'hover:bg-white/10 text-slate-200'
            }`}
          >
            <span className="text-[13px] leading-none -translate-y-0.5">▲</span>
          </button>

          {appleMenuOpen && (
            <div className="absolute top-7 left-0 w-56 bg-slate-900/90 backdrop-blur-3xl border border-white/15 rounded-xl shadow-2xl shadow-black p-1.5 flex flex-col gap-0.5 z-50 text-xs animate-in fade-in duration-100">
              <button
                onClick={() => {
                  setAppleMenuOpen(false);
                  openApp('about');
                }}
                className="w-full px-2.5 py-1 text-left rounded-md hover:bg-blue-600 hover:text-white transition-colors"
              >
                About This AxisPC
              </button>
              <div className="my-1 border-t border-white/10" />
              <button
                onClick={() => {
                  setAppleMenuOpen(false);
                  setIsAppMenuOpen(true);
                }}
                className="w-full px-2.5 py-1 text-left rounded-md hover:bg-blue-600 hover:text-white transition-colors flex items-center justify-between"
              >
                <span>Applications Menu</span>
                <span className="text-[10px] opacity-70 font-mono">⊞ Win</span>
              </button>
              <button
                onClick={() => {
                  setAppleMenuOpen(false);
                  openApp('settings');
                }}
                className="w-full px-2.5 py-1 text-left rounded-md hover:bg-blue-600 hover:text-white transition-colors flex items-center justify-between"
              >
                <span>System Settings...</span>
              </button>
              <button
                onClick={() => {
                  setAppleMenuOpen(false);
                  openApp('software');
                }}
                className="w-full px-2.5 py-1 text-left rounded-md hover:bg-blue-600 hover:text-white transition-colors"
              >
                Axis Store (App Center)...
              </button>
              <div className="my-1 border-t border-white/10" />
              <button
                onClick={() => {
                  setAppleMenuOpen(false);
                  setPowerModalOpen(true);
                }}
                className="w-full px-2.5 py-1 text-left rounded-md hover:bg-blue-600 hover:text-white transition-colors"
              >
                Sleep
              </button>
              <button
                onClick={() => {
                  setAppleMenuOpen(false);
                  setPowerModalOpen(true);
                }}
                className="w-full px-2.5 py-1 text-left rounded-md hover:bg-blue-600 hover:text-white transition-colors"
              >
                Restart...
              </button>
              <button
                onClick={() => {
                  setAppleMenuOpen(false);
                  setPowerModalOpen(true);
                }}
                className="w-full px-2.5 py-1 text-left rounded-md hover:bg-rose-600 hover:text-white transition-colors text-rose-300"
              >
                Shut Down...
              </button>
              <div className="my-1 border-t border-white/10" />
              <button
                onClick={() => {
                  setAppleMenuOpen(false);
                  setPowerModalOpen(true);
                }}
                className="w-full px-2.5 py-1 text-left rounded-md hover:bg-blue-600 hover:text-white transition-colors flex items-center justify-between"
              >
                <span>Lock Screen</span>
                <span className="text-[10px] text-slate-400">⌃⌘Q</span>
              </button>
            </div>
          )}
        </div>

        {/* Focused App Name in Bold */}
        <span className="font-bold text-white px-2 py-0.5 rounded cursor-default">
          {activeAppName}
        </span>

        {/* Dynamic App Menus */}
        {['File', 'Edit', 'View', 'Window', 'Help'].map((menu) => (
          <div key={menu} className="relative">
            <button
              onClick={() => setActiveMenuDropdown(activeMenuDropdown === menu ? null : menu)}
              className={`px-2 py-0.5 rounded transition-colors hidden sm:inline-block ${
                activeMenuDropdown === menu ? 'bg-white/20 text-white' : 'hover:bg-white/10 text-slate-300'
              }`}
            >
              {menu}
            </button>

            {activeMenuDropdown === menu && (
              <div className="absolute top-7 left-0 w-48 bg-slate-900/90 backdrop-blur-3xl border border-white/15 rounded-xl shadow-2xl shadow-black p-1.5 flex flex-col gap-0.5 z-50 text-xs animate-in fade-in duration-100">
                <button
                  onClick={() => {
                    setActiveMenuDropdown(null);
                    if (activeAppId) openApp(activeAppId);
                  }}
                  className="w-full px-2.5 py-1 text-left rounded-md hover:bg-blue-600 hover:text-white transition-colors flex items-center justify-between"
                >
                  <span>New Window</span>
                  <span className="text-[10px] opacity-70">⌘N</span>
                </button>
                <button
                  onClick={() => {
                    setActiveMenuDropdown(null);
                    if (activeWindowId) closeWindow(activeWindowId);
                  }}
                  className="w-full px-2.5 py-1 text-left rounded-md hover:bg-blue-600 hover:text-white transition-colors flex items-center justify-between"
                >
                  <span>Close Window</span>
                  <span className="text-[10px] opacity-70">⌘W</span>
                </button>
              </div>
            )}
          </div>
        ))}
      </div>

      {/* Right Menu Items: Installer trigger, Network, Battery, Spotlight, Control Center, Clock */}
      <div className="flex items-center space-x-2 text-slate-300">
        {/* Live Install pill */}
        {isLiveEnvironment && (
          <button
            onClick={() => openApp('installer')}
            className="flex items-center space-x-1.5 px-2 py-0.5 rounded-full bg-blue-600/80 hover:bg-blue-600 text-white text-[11px] font-semibold transition-transform hover:scale-102 shadow-sm"
          >
            <Sparkles className="w-2.5 h-2.5 text-blue-200" />
            <span>Install AxisOS</span>
          </button>
        )}

        {/* Wi-Fi Icon */}
        <div className="px-1 py-0.5 rounded hover:bg-white/10 cursor-pointer">
          {wifiConnected ? (
            <Wifi className="w-3.5 h-3.5 text-slate-200" />
          ) : (
            <WifiOff className="w-3.5 h-3.5 text-rose-400" />
          )}
        </div>

        {/* Battery with percentage */}
        <div className="flex items-center space-x-1 px-1 py-0.5 rounded hover:bg-white/10 cursor-pointer font-mono text-[11px]">
          {isCharging ? (
            <BatteryCharging className="w-4 h-4 text-emerald-400" />
          ) : (
            <Battery className="w-4 h-4 text-slate-200" />
          )}
          <span>{batteryLevel}%</span>
        </div>

        {/* Spotlight Search Icon */}
        <button
          onClick={() => setIsSpotlightOpen(!isSpotlightOpen)}
          className={`p-1 rounded transition-colors ${
            isSpotlightOpen ? 'bg-white/20 text-white' : 'hover:bg-white/10 text-slate-300'
          }`}
          title="Spotlight Search (⌘Space)"
        >
          <Search className="w-3.5 h-3.5" />
        </button>

        {/* macOS Control Center Sliders Icon */}
        <button
          id="quick-settings-trigger"
          onClick={() => setIsQuickSettingsOpen(!isQuickSettingsOpen)}
          className={`p-1 rounded transition-colors ${
            isQuickSettingsOpen ? 'bg-white/20 text-white' : 'hover:bg-white/10 text-slate-300'
          }`}
          title="Control Center"
        >
          <Sliders className="w-3.5 h-3.5" />
        </button>

        {/* Date & Clock */}
        <div className="flex items-center space-x-1.5 px-2 py-0.5 rounded hover:bg-white/10 cursor-pointer text-slate-200 font-medium">
          <span>{currentDate}</span>
          <span className="font-semibold">{currentTime}</span>
        </div>
      </div>
    </header>
  );
};
