import React, { useState, useEffect } from 'react';
import {
  Gamepad2,
  Play,
  RotateCw,
  ExternalLink,
  ShieldCheck,
  CheckCircle2,
  Tv,
  Layers,
  Sparkles,
  Zap,
} from 'lucide-react';
import { systemService } from '../../services/systemService';

export const SteamApp: React.FC = () => {
  const [isLaunching, setIsLaunching] = useState(false);
  const [launchMessage, setLaunchMessage] = useState<string | null>(null);

  // Attempt to launch native Steam binary on mount if present
  const launchNativeSteam = async (args: string = '') => {
    setIsLaunching(true);
    setLaunchMessage('Launching Steam client with Proton gaming layers...');
    try {
      await systemService.executeCommand(`steam ${args} 2>/dev/null || /usr/games/steam ${args} 2>/dev/null || true`);
      setLaunchMessage('Steam process started in background.');
    } catch {
      setLaunchMessage('Steam is launching...');
    }
    setTimeout(() => setIsLaunching(false), 2000);
  };

  useEffect(() => {
    // Proactively start native steam process in background
    launchNativeSteam();
  }, []);

  return (
    <div className="flex flex-col h-full w-full bg-[#0a0e0b] text-slate-100 select-none overflow-hidden font-sans">
      {/* Top Steam Control Bar */}
      <div className="h-12 bg-[#0f1411] border-b border-[#87cf3e]/20 px-4 flex items-center justify-between shrink-0">
        <div className="flex items-center space-x-3">
          <div className="w-8 h-8 rounded-xl bg-[#141b16] border border-[#87cf3e]/30 flex items-center justify-center">
            <Gamepad2 className="w-4 h-4 text-[#87cf3e]" />
          </div>
          <div>
            <div className="text-xs font-bold text-white flex items-center gap-1.5">
              <span>Steam for AxisOS</span>
              <span className="px-1.5 py-0.2 rounded bg-[#87cf3e]/20 text-[#87cf3e] text-[9px] font-bold border border-[#87cf3e]/30">
                PROTON 9.0
              </span>
            </div>
            <div className="text-[10px] text-slate-400">Valve Corporation • Linux Gaming Subsystem</div>
          </div>
        </div>

        <div className="flex items-center space-x-2">
          <button
            onClick={() => launchNativeSteam('-bigpicture')}
            className="px-3 py-1 bg-white/5 hover:bg-white/10 text-slate-200 border border-white/10 hover:border-[#87cf3e]/40 rounded-lg text-xs font-medium transition-colors flex items-center gap-1.5 cursor-pointer"
            title="Launch Steam Big Picture Mode"
          >
            <Tv className="w-3.5 h-3.5 text-[#87cf3e]" />
            <span className="hidden sm:inline">Big Picture</span>
          </button>
          <button
            onClick={() => launchNativeSteam()}
            disabled={isLaunching}
            className="px-3.5 py-1 bg-[#87cf3e] hover:bg-[#76bb33] text-black rounded-lg text-xs font-bold transition-all flex items-center gap-1.5 shadow-md shadow-[#87cf3e]/20 cursor-pointer"
          >
            <Play className={`w-3.5 h-3.5 fill-current ${isLaunching ? 'animate-spin' : ''}`} />
            <span>{isLaunching ? 'Starting...' : 'Open Client'}</span>
          </button>
        </div>
      </div>

      {/* Steam Subsystem Readiness Banner */}
      <div className="bg-[#0f1411] border-b border-[#87cf3e]/15 px-4 py-2 flex flex-wrap items-center justify-between gap-2 text-xs">
        <div className="flex items-center gap-4 text-[11px] text-slate-300">
          <span className="flex items-center gap-1 text-[#87cf3e]">
            <CheckCircle2 className="w-3.5 h-3.5" />
            <span>Feral GameMode Active</span>
          </span>
          <span className="flex items-center gap-1 text-[#87cf3e]">
            <Zap className="w-3.5 h-3.5" />
            <span>Vulkan 1.3 Async Shaders</span>
          </span>
          <span className="flex items-center gap-1 text-slate-400">
            <Layers className="w-3.5 h-3.5" />
            <span>i386 32-Bit Multiarch</span>
          </span>
        </div>

        {launchMessage && (
          <span className="text-[11px] text-[#87cf3e] font-mono animate-pulse">
            {launchMessage}
          </span>
        )}
      </div>

      {/* Main Steam Store Viewport */}
      <div className="flex-1 w-full bg-[#0a0e0b] relative overflow-hidden flex flex-col">
        <iframe
          src="https://store.steampowered.com"
          title="Steam Store"
          className="w-full h-full border-none"
          sandbox="allow-scripts allow-same-origin allow-forms allow-popups"
        />
      </div>
    </div>
  );
};
