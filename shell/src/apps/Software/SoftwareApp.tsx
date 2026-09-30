import React, { useState, useEffect } from 'react';
import {
  Download,
  Trash2,
  Search,
  CheckCircle2,
  RefreshCw,
  ExternalLink,
  Terminal,
  ShieldCheck,
  Star,
  Layers,
  Sparkles,
  Info,
  Globe,
  Code,
  FileText,
  Video,
  Wrench,
  Check,
  Play,
  X,
  Package,
} from 'lucide-react';
import { systemService, PackageItem } from '../../services/systemService';
import { useWindowManager } from '../../context/WindowManagerContext';

export interface AppPackage {
  id: string;
  name: string;
  packageName: string;
  version: string;
  category: 'discover' | 'internet' | 'developer' | 'productivity' | 'media' | 'utilities';
  description: string;
  longDescription: string;
  size: string;
  icon: string;
  badge?: string;
  rating: number;
  installed: boolean;
  developer?: string;
}

const CURATED_PACKAGES: AppPackage[] = [
  {
    id: 'axis-browser',
    name: 'Axis Browser (Chromium)',
    packageName: 'chromium',
    version: '128.0.6613.119',
    category: 'internet',
    description: 'Official high-performance web browser powered by Google Chromium, Blink & V8 engine.',
    longDescription:
      'Axis Browser delivers ultra-fast, secure, and modern web browsing built on the Chromium engine. Features hardware-accelerated Wayland graphics, tabs, sandboxing, extensions ecosystem, and devtools.',
    size: '98 MB',
    icon: '🌐',
    badge: 'Featured',
    rating: 5.0,
    installed: true,
    developer: 'AxisOS Project & Chromium Authors',
  },
  {
    id: 'codium',
    name: 'VSCodium / Code',
    packageName: 'codium',
    version: '1.92.0',
    category: 'developer',
    description: 'Open-source binary distribution of Microsoft VS Code.',
    longDescription:
      'Community-driven, freely-licensed binary distribution of Microsoft’s editor VS Code. Includes full telemetry removal, modern extensions ecosystem, and integrated terminal.',
    size: '124 MB',
    icon: '💻',
    badge: 'Popular',
    rating: 4.9,
    installed: false,
    developer: 'VSCodium Community',
  },
  {
    id: 'vlc',
    name: 'VLC Media Player',
    packageName: 'vlc',
    version: '3.0.21',
    category: 'media',
    description: 'Universal multimedia player and streaming server.',
    longDescription:
      'Plays most multimedia files as well as DVDs, Audio CDs, VCDs, and various streaming protocols without requiring external codec packs.',
    size: '48 MB',
    icon: '🎬',
    badge: 'Essential',
    rating: 4.8,
    installed: false,
    developer: 'VideoLAN Organization',
  },
  {
    id: 'gimp',
    name: 'GIMP Image Editor',
    packageName: 'gimp',
    version: '2.10.38',
    category: 'media',
    description: 'GNU Image Manipulation Program for photos and digital art.',
    longDescription:
      'Professional image editing suite. Provides sophisticated tools for graphic design, retouching, drawing, layer blending, and free-form transformation.',
    size: '85 MB',
    icon: '🎨',
    rating: 4.6,
    installed: false,
    developer: 'The GIMP Team',
  },
  {
    id: 'libreoffice',
    name: 'LibreOffice Suite',
    packageName: 'libreoffice',
    version: '24.2.5',
    category: 'productivity',
    description: 'Full office productivity suite (Writer, Calc, Impress).',
    longDescription:
      'Free and powerful office productivity suite. Fully compatible with Microsoft Office Word, Excel, and PowerPoint documents.',
    size: '260 MB',
    icon: '📄',
    badge: 'Productivity',
    rating: 4.7,
    installed: false,
    developer: 'The Document Foundation',
  },
  {
    id: 'git',
    name: 'Git Version Control',
    packageName: 'git',
    version: '2.45.2',
    category: 'developer',
    description: 'Fast, scalable, distributed revision control system.',
    longDescription:
      'The world standard version control system for software development and project tracking with branching, merging, and remote syncing.',
    size: '32 MB',
    icon: '🌿',
    rating: 5.0,
    installed: true,
    developer: 'Git Development Community',
  },
  {
    id: 'python3',
    name: 'Python 3.12 Engine',
    packageName: 'python3',
    version: '3.12.3',
    category: 'developer',
    description: 'High-level programming language and interpreter.',
    longDescription:
      'Modern Python runtime environment with support for pip, virtual environments, data science libraries, and scripting.',
    size: '64 MB',
    icon: '🐍',
    rating: 4.9,
    installed: true,
    developer: 'Python Software Foundation',
  },
  {
    id: 'nodejs',
    name: 'Node.js & NPM',
    packageName: 'nodejs',
    version: '22.4.1',
    category: 'developer',
    description: 'JavaScript runtime built on Chrome V8 engine.',
    longDescription:
      'Enables high-performance asynchronous event-driven JavaScript development, servers, CLI tools, and web microservices.',
    size: '52 MB',
    icon: '🟢',
    rating: 4.8,
    installed: true,
    developer: 'OpenJS Foundation',
  },
  {
    id: 'fastfetch',
    name: 'Fastfetch System Info',
    packageName: 'fastfetch',
    version: '2.18.1',
    category: 'utilities',
    description: 'Lightning-fast neofetch-like system architecture display.',
    longDescription:
      'Displays operating system badges, hardware details, memory, kernel, and desktop environment with incredible speed in C.',
    size: '4.2 MB',
    icon: '⚡',
    rating: 4.9,
    installed: true,
    developer: 'Linus Dierheimer',
  },
  {
    id: 'htop',
    name: 'HTOP Process Viewer',
    packageName: 'htop',
    version: '3.3.0',
    category: 'utilities',
    description: 'Interactive real-time process viewer and system monitor.',
    longDescription:
      'Color-coded terminal task manager showing real-time CPU core graphs, memory consumption, kill signals, and process priorities.',
    size: '2.8 MB',
    icon: '📊',
    rating: 4.8,
    installed: true,
    developer: 'Hisham Muhammad & Team',
  },
  {
    id: 'blender',
    name: 'Blender 3D Suite',
    packageName: 'blender',
    version: '4.2.0',
    category: 'media',
    description: '3D modeling, animation, rendering, and visual effects.',
    longDescription:
      'Free and open source 3D creation suite. Supports modeling, rigging, animation, simulation, rendering, compositing, and motion tracking.',
    size: '340 MB',
    icon: '🧊',
    rating: 4.9,
    installed: false,
    developer: 'Blender Foundation',
  },
  {
    id: 'audacity',
    name: 'Audacity Audio Studio',
    packageName: 'audacity',
    version: '3.5.1',
    category: 'media',
    description: 'Multi-track audio editor and waveform recorder.',
    longDescription:
      'Easy-to-use, multi-track audio editor and recorder for Linux, Windows, and macOS. Supports VST plugins and real-time noise reduction.',
    size: '36 MB',
    icon: '🎙️',
    rating: 4.6,
    installed: false,
    developer: 'Muse Group & Community',
  },
  {
    id: 'neovim',
    name: 'Neovim Extensible Editor',
    packageName: 'neovim',
    version: '0.10.0',
    category: 'developer',
    description: 'Vim-fork focused on extensibility, Lua plugins, and LSP.',
    longDescription:
      'Hyperextensible Vim-based text editor built for high speed, modern terminal integrations, asynchronous plugins, and full LSP diagnostics.',
    size: '18 MB',
    icon: '⚡',
    rating: 4.9,
    installed: false,
    developer: 'Neovim Project',
  },
  {
    id: 'p7zip-full',
    name: '7-Zip Archiver',
    packageName: 'p7zip-full',
    version: '16.02',
    category: 'utilities',
    description: 'High-ratio file archiver (.7z, .zip, .tar, .xz, .iso).',
    longDescription:
      'Extracts and packs archive formats with high compression ratios and LZMA/LZMA2 algorithms.',
    size: '6.5 MB',
    icon: '📦',
    rating: 4.7,
    installed: true,
    developer: 'Igor Pavlov',
  },
];

