import React from 'react';
import {
  Wifi,
  WifiOff,
  Bluetooth,
  Moon,
  Sun,
  Volume2,
  VolumeX,
  Play,
  Sliders,
  Airplay,
} from 'lucide-react';
import { useSystemState } from '../context/SystemStateContext';
import { useWindowManager } from '../context/WindowManagerContext';

export const QuickSettings: React.FC = () => {
  const {
    wifiConnected,
    wifiSsid,
    bluetoothEnabled,
    theme,
    setTheme,
    volume,
    isMuted,
    brightness,
    setBrightness,
    toggleWifi,
    toggleBluetooth,
    changeVolume,
    toggleMute,
  } = useSystemState();

  const { openApp } = useWindowManager();

  return (
    <div
      id="quick-settings-panel"
      className="absolute top-8 right-2.5 w-80 p-3 rounded-2xl bg-slate-900/85 backdrop-blur-3xl border border-white/15 shadow-[0_25px_60px_rgba(0,0,0,0.8)] z-50 text-slate-100 flex flex-col gap-2.5 select-none animate-in fade-in slide-in-from-top-1 duration-150"
    >
      {/* Top 2-Column Section */}
      <div className="grid grid-cols-2 gap-2">
        {/* Left Column: Connectivity Platter */}
        <div className="p-2.5 rounded-2xl bg-white/5 border border-white/5 flex flex-col gap-2.5">
          {/* Wi-Fi */}
          <button
            onClick={toggleWifi}
            className="flex items-center space-x-2.5 text-left w-full group cursor-pointer"
          >
            <div
              className={`w-7 h-7 rounded-full flex items-center justify-center transition-colors ${
                wifiConnected ? 'bg-blue-500 text-white shadow-md' : 'bg-white/10 text-slate-400'
              }`}
            >
              {wifiConnected ? <Wifi className="w-3.5 h-3.5" /> : <WifiOff className="w-3.5 h-3.5" />}
            </div>
            <div className="overflow-hidden">
              <div className="text-xs font-semibold leading-tight">Wi-Fi</div>
              <div className="text-[10px] text-slate-400 truncate">
                {wifiConnected ? wifiSsid : 'Off'}
              </div>
            </div>
          </button>

          {/* Bluetooth */}
          <button
            onClick={toggleBluetooth}
            className="flex items-center space-x-2.5 text-left w-full group cursor-pointer"
          >
            <div
              className={`w-7 h-7 rounded-full flex items-center justify-center transition-colors ${
                bluetoothEnabled ? 'bg-blue-500 text-white shadow-md' : 'bg-white/10 text-slate-400'
              }`}
            >
              <Bluetooth className="w-3.5 h-3.5" />
            </div>
            <div className="overflow-hidden">
              <div className="text-xs font-semibold leading-tight">Bluetooth</div>
              <div className="text-[10px] text-slate-400 truncate">
                {bluetoothEnabled ? 'On' : 'Off'}
              </div>
            </div>
          </button>
        </div>

        {/* Right Column: Focus & Dark Mode */}
        <div className="flex flex-col gap-2">
          {/* Focus / Do Not Disturb */}
          <button
            onClick={() => {}}
            className="p-2.5 rounded-2xl bg-white/5 border border-white/5 flex items-center space-x-2.5 text-left hover:bg-white/10 transition-colors h-1/2"
          >
            <div className="w-7 h-7 rounded-full bg-indigo-500 text-white flex items-center justify-center shadow-md">
              <Moon className="w-3.5 h-3.5" />
            </div>
            <div>
              <div className="text-xs font-semibold leading-tight">Focus</div>
              <div className="text-[10px] text-slate-400">Do Not Disturb</div>
            </div>
          </button>

          {/* Dark Mode / Night Shift */}
          <button
            onClick={() => setTheme(theme === 'dark' ? 'light' : 'dark')}
            className="p-2.5 rounded-2xl bg-white/5 border border-white/5 flex items-center space-x-2.5 text-left hover:bg-white/10 transition-colors h-1/2"
          >
            <div
              className={`w-7 h-7 rounded-full flex items-center justify-center transition-colors ${
                theme === 'dark' ? 'bg-blue-500 text-white shadow-md' : 'bg-amber-500 text-white'
              }`}
            >
              {theme === 'dark' ? <Moon className="w-3.5 h-3.5" /> : <Sun className="w-3.5 h-3.5" />}
            </div>
            <div>
              <div className="text-xs font-semibold leading-tight">Dark Mode</div>
              <div className="text-[10px] text-slate-400">{theme === 'dark' ? 'On' : 'Off'}</div>
            </div>
          </button>
        </div>
      </div>

      {/* Display Slider */}
      <div className="p-3 rounded-2xl bg-white/5 border border-white/5 flex flex-col gap-2">
        <span className="text-xs font-semibold text-slate-200">Display</span>
        <div className="flex items-center gap-2.5">
          <Sun className="w-4 h-4 text-slate-400" />
          <input
            type="range"
            min="10"
            max="100"
            value={brightness}
            onChange={(e) => setBrightness(Number(e.target.value))}
            className="flex-1 h-2 bg-slate-700/60 rounded-full appearance-none cursor-pointer accent-blue-500"
          />
          <span className="text-[11px] font-mono text-slate-400 w-8 text-right">{brightness}%</span>
        </div>
      </div>

      {/* Sound Slider */}
      <div className="p-3 rounded-2xl bg-white/5 border border-white/5 flex flex-col gap-2">
        <div className="flex justify-between items-center text-xs">
          <span className="font-semibold text-slate-200">Sound</span>
          <span className="text-[10px] text-slate-400">Master Output</span>
        </div>
        <div className="flex items-center gap-2.5">
          <button
            onClick={toggleMute}
            className="text-slate-400 hover:text-white cursor-pointer"
          >
            {isMuted || volume === 0 ? (
              <VolumeX className="w-4 h-4 text-rose-400" />
            ) : (
              <Volume2 className="w-4 h-4" />
            )}
          </button>
          <input
            type="range"
            min="0"
            max="100"
            value={isMuted ? 0 : volume}
            onChange={(e) => changeVolume(Number(e.target.value))}
            className="flex-1 h-2 bg-slate-700/60 rounded-full appearance-none cursor-pointer accent-blue-500"
          />
          <span className="text-[11px] font-mono text-slate-400 w-8 text-right">
            {isMuted ? '0%' : `${volume}%`}
          </span>
        </div>
      </div>

      {/* Media Player Card ("Now Playing") */}
      <div className="p-2.5 rounded-2xl bg-white/5 border border-white/5 flex items-center justify-between">
        <div className="flex items-center space-x-2.5">
          <div className="w-10 h-10 rounded-xl bg-gradient-to-tr from-rose-500 to-amber-500 flex items-center justify-center text-white shadow-md">
            <Play className="w-4 h-4 fill-white" />
          </div>
          <div>
            <div className="text-xs font-semibold text-slate-100">Horizon Symphony</div>
            <div className="text-[10px] text-slate-400">AxisOS Sound Engine</div>
          </div>
        </div>
        <Airplay className="w-4 h-4 text-slate-400 mr-1" />
      </div>
    </div>
  );
};
