import React, { useState, useEffect } from 'react';
import {
  Lock,
  Unlock,
  ArrowRight,
  Power,
  RotateCcw,
  Moon,
  User,
  ShieldCheck,
} from 'lucide-react';
import { useSystemState } from '../context/SystemStateContext';
import { AxisLogo } from './AxisLogo';
import { systemService } from '../services/systemService';

export const LockScreen: React.FC = () => {
  const {
    isLocked,
    setIsLocked,
    wallpaper,
    batteryLevel,
    isCharging,
    wifiConnected,
    setPowerModalOpen,
  } = useSystemState();

  const [passwordInput, setPasswordInput] = useState('');
  const [errorShake, setErrorShake] = useState(false);
  const [isUnlocking, setIsUnlocking] = useState(false);
  const [currentTime, setCurrentTime] = useState('');
  const [currentDate, setCurrentDate] = useState('');

  // Retrieve stored password (if set in Settings or Installer)
  const savedPassword = localStorage.getItem('axisos_user_password') || '';
  const hasPassword = Boolean(savedPassword.trim());

  useEffect(() => {
    const updateTime = () => {
      const now = new Date();
      setCurrentTime(
        now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit', hour12: false })
      );
      setCurrentDate(
        now.toLocaleDateString([], { weekday: 'long', month: 'long', day: 'numeric' })
      );
    };

    updateTime();
    const interval = setInterval(updateTime, 1000);
    return () => clearInterval(interval);
  }, []);

  if (!isLocked) return null;

  const handleUnlock = () => {
    if (!hasPassword) {
      // Direct sign in
      setIsUnlocking(true);
      setTimeout(() => {
        setIsLocked(false);
        setIsUnlocking(false);
        setPasswordInput('');
      }, 300);
      return;
    }

    if (passwordInput === savedPassword) {
      setIsUnlocking(true);
      setTimeout(() => {
        setIsLocked(false);
        setIsUnlocking(false);
        setPasswordInput('');
      }, 300);
    } else {
      setErrorShake(true);
      setTimeout(() => setErrorShake(false), 500);
      setPasswordInput('');
    }
  };

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Enter') {
      handleUnlock();
    }
  };

  return (
    <div
      onKeyDown={handleKeyDown}
      tabIndex={0}
      className={`fixed inset-0 z-[9000] flex flex-col justify-between p-10 select-none bg-cover bg-center transition-all duration-500 ease-out font-sans ${
        isUnlocking ? 'opacity-0 scale-105 pointer-events-none' : 'opacity-100 scale-100'
      }`}
      style={{ background: wallpaper.gradient }}
    >
      {/* Heavy Blur Overlay */}
      <div className="absolute inset-0 bg-black/40 backdrop-blur-3xl pointer-events-none" />

      {/* TOP HEADER: Lock status & Telemetry */}
      <div className="relative z-10 flex items-center justify-between text-white/80 text-xs">
        <div className="flex items-center gap-2">
          <AxisLogo size={20} variant="white" />
          <span className="font-semibold tracking-wider uppercase text-[11px]">AxisOS 2.0</span>
        </div>

        <div className="flex items-center gap-4">
          {wifiConnected && (
            <span className="text-[11px] text-white/70">Connected</span>
          )}
          {batteryLevel !== null && (
            <span className="text-[11px] text-white/70 font-mono">
              {batteryLevel}% {isCharging ? '⚡' : ''}
            </span>
          )}
        </div>
      </div>

      {/* CENTER: Clock, Avatar, Password / Sign In */}
      <div className="relative z-10 flex flex-col items-center justify-center text-center -translate-y-4">
        {/* Large iOS-Style Lock Clock */}
        <div className="text-7xl sm:text-8xl font-light tracking-tight text-white/95 mb-2 font-sans drop-shadow-lg">
          {currentTime || '12:00'}
        </div>
        <div className="text-base sm:text-lg font-medium text-white/80 mb-8 drop-shadow-md">
          {currentDate}
        </div>

        {/* User Card */}
        <div className="flex flex-col items-center gap-3">
          <div className="w-20 h-20 rounded-full bg-gradient-to-tr from-white/25 to-white/10 backdrop-blur-md border border-white/20 flex items-center justify-center shadow-xl text-white">
            <User className="w-10 h-10 text-white/90" />
          </div>

          <div className="text-sm font-semibold text-white tracking-wide">
            Axis User
          </div>

          {/* Password Input or Direct Sign In */}
          {hasPassword ? (
            <div
              className={`flex items-center gap-2 mt-2 transition-transform ${
                errorShake ? 'animate-[shake_0.4s_ease-in-out]' : ''
              }`}
            >
              <div className="relative flex items-center">
                <input
                  type="password"
                  value={passwordInput}
                  onChange={(e) => setPasswordInput(e.target.value)}
                  placeholder="Enter Password"
                  autoFocus
                  className="w-56 px-4 py-2 rounded-full bg-white/15 backdrop-blur-xl border border-white/20 text-white text-xs placeholder-white/50 outline-none focus:ring-2 focus:ring-[#007AFF]/60 shadow-lg text-center"
                />
                <button
                  onClick={handleUnlock}
                  className="absolute right-1.5 p-1 rounded-full bg-[#007AFF] hover:bg-[#0062CC] text-white transition-colors cursor-pointer"
                  title="Unlock"
                >
                  <ArrowRight className="w-3.5 h-3.5" />
                </button>
              </div>
            </div>
          ) : (
            <button
              onClick={handleUnlock}
              className="mt-3 px-6 py-2 rounded-full bg-white/20 hover:bg-white/30 backdrop-blur-xl border border-white/30 text-xs font-semibold text-white tracking-wide shadow-lg transition-all active:scale-95 cursor-pointer flex items-center gap-2"
            >
              <Unlock className="w-3.5 h-3.5" />
              <span>Sign In</span>
            </button>
          )}

          {errorShake && (
            <span className="text-xs text-rose-300 font-medium mt-1">
              Incorrect password. Please try again.
            </span>
          )}
        </div>
      </div>

      {/* BOTTOM FOOTER: Quick Power Controls */}
      <div className="relative z-10 flex items-center justify-between text-xs text-white/70">
        <div className="flex items-center gap-1.5 text-[11px] text-white/50">
          <ShieldCheck className="w-3.5 h-3.5 text-emerald-400" />
          <span>Session Encrypted & Secure</span>
        </div>

        <div className="flex items-center gap-3">
          <button
            onClick={() => setPowerModalOpen(true)}
            className="p-2.5 rounded-full bg-white/10 hover:bg-white/20 text-white transition-all cursor-pointer"
            title="Power Menu"
          >
            <Power className="w-4 h-4" />
          </button>
        </div>
      </div>
    </div>
  );
};
