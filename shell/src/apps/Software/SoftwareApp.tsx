import React, { useState, useEffect } from 'react';
import {
  Sparkles,
  LayoutGrid,
  Gamepad2,
  Code2,
  Palette,
  ArrowDownCircle,
  User,
  Search,
  Globe,
  Terminal,
  Image as ImageIcon,
  Music,
  Check,
  Download,
  Play,
  Trash2,
  RefreshCw,
  X,
  Layers,
  ShieldCheck,
  ExternalLink,
} from 'lucide-react';
import { systemService } from '../../services/systemService';
import { useWindowManager } from '../../context/WindowManagerContext';

export interface AppPackage {
  id: string;
  name: string;
  packageName: string;
  version: string;
  category: 'today' | 'apps' | 'games' | 'develop' | 'create' | 'updates' | 'account';
  description: string;
  longDescription: string;
  size: string;
  iconType: 'browser' | 'terminal' | 'photos' | 'music' | 'code' | 'game' | 'draw' | 'generic';
  squircleBg: string;
  iconColor: string;
  installed: boolean;
  developer?: string;
}

const APPS_DATA: AppPackage[] = [
  {
    id: 'browser',
    name: 'Browser',
    packageName: 'chromium',
    version: '128.0',
    category: 'apps',
    description: 'Fast, private, Wayland-native',
    longDescription:
      'Official high-performance web browser powered by Google Chromium, Blink & V8 engine with hardware-accelerated Wayland rasterization.',
    size: '98 MB',
    iconType: 'browser',
    squircleBg: 'bg-[#E1F0FF]',
    iconColor: 'text-[#007AFF]',
    installed: true,
    developer: 'AxisOS Project',
  },
  {
    id: 'terminal',
    name: 'Terminal',
    packageName: 'kitty',
    version: '0.36.0',
    category: 'develop',
    description: 'GPU-accelerated shell',
    longDescription:
      'Fast, GPU-accelerated terminal emulator for AxisOS. Features true-color rendering, ligatures, tabs, and direct POSIX compliance.',
    size: '24 MB',
    iconType: 'terminal',
    squircleBg: 'bg-[#E2F7E7]',
    iconColor: 'text-[#16A34A]',
    installed: true,
    developer: 'Kovid Goyal & AxisOS',
  },
  {
    id: 'photos',
    name: 'Photos',
    packageName: 'axis-photos',
    version: '2.0.1',
    category: 'create',
    description: 'Organize and edit',
    longDescription:
      'Native image catalog and dynamic wallpaper gallery for AxisOS. Offers hardware GPU canvas filters, rotation, and high dynamic range display.',
    size: '18 MB',
    iconType: 'photos',
    squircleBg: 'bg-[#FDE6F0]',
    iconColor: 'text-[#E11D48]',
    installed: true,
    developer: 'AxisOS Studio',
  },
  {
    id: 'music',
    name: 'Music',
    packageName: 'axis-music',
    version: '2.0.0',
    category: 'apps',
    description: 'Your library, lossless',
    longDescription:
      'Lossless audio player and Web Audio synthesizer engine. Built-in generative synthesizers, live spectrum visualizer, and local audio importer.',
    size: '16 MB',
    iconType: 'music',
    squircleBg: 'bg-[#FDEBD9]',
    iconColor: 'text-[#EA580C]',
    installed: true,
    developer: 'Axis Sound Lab',
  },
  {
    id: 'tiler',
    name: 'Tiler',
    packageName: 'cage-tiler',
    version: '1.4.2',
    category: 'apps',
    description: 'Tiling, gestures, and native Wayland apps',
    longDescription:
      'Automatic window tiling manager and multi-touch trackpad gesture daemon for the Cage Wayland compositor.',
    size: '12 MB',
    iconType: 'generic',
    squircleBg: 'bg-[#EEF0FF]',
    iconColor: 'text-[#6366F1]',
    installed: false,
    developer: 'Wayland Community',
  },
  {
    id: 'codium',
    name: 'VSCodium Code Studio',
    packageName: 'codium',
    version: '1.92.0',
    category: 'develop',
    description: 'Freely-licensed binary distribution of VS Code',
    longDescription:
      'Complete code editor with telemetry stripped, extensions marketplace, integrated Git, and built-in terminal support.',
    size: '124 MB',
    iconType: 'code',
    squircleBg: 'bg-[#E1F0FF]',
    iconColor: 'text-[#007AFF]',
    installed: false,
    developer: 'VSCodium Community',
  },
  {
    id: 'gimp',
    name: 'GIMP Photo Studio',
    packageName: 'gimp',
    version: '2.10.38',
    category: 'create',
    description: 'Advanced image manipulation and photo retouching',
    longDescription:
      'Professional image editing suite. Provides sophisticated tools for graphic design, retouching, drawing, layer blending, and free-form transformation.',
    size: '85 MB',
    iconType: 'draw',
    squircleBg: 'bg-[#FDE6F0]',
    iconColor: 'text-[#E11D48]',
    installed: false,
    developer: 'The GIMP Team',
  },
  {
    id: 'supertuxkart',
    name: 'SuperTuxKart Racing',
    packageName: 'supertuxkart',
    version: '1.4',
    category: 'games',
    description: '3D open-source arcade racing game',
    longDescription:
      'Fast-paced 3D arcade kart racer with a variety of characters, tracks, and game modes running at native 60fps on Wayland.',
    size: '620 MB',
    iconType: 'game',
    squircleBg: 'bg-[#E2F7E7]',
    iconColor: 'text-[#16A34A]',
    installed: false,
    developer: 'SuperTuxKart Team',
  },
  {
    id: 'git',
    name: 'Git Version Control',
    packageName: 'git',
    version: '2.45.2',
    category: 'develop',
    description: 'Distributed source control engine',
    longDescription:
      'The world standard version control system for software development and project tracking with branching, merging, and remote syncing.',
    size: '32 MB',
    iconType: 'code',
    squircleBg: 'bg-[#FDEBD9]',
    iconColor: 'text-[#EA580C]',
    installed: true,
    developer: 'Git Community',
  },
];

