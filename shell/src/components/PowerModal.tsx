import React, { useState } from 'react';
import { Power, RotateCcw, Lock, Moon, X } from 'lucide-react';
import { useSystemState } from '../context/SystemStateContext';

export const PowerModal: React.FC = () => {
  const { powerModalOpen, setPowerModalOpen } = useSystemState();
  const [poweringOff, setPoweringOff] = useState<string | null>(null);

  if (!powerModalOpen) return null;

  const handleAction = (action: string) => {
    setPoweringOff(action);
    setTimeout(() => {
      setPoweringOff(null);
      setPowerModalOpen(false);
    }, 2500);
  };

  return (
    <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/60 backdrop-blur-md select-none animate-in fade-in duration-200">
      <div className="w-full max-w-md bg-slate-950/95 border border-white/10 rounded-2xl p-6 shadow-2xl shadow-black flex flex-col gap-6 text-center relative">
        <button
          onClick={() => setPowerModalOpen(false)}
          className="absolute top-4 right-4 text-slate-400 hover:text-white"
        >
          <X className="w-5 h-5" />
        </button>

        {poweringOff ? (
          <div className="py-8 flex flex-col items-center gap-4">
            <div className="w-10 h-10 border-4 border-cyan-500 border-t-transparent rounded-full animate-spin"></div>
            <div className="text-base font-semibold text-slate-100">
              {poweringOff === 'poweroff' ? 'Shutting down AxisOS...' : 'Rebooting AxisOS system...'}
            </div>
            <div className="text-xs text-slate-400 font-mono">systemd: syncing disks & stopping services</div>
          </div>
        ) : (
          <>
            <div>
              <h2 className="text-lg font-bold text-slate-100">Power Options</h2>
              <p className="text-xs text-slate-400 mt-1">
                Choose an action for your AxisOS session
              </p>
            </div>

            <div className="grid grid-cols-3 gap-3">
              <button
                onClick={() => handleAction('poweroff')}
                className="flex flex-col items-center gap-2.5 p-4 rounded-xl bg-white/5 hover:bg-rose-500/20 border border-white/5 hover:border-rose-500/40 text-slate-300 hover:text-rose-200 transition-all group"
              >
                <div className="w-12 h-12 rounded-xl bg-rose-500/20 flex items-center justify-center group-hover:scale-105 transition-transform text-rose-400">
                  <Power className="w-6 h-6" />
                </div>
                <span className="text-xs font-semibold">Power Off</span>
              </button>

              <button
                onClick={() => handleAction('reboot')}
                className="flex flex-col items-center gap-2.5 p-4 rounded-xl bg-white/5 hover:bg-amber-500/20 border border-white/5 hover:border-amber-500/40 text-slate-300 hover:text-amber-200 transition-all group"
              >
                <div className="w-12 h-12 rounded-xl bg-amber-500/20 flex items-center justify-center group-hover:scale-105 transition-transform text-amber-400">
                  <RotateCcw className="w-6 h-6" />
                </div>
                <span className="text-xs font-semibold">Restart</span>
              </button>

              <button
                onClick={() => handleAction('lock')}
                className="flex flex-col items-center gap-2.5 p-4 rounded-xl bg-white/5 hover:bg-cyan-500/20 border border-white/5 hover:border-cyan-500/40 text-slate-300 hover:text-cyan-200 transition-all group"
              >
                <div className="w-12 h-12 rounded-xl bg-cyan-500/20 flex items-center justify-center group-hover:scale-105 transition-transform text-cyan-400">
                  <Lock className="w-6 h-6" />
                </div>
                <span className="text-xs font-semibold">Lock</span>
              </button>
            </div>

            <button
              onClick={() => setPowerModalOpen(false)}
              className="py-2.5 px-4 rounded-xl bg-white/5 hover:bg-white/10 text-xs font-medium text-slate-300 transition-colors"
            >
              Cancel
            </button>
          </>
        )}
      </div>
    </div>
  );
};
