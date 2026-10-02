import React, { useState, useEffect } from 'react';
import {
  Wifi,
  BatteryCharging,
  Download,
  Music,
  Play,
  Pause,
  Volume2,
  Check,
  Sparkles,
} from 'lucide-react';
import { useSystemState } from '../context/SystemStateContext';
import { useWindowManager } from '../context/WindowManagerContext';

export interface DynamicIslandEvent {
  id: string;
  type: 'music' | 'install' | 'wifi' | 'battery' | 'volume' | 'generic';
  title: string;
  subtitle?: string;
  progress?: number;
  duration?: number;
}

export const DynamicIsland: React.FC = () => {
  const {
    wifiConnected,
    wifiSsid,
    batteryLevel,
    isCharging,
    volume,
    isMuted,
  } = useSystemState();
  const { openApp } = useWindowManager();

  const [activeEvent, setActiveEvent] = useState<DynamicIslandEvent | null>(null);
  const [isExpanded, setIsExpanded] = useState(false);
  const [isPlayingMusic, setIsPlayingMusic] = useState(false);

  // Listen for custom dynamic island notifications across the shell
  useEffect(() => {
    const handleIslandNotify = (e: CustomEvent<DynamicIslandEvent>) => {
      setActiveEvent(e.detail);
      if (e.detail.duration) {
        setTimeout(() => {
          setActiveEvent((cur) => (cur?.id === e.detail.id ? null : cur));
        }, e.detail.duration);
      }
    };

    window.addEventListener('axisos-island-notify' as any, handleIslandNotify as any);
    return () => window.removeEventListener('axisos-island-notify' as any, handleIslandNotify as any);
  }, []);

  // Listen for package manager installations to show Dynamic Island progress
  useEffect(() => {
    const handlePkgStart = (e: CustomEvent<{ packageName: string }>) => {
      setActiveEvent({
        id: `pkg-${Date.now()}`,
        type: 'install',
        title: `Installing ${e.detail?.packageName || 'Software'}...`,
        subtitle: 'APT Debian Bookworm',
        duration: 5000,
      });
    };

    window.addEventListener('axisos-package-installing' as any, handlePkgStart as any);
    return () => window.removeEventListener('axisos-package-installing' as any, handlePkgStart as any);
  }, []);

  // Show dynamic notification when Wi-Fi connects
  useEffect(() => {
    if (wifiConnected && wifiSsid && wifiSsid !== 'Not Connected') {
      setActiveEvent({
        id: `wifi-${Date.now()}`,
        type: 'wifi',
        title: 'Wi-Fi Connected',
        subtitle: wifiSsid,
        duration: 3500,
      });
    }
  }, [wifiConnected, wifiSsid]);

  // Default compact pill state vs expanded state
  const isDefault = !activeEvent;

  return (
    <div className="fixed top-1 left-1/2 -translate-x-1/2 z-50 pointer-events-auto">
      <div
        onClick={() => setIsExpanded(!isExpanded)}
        onMouseEnter={() => setIsExpanded(true)}
        onMouseLeave={() => setIsExpanded(false)}
        className={`group bg-black/90 backdrop-blur-2xl text-white rounded-full shadow-[0_10px_35px_rgba(0,0,0,0.5)] border border-white/15 cursor-pointer transition-all duration-300 ease-[cubic-bezier(0.16,1,0.3,1)] flex items-center justify-between overflow-hidden select-none ${
          isExpanded
            ? 'w-80 h-12 px-4 rounded-3xl'
            : isDefault
            ? 'w-28 h-6 px-2.5 hover:w-32'
            : 'w-64 h-8 px-3'
        }`}
      >
        {/* COMPACT DEFAULT STATE */}
        {isDefault && !isExpanded && (
          <div className="w-full flex items-center justify-between text-[11px] font-medium tracking-tight text-white/90">
            <span className="w-1.5 h-1.5 rounded-full bg-[#34C759] animate-pulse" />
            <span className="text-[10px] font-mono tracking-wider text-white/70">AxisOS</span>
            <span className="w-1.5 h-1.5 rounded-full bg-[#007AFF]" />
          </div>
        )}

        {/* ACTIVE EVENT COMPACT STATE */}
        {!isDefault && !isExpanded && (
          <div className="w-full flex items-center justify-between text-xs font-medium">
            <div className="flex items-center gap-2 truncate">
              {activeEvent.type === 'install' && (
                <Download className="w-3.5 h-3.5 text-[#007AFF] animate-bounce shrink-0" />
              )}
              {activeEvent.type === 'wifi' && (
                <Wifi className="w-3.5 h-3.5 text-[#34C759] shrink-0" />
              )}
              {activeEvent.type === 'music' && (
                <Music className="w-3.5 h-3.5 text-[#EA4335] shrink-0" />
              )}
              {activeEvent.type === 'battery' && (
                <BatteryCharging className="w-3.5 h-3.5 text-[#34C759] shrink-0" />
              )}
              <span className="truncate text-[11px] text-white/90">{activeEvent.title}</span>
            </div>

            {/* Right mini indicator */}
            <div className="flex items-center gap-1 shrink-0 ml-2">
              {activeEvent.type === 'music' ? (
                <div className="flex items-end gap-0.5 h-3">
                  <span className="w-0.5 bg-[#EA4335] animate-[pulse_0.6s_ease-in-out_infinite] h-2" />
                  <span className="w-0.5 bg-[#EA4335] animate-[pulse_0.8s_ease-in-out_infinite] h-3" />
                  <span className="w-0.5 bg-[#EA4335] animate-[pulse_0.5s_ease-in-out_infinite] h-1.5" />
                </div>
              ) : (
                <span className="text-[10px] text-white/60 font-mono truncate">
                  {activeEvent.subtitle || 'Active'}
                </span>
              )}
            </div>
          </div>
        )}

        {/* EXPANDED INTERACTIVE STATE */}
        {isExpanded && (
          <div className="w-full flex items-center justify-between gap-3 text-xs animate-in fade-in duration-200">
            {/* Left Info */}
            <div className="flex items-center gap-2.5 min-w-0">
              <div className="w-7 h-7 rounded-full bg-white/10 flex items-center justify-center shrink-0">
                {activeEvent?.type === 'install' ? (
                  <Download className="w-3.5 h-3.5 text-[#007AFF] animate-spin" />
                ) : activeEvent?.type === 'wifi' ? (
                  <Wifi className="w-3.5 h-3.5 text-[#34C759]" />
                ) : (
                  <Music className="w-3.5 h-3.5 text-[#BA7517]" />
                )}
              </div>
              <div className="flex flex-col min-w-0">
                <span className="text-[11px] font-semibold text-white truncate">
                  {activeEvent?.title || 'Axis Media & System'}
                </span>
                <span className="text-[10px] text-white/50 truncate">
                  {activeEvent?.subtitle || (wifiConnected ? wifiSsid : 'Horizon Live Engine')}
                </span>
              </div>
            </div>

            {/* Right Quick Controls */}
            <div className="flex items-center gap-1.5 shrink-0">
              <button
                onClick={(e) => {
                  e.stopPropagation();
                  openApp('music');
                }}
                className="p-1 rounded-full hover:bg-white/10 text-white/80 hover:text-white transition-colors"
                title="Open Music Player"
              >
                <Music className="w-3.5 h-3.5" />
              </button>
              <button
                onClick={(e) => {
                  e.stopPropagation();
                  openApp('settings');
                }}
                className="p-1 rounded-full hover:bg-white/10 text-white/80 hover:text-white transition-colors"
                title="Settings"
              >
                <Wifi className="w-3.5 h-3.5" />
              </button>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};