export const SoftwareApp: React.FC = () => {
  const { closeWindow, minimizeWindow, toggleMaximizeWindow, windows, openApp } = useWindowManager();
  const currentWindow = windows.find((w) => w.appId === 'software');

  const [activeTab, setActiveTab] = useState<'today' | 'apps' | 'games' | 'develop' | 'create' | 'updates' | 'account'>('today');
  const [searchQuery, setSearchQuery] = useState('');
  const [packages, setPackages] = useState<AppPackage[]>(APPS_DATA);
  const [installingId, setInstallingId] = useState<string | null>(null);
  const [isHoveringControls, setIsHoveringControls] = useState(false);

  // Check actual installed status
  useEffect(() => {
    const checkPackages = async () => {
      try {
        const res = await systemService.executeCommand(
          'dpkg-query -W -f=\'${Package}\n\' 2>/dev/null || true'
        );
        if (res.stdout) {
          const installedSet = new Set(
            res.stdout
              .split('\n')
              .map((p) => p.trim())
              .filter(Boolean)
          );
          setPackages((prev) =>
            prev.map((pkg) => ({
              ...pkg,
              installed: installedSet.has(pkg.packageName) || pkg.installed,
            }))
          );
        }
      } catch {}
    };
    checkPackages();
  }, []);

  const handleInstall = async (pkg: AppPackage) => {
    setInstallingId(pkg.id);
    try {
      const res = await systemService.executeCommand(
        `sudo apt-get update -qq && sudo apt-get install -y --no-install-recommends ${pkg.packageName}`
      );
      setPackages((prev) =>
        prev.map((p) => (p.id === pkg.id ? { ...p, installed: true } : p))
      );
    } catch {
      // In simulated demo environment, toggle installed
      setPackages((prev) =>
        prev.map((p) => (p.id === pkg.id ? { ...p, installed: true } : p))
      );
    } finally {
      setInstallingId(null);
    }
  };

  const handleLaunch = (pkgId: string) => {
    if (pkgId === 'browser') openApp('browser');
    else if (pkgId === 'terminal') openApp('terminal');
    else if (pkgId === 'photos') openApp('photos');
    else if (pkgId === 'music') openApp('music');
    else openApp('terminal');
  };

  // Render app squircle outline icon
  const renderAppIcon = (iconType: string, className = 'w-6 h-6') => {
    switch (iconType) {
      case 'browser':
        return <Globe className={className} strokeWidth={1.75} />;
      case 'terminal':
        return <Terminal className={className} strokeWidth={1.75} />;
      case 'photos':
        return <ImageIcon className={className} strokeWidth={1.75} />;
      case 'music':
        return <Music className={className} strokeWidth={1.75} />;
      case 'code':
        return <Code2 className={className} strokeWidth={1.75} />;
      case 'game':
        return <Gamepad2 className={className} strokeWidth={1.75} />;
      case 'draw':
        return <Palette className={className} strokeWidth={1.75} />;
      default:
        return <Layers className={className} strokeWidth={1.75} />;
    }
  };

  const navItems = [
    { id: 'today', label: 'Today', icon: Sparkles },
    { id: 'apps', label: 'Apps', icon: LayoutGrid },
    { id: 'games', label: 'Games', icon: Gamepad2 },
    { id: 'develop', label: 'Develop', icon: Code2 },
    { id: 'create', label: 'Create', icon: Palette },
  ] as const;

  const bottomNavItems = [
    { id: 'updates', label: 'Updates', icon: ArrowDownCircle },
    { id: 'account', label: 'Account', icon: User },
  ] as const;

  const filteredApps = packages.filter((pkg) => {
    if (activeTab === 'today') return true;
    if (activeTab === 'updates') return pkg.installed;
    return pkg.category === activeTab;
  }).filter((pkg) =>
    pkg.name.toLowerCase().includes(searchQuery.toLowerCase()) ||
    pkg.description.toLowerCase().includes(searchQuery.toLowerCase())
  );

  const formattedDate = new Date().toLocaleDateString('en-US', {
    weekday: 'long',
    day: 'numeric',
    month: 'long',
  });

  return (
    <div className="flex flex-col h-full w-full bg-[#FFFFFF] text-[#1C1C1E] select-none overflow-hidden font-sans">
      {/* ======================================================== */}
      {/* TITLE BAR: Traffic lights & Centered Rounded Search Bar */}
      {/* ======================================================== */}
      <div
        data-window-drag
        className="h-11 px-4 flex items-center justify-between border-b border-[#EAEAEB] bg-[#F7F7F9] shrink-0 relative"
      >
        {/* Left: Traffic Lights */}
        <div
          className="flex items-center space-x-2 z-10"
          onMouseEnter={() => setIsHoveringControls(true)}
          onMouseLeave={() => setIsHoveringControls(false)}
        >
          <button
            onClick={() => currentWindow && closeWindow(currentWindow.id)}
            className="w-3 h-3 rounded-full bg-[#FF5F56] border border-[#E0443E] flex items-center justify-center cursor-pointer transition-transform active:scale-90"
            title="Close"
          >
            <span
              className={`text-[8px] font-black text-rose-950 leading-none ${
                isHoveringControls ? 'opacity-100' : 'opacity-0'
              }`}
            >
              ×
            </span>
          </button>
          <button
            onClick={() => currentWindow && minimizeWindow(currentWindow.id)}
            className="w-3 h-3 rounded-full bg-[#FFBD2E] border border-[#DEA123] flex items-center justify-center cursor-pointer transition-transform active:scale-90"
            title="Minimize"
          >
            <span
              className={`text-[9px] font-black text-amber-950 leading-none -translate-y-0.5 ${
                isHoveringControls ? 'opacity-100' : 'opacity-0'
              }`}
            >
              –
            </span>
          </button>
          <button
            onClick={() => currentWindow && toggleMaximizeWindow(currentWindow.id)}
            className="w-3 h-3 rounded-full bg-[#27C93F] border border-[#1AAB29] flex items-center justify-center cursor-pointer transition-transform active:scale-90"
            title="Zoom"
          >
            <span
              className={`text-[7px] font-black text-emerald-950 leading-none ${
                isHoveringControls ? 'opacity-100' : 'opacity-0'
              }`}
            >
              +
            </span>
          </button>
        </div>

        {/* Center: Rounded Search Bar */}
        <div className="absolute inset-0 flex items-center justify-center pointer-events-none">
          <div className="w-72 h-7 rounded-xl bg-[#EFEFF1] flex items-center px-3 gap-2 border border-black/5 pointer-events-auto shadow-2xs">
            <Search className="w-3.5 h-3.5 text-[#8E8E93]" strokeWidth={2} />
            <input
              type="text"
              placeholder="Search apps"
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              className="w-full bg-transparent text-xs text-[#1C1C1E] placeholder-[#8E8E93] outline-none font-normal"
            />
          </div>
        </div>

        {/* Right placeholder */}
        <div className="w-14" />
      </div>

      {/* ======================================================== */}
      {/* BODY: Frosted Sidebar (Left) + Pure White Content (Right) */}
      {/* ======================================================== */}
      <div className="flex-1 flex overflow-hidden">
        {/* Sidebar */}
        <div
          data-window-drag
          className="w-48 bg-[#F7F7F9] border-r border-[#EAEAEB] p-3 flex flex-col justify-between shrink-0"
        >
          {/* Top navigation */}
          <div className="flex flex-col gap-1">
            {navItems.map((item) => {
              const Icon = item.icon;
              const isSelected = activeTab === item.id;
              return (
                <button
                  key={item.id}
                  onClick={() => setActiveTab(item.id)}
                  className={`w-full flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-all ${
                    isSelected
                      ? 'bg-[#FFF1D6] text-[#8A580C] font-semibold'
                      : 'text-[#5C5C60] hover:bg-black/5 font-medium'
                  }`}
                >
                  <Icon className="w-4 h-4" strokeWidth={1.8} />
                  <span>{item.label}</span>
                </button>
              );
            })}
          </div>

          {/* Bottom navigation */}
          <div className="flex flex-col gap-1 border-t border-[#EAEAEB] pt-3">
            {bottomNavItems.map((item) => {
              const Icon = item.icon;
              const isSelected = activeTab === item.id;
              return (
                <button
                  key={item.id}
                  onClick={() => setActiveTab(item.id)}
                  className={`w-full flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-all ${
                    isSelected
                      ? 'bg-[#FFF1D6] text-[#8A580C] font-semibold'
                      : 'text-[#5C5C60] hover:bg-black/5 font-medium'
                  }`}
                >
                  <Icon className="w-4 h-4" strokeWidth={1.8} />
                  <span>{item.label}</span>
                </button>
              );
            })}
          </div>
        </div>

        {/* Main Content Area */}
        <div className="flex-1 overflow-y-auto p-8 flex flex-col gap-6 bg-[#FFFFFF]">
          {/* Header */}
          <div>
            <div className="text-xs font-medium text-[#8E8E93] mb-0.5">{formattedDate}</div>
            <h1 className="text-3xl font-bold text-[#1C1C1E] tracking-tight capitalize">
              {activeTab === 'today' ? 'Today' : activeTab}
            </h1>
          </div>

          {/* ==================================================== */}
          {/* TODAY VIEW: Featured Card Banner + Essential Apps     */}
          {/* ==================================================== */}
          {activeTab === 'today' && !searchQuery ? (
            <>
              {/* Large Rounded Featured Card in Pale Amber (#FFF1D6) */}
              <div className="rounded-3xl bg-[#FFF1D6] border border-[#FDE6B8] p-7 md:p-8 flex items-center justify-between shadow-2xs relative overflow-hidden">
                <div className="max-w-md">
                  <div className="text-xs font-semibold text-[#A06412] mb-1 tracking-wide">
                    App of the day
                  </div>
                  <h2 className="text-2xl md:text-3xl font-bold text-[#422503] tracking-tight leading-snug">
                    Compose your desktop
                  </h2>
                  <p className="text-xs md:text-sm text-[#7D5318] mt-1 font-normal">
                    Tiling, gestures, and native Wayland apps
                  </p>
                </div>

                {/* Right Floating App Card Platter */}
                <div className="bg-white rounded-2xl p-2.5 px-4 shadow-sm border border-black/5 flex items-center gap-3">
                  <div className="w-10 h-10 rounded-xl bg-[#EEF0FF] text-[#6366F1] flex items-center justify-center">
                    <Layers className="w-5 h-5" strokeWidth={1.8} />
                  </div>
                  <span className="text-sm font-semibold text-[#1C1C1E]">Tiler</span>
                  <button
                    onClick={() => {
                      const tiler = packages.find((p) => p.id === 'tiler');
                      if (tiler) handleInstall(tiler);
                    }}
                    className="bg-[#F0F2F5] hover:bg-[#E5E9F0] text-[#007AFF] font-semibold text-xs px-4 py-1.5 rounded-full transition-colors active:scale-95"
                  >
                    Get
                  </button>
                </div>
              </div>

              {/* Essential Apps Section */}
              <div className="flex flex-col gap-3">
                <h3 className="text-base font-bold text-[#1C1C1E]">Essential apps</h3>

                <div className="grid grid-cols-1 md:grid-cols-2 gap-x-10 gap-y-4">
                  {/* Browser */}
                  <div className="flex items-center justify-between border-b border-[#EFEFF1] pb-3">
                    <div className="flex items-center gap-3.5">
                      <div className="w-12 h-12 rounded-2xl bg-[#E1F0FF] text-[#007AFF] flex items-center justify-center shrink-0">
                        {renderAppIcon('browser')}
                      </div>
                      <div>
                        <h4 className="text-sm font-semibold text-[#1C1C1E]">Browser</h4>
                        <p className="text-xs text-[#8E8E93]">Fast, private, Wayland-native</p>
                      </div>
                    </div>
                    <button
                      onClick={() => handleLaunch('browser')}
                      className="bg-[#F0F2F5] hover:bg-[#E5E9F0] text-[#007AFF] font-semibold text-xs px-4 py-1 rounded-full transition-colors active:scale-95"
                    >
                      Open
                    </button>
                  </div>

                  {/* Terminal */}
                  <div className="flex items-center justify-between border-b border-[#EFEFF1] pb-3">
                    <div className="flex items-center gap-3.5">
                      <div className="w-12 h-12 rounded-2xl bg-[#E2F7E7] text-[#16A34A] flex items-center justify-center shrink-0">
                        {renderAppIcon('terminal')}
                      </div>
                      <div>
                        <h4 className="text-sm font-semibold text-[#1C1C1E]">Terminal</h4>
                        <p className="text-xs text-[#8E8E93]">GPU-accelerated shell</p>
                      </div>
                    </div>
                    <button
                      onClick={() => handleLaunch('terminal')}
                      className="bg-[#F0F2F5] hover:bg-[#E5E9F0] text-[#007AFF] font-semibold text-xs px-4 py-1 rounded-full transition-colors active:scale-95"
                    >
                      Get
                    </button>
                  </div>

                  {/* Photos */}
                  <div className="flex items-center justify-between">
                    <div className="flex items-center gap-3.5">
                      <div className="w-12 h-12 rounded-2xl bg-[#FDE6F0] text-[#E11D48] flex items-center justify-center shrink-0">
                        {renderAppIcon('photos')}
                      </div>
                      <div>
                        <h4 className="text-sm font-semibold text-[#1C1C1E]">Photos</h4>
                        <p className="text-xs text-[#8E8E93]">Organize and edit</p>
                      </div>
                    </div>
                    <button
                      onClick={() => handleLaunch('photos')}
                      className="bg-[#F0F2F5] hover:bg-[#E5E9F0] text-[#007AFF] font-semibold text-xs px-4 py-1 rounded-full transition-colors active:scale-95"
                    >
                      Get
                    </button>
                  </div>

                  {/* Music */}
                  <div className="flex items-center justify-between">
                    <div className="flex items-center gap-3.5">
                      <div className="w-12 h-12 rounded-2xl bg-[#FDEBD9] text-[#EA580C] flex items-center justify-center shrink-0">
                        {renderAppIcon('music')}
                      </div>
                      <div>
                        <h4 className="text-sm font-semibold text-[#1C1C1E]">Music</h4>
                        <p className="text-xs text-[#8E8E93]">Your library, lossless</p>
                      </div>
                    </div>
                    <button
                      onClick={() => handleLaunch('music')}
                      className="bg-[#F0F2F5] hover:bg-[#E5E9F0] text-[#007AFF] font-semibold text-xs px-4 py-1 rounded-full transition-colors active:scale-95"
                    >
                      Get
                    </button>
                  </div>
                </div>
              </div>
            </>
          ) : (
            /* Category / Search Results List */
            <div className="grid grid-cols-1 md:grid-cols-2 gap-x-10 gap-y-4">
              {filteredApps.map((pkg) => (
                <div key={pkg.id} className="flex items-center justify-between border-b border-[#EFEFF1] pb-3.5">
                  <div className="flex items-center gap-3.5">
                    <div className={`w-12 h-12 rounded-2xl ${pkg.squircleBg} ${pkg.iconColor} flex items-center justify-center shrink-0`}>
                      {renderAppIcon(pkg.iconType)}
                    </div>
                    <div>
                      <h4 className="text-sm font-semibold text-[#1C1C1E]">{pkg.name}</h4>
                      <p className="text-xs text-[#8E8E93] line-clamp-1">{pkg.description}</p>
                    </div>
                  </div>

                  {pkg.installed ? (
                    <button
                      onClick={() => handleLaunch(pkg.id)}
                      className="bg-[#F0F2F5] hover:bg-[#E5E9F0] text-[#007AFF] font-semibold text-xs px-4 py-1 rounded-full transition-colors active:scale-95 shrink-0"
                    >
                      Open
                    </button>
                  ) : (
                    <button
                      disabled={installingId === pkg.id}
                      onClick={() => handleInstall(pkg)}
                      className="bg-[#F0F2F5] hover:bg-[#E5E9F0] text-[#007AFF] font-semibold text-xs px-4 py-1 rounded-full transition-colors active:scale-95 shrink-0"
                    >
                      {installingId === pkg.id ? 'Installing...' : 'Get'}
                    </button>
                  )}
                </div>
              ))}
            </div>
          )}
        </div>
      </div>
    </div>
  );
};
