import React, { useState, useEffect } from 'react';
import { AxisLogo } from './AxisLogo';

interface BootSplashScreenProps {
  onComplete?: () => void;
  durationMs?: number;
}

export const BootSplashScreen: React.FC<BootSplashScreenProps> = ({
  onComplete,
  durationMs = 1600,
}) => {
  const [progress, setProgress] = useState(0);
  const [isFading, setIsFading] = useState(false);
  const [isVisible, setIsVisible] = useState(true);

  useEffect(() => {
    const startTime = Date.now();
    const interval = setInterval(() => {
      const elapsed = Date.now() - startTime;
      const pct = Math.min(100, Math.round((elapsed / durationMs) * 100));
      setProgress(pct);

      if (elapsed >= durationMs) {
        clearInterval(interval);
        setIsFading(true);
        setTimeout(() => {
          setIsVisible(false);
          if (onComplete) onComplete();
        }, 400);
      }
    }, 20);

    return () => clearInterval(interval);
  }, [durationMs, onComplete]);

  if (!isVisible) return null;

  return (
    <div
      className={`fixed inset-0 z-[9999] flex flex-col items-center justify-center bg-[#000000] text-white select-none transition-opacity duration-400 ease-out ${
        isFading ? 'opacity-0 pointer-events-none' : 'opacity-100'
      }`}
    >
      {/* Centered Ambient Aura */}
      <div className="absolute w-72 h-72 rounded-full bg-gradient-to-tr from-[#007AFF]/20 to-[#BA7517]/20 blur-3xl pointer-events-none animate-pulse" />

      {/* AxisOS Official Logo */}
      <div className="relative mb-6 transform transition-transform duration-700 ease-out animate-in zoom-in-95">
        <AxisLogo size={76} glow={true} variant="primary" />
      </div>

      {/* OS Wordmark */}
      <h1 className="text-xl font-bold tracking-tight text-white/95 mb-2 font-sans">
        AxisOS 2.0
      </h1>
      <p className="text-[11px] font-medium tracking-widest uppercase text-white/40 mb-8 font-mono">
        Debian Bookworm &bull; Wayland
      </p>

      {/* Sleek Debian Linux Wayland Progress Bar */}
      <div className="w-52 h-1 bg-white/10 rounded-full overflow-hidden relative">
        <div
          className="h-full bg-gradient-to-r from-[#007AFF] via-[#5856D6] to-[#BA7517] rounded-full transition-all duration-75 ease-out shadow-[0_0_8px_rgba(0,122,255,0.8)]"
          style={{ width: `${progress}%` }}
        />
      </div>
    </div>
  );
};
