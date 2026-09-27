import React, { useState } from 'react';
import { Sparkles, Check, RefreshCw, ExternalLink } from 'lucide-react';
import { useSystemState } from '../../context/SystemStateContext';

export const AboutApp: React.FC = () => {
  const { systemInfo } = useSystemState();
  const [checkingUpdates, setCheckingUpdates] = useState(false);
  const [updateStatus, setUpdateStatus] = useState<string | null>(null);

  const handleCheckUpdates = () => {
    setCheckingUpdates(true);
    setUpdateStatus(null);
    setTimeout(() => {
      setCheckingUpdates(false);
      setUpdateStatus('Your AxisOS system is up to date (Kernel 6.12 & packages current).');
    }, 1500);
  };

  return (
    <div className="flex flex-col items-center justify-center p-6 text-center h-full bg-slate-950 text-slate-100 select-none">
      {/* OS Logo Badge */}
      <div className="w-16 h-16 rounded-2xl bg-gradient-to-tr from-cyan-500 to-blue-600 flex items-center justify-center font-black text-white text-2xl shadow-xl shadow-cyan-950/60 mb-3">
        ▲
      </div>

      <h1 className="text-xl font-black tracking-tight">{systemInfo.osName}</h1>
      <div className="text-xs text-cyan-400 font-mono mt-0.5">{systemInfo.osVersion}</div>

      <div className="mt-6 w-full max-w-sm bg-slate-900 border border-white/10 rounded-2xl p-4 text-xs flex flex-col gap-2.5 text-left">
        <div className="flex justify-between">
          <span className="text-slate-400">Linux Kernel</span>
          <span className="font-semibold text-slate-200 font-mono">6.12.0-axisos</span>
        </div>
        <div className="flex justify-between border-t border-white/5 pt-2">
          <span className="text-slate-400">Display Compositor</span>
          <span className="font-semibold text-slate-200">Wayland Native</span>
        </div>
        <div className="flex justify-between border-t border-white/5 pt-2">
          <span className="text-slate-400">Base Userspace</span>
          <span className="font-semibold text-slate-200">Debian Stable Core</span>
        </div>
        <div className="flex justify-between border-t border-white/5 pt-2">
          <span className="text-slate-400">Repository</span>
          <a
            href="https://github.com/jackyphuti/AxisOS"
            target="_blank"
            rel="noreferrer"
            className="font-semibold text-cyan-400 hover:underline flex items-center gap-1"
          >
            <span>GitHub</span>
            <ExternalLink className="w-3 h-3" />
          </a>
        </div>
      </div>

      {updateStatus && (
        <div className="mt-4 text-[11px] text-emerald-400 flex items-center gap-1 font-mono">
          <Check className="w-3.5 h-3.5" />
          <span>{updateStatus}</span>
        </div>
      )}

      <button
        onClick={handleCheckUpdates}
        disabled={checkingUpdates}
        className="mt-5 px-4 py-2 rounded-xl bg-white/5 hover:bg-white/10 border border-white/10 text-xs font-semibold text-slate-200 transition-colors flex items-center gap-2"
      >
        <RefreshCw className={`w-3.5 h-3.5 ${checkingUpdates ? 'animate-spin text-cyan-400' : ''}`} />
        <span>{checkingUpdates ? 'Checking Repositories...' : 'Check for System Updates'}</span>
      </button>
    </div>
  );
};
