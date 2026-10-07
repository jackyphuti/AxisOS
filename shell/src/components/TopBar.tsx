import React, { useState, useEffect } from 'react';
import {
  Wifi,
  WifiOff,
  Battery,
  BatteryCharging,
  Plug,
  Search,
  Sliders,
  Sparkles,
} from 'lucide-react';
import { useSystemState } from '../context/SystemStateContext';
import { useWindowManager, APP_REGISTRY } from '../context/WindowManagerContext';
import { AxisLogo } from './AxisLogo';

export const TopBar: React.FC = () => {
  const {
    isQuickSettingsOpen,
    setIsQuickSettingsOpen,
    isSpotlightOpen,
    setIsSpotlightOpen,
    isAppMenuOpen,
    setIsAppMenuOpen,
    setPowerModalOpen,
    lockSession,
    wifiEnabled,
    wifiConnected,
    batteryLevel,
    isCharging,
    hasBattery,
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
    <header className="h-7 w-full bg-[#18201b]/80 dark:bg-[#111713]/90 backdrop-blur-2xl border-b border-[#87cf3e]/20 px-3 flex items-center justify-between z-40 select-none text-[12px] font-normal text-slate-200">
      {/* Left: Apple / Axis Menu & Active App Menus */}
      <div className="flex items-center space-x-1" id="app-menus-container">
        {/* Apple/Axis Brand Menu */}
        <div className="relative" id="apple-menu">
          <button
            onClick={() => setAppleMenuOpen(!appleMenuOpen)}
            className={`px-2 py-0.5 rounded transition-colors flex items-center justify-center ${
              appleMenuOpen ? 'bg-[#87cf3e]/30 text-[#87cf3e]' : 'hover:bg-white/10 text-slate-200'
            }`}
          >
            <AxisLogo size={14} variant="white" />
          </button>

          {appleMenuOpen && (
            <div className="absolute top-7 left-0 w-56 bg-[#18221b]/95 backdrop-blur-3xl border border-[#87cf3e]/25 rounded-xl shadow-2xl shadow-black p-1.5 flex flex-col gap-0.5 z-50 text-xs animate-in fade-in duration-100 text-slate-100">
              <button
                onClick={() => {
                  setAppleMenuOpen(false);
                  openApp('about');
                }}
                className="w-full px-2.5 py-1 text-left rounded-md hover:bg-[#87cf3e] hover:text-black transition-colors font-medium"
              >
                About This AxisPC
              </button>
              <div className="my-1 border-t border-[#87cf3e]/15" />
              <button
                onClick={() => {
                  setAppleMenuOpen(false);
                  setIsAppMenuOpen(true);
                }}
                className="w-full px-2.5 py-1 text-left rounded-md hover:bg-[#87cf3e] hover:text-black transition-colors font-medium flex items-center justify-between"
              >
                <span>Applications Menu</span>
                <span className="text-[10px] opacity-70 font-mono">⊞ Win</span>
              </button>
              <button
                onClick={() => {
                  setAppleMenuOpen(false);
                  openApp('settings');
                }}
                className="w-full px-2.5 py-1 text-left rounded-md hover:bg-[#87cf3e] hover:text-black transition-colors font-medium flex items-center justify-between"
              >
                <span>System Settings...</span>
              </button>
              <button
                onClick={() => {
                  setAppleMenuOpen(false);
                  openApp('software');
                }}
                className="w-full px-2.5 py-1 text-left rounded-md hover:bg-[#87cf3e] hover:text-black transition-colors font-medium"
              >
                Axis Store (App Center)...
              </button>
              <div className="my-1 border-t border-[#87cf3e]/15" />
              <button
                onClick={() => {
                  setAppleMenuOpen(false);
                  setPowerModalOpen(true);
                }}
                className="w-full px-2.5 py-1 text-left rounded-md hover:bg-[#87cf3e] hover:text-black transition-colors font-medium"
              >
                Sleep
              </button>
              <button
                onClick={() => {
                  setAppleMenuOpen(false);
                  setPowerModalOpen(true);
                }}
                className="w-full px-2.5 py-1 text-left rounded-md hover:bg-[#87cf3e] hover:text-black transition-colors font-medium"
              >
                Restart...
              </button>
              <button
                onClick={() => {
                  setAppleMenuOpen(false);
                  setPowerModalOpen(true);
                }}
                className="w-full px-2.5 py-1 text-left rounded-md hover:bg-rose-600 hover:text-white transition-colors text-rose-300 font-medium"
              >
                Shut Down...
              </button>
              <div className="my-1 border-t border-[#87cf3e]/15" />
              <button
                onClick={() => {
                  setAppleMenuOpen(false);
                  lockSession();
                }}
                className="w-full px-2.5 py-1 text-left rounded-md hover:bg-[#87cf3e] hover:text-black transition-colors font-medium flex items-center justify-between"
              >
                <span>Lock Screen</span>
                <span className="text-[10px] text-slate-400">⊞ L</span>
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
                  className="w-full px-2.5 py-1 text-left rounded-md hover:bg-[#87cf3e] hover:text-black transition-colors flex items-center justify-between"
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
            className="flex items-center space-x-1.5 px-2.5 py-0.5 rounded-full bg-[#87cf3e] hover:bg-[#76bb33] text-black text-[11px] font-bold transition-transform hover:scale-102 shadow-sm"
          >
            <Sparkles className="w-2.5 h-2.5 text-black" />
            <span>Install AxisOS</span>
          </button>
        )}

        {/* Wi-Fi Icon */}
        <div
          onClick={() => setIsQuickSettingsOpen(!isQuickSettingsOpen)}
          className="px-1 py-0.5 rounded hover:bg-white/10 cursor-pointer"
          title={wifiConnected ? 'Wi-Fi: Connected' : wifiEnabled ? 'Wi-Fi: On (Not Connected)' : 'Wi-Fi: Disabled'}
        >
          {wifiConnected ? (
            <Wifi className="w-3.5 h-3.5 text-slate-200" />
          ) : wifiEnabled ? (
            <Wifi className="w-3.5 h-3.5 text-slate-400" />
          ) : (
            <WifiOff className="w-3.5 h-3.5 text-rose-400" />
          )}
        </div>

        {/* Battery with percentage */}
        <div
          title={hasBattery ? (isCharging ? `Charging: ${batteryLevel}%` : `Battery: ${batteryLevel}%`) : 'Connected to AC Power'}
          className="flex items-center space-x-1 px-1 py-0.5 rounded hover:bg-white/10 cursor-pointer font-mono text-[11px]"
        >
          {hasBattery ? (
            <>
              {isCharging ? (
                <BatteryCharging className="w-4 h-4 text-emerald-400" />
              ) : (
                <Battery className="w-4 h-4 text-slate-200" />
              )}
              <span>{batteryLevel}%</span>
            </>
          ) : (
            <>
              <Plug className="w-3.5 h-3.5 text-emerald-400" />
              <span>100%</span>
            </>
          )}
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
