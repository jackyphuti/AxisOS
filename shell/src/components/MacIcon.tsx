import React from 'react';
import { AppId } from '../types/os';

interface MacIconProps {
  id: AppId | 'trash' | 'browser' | 'calculator';
  size?: number; // size in px, defaults to 54
  className?: string;
}

export const MacIcon: React.FC<MacIconProps> = ({ id, size = 54, className = '' }) => {
  const containerStyle = {
    width: `${size}px`,
    height: `${size}px`,
  };

  switch (id) {
    case 'file-manager':
      // macOS Finder Icon (Blue smiley face)
      return (
        <div
          style={containerStyle}
          className={`relative rounded-[22%] shadow-lg shadow-blue-950/40 overflow-hidden flex items-center justify-center transition-transform ${className}`}
        >
          {/* Base gradient */}
          <div className="absolute inset-0 bg-gradient-to-b from-[#60a5fa] via-[#3b82f6] to-[#1d4ed8]"></div>
          {/* Subtle glass reflection highlight */}
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/35 to-transparent"></div>
          {/* Finder Face SVG */}
          <svg viewBox="0 0 100 100" className="w-[82%] h-[82%] z-10 drop-shadow-md">
            {/* Split face background tone */}
            <path
              d="M18,18 Q50,14 82,18 Q86,50 82,82 Q50,86 18,82 Q14,50 18,18 Z"
              fill="#93c5fd"
            />
            {/* Left face shadow */}
            <path
              d="M18,18 Q50,14 50,50 Q50,86 18,82 Q14,50 18,18 Z"
              fill="#60a5fa"
            />
            {/* Eyes */}
            <ellipse cx="36" cy="38" rx="4.5" ry="6" fill="#1e293b" />
            <ellipse cx="64" cy="38" rx="4.5" ry="6" fill="#1e293b" />
            {/* Nose line */}
            <path
              d="M50,30 Q46,48 40,54 L52,54"
              stroke="#1e293b"
              strokeWidth="4"
              strokeLinecap="round"
              strokeLinejoin="round"
              fill="none"
            />
            {/* Smiling mouth */}
            <path
              d="M32,66 Q50,80 68,66"
              stroke="#1e293b"
              strokeWidth="4.5"
              strokeLinecap="round"
              fill="none"
            />
          </svg>
          <div className="absolute inset-0 rounded-[22%] ring-1 ring-inset ring-white/30 pointer-events-none"></div>
        </div>
      );

    case 'terminal':
      // macOS Terminal Icon
      return (
        <div
          style={containerStyle}
          className={`relative rounded-[22%] shadow-lg shadow-black/60 overflow-hidden flex items-center justify-center ${className}`}
        >
          {/* Deep dark brushed slate */}
          <div className="absolute inset-0 bg-gradient-to-b from-[#334155] via-[#1e293b] to-[#0f172a]"></div>
          {/* Glass glare */}
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/20 to-transparent"></div>
          {/* Terminal Screen frame */}
          <div className="w-[82%] h-[82%] rounded-[18%] bg-black/90 border border-slate-700/80 p-2 flex flex-col justify-between z-10 shadow-inner">
            <div className="flex items-center space-x-1">
              <span className="text-[#38bdf8] font-mono font-black text-sm select-none leading-none">&gt;_</span>
            </div>
            <div className="w-2.5 h-1 bg-[#34d399] rounded-xs animate-pulse"></div>
          </div>
          <div className="absolute inset-0 rounded-[22%] ring-1 ring-inset ring-white/20 pointer-events-none"></div>
        </div>
      );

    case 'browser':
      // Axis Browser Icon (Chromium-Powered Modern Squircle)
      return (
        <div
          style={containerStyle}
          className={`relative rounded-[22%] shadow-lg shadow-cyan-950/50 overflow-hidden flex items-center justify-center ${className}`}
        >
          {/* Base gradient */}
          <div className="absolute inset-0 bg-gradient-to-b from-[#0ea5e9] via-[#0284c7] to-[#0f172a]"></div>
          {/* Glass reflection highlight */}
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/35 to-transparent"></div>
          
          {/* Chromium Tri-Color Orbit Ring & Core */}
          <div className="relative w-[78%] h-[78%] rounded-full bg-slate-950/80 p-1 flex items-center justify-center shadow-inner">
            <svg viewBox="0 0 100 100" className="w-full h-full drop-shadow">
              {/* Top segment (Cyan) */}
              <path
                d="M50,12 A38,38 0 0,1 83,31 L64,64 L50,50 Z"
                fill="#38bdf8"
              />
              {/* Bottom right segment (Emerald) */}
              <path
                d="M83,31 A38,38 0 0,1 50,88 L50,50 L64,64 Z"
                fill="#34d399"
              />
              {/* Left segment (Amber/Rose) */}
              <path
                d="M50,88 A38,38 0 0,1 17,31 L50,50 Z"
                fill="#fbbf24"
              />
              {/* Center orb */}
              <circle cx="50" cy="50" r="18" fill="#ffffff" />
              <circle cx="50" cy="50" r="14" fill="#0284c7" />
              <circle cx="45" cy="45" r="4" fill="#ffffff" opacity="0.6" />
            </svg>
          </div>
          <div className="absolute inset-0 rounded-[22%] ring-1 ring-inset ring-white/35 pointer-events-none"></div>
        </div>
      );

    case 'settings':
      // macOS System Settings Mechanical Gears
      return (
        <div
          style={containerStyle}
          className={`relative rounded-[22%] shadow-lg shadow-slate-950/50 overflow-hidden flex items-center justify-center ${className}`}
        >
          {/* Brushed metallic aluminum background */}
          <div className="absolute inset-0 bg-gradient-to-b from-[#94a3b8] via-[#64748b] to-[#475569]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/35 to-transparent"></div>
          {/* Gear icon */}
          <svg viewBox="0 0 24 24" className="w-[62%] h-[62%] z-10 text-white drop-shadow-md">
            <path
              fill="currentColor"
              d="M12 15.5A3.5 3.5 0 0 1 8.5 12A3.5 3.5 0 0 1 12 8.5a3.5 3.5 0 0 1 3.5 3.5a3.5 3.5 0 0 1-3.5 3.5m7.43-2.53c.04-.32.07-.64.07-.97c0-.33-.03-.66-.07-1l2.11-1.63c.19-.15.24-.42.12-.64l-2-3.46c-.12-.22-.39-.31-.61-.22l-2.49 1c-.52-.39-1.06-.73-1.69-.98l-.37-2.65A.506.506 0 0 0 14 2h-4c-.25 0-.46.18-.5.42l-.37 2.65c-.63.25-1.17.59-1.69.98l-2.49-1c-.22-.09-.49 0-.61.22l-2 3.46c-.13.22-.07.49.12.64L4.57 11c-.04.34-.07.67-.07 1c0 .33.03.65.07.97l-2.11 1.66c-.19.15-.25.42-.12.64l2 3.46c.12.22.39.3.61.22l2.49-1.01c.52.4 1.06.74 1.69.99l.37 2.65c.04.24.25.42.5.42h4c.25 0 .46-.18.5-.42l.37-2.65c.63-.26 1.17-.59 1.69-.99l2.49 1.01c.22.08.49 0 .61-.22l2-3.46c.12-.22.07-.49-.12-.64l-2.11-1.66Z"
            />
          </svg>
          <div className="absolute inset-0 rounded-[22%] ring-1 ring-inset ring-white/30 pointer-events-none"></div>
        </div>
      );

    case 'system-monitor':
      // Activity Monitor
      return (
        <div
          style={containerStyle}
          className={`relative rounded-[22%] shadow-lg shadow-black/50 overflow-hidden flex items-center justify-center ${className}`}
        >
          {/* Deep dark screen */}
          <div className="absolute inset-0 bg-gradient-to-b from-[#1e293b] via-[#0f172a] to-[#020617]"></div>
          {/* Grid lines */}
          <div className="absolute inset-2 grid grid-cols-4 grid-rows-4 border border-cyan-500/20 rounded-md">
            <div className="border-r border-b border-cyan-500/10"></div>
            <div className="border-r border-b border-cyan-500/10"></div>
            <div className="border-r border-b border-cyan-500/10"></div>
            <div className="border-b border-cyan-500/10"></div>
          </div>
          {/* Neon ECG wave */}
          <svg viewBox="0 0 100 60" className="w-[80%] h-[60%] z-10 drop-shadow-[0_0_8px_rgba(52,211,153,0.8)]">
            <path
              d="M0,30 L25,30 L35,10 L45,50 L55,20 L65,35 L72,30 L100,30"
              fill="none"
              stroke="#34d399"
              strokeWidth="4"
              strokeLinecap="round"
              strokeLinejoin="round"
            />
          </svg>
          <div className="absolute inset-0 rounded-[22%] ring-1 ring-inset ring-white/20 pointer-events-none"></div>
        </div>
      );

    case 'text-editor':
      // macOS Notes / TextEdit Icon
      return (
        <div
          style={containerStyle}
          className={`relative rounded-[22%] shadow-lg shadow-amber-950/40 overflow-hidden flex items-center justify-center ${className}`}
        >
          {/* Yellow Legal Pad */}
          <div className="absolute inset-0 bg-gradient-to-b from-[#fef08a] via-[#fde047] to-[#eab308]"></div>
          {/* Top binder strip */}
          <div className="absolute inset-x-0 top-0 h-4 bg-gradient-to-r from-amber-600 to-amber-700 border-b border-amber-800"></div>
          {/* Ruled lines */}
          <div className="w-[78%] h-[65%] mt-3 flex flex-col justify-around z-10">
            <div className="h-[1.5px] bg-amber-400/80 w-full"></div>
            <div className="h-[1.5px] bg-amber-400/80 w-full"></div>
            <div className="h-[1.5px] bg-amber-400/80 w-4/5"></div>
            <div className="h-[1.5px] bg-amber-400/80 w-3/5"></div>
          </div>
          {/* Pencil */}
          <div className="absolute bottom-2 right-2 w-7 h-7 -rotate-45 drop-shadow-md">
            <svg viewBox="0 0 24 24" className="w-full h-full text-slate-800 fill-amber-500">
              <path d="M3 17.25V21h3.75L17.81 9.94l-3.75-3.75L3 17.25zM20.71 7.04c.39-.39.39-1.02 0-1.41l-2.34-2.34c-.39-.39-1.02-.39-1.41 0l-1.83 1.83 3.75 3.75 1.83-1.83z" />
            </svg>
          </div>
          <div className="absolute inset-0 rounded-[22%] ring-1 ring-inset ring-white/30 pointer-events-none"></div>
        </div>
      );

    case 'calculator':
      // macOS Calculator
      return (
        <div
          style={containerStyle}
          className={`relative rounded-[22%] shadow-lg shadow-black/50 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-b from-[#334155] via-[#1e293b] to-[#0f172a]"></div>
          <div className="grid grid-cols-2 gap-1.5 p-2 z-10 w-full h-full items-center justify-center">
            <div className="w-4 h-4 rounded-full bg-slate-600 flex items-center justify-center text-[9px] font-bold text-white">C</div>
            <div className="w-4 h-4 rounded-full bg-amber-500 flex items-center justify-center text-[9px] font-bold text-white">÷</div>
            <div className="w-4 h-4 rounded-full bg-slate-700 flex items-center justify-center text-[9px] font-bold text-white">7</div>
            <div className="w-4 h-4 rounded-full bg-amber-500 flex items-center justify-center text-[9px] font-bold text-white">=</div>
          </div>
          <div className="absolute inset-0 rounded-[22%] ring-1 ring-inset ring-white/20 pointer-events-none"></div>
        </div>
      );

    case 'installer':
      // AxisOS Installer
      return (
        <div
          style={containerStyle}
          className={`relative rounded-[22%] shadow-lg shadow-cyan-950/50 overflow-hidden flex items-center justify-center ${className}`}
        >
          {/* Deep royal gradient */}
          <div className="absolute inset-0 bg-gradient-to-b from-[#06b6d4] via-[#2563eb] to-[#4f46e5]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/35 to-transparent"></div>
          {/* Disk and arrow */}
          <svg viewBox="0 0 60 60" className="w-[70%] h-[70%] z-10 drop-shadow-md text-white">
            <path
              fill="currentColor"
              d="M30 10 L44 26 L34 26 L34 40 L26 40 L26 26 L16 26 Z"
            />
            <path
              fill="currentColor"
              d="M10 46 L50 46 A 4 4 0 0 1 50 54 L10 54 A 4 4 0 0 1 10 46 Z"
            />
          </svg>
          <div className="absolute inset-0 rounded-[22%] ring-1 ring-inset ring-white/30 pointer-events-none"></div>
        </div>
      );

    case 'music':
      // Apple Music Style Icon
      return (
        <div
          style={containerStyle}
          className={`relative rounded-[22%] shadow-lg shadow-rose-950/40 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-b from-[#fb7185] via-[#f43f5e] to-[#e11d48]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/35 to-transparent"></div>
          {/* Musical Notes SVG */}
          <svg viewBox="0 0 60 60" className="w-[65%] h-[65%] z-10 text-white drop-shadow-md">
            <path
              fill="currentColor"
              d="M24 12 L44 8 L44 38 A 7 7 0 1 1 38 32 L38 20 L24 23 L24 44 A 7 7 0 1 1 18 38 L18 12 Z"
            />
          </svg>
          <div className="absolute inset-0 rounded-[22%] ring-1 ring-inset ring-white/30 pointer-events-none"></div>
        </div>
      );

    case 'photos':
      // Apple Photos Flower / Petals Style Icon
      return (
        <div
          style={containerStyle}
          className={`relative rounded-[22%] shadow-lg shadow-black/40 overflow-hidden flex items-center justify-center bg-white ${className}`}
        >
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white to-transparent pointer-events-none"></div>
          {/* Flower Petals */}
          <div className="relative w-[75%] h-[75%] flex items-center justify-center z-10">
            <span className="text-3xl">🌸</span>
          </div>
          <div className="absolute inset-0 rounded-[22%] ring-1 ring-inset ring-black/10 pointer-events-none"></div>
        </div>
      );

    case 'notes':
      // macOS Notes App Icon
      return (
        <div
          style={containerStyle}
          className={`relative rounded-[22%] shadow-lg shadow-amber-950/40 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-b from-[#fef08a] via-[#fde047] to-[#eab308]"></div>
          <div className="absolute inset-x-0 top-0 h-3.5 bg-gradient-to-r from-amber-600 to-amber-700 border-b border-amber-800"></div>
          <div className="w-[78%] h-[60%] mt-3 flex flex-col justify-around z-10">
            <div className="h-[1.5px] bg-amber-500/80 w-full"></div>
            <div className="h-[1.5px] bg-amber-500/80 w-full"></div>
            <div className="h-[1.5px] bg-amber-500/80 w-3/4"></div>
          </div>
          <div className="absolute inset-0 rounded-[22%] ring-1 ring-inset ring-white/30 pointer-events-none"></div>
        </div>
      );

    case 'software':
      // macOS App Store Icon
      return (
        <div
          style={containerStyle}
          className={`relative rounded-[22%] shadow-lg shadow-blue-950/50 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-b from-[#38bdf8] via-[#0284c7] to-[#1e40af]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/35 to-transparent"></div>
          {/* App Store 'A' made of ruler/pencil rods */}
          <svg viewBox="0 0 64 64" className="w-[68%] h-[68%] z-10 drop-shadow-md">
            <line x1="16" y1="52" x2="32" y2="14" stroke="#ffffff" strokeWidth="6" strokeLinecap="round" />
            <line x1="48" y1="52" x2="32" y2="14" stroke="#ffffff" strokeWidth="6" strokeLinecap="round" />
            <line x1="12" y1="42" x2="52" y2="42" stroke="#ffffff" strokeWidth="6" strokeLinecap="round" />
          </svg>
          <div className="absolute inset-0 rounded-[22%] ring-1 ring-inset ring-white/30 pointer-events-none"></div>
        </div>
      );

    case 'clock':
      // macOS Clock Icon
      return (
        <div
          style={containerStyle}
          className={`relative rounded-[22%] shadow-lg shadow-black/50 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-b from-[#1e293b] to-[#0f172a]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/20 to-transparent"></div>
          {/* Clock face */}
          <div className="relative w-[78%] h-[78%] rounded-full bg-slate-900 border border-slate-700 flex items-center justify-center shadow-inner">
            {/* Ticks */}
            <div className="absolute top-1 w-0.5 h-1.5 bg-slate-300"></div>
            <div className="absolute bottom-1 w-0.5 h-1.5 bg-slate-300"></div>
            <div className="absolute left-1 w-1.5 h-0.5 bg-slate-300"></div>
            <div className="absolute right-1 w-1.5 h-0.5 bg-slate-300"></div>
            {/* Hands */}
            <div className="absolute w-0.75 h-5 bg-white -translate-y-2 rounded-full origin-bottom rotate-45"></div>
            <div className="absolute w-0.5 h-7 bg-orange-400 -translate-y-3 rounded-full origin-bottom -rotate-45"></div>
            <div className="w-1.5 h-1.5 rounded-full bg-orange-500 z-10 border border-white"></div>
          </div>
          <div className="absolute inset-0 rounded-[22%] ring-1 ring-inset ring-white/25 pointer-events-none"></div>
        </div>
      );

    case 'weather':
      // macOS Weather Icon
      return (
        <div
          style={containerStyle}
          className={`relative rounded-[22%] shadow-lg shadow-sky-950/40 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-b from-[#38bdf8] via-[#0284c7] to-[#0369a1]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/35 to-transparent"></div>
          {/* Sun & Cloud */}
          <div className="relative w-[75%] h-[75%] flex items-center justify-center">
            {/* Sun */}
            <div className="absolute -top-1 right-1 w-7 h-7 rounded-full bg-amber-400 shadow-md shadow-amber-500/50"></div>
            {/* Cloud */}
            <svg viewBox="0 0 40 40" className="w-[85%] h-[85%] z-10 drop-shadow-md">
              <path
                d="M10,28 Q6,28 6,24 Q6,20 10,20 Q11,14 17,14 Q22,14 24,18 Q28,18 28,22 Q30,22 30,25 Q30,28 26,28 Z"
                fill="#ffffff"
                opacity="0.95"
              />
            </svg>
          </div>
          <div className="absolute inset-0 rounded-[22%] ring-1 ring-inset ring-white/30 pointer-events-none"></div>
        </div>
      );

    case 'camera':
      // macOS Camera / Photo Booth Icon
      return (
        <div
          style={containerStyle}
          className={`relative rounded-[22%] shadow-lg shadow-slate-950/60 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-b from-[#64748b] via-[#475569] to-[#334155]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/30 to-transparent"></div>
          {/* Camera body and lens */}
          <div className="relative w-[78%] h-[70%] rounded-xl bg-gradient-to-b from-slate-800 to-slate-950 border border-slate-600 flex items-center justify-center shadow-md">
            {/* Lens outer */}
            <div className="w-8 h-8 rounded-full bg-gradient-to-b from-slate-900 to-black border-2 border-slate-500 flex items-center justify-center shadow-inner">
              {/* Lens glass reflection */}
              <div className="w-5 h-5 rounded-full bg-gradient-to-tr from-sky-900 to-blue-500 opacity-80 flex items-center justify-center">
                <div className="w-1.5 h-1.5 rounded-full bg-white/70 -translate-x-0.5 -translate-y-0.5"></div>
              </div>
            </div>
            {/* Flash / Tally dot */}
            <div className="absolute top-1.5 right-2 w-1.5 h-1.5 rounded-full bg-red-500 shadow-sm shadow-red-500"></div>
          </div>
          <div className="absolute inset-0 rounded-[22%] ring-1 ring-inset ring-white/30 pointer-events-none"></div>
        </div>
      );

    case 'trash':
      // macOS Trash Can
      return (
        <div
          style={containerStyle}
          className={`relative rounded-[22%] overflow-hidden flex items-center justify-center ${className}`}
        >
          <svg viewBox="0 0 60 60" className="w-[80%] h-[80%] drop-shadow text-slate-300">
            {/* Trash mesh body */}
            <path
              d="M16,20 L20,52 Q21,55 24,55 L36,55 Q39,55 40,52 L44,20 Z"
              fill="rgba(255,255,255,0.15)"
              stroke="currentColor"
              strokeWidth="2.5"
            />
            {/* Vertical lines */}
            <line x1="25" y1="23" x2="27" y2="49" stroke="currentColor" strokeWidth="1.5" />
            <line x1="30" y1="23" x2="30" y2="49" stroke="currentColor" strokeWidth="1.5" />
            <line x1="35" y1="23" x2="33" y2="49" stroke="currentColor" strokeWidth="1.5" />
            {/* Rim */}
            <ellipse cx="30" cy="18" rx="15" ry="4" fill="none" stroke="currentColor" strokeWidth="2.5" />
          </svg>
        </div>
      );

    default:
      return (
        <div
          style={containerStyle}
          className={`relative rounded-[22%] bg-gradient-to-tr from-cyan-600 to-blue-600 flex items-center justify-center shadow-md text-white font-bold ${className}`}
        >
          ▲
        </div>
      );
  }
};
