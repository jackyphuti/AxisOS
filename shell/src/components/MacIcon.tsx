import React from 'react';
import { AppId } from '../types/os';

interface IconProps {
  id: AppId | 'trash' | 'browser' | 'calculator';
  size?: number; // size in px, defaults to 54
  className?: string;
}

/**
 * AxisOS Native Icon Engine
 * =========================
 * Replaces generic macOS squircles and skeuomorphic clones with an original,
 * high-tech Linux Mint & AxisOS visual identity.
 * Features:
 * - Geometric modern tiles (16px radius, subtle border rings, ambient shadows)
 * - Mint Green (#87cf3e) and Charcoal Slate (#141b16 / #18201b) signature palette
 * - Vector-crafted iconography for every native application
 */
export const MacIcon: React.FC<IconProps> = ({ id, size = 54, className = '' }) => {
  const containerStyle = {
    width: `${size}px`,
    height: `${size}px`,
  };

  switch (id) {
    case 'file-manager':
      // Axis Files: Dual-pane folder with cyber compass bookmark in Mint Charcoal
      return (
        <div
          style={containerStyle}
          className={`relative rounded-2xl shadow-lg shadow-black/50 overflow-hidden flex items-center justify-center transition-transform ${className}`}
        >
          {/* Base Charcoal Slate */}
          <div className="absolute inset-0 bg-gradient-to-br from-[#1e2922] via-[#141b16] to-[#0c110e]"></div>
          {/* Subtle Top Specular Glass */}
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/15 to-transparent"></div>
          
          {/* Vector Folder Icon */}
          <svg viewBox="0 0 64 64" className="w-[72%] h-[72%] z-10 drop-shadow-md">
            {/* Folder Back Tab */}
            <path
              d="M10,18 Q10,14 14,14 L24,14 L28,18 L50,18 Q54,18 54,22 L54,46 Q54,50 50,50 L14,50 Q10,50 10,46 Z"
              fill="#26382c"
            />
            {/* Document sheet sticking out */}
            <rect x="18" y="20" width="28" height="20" rx="2" fill="#d1ddd5" opacity="0.85" />
            <line x1="22" y1="26" x2="36" y2="26" stroke="#18201b" strokeWidth="2" strokeLinecap="round" />
            <line x1="22" y1="31" x2="42" y2="31" stroke="#18201b" strokeWidth="2" strokeLinecap="round" />
            {/* Folder Front Flap */}
            <path
              d="M8,26 Q8,22 12,22 L52,22 Q56,22 56,26 L54,46 Q54,50 50,50 L14,50 Q10,50 8,46 Z"
              fill="url(#folderGrad)"
            />
            {/* Cyber Compass Bookmark */}
            <circle cx="32" cy="36" r="6" fill="#141b16" stroke="#87cf3e" strokeWidth="2" />
            <polygon points="32,32 34,36 32,40 30,36" fill="#87cf3e" />

            <defs>
              <linearGradient id="folderGrad" x1="0%" y1="0%" x2="100%" y2="100%">
                <stop offset="0%" stopColor="#3d5743" />
                <stop offset="100%" stopColor="#1a261e" />
              </linearGradient>
            </defs>
          </svg>
          <div className="absolute inset-0 rounded-2xl ring-1 ring-inset ring-[#87cf3e]/30 pointer-events-none"></div>
        </div>
      );

    case 'terminal':
      // Axis Console: Deep obsidian tile with glowing neon mint prompt and matrix scanline
      return (
        <div
          style={containerStyle}
          className={`relative rounded-2xl shadow-lg shadow-black/60 overflow-hidden flex items-center justify-center ${className}`}
        >
          {/* Deep dark brushed slate */}
          <div className="absolute inset-0 bg-gradient-to-b from-[#18201b] via-[#101612] to-[#080d0a]"></div>
          {/* Glass glare */}
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/10 to-transparent"></div>
          
          {/* Terminal Screen frame */}
          <div className="w-[82%] h-[82%] rounded-xl bg-black/95 border border-[#87cf3e]/30 p-2 flex flex-col justify-between z-10 shadow-inner">
            <div className="flex items-center space-x-1.5">
              <span className="text-[#87cf3e] font-mono font-black text-sm select-none leading-none">&gt;</span>
              <span className="text-slate-300 font-mono text-[9px]">axis:~#</span>
            </div>
            <div className="flex items-center justify-between">
              <div className="w-2.5 h-1.5 bg-[#87cf3e] rounded-xs animate-pulse"></div>
              <span className="text-[7px] font-mono text-[#87cf3e]/60">sh</span>
            </div>
          </div>
          <div className="absolute inset-0 rounded-2xl ring-1 ring-inset ring-[#87cf3e]/25 pointer-events-none"></div>
        </div>
      );

    case 'browser':
      // Axis Web: Dimensional planetary nexus with glowing orbital rings & mint pulse
      return (
        <div
          style={containerStyle}
          className={`relative rounded-2xl shadow-lg shadow-cyan-950/60 overflow-hidden flex items-center justify-center ${className}`}
        >
          {/* Base gradient */}
          <div className="absolute inset-0 bg-gradient-to-br from-[#0c2e36] via-[#091b22] to-[#040d12]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/20 to-transparent"></div>
          
          {/* Dimensional Orbit & Globe */}
          <svg viewBox="0 0 64 64" className="w-[78%] h-[78%] z-10 drop-shadow-md">
            {/* Outer Orbit Ring */}
            <ellipse cx="32" cy="32" rx="26" ry="10" fill="none" stroke="#00e5ff" strokeWidth="1.5" strokeDasharray="4 2" opacity="0.6" transform="rotate(-30 32 32)" />
            {/* Core Nexus Sphere */}
            <circle cx="32" cy="32" r="18" fill="url(#globeGrad)" stroke="#00e5ff" strokeWidth="1.5" />
            {/* Latitude & Longitude curves */}
            <ellipse cx="32" cy="32" rx="18" ry="7" fill="none" stroke="#87cf3e" strokeWidth="1.2" opacity="0.75" />
            <ellipse cx="32" cy="32" rx="7" ry="18" fill="none" stroke="#87cf3e" strokeWidth="1.2" opacity="0.75" />
            {/* Navigational Pulse Beacon */}
            <circle cx="44" cy="22" r="3.5" fill="#87cf3e" />
            <circle cx="44" cy="22" r="6" fill="none" stroke="#87cf3e" strokeWidth="1" opacity="0.6" className="animate-ping" />

            <defs>
              <radialGradient id="globeGrad" cx="35%" cy="35%" r="70%">
                <stop offset="0%" stopColor="#00b4d8" />
                <stop offset="70%" stopColor="#03045e" />
                <stop offset="100%" stopColor="#000814" />
              </radialGradient>
            </defs>
          </svg>
          <div className="absolute inset-0 rounded-2xl ring-1 ring-inset ring-cyan-400/30 pointer-events-none"></div>
        </div>
      );

    case 'settings':
      // Control Center: Dual precision sliders with glowing knobs and hexagonal cog
      return (
        <div
          style={containerStyle}
          className={`relative rounded-2xl shadow-lg shadow-black/50 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-br from-[#27332c] via-[#1a231e] to-[#101713]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/15 to-transparent"></div>

          <svg viewBox="0 0 64 64" className="w-[74%] h-[74%] z-10 text-white drop-shadow-md">
            {/* Hexagonal Cog in center */}
            <path
              d="M32 14 L36 14 L38 18 L43 19 L46 16 L49 19 L46 22 L47 27 L51 29 L51 33 L47 35 L46 40 L49 43 L46 46 L43 43 L38 44 L36 48 L32 48 L30 44 L25 43 L22 46 L19 43 L22 40 L21 35 L17 33 L17 29 L21 27 L22 22 L19 19 L22 16 L25 19 L30 18 Z"
              fill="#2a3d31"
              stroke="#87cf3e"
              strokeWidth="1.5"
            />
            <circle cx="32" cy="31" r="5" fill="#141b16" stroke="#87cf3e" strokeWidth="2" />
            
            {/* Horizontal precision control tracks */}
            <line x1="12" y1="52" x2="52" y2="52" stroke="#ffffff" strokeWidth="2" opacity="0.2" strokeLinecap="round" />
            <circle cx="24" cy="52" r="3.5" fill="#87cf3e" />
            <line x1="12" y1="10" x2="52" y2="10" stroke="#ffffff" strokeWidth="2" opacity="0.2" strokeLinecap="round" />
            <circle cx="40" cy="10" r="3.5" fill="#87cf3e" />
          </svg>
          <div className="absolute inset-0 rounded-2xl ring-1 ring-inset ring-[#87cf3e]/25 pointer-events-none"></div>
        </div>
      );

    case 'system-monitor':
      // Axis Diagnostics: Digital oscilloscope with CPU waveform and memory telemetry
      return (
        <div
          style={containerStyle}
          className={`relative rounded-2xl shadow-lg shadow-black/50 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-br from-[#101813] via-[#0b100d] to-[#040605]"></div>
          
          {/* Oscilloscope Grid */}
          <div className="absolute inset-2 grid grid-cols-4 grid-rows-4 border border-[#87cf3e]/20 rounded-lg">
            <div className="border-r border-b border-[#87cf3e]/10"></div>
            <div className="border-r border-b border-[#87cf3e]/10"></div>
            <div className="border-r border-b border-[#87cf3e]/10"></div>
            <div className="border-b border-[#87cf3e]/10"></div>
          </div>
          
          {/* Vector Pulse Waveform (Mint & Cyan) */}
          <svg viewBox="0 0 100 60" className="w-[84%] h-[64%] z-10 drop-shadow-[0_0_8px_rgba(135,207,62,0.8)]">
            <path
              d="M0,30 L22,30 L30,12 L40,48 L50,18 L60,36 L68,30 L100,30"
              fill="none"
              stroke="#87cf3e"
              strokeWidth="4"
              strokeLinecap="round"
              strokeLinejoin="round"
            />
          </svg>

          {/* Microchip Telemetry Nodes */}
          <div className="absolute bottom-2 left-3 flex space-x-1 z-10">
            <div className="w-1.5 h-1.5 rounded-full bg-[#00e5ff]"></div>
            <div className="w-1.5 h-1.5 rounded-full bg-[#87cf3e]"></div>
            <div className="w-1.5 h-1.5 rounded-full bg-[#ffab00]"></div>
          </div>
          <div className="absolute inset-0 rounded-2xl ring-1 ring-inset ring-[#87cf3e]/30 pointer-events-none"></div>
        </div>
      );

    case 'text-editor':
      // Axis Write: Modern code notebook with syntax-highlighted lines and laser stylus
      return (
        <div
          style={containerStyle}
          className={`relative rounded-2xl shadow-lg shadow-black/50 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-br from-[#2a3830] via-[#1c2721] to-[#121915]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/15 to-transparent"></div>

          <svg viewBox="0 0 64 64" className="w-[75%] h-[75%] z-10 drop-shadow-md">
            {/* Document sheet */}
            <rect x="12" y="10" width="40" height="44" rx="4" fill="#141c17" stroke="#87cf3e" strokeWidth="1.5" />
            {/* Syntax lines */}
            <line x1="18" y1="18" x2="32" y2="18" stroke="#87cf3e" strokeWidth="2.5" strokeLinecap="round" />
            <line x1="36" y1="18" x2="44" y2="18" stroke="#00e5ff" strokeWidth="2.5" strokeLinecap="round" />
            
            <line x1="18" y1="26" x2="26" y2="26" stroke="#fbbf24" strokeWidth="2.5" strokeLinecap="round" />
            <line x1="30" y1="26" x2="46" y2="26" stroke="#e0e8e3" strokeWidth="2.5" strokeLinecap="round" />

            <line x1="18" y1="34" x2="38" y2="34" stroke="#87cf3e" strokeWidth="2.5" strokeLinecap="round" />
            <line x1="18" y1="42" x2="30" y2="42" stroke="#00e5ff" strokeWidth="2.5" strokeLinecap="round" />

            {/* Angular Laser Stylus / Pen Nib */}
            <polygon points="48,34 56,42 42,52 38,48" fill="#87cf3e" stroke="#ffffff" strokeWidth="1" />
            <circle cx="54" cy="40" r="1.5" fill="#ffffff" />
          </svg>
          <div className="absolute inset-0 rounded-2xl ring-1 ring-inset ring-[#87cf3e]/25 pointer-events-none"></div>
        </div>
      );

    case 'calculator':
      // Axis Calc: Modern digital LED arithmetic console
      return (
        <div
          style={containerStyle}
          className={`relative rounded-2xl shadow-lg shadow-black/50 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-br from-[#1e2722] via-[#141b17] to-[#0c120e]"></div>
          
          <div className="grid grid-cols-2 gap-1.5 p-2.5 z-10 w-[84%] h-[84%] items-center justify-center bg-black/70 rounded-xl border border-[#87cf3e]/30 shadow-inner">
            <div className="w-5 h-5 rounded-md bg-[#25362b] flex items-center justify-center text-[10px] font-bold text-[#87cf3e]">C</div>
            <div className="w-5 h-5 rounded-md bg-[#87cf3e] flex items-center justify-center text-[10px] font-bold text-black">÷</div>
            <div className="w-5 h-5 rounded-md bg-[#1f2a23] flex items-center justify-center text-[10px] font-bold text-white">7</div>
            <div className="w-5 h-5 rounded-md bg-[#ffab00] flex items-center justify-center text-[10px] font-bold text-black">=</div>
          </div>
          <div className="absolute inset-0 rounded-2xl ring-1 ring-inset ring-[#87cf3e]/20 pointer-events-none"></div>
        </div>
      );

    case 'installer':
      // Axis Setup: Diamond rocket / chevron storage installer
      return (
        <div
          style={containerStyle}
          className={`relative rounded-2xl shadow-lg shadow-black/60 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-br from-[#183a27] via-[#0f2419] to-[#08150e]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/20 to-transparent"></div>

          <svg viewBox="0 0 64 64" className="w-[74%] h-[74%] z-10 text-white drop-shadow-md">
            {/* Deploy Arrow / Rocket Chevron */}
            <path
              fill="#87cf3e"
              d="M32 10 L46 26 L36 26 L36 40 L28 40 L28 26 L18 26 Z"
            />
            {/* Storage Drive Platter */}
            <rect x="12" y="44" width="40" height="10" rx="3" fill="#1e2c23" stroke="#87cf3e" strokeWidth="1.5" />
            <circle cx="20" cy="49" r="2" fill="#00e5ff" />
            <circle cx="26" cy="49" r="2" fill="#87cf3e" />
          </svg>
          <div className="absolute inset-0 rounded-2xl ring-1 ring-inset ring-[#87cf3e]/30 pointer-events-none"></div>
        </div>
      );

    case 'music':
      // Axis Audio: Audio spectrum equalizer & acoustic resonance wave
      return (
        <div
          style={containerStyle}
          className={`relative rounded-2xl shadow-lg shadow-black/50 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-br from-[#1b2b20] via-[#121c16] to-[#090e0b]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/15 to-transparent"></div>

          <svg viewBox="0 0 64 64" className="w-[74%] h-[74%] z-10 drop-shadow-md">
            {/* Sound Equalizer Bars */}
            <rect x="14" y="28" width="4" height="18" rx="2" fill="#87cf3e" />
            <rect x="22" y="16" width="4" height="30" rx="2" fill="#00e5ff" />
            <rect x="30" y="22" width="4" height="24" rx="2" fill="#87cf3e" />
            <rect x="38" y="12" width="4" height="34" rx="2" fill="#00e5ff" />
            <rect x="46" y="24" width="4" height="22" rx="2" fill="#87cf3e" />

            {/* Concentric soundwaves */}
            <circle cx="32" cy="32" r="24" fill="none" stroke="#87cf3e" strokeWidth="1" strokeDasharray="3 3" opacity="0.4" />
          </svg>
          <div className="absolute inset-0 rounded-2xl ring-1 ring-inset ring-[#87cf3e]/30 pointer-events-none"></div>
        </div>
      );

    case 'photos':
      // Axis Gallery: Aperture lens framing geometric mountain sunrise
      return (
        <div
          style={containerStyle}
          className={`relative rounded-2xl shadow-lg shadow-black/50 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-br from-[#1a2e22] via-[#111e16] to-[#09110d]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/20 to-transparent"></div>

          <svg viewBox="0 0 64 64" className="w-[76%] h-[76%] z-10 drop-shadow-md">
            {/* Outer Frame */}
            <rect x="10" y="12" width="44" height="40" rx="6" fill="#141c17" stroke="#87cf3e" strokeWidth="1.5" />
            {/* Solar Orb */}
            <circle cx="42" cy="24" r="5" fill="#ffab00" />
            {/* Mountain Peaks */}
            <polygon points="12,48 28,28 38,40 52,22 52,48" fill="#253d2d" />
            <polygon points="28,28 38,40 52,22 52,48 20,48" fill="#3a5c44" opacity="0.8" />
            {/* Horizon Line */}
            <line x1="12" y1="48" x2="52" y2="48" stroke="#87cf3e" strokeWidth="2" strokeLinecap="round" />
          </svg>
          <div className="absolute inset-0 rounded-2xl ring-1 ring-inset ring-[#87cf3e]/25 pointer-events-none"></div>
        </div>
      );

    case 'notes':
      // Axis Memo: Structured task board with glowing checkmark
      return (
        <div
          style={containerStyle}
          className={`relative rounded-2xl shadow-lg shadow-black/50 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-br from-[#2a3627] via-[#1c241a] to-[#10150f]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/15 to-transparent"></div>

          <svg viewBox="0 0 64 64" className="w-[74%] h-[74%] z-10 drop-shadow-md">
            {/* Memo Board */}
            <rect x="12" y="10" width="40" height="44" rx="4" fill="#141b14" stroke="#ffab00" strokeWidth="1.5" />
            {/* Top Binder Clip */}
            <rect x="24" y="8" width="16" height="5" rx="1.5" fill="#ffab00" />

            {/* Checkmark 1 */}
            <circle cx="20" cy="22" r="3" fill="#87cf3e" />
            <line x1="26" y1="22" x2="44" y2="22" stroke="#e0e8e3" strokeWidth="2" strokeLinecap="round" />

            {/* Checkmark 2 */}
            <circle cx="20" cy="32" r="3" fill="#87cf3e" />
            <line x1="26" y1="32" x2="40" y2="32" stroke="#e0e8e3" strokeWidth="2" strokeLinecap="round" />

            {/* Task 3 */}
            <circle cx="20" cy="42" r="3" fill="none" stroke="#87cf3e" strokeWidth="1.5" />
            <line x1="26" y1="42" x2="36" y2="42" stroke="#87cf3e" strokeWidth="2" strokeLinecap="round" />
          </svg>
          <div className="absolute inset-0 rounded-2xl ring-1 ring-inset ring-amber-400/25 pointer-events-none"></div>
        </div>
      );

    case 'software':
      // Axis Store: Layered isometric package prism with mint download vector
      return (
        <div
          style={containerStyle}
          className={`relative rounded-2xl shadow-lg shadow-black/60 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-br from-[#143d2b] via-[#0d261b] to-[#06140e]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/20 to-transparent"></div>

          <svg viewBox="0 0 64 64" className="w-[76%] h-[76%] z-10 drop-shadow-md">
            {/* Isometric Package Cube */}
            <polygon points="32,12 50,22 32,32 14,22" fill="#2d5e42" stroke="#87cf3e" strokeWidth="1.5" />
            <polygon points="14,22 32,32 32,52 14,42" fill="#1a3d2b" stroke="#87cf3e" strokeWidth="1.5" />
            <polygon points="50,22 32,32 32,52 50,42" fill="#224d36" stroke="#87cf3e" strokeWidth="1.5" />
            
            {/* Glowing Download Vector */}
            <path
              d="M32,20 L32,36 M27,31 L32,36 L37,31"
              stroke="#ffffff"
              strokeWidth="3"
              strokeLinecap="round"
              strokeLinejoin="round"
            />
          </svg>
          <div className="absolute inset-0 rounded-2xl ring-1 ring-inset ring-[#87cf3e]/35 pointer-events-none"></div>
        </div>
      );

    case 'clock':
      // Axis Chrono: Precision chronograph dial with tachymeter markings
      return (
        <div
          style={containerStyle}
          className={`relative rounded-2xl shadow-lg shadow-black/50 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-br from-[#18241d] via-[#101813] to-[#0a0e0c]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/15 to-transparent"></div>

          <svg viewBox="0 0 64 64" className="w-[78%] h-[78%] z-10 drop-shadow-md">
            <circle cx="32" cy="32" r="24" fill="#121a14" stroke="#87cf3e" strokeWidth="1.5" />
            {/* Hour markers */}
            <line x1="32" y1="12" x2="32" y2="16" stroke="#87cf3e" strokeWidth="2" />
            <line x1="32" y1="48" x2="32" y2="52" stroke="#87cf3e" strokeWidth="2" />
            <line x1="12" y1="32" x2="16" y2="32" stroke="#87cf3e" strokeWidth="2" />
            <line x1="48" y1="32" x2="52" y2="32" stroke="#87cf3e" strokeWidth="2" />

            {/* Hands */}
            <line x1="32" y1="32" x2="42" y2="24" stroke="#ffffff" strokeWidth="2" strokeLinecap="round" />
            <line x1="32" y1="32" x2="32" y2="20" stroke="#00e5ff" strokeWidth="2" strokeLinecap="round" />
            <line x1="32" y1="32" x2="24" y2="38" stroke="#87cf3e" strokeWidth="1.5" strokeLinecap="round" />
            <circle cx="32" cy="32" r="2.5" fill="#87cf3e" />
          </svg>
          <div className="absolute inset-0 rounded-2xl ring-1 ring-inset ring-[#87cf3e]/25 pointer-events-none"></div>
        </div>
      );

    case 'weather':
      // Axis Climate: Dynamic solar orb with atmospheric isobar curves
      return (
        <div
          style={containerStyle}
          className={`relative rounded-2xl shadow-lg shadow-black/50 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-br from-[#123038] via-[#0b1e24] to-[#061216]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/20 to-transparent"></div>

          <svg viewBox="0 0 64 64" className="w-[78%] h-[78%] z-10 drop-shadow-md">
            {/* Solar Flare Orb */}
            <circle cx="26" cy="24" r="10" fill="#ffab00" />
            {/* Atmospheric Cloud Curve */}
            <path
              d="M18,44 Q14,44 14,38 Q14,32 20,32 Q22,22 32,22 Q40,22 43,28 Q48,28 48,34 Q50,34 50,38 Q50,44 44,44 Z"
              fill="#223e44"
              stroke="#00e5ff"
              strokeWidth="1.5"
            />
            {/* Rain Crystal Particles */}
            <line x1="24" y1="48" x2="22" y2="52" stroke="#87cf3e" strokeWidth="2" strokeLinecap="round" />
            <line x1="32" y1="48" x2="30" y2="52" stroke="#00e5ff" strokeWidth="2" strokeLinecap="round" />
            <line x1="40" y1="48" x2="38" y2="52" stroke="#87cf3e" strokeWidth="2" strokeLinecap="round" />
          </svg>
          <div className="absolute inset-0 rounded-2xl ring-1 ring-inset ring-cyan-400/30 pointer-events-none"></div>
        </div>
      );

    case 'camera':
      // Axis Lens: Multi-coated optical objective with aperture reflection
      return (
        <div
          style={containerStyle}
          className={`relative rounded-2xl shadow-lg shadow-black/60 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-br from-[#202924] via-[#141b17] to-[#0a0e0c]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-white/15 to-transparent"></div>

          <svg viewBox="0 0 64 64" className="w-[78%] h-[78%] z-10 drop-shadow-md">
            <rect x="10" y="16" width="44" height="36" rx="6" fill="#18221b" stroke="#87cf3e" strokeWidth="1.5" />
            {/* Shutter Button & Sensor */}
            <rect x="18" y="12" width="8" height="4" rx="1" fill="#87cf3e" />
            <circle cx="46" cy="22" r="2" fill="#ff5555" />

            {/* Lens Outer Bezel */}
            <circle cx="32" cy="34" r="14" fill="#101712" stroke="#87cf3e" strokeWidth="2" />
            {/* Lens Glass Iris Reflection */}
            <circle cx="32" cy="34" r="9" fill="#00e5ff" opacity="0.3" />
            <circle cx="30" cy="32" r="3" fill="#ffffff" opacity="0.8" />
          </svg>
          <div className="absolute inset-0 rounded-2xl ring-1 ring-inset ring-[#87cf3e]/25 pointer-events-none"></div>
        </div>
      );

    case 'trash':
      // Wastebasket: Minimalist geometric recycling prism
      return (
        <div
          style={containerStyle}
          className={`relative rounded-2xl overflow-hidden flex items-center justify-center ${className}`}
        >
          <svg viewBox="0 0 64 64" className="w-[74%] h-[74%] drop-shadow text-slate-300">
            {/* Receptacle Frame */}
            <path
              d="M18,22 L22,50 Q23,54 27,54 L37,54 Q41,54 42,50 L46,22 Z"
              fill="rgba(135,207,62,0.12)"
              stroke="#87cf3e"
              strokeWidth="2"
            />
            {/* Top Rim */}
            <rect x="14" y="18" width="36" height="4" rx="2" fill="#243328" stroke="#87cf3e" strokeWidth="1.5" />
            <rect x="26" y="14" width="12" height="4" rx="1" fill="#87cf3e" />
            {/* Vertical Flutes */}
            <line x1="26" y1="26" x2="28" y2="48" stroke="#87cf3e" strokeWidth="1.5" opacity="0.7" />
            <line x1="32" y1="26" x2="32" y2="48" stroke="#87cf3e" strokeWidth="1.5" opacity="0.7" />
            <line x1="38" y1="26" x2="36" y2="48" stroke="#87cf3e" strokeWidth="1.5" opacity="0.7" />
          </svg>
        </div>
      );

    case 'steam':
      // Steam: Valve gaming platform icon with precision mechanical linkage
      return (
        <div
          style={containerStyle}
          className={`relative rounded-2xl shadow-lg shadow-black/60 overflow-hidden flex items-center justify-center ${className}`}
        >
          <div className="absolute inset-0 bg-gradient-to-br from-[#172332] via-[#121a24] to-[#0a0f16]"></div>
          <div className="absolute inset-x-0 top-0 h-1/2 bg-gradient-to-b from-[#87cf3e]/15 to-transparent"></div>

          <svg viewBox="0 0 64 64" className="w-[82%] h-[82%] z-10 drop-shadow-md">
            {/* Connecting piston shaft */}
            <line x1="24" y1="40" x2="42" y2="24" stroke="#ffffff" strokeWidth="6" strokeLinecap="round" />
            <line x1="24" y1="40" x2="42" y2="24" stroke="#87cf3e" strokeWidth="3" strokeLinecap="round" />

            {/* Small top pivot */}
            <circle cx="42" cy="24" r="9" fill="#1b2838" stroke="#87cf3e" strokeWidth="2.5" />
            <circle cx="42" cy="24" r="4.5" fill="#ffffff" />

            {/* Main large drive wheel */}
            <circle cx="24" cy="40" r="13" fill="#101822" stroke="#87cf3e" strokeWidth="3" />
            <circle cx="24" cy="40" r="7" fill="none" stroke="#ffffff" strokeWidth="2" />
            <circle cx="24" cy="40" r="3" fill="#87cf3e" />
          </svg>
          <div className="absolute inset-0 rounded-2xl ring-1 ring-inset ring-[#87cf3e]/30 pointer-events-none"></div>
        </div>
      );

    default:
      return (
        <div
          style={containerStyle}
          className={`relative rounded-2xl bg-gradient-to-br from-[#18201b] to-[#121814] border border-[#87cf3e]/30 flex items-center justify-center shadow-md text-[#87cf3e] font-bold ${className}`}
        >
          ◈
        </div>
      );
  }
};

// Aliased export for modern architecture
export const AxisIcon = MacIcon;