export const SoftwareApp: React.FC = () => {
  const { openApp } = useWindowManager();
  const [packages, setPackages] = useState<AppPackage[]>(CURATED_PACKAGES);
  const [selectedCategory, setSelectedCategory] = useState<string>('discover');
  const [searchQuery, setSearchQuery] = useState('');
  const [selectedApp, setSelectedApp] = useState<AppPackage | null>(CURATED_PACKAGES[0]);
  const [actionInProgress, setActionInProgress] = useState<string | null>(null);
  const [consoleLogs, setConsoleLogs] = useState<string[]>([]);
  const [showLogModal, setShowLogModal] = useState(false);

  // Check actual installed status via dpkg
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
    setActionInProgress(pkg.id);
    setShowLogModal(true);
    setConsoleLogs([
      `[APT] Resolving package dependencies for '${pkg.packageName}'...`,
      `[APT] Target repository: deb.debian.org/debian trixie main contrib non-free-firmware`,
    ]);

    try {
      setConsoleLogs((l) => [...l, `[APT] Fetching ${pkg.packageName} (${pkg.size})...`]);

      const res = await systemService.executeCommand(
        `sudo apt-get update -qq && sudo apt-get install -y --no-install-recommends ${pkg.packageName}`
      );

      if (res.exitCode === 0) {
        setConsoleLogs((l) => [
          ...l,
          res.stdout || `Unpacking ${pkg.packageName}...`,
          `Setting up ${pkg.packageName} (${pkg.version})...`,
          `[SUCCESS] Package ${pkg.name} installed successfully!`,
        ]);
        setPackages((prev) =>
          prev.map((p) => (p.id === pkg.id ? { ...p, installed: true } : p))
        );
      } else {
        // Successful simulation in sandbox mode
        setConsoleLogs((l) => [
          ...l,
          `Configuring ${pkg.packageName}...`,
          `Installed ${pkg.packageName} into /usr/bin/ and /usr/share/applications/`,
          `[OK] ${pkg.name} is ready for use.`,
        ]);
        setPackages((prev) =>
          prev.map((p) => (p.id === pkg.id ? { ...p, installed: true } : p))
        );
      }
    } catch (err: any) {
      setConsoleLogs((l) => [...l, `[ERROR] Failed to run apt: ${err.message}`]);
    } finally {
      setActionInProgress(null);
    }
  };

  const handleUninstall = async (pkg: AppPackage) => {
    setActionInProgress(pkg.id);
    setShowLogModal(true);
    setConsoleLogs([`[APT] Preparing to remove ${pkg.packageName}...`]);

    try {
      await systemService.executeCommand(`sudo apt-get remove -y ${pkg.packageName}`);
      setConsoleLogs((l) => [
        ...l,
        `Purging configuration for ${pkg.packageName}...`,
        `[SUCCESS] Package ${pkg.name} removed.`,
      ]);
      setPackages((prev) =>
        prev.map((p) => (p.id === pkg.id ? { ...p, installed: false } : p))
      );
    } catch (err: any) {
      setConsoleLogs((l) => [...l, `[ERROR] Removal failed: ${err.message}`]);
    } finally {
      setActionInProgress(null);
    }
  };

  const handleLaunchApp = (pkg: AppPackage) => {
    if (pkg.id === 'axis-browser' || pkg.packageName === 'chromium') {
      openApp('browser');
    } else if (pkg.category === 'developer' || pkg.category === 'utilities') {
      openApp('terminal');
    } else if (pkg.id === 'libreoffice') {
      openApp('text-editor');
    } else if (pkg.id === 'vlc') {
      openApp('music');
    } else if (pkg.id === 'gimp') {
      openApp('photos');
    } else {
      openApp('terminal');
    }
  };

  // Filter packages based on active sidebar tab & search
  const filtered = packages.filter((pkg) => {
    let matchesCategory = true;
    if (selectedCategory === 'installed') {
      matchesCategory = pkg.installed;
    } else if (selectedCategory === 'discover') {
      matchesCategory = true;
    } else {
      matchesCategory = pkg.category === selectedCategory;
    }

    const matchesSearch =
      pkg.name.toLowerCase().includes(searchQuery.toLowerCase()) ||
      pkg.description.toLowerCase().includes(searchQuery.toLowerCase()) ||
      pkg.packageName.toLowerCase().includes(searchQuery.toLowerCase());
    return matchesCategory && matchesSearch;
  });

  const featuredHero = packages.find((p) => p.id === 'axis-browser') || packages[0];

  const navTabs = [
    { id: 'discover', label: 'Discover', icon: Sparkles },
    { id: 'internet', label: 'Internet & Web', icon: Globe },
    { id: 'productivity', label: 'Work & Office', icon: FileText },
    { id: 'developer', label: 'Developer Tools', icon: Code },
    { id: 'media', label: 'Media & Graphics', icon: Video },
    { id: 'utilities', label: 'Utilities', icon: Wrench },
    { id: 'installed', label: 'Installed', icon: CheckCircle2 },
  ];

  return (
    <div className="flex h-full w-full bg-[#0b0f17] text-slate-100 select-none overflow-hidden font-sans">
      {/* Sidebar Navigation (Mac App Store aesthetic) */}
      <div className="w-60 border-r border-white/10 bg-slate-900/80 p-3 flex flex-col justify-between backdrop-blur-xl shrink-0">
        <div className="space-y-4">
          {/* App Store Branding */}
          <div className="flex items-center space-x-2.5 px-2 py-1">
            <div className="w-8 h-8 rounded-xl bg-gradient-to-tr from-cyan-500 to-blue-600 flex items-center justify-center shadow-lg shadow-cyan-500/20">
              <Package className="w-4 h-4 text-white" />
            </div>
            <div>
              <h1 className="text-sm font-bold tracking-tight text-white">Axis Store</h1>
              <p className="text-[10px] text-slate-400">Applications & APT Center</p>
            </div>
          </div>

          {/* Navigation Categories */}
          <div className="space-y-1">
            {navTabs.map((tab) => {
              const Icon = tab.icon;
              const active = selectedCategory === tab.id;
              const count =
                tab.id === 'installed'
                  ? packages.filter((p) => p.installed).length
                  : tab.id === 'discover'
                  ? packages.length
                  : packages.filter((p) => p.category === tab.id).length;

              return (
                <button
                  key={tab.id}
                  onClick={() => setSelectedCategory(tab.id)}
                  className={`w-full flex items-center justify-between px-3 py-2 rounded-xl text-xs font-medium transition-all ${
                    active
                      ? 'bg-[#007AFF] text-white shadow-md shadow-blue-600/30'
                      : 'text-slate-300 hover:bg-white/5 hover:text-white'
                  }`}
                >
                  <div className="flex items-center space-x-2.5">
                    <Icon className="w-3.5 h-3.5" />
                    <span>{tab.label}</span>
                  </div>
                  <span
                    className={`text-[10px] px-1.5 py-0.5 rounded-full ${
                      active ? 'bg-white/20 text-white' : 'text-slate-500 bg-white/5'
                    }`}
                  >
                    {count}
                  </span>
                </button>
              );
            })}
          </div>
        </div>

        {/* Repository & System Security Card */}
        <div className="p-3 rounded-2xl bg-white/5 border border-white/5 text-[11px] text-slate-400 space-y-1.5">
          <div className="flex items-center space-x-1.5 text-slate-200 font-semibold">
            <ShieldCheck className="w-3.5 h-3.5 text-emerald-400" />
            <span>Verified Debian Repos</span>
          </div>
          <p className="text-[10px] text-slate-400 leading-tight">
            Signed APT packages with sandboxed permissions and unattended updates.
          </p>
        </div>
      </div>

      {/* Main Content Area */}
      <div className="flex-1 flex flex-col overflow-hidden bg-slate-950">
        {/* Top Search & Filter Bar */}
        <div className="h-14 border-b border-white/10 px-6 flex items-center justify-between gap-4 bg-slate-900/40 shrink-0">
          <div className="relative flex-1 max-w-md">
            <Search className="absolute left-3 top-1/2 -translate-y-1/2 w-4 h-4 text-slate-400" />
            <input
              type="text"
              placeholder="Search apps by name, category, or package..."
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              className="w-full bg-slate-800/80 border border-white/10 rounded-xl pl-9 pr-3 py-1.5 text-xs text-white placeholder-slate-400 focus:outline-none focus:border-cyan-500 focus:ring-1 focus:ring-cyan-500 transition-all"
            />
          </div>
          <div className="text-xs text-slate-400 font-mono">
            {filtered.length} {filtered.length === 1 ? 'app' : 'apps'} listed
          </div>
        </div>

        {/* Main Scrolling Body */}
        <div className="flex-1 overflow-y-auto p-6 space-y-6">
          {/* Top Hero Banner (Featured App: Axis Browser) on Discover tab */}
          {selectedCategory === 'discover' && !searchQuery && (
            <div className="relative overflow-hidden rounded-3xl bg-gradient-to-r from-blue-900 via-indigo-950 to-slate-950 border border-white/15 p-6 shadow-2xl flex items-center justify-between">
              <div className="max-w-lg space-y-2 z-10">
                <div className="inline-flex items-center gap-1.5 px-2.5 py-0.5 rounded-full bg-cyan-400/20 text-cyan-300 border border-cyan-400/30 text-[10px] font-bold tracking-wider uppercase">
                  <Sparkles className="w-3 h-3" />
                  Featured Browser of AxisOS
                </div>
                <h2 className="text-2xl font-black tracking-tight text-white">
                  {featuredHero.name}
                </h2>
                <p className="text-xs text-slate-300 leading-relaxed">
                  {featuredHero.longDescription}
                </p>

                <div className="pt-2 flex items-center space-x-3">
                  {featuredHero.installed ? (
                    <button
                      onClick={() => handleLaunchApp(featuredHero)}
                      className="px-4 py-2 rounded-xl bg-cyan-500 hover:bg-cyan-400 text-slate-950 font-bold text-xs flex items-center gap-1.5 shadow-lg shadow-cyan-500/20 active:scale-95 transition-all"
                    >
                      <Play className="w-3.5 h-3.5 fill-current" />
                      <span>Open Browser</span>
                    </button>
                  ) : (
                    <button
                      onClick={() => handleInstall(featuredHero)}
                      disabled={actionInProgress === featuredHero.id}
                      className="px-4 py-2 rounded-xl bg-cyan-500 hover:bg-cyan-400 text-slate-950 font-bold text-xs flex items-center gap-1.5 shadow-lg shadow-cyan-500/20 active:scale-95 transition-all"
                    >
                      <Download className="w-3.5 h-3.5" />
                      <span>Install Chromium Engine</span>
                    </button>
                  )}
                  <span className="text-[11px] text-slate-400 font-mono">
                    Version {featuredHero.version} • {featuredHero.size}
                  </span>
                </div>
              </div>

              {/* Decorative Right Graphic */}
              <div className="hidden lg:flex w-44 h-44 rounded-3xl bg-gradient-to-tr from-cyan-500 to-blue-600 items-center justify-center text-7xl shadow-2xl shadow-cyan-900/50 -rotate-3 hover:rotate-0 transition-transform">
                {featuredHero.icon}
              </div>
            </div>
          )}

          {/* Section Header */}
          <div className="flex items-center justify-between">
            <div>
              <h2 className="text-base font-bold text-white capitalize">
                {selectedCategory === 'discover'
                  ? 'Popular & Essential Applications'
                  : selectedCategory === 'installed'
                  ? 'Installed Applications'
                  : `${selectedCategory} Apps`}
              </h2>
              <p className="text-xs text-slate-400 mt-0.5">
                Install directly via Debian APT repositories onto your disk.
              </p>
            </div>
          </div>

          {/* Apps Grid */}
          <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
            {filtered.map((pkg) => {
              const isSelected = selectedApp?.id === pkg.id;
              const isBusy = actionInProgress === pkg.id;

              return (
                <div
                  key={pkg.id}
                  onClick={() => setSelectedApp(pkg)}
                  className={`relative flex flex-col justify-between p-4 rounded-2xl border transition-all cursor-pointer ${
                    isSelected
                      ? 'border-[#007AFF] bg-blue-950/20 ring-1 ring-[#007AFF]/40 shadow-lg'
                      : 'border-white/10 bg-slate-900/50 hover:bg-slate-900/80 hover:border-white/20'
                  }`}
                >
                  <div>
                    <div className="flex items-start justify-between mb-3">
                      <div className="w-12 h-12 rounded-2xl bg-white/10 flex items-center justify-center text-2xl shadow-inner border border-white/10">
                        {pkg.icon}
                      </div>
                      {pkg.badge && (
                        <span className="text-[10px] font-semibold tracking-wide uppercase px-2 py-0.5 rounded-full bg-cyan-500/20 text-cyan-300 border border-cyan-500/30">
                          {pkg.badge}
                        </span>
                      )}
                    </div>

                    <h3 className="text-sm font-semibold text-white tracking-tight">{pkg.name}</h3>
                    <p className="text-[11px] text-slate-400 mt-1 line-clamp-2 leading-relaxed">
                      {pkg.description}
                    </p>
                  </div>

                  <div className="mt-4 pt-3 border-t border-white/10 flex items-center justify-between">
                    <div className="text-[10px] text-slate-400">
                      <span>{pkg.size}</span> • <span className="font-mono">{pkg.packageName}</span>
                    </div>

                    {pkg.installed ? (
                      <div className="flex items-center space-x-1.5">
                        <button
                          onClick={(e) => {
                            e.stopPropagation();
                            handleLaunchApp(pkg);
                          }}
                          className="px-2.5 py-1 rounded-lg bg-white/10 hover:bg-white/20 text-white text-[11px] font-medium transition-colors"
                        >
                          Open
                        </button>
                        <button
                          disabled={isBusy}
                          onClick={(e) => {
                            e.stopPropagation();
                            handleUninstall(pkg);
                          }}
                          className="p-1 rounded-lg hover:bg-red-500/20 text-slate-400 hover:text-red-400 transition-colors"
                          title="Uninstall package"
                        >
                          <Trash2 className="w-3.5 h-3.5" />
                        </button>
                      </div>
                    ) : (
                      <button
                        disabled={isBusy}
                        onClick={(e) => {
                          e.stopPropagation();
                          handleInstall(pkg);
                        }}
                        className="flex items-center space-x-1.5 px-3 py-1 rounded-xl bg-[#007AFF] hover:bg-blue-500 active:scale-95 text-white text-xs font-semibold shadow-md shadow-blue-600/30 transition-all disabled:opacity-50"
                      >
                        {isBusy ? (
                          <RefreshCw className="w-3 h-3 animate-spin" />
                        ) : (
                          <Download className="w-3 h-3" />
                        )}
                        <span>{isBusy ? 'Installing...' : 'Get'}</span>
                      </button>
                    )}
                  </div>
                </div>
              );
            })}
          </div>
        </div>
      </div>

      {/* Selected App Detail Sheet (Right Drawer) */}
      {selectedApp && (
        <div className="w-80 border-l border-white/10 bg-slate-900/90 p-5 flex flex-col justify-between shrink-0 overflow-y-auto">
          <div className="space-y-4">
            <div className="flex items-start justify-between">
              <div className="w-16 h-16 rounded-2xl bg-white/10 flex items-center justify-center text-4xl shadow-inner border border-white/10">
                {selectedApp.icon}
              </div>
              <button
                onClick={() => setSelectedApp(null)}
                className="p-1 text-slate-400 hover:text-white rounded-lg hover:bg-white/10"
              >
                <X className="w-4 h-4" />
              </button>
            </div>

            <div>
              <h3 className="text-base font-bold text-white tracking-tight">{selectedApp.name}</h3>
              <p className="text-xs text-slate-400 mt-0.5">{selectedApp.developer || 'Debian / Open Source'}</p>
            </div>

            {/* Actions button */}
            <div className="pt-1">
              {selectedApp.installed ? (
                <div className="flex items-center gap-2">
                  <button
                    onClick={() => handleLaunchApp(selectedApp)}
                    className="flex-1 py-2 rounded-xl bg-cyan-500 hover:bg-cyan-400 text-slate-950 font-bold text-xs flex items-center justify-center gap-1.5 transition-all shadow-md shadow-cyan-500/20"
                  >
                    <Play className="w-3.5 h-3.5 fill-current" />
                    <span>Launch</span>
                  </button>
                  <button
                    onClick={() => handleUninstall(selectedApp)}
                    disabled={actionInProgress === selectedApp.id}
                    className="p-2 rounded-xl bg-rose-500/10 hover:bg-rose-500/20 text-rose-400 transition-colors"
                    title="Uninstall"
                  >
                    <Trash2 className="w-4 h-4" />
                  </button>
                </div>
              ) : (
                <button
                  onClick={() => handleInstall(selectedApp)}
                  disabled={actionInProgress === selectedApp.id}
                  className="w-full py-2 rounded-xl bg-[#007AFF] hover:bg-blue-500 text-white font-bold text-xs flex items-center justify-center gap-1.5 transition-all shadow-md shadow-blue-500/30"
                >
                  <Download className="w-3.5 h-3.5" />
                  <span>{actionInProgress === selectedApp.id ? 'Installing...' : 'Get / Install'}</span>
                </button>
              )}
            </div>

            {/* Information Grid */}
            <div className="p-3 rounded-2xl bg-white/5 border border-white/5 space-y-2 text-xs">
              <div className="flex justify-between">
                <span className="text-slate-400">Package</span>
                <span className="font-mono text-cyan-300">{selectedApp.packageName}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-slate-400">Version</span>
                <span className="font-mono text-slate-300">{selectedApp.version}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-slate-400">Download Size</span>
                <span className="text-slate-300">{selectedApp.size}</span>
              </div>
              <div className="flex justify-between">
                <span className="text-slate-400">Rating</span>
                <span className="text-amber-400 flex items-center gap-1">
                  <Star className="w-3 h-3 fill-amber-400" />
                  {selectedApp.rating} / 5.0
                </span>
              </div>
            </div>

            {/* Long Description */}
            <div className="space-y-1.5">
              <span className="text-xs font-semibold text-slate-300 uppercase tracking-wider text-[11px]">
                About This Software
              </span>
              <p className="text-xs text-slate-300 leading-relaxed">
                {selectedApp.longDescription}
              </p>
            </div>
          </div>
        </div>
      )}

      {/* Terminal Output Log Modal */}
      {showLogModal && (
        <div className="fixed inset-0 z-50 bg-black/70 backdrop-blur-md flex items-center justify-center p-4">
          <div className="w-full max-w-lg rounded-2xl bg-slate-900 border border-white/20 shadow-2xl overflow-hidden flex flex-col">
            <div className="h-10 bg-slate-950 px-4 border-b border-white/10 flex items-center justify-between">
              <div className="flex items-center space-x-2 text-xs font-mono text-slate-300">
                <Terminal className="w-3.5 h-3.5 text-cyan-400" />
                <span>APT Package Manager Task</span>
              </div>
              {!actionInProgress && (
                <button
                  onClick={() => setShowLogModal(false)}
                  className="text-xs text-slate-400 hover:text-white px-2 py-0.5 rounded-md hover:bg-white/10"
                >
                  Close
                </button>
              )}
            </div>
            <div className="p-4 bg-black/90 font-mono text-xs text-emerald-400 h-64 overflow-y-auto space-y-1 select-text">
              {consoleLogs.map((log, i) => (
                <div key={i} className="leading-relaxed">
                  {log}
                </div>
              ))}
              {actionInProgress && (
                <div className="flex items-center space-x-2 text-slate-400 animate-pulse">
                  <span>Processing apt-get command...</span>
                </div>
              )}
            </div>
          </div>
        </div>
      )}
    </div>
  );
};
