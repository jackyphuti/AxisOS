import React, { useState } from 'react';
import { HardDrive, Sparkles, X, ShieldCheck, ArrowRight, CheckCircle2 } from 'lucide-react';
import { useSystemState, ACCENT_COLOR_MAP } from '../context/SystemStateContext';
import { useWindowManager } from '../context/WindowManagerContext';

export const LiveWelcomeModal: React.FC = () => {
  const { isLiveEnvironment, autoInstall, accentColor } = useSystemState();
  const { openApp, closeApp } = useWindowManager();
  const [isOpen, setIsOpen] = useState(true);
  const accent = ACCENT_COLOR_MAP[accentColor];

  if (!isLiveEnvironment || autoInstall || !isOpen) {
    return null;
  }

  const handleStartInstaller = () => {
    setIsOpen(false);
    openApp('installer');
  };

  const handleTryLive = () => {
    setIsOpen(false);
    closeApp('installer');
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/60 backdrop-blur-md animate-in fade-in duration-300 p-4 select-none">
      <div className="relative w-full max-w-xl rounded-3xl bg-slate-900/90 border border-white/15 p-8 shadow-2xl shadow-black/80 flex flex-col items-center text-center overflow-hidden">
        {/* Ambient Glow */}
        <div className="absolute -top-24 -left-24 w-60 h-60 bg-blue-500/20 rounded-full blur-3xl pointer-events-none" />
        <div className="absolute -bottom-24 -right-24 w-60 h-60 bg-cyan-500/20 rounded-full blur-3xl pointer-events-none" />

        {/* Close Button */}
        <button
          onClick={handleTryLive}
          className="absolute top-5 right-5 p-2 rounded-full bg-white/5 hover:bg-white/10 text-slate-400 hover:text-white transition-colors"
          title="Explore Live Session"
        >
          <X className="w-4 h-4" />
        </button>

        {/* Logo Badge */}
        <div className="w-16 h-16 rounded-2xl bg-gradient-to-tr from-cyan-500 via-blue-600 to-indigo-600 flex items-center justify-center text-white shadow-xl shadow-cyan-950/60 mb-5 ring-4 ring-white/10">
          <span className="text-2xl font-black tracking-tighter">▲</span>
        </div>

        {/* Title & Description */}
        <h1 className="text-2xl font-black text-slate-100 tracking-tight">
          Welcome to AxisOS 2.0 "Horizon"
        </h1>
        <p className="text-xs text-slate-400 mt-2 max-w-md leading-relaxed">
          You are currently running in <span className="text-cyan-400 font-semibold">Live Mode</span> directly from RAM.
          Your internal disks and existing operating systems are 100% safe and untouched.
        </p>

        {/* Secure Boot & UEFI Certification Badge */}
        <div className="mt-4 px-3.5 py-1.5 rounded-full bg-emerald-500/10 border border-emerald-500/20 text-emerald-400 text-[11px] font-medium flex items-center gap-1.5">
          <ShieldCheck className="w-3.5 h-3.5" />
          <span>Microsoft UEFI CA Signed & Compatible with Secure Boot</span>
        </div>

        {/* Action Cards */}
        <div className="grid grid-cols-1 sm:grid-cols-2 gap-4 w-full mt-7 text-left">
          {/* Option 1: Install to Drive */}
          <button
            onClick={handleStartInstaller}
            className="group relative p-5 rounded-2xl bg-gradient-to-b from-white/10 to-white/5 hover:from-cyan-500/20 hover:to-blue-600/20 border border-white/15 hover:border-cyan-500/40 transition-all duration-200 flex flex-col justify-between shadow-lg cursor-pointer"
          >
            <div>
              <div className="w-10 h-10 rounded-xl bg-gradient-to-tr from-cyan-500 to-blue-600 flex items-center justify-center text-white shadow-md mb-3 group-hover:scale-105 transition-transform">
                <HardDrive className="w-5 h-5 text-cyan-100" />
              </div>
              <div className="text-sm font-bold text-slate-100 group-hover:text-cyan-300 transition-colors">
                Install AxisOS
              </div>
              <p className="text-[11px] text-slate-400 mt-1 leading-normal">
                Open the graphical installer. Partition an internal or external drive with UEFI Secure Boot support.
              </p>
            </div>
            <div className="mt-4 flex items-center gap-1.5 text-xs font-semibold text-cyan-400 group-hover:translate-x-1 transition-transform">
              <span>Start Installation</span>
              <ArrowRight className="w-3.5 h-3.5" />
            </div>
          </button>

          {/* Option 2: Try Live */}
          <button
            onClick={handleTryLive}
            className="group relative p-5 rounded-2xl bg-white/5 hover:bg-white/10 border border-white/10 hover:border-white/20 transition-all duration-200 flex flex-col justify-between shadow-lg cursor-pointer"
          >
            <div>
              <div className="w-10 h-10 rounded-xl bg-slate-800 flex items-center justify-center text-slate-300 shadow-md mb-3 group-hover:scale-105 transition-transform">
                <Sparkles className="w-5 h-5 text-amber-400" />
              </div>
              <div className="text-sm font-bold text-slate-200 group-hover:text-white transition-colors">
                Try Live Demo
              </div>
              <p className="text-[11px] text-slate-400 mt-1 leading-normal">
                Explore the Horizon desktop shell, test Axis Browser (Chromium), Axis Store, Terminal, and hardware without modifying any disks.
              </p>
            </div>
            <div className="mt-4 flex items-center gap-1.5 text-xs font-semibold text-slate-300 group-hover:translate-x-1 transition-transform">
              <span>Explore Desktop</span>
              <ArrowRight className="w-3.5 h-3.5" />
            </div>
          </button>
        </div>

        {/* Footer Features */}
        <div className="mt-6 pt-4 border-t border-white/5 w-full flex items-center justify-around text-[10px] text-slate-500">
          <div className="flex items-center gap-1">
            <CheckCircle2 className="w-3 h-3 text-emerald-400" />
            <span>Automatic UEFI Registration</span>
          </div>
          <div className="flex items-center gap-1">
            <CheckCircle2 className="w-3 h-3 text-emerald-400" />
            <span>BitLocker Safety Checks</span>
          </div>
          <div className="flex items-center gap-1">
            <CheckCircle2 className="w-3 h-3 text-emerald-400" />
            <span>Btrfs zstd Compression</span>
          </div>
        </div>
      </div>
    </div>
  );
};
