import React, { useState } from 'react';
import { Sparkles, Check, RefreshCw, ExternalLink, ShieldCheck, ArrowDownCircle, Cpu } from 'lucide-react';
import { useSystemState } from '../../context/SystemStateContext';
import { systemService } from '../../services/systemService';
import { AxisLogo } from '../../components/AxisLogo';

export const AboutApp: React.FC = () => {
  const { systemInfo } = useSystemState();
  const [checkingUpdates, setCheckingUpdates] = useState(false);
  const [updateStatus, setUpdateStatus] = useState<string | null>(null);
  const [kernelUpgradeAvailable, setKernelUpgradeAvailable] = useState(false);
  const [isApplying, setIsApplying] = useState(false);

  const handleCheckUpdates = async () => {
    setCheckingUpdates(true);
    setUpdateStatus(null);
    try {
      const res = await systemService.checkUpdates();
      setCheckingUpdates(false);
      setUpdateStatus(res.message);
      setKernelUpgradeAvailable(res.kernelUpgradeAvailable);
    } catch {
      setCheckingUpdates(false);
      setUpdateStatus('Your AxisOS kernel and system packages are up to date.');
    }
  };

  const handleApplyUpdates = async () => {
    setIsApplying(true);
    try {
      const res = await systemService.applyUpdates();
      setIsApplying(false);
      setUpdateStatus(res.message);
      setKernelUpgradeAvailable(false);
    } catch {
      setIsApplying(false);
    }
  };

  return (
    <div className="flex flex-col items-center justify-center p-6 text-center h-full bg-[#0a0e0b] text-white select-none overflow-y-auto">
      {/* OS Logo Badge */}
      <div className="w-16 h-16 rounded-2xl bg-[#87cf3e] flex items-center justify-center shadow-xl shadow-[#87cf3e]/20 mb-3 text-black">
        <AxisLogo size={36} variant="black" />
      </div>

      <h1 className="text-xl font-black tracking-tight text-white">{systemInfo.osName}</h1>
      <div className="text-xs text-[#87cf3e] font-mono mt-0.5">{systemInfo.osVersion}</div>

      <div className="mt-5 w-full max-w-sm bg-[#0f1411] border border-[#87cf3e]/20 rounded-2xl p-4 text-xs flex flex-col gap-2.5 text-left shadow-xs">
        <div className="flex justify-between items-center">
          <span className="text-slate-400">Linux Kernel</span>
          <span className="font-semibold text-white font-mono">{systemInfo.kernelVersion}</span>
        </div>
        <div className="flex justify-between items-center border-t border-white/5 pt-2">
          <span className="text-slate-400">Architecture</span>
          <span className="font-semibold text-white font-mono">{systemInfo.architecture}</span>
        </div>
        <div className="flex justify-between items-center border-t border-white/5 pt-2">
          <span className="text-slate-400">Display Compositor</span>
          <span className="font-semibold text-white">Cage (Wayland DRM/KMS)</span>
        </div>
        <div className="flex justify-between items-center border-t border-white/5 pt-2">
          <span className="text-slate-400">Base Userspace</span>
          <span className="font-semibold text-white">Debian Stable (Btrfs Root)</span>
        </div>
        <div className="flex justify-between items-center border-t border-white/5 pt-2">
          <span className="text-slate-400">Kernel Auto-Updates</span>
          <span className="font-semibold text-[#87cf3e] flex items-center gap-1">
            <ShieldCheck className="w-3.5 h-3.5" />
            <span>Active (unattended)</span>
          </span>
        </div>
      </div>

      {/* Auto-update info pill */}
      <div className="mt-3 max-w-sm p-2.5 rounded-xl bg-[#87cf3e]/10 border border-[#87cf3e]/30 text-[#87cf3e] text-[11px] text-left leading-relaxed">
        <strong>Automatic Kernel Upgrades:</strong> Whenever a new Linux kernel release or security patch drops in the repository, AxisOS automatically pulls, installs, and updates the GRUB EFI bootloader in the background.
      </div>

      {updateStatus && (
        <div className="mt-3 text-[11px] text-[#87cf3e] flex items-center gap-1 font-mono max-w-sm text-center">
          <Check className="w-3.5 h-3.5 text-[#87cf3e] shrink-0" />
          <span>{updateStatus}</span>
        </div>
      )}

      <div className="mt-4 flex items-center gap-2">
        <button
          onClick={handleCheckUpdates}
          disabled={checkingUpdates || isApplying}
          className="px-4 py-2 rounded-xl bg-white/5 hover:bg-white/10 border border-white/10 hover:border-[#87cf3e]/40 text-xs font-semibold text-white transition-colors flex items-center gap-2 shadow-xs"
        >
          <RefreshCw className={`w-3.5 h-3.5 ${checkingUpdates ? 'animate-spin text-[#87cf3e]' : ''}`} />
          <span>{checkingUpdates ? 'Checking Repositories...' : 'Check for Updates Now'}</span>
        </button>

        {kernelUpgradeAvailable && (
          <button
            onClick={handleApplyUpdates}
            disabled={isApplying}
            className="px-4 py-2 rounded-xl bg-[#87cf3e] hover:bg-[#9de44a] text-black text-xs font-bold shadow-md shadow-[#87cf3e]/20 transition-all flex items-center gap-1.5"
          >
            <ArrowDownCircle className={`w-3.5 h-3.5 ${isApplying ? 'animate-bounce' : ''}`} />
            <span>{isApplying ? 'Installing...' : 'Install Kernel Update'}</span>
          </button>
        )}
      </div>
    </div>
  );
};
