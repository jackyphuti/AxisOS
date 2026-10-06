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
  Layers,
  MessageSquare,
  Film,
  FileSpreadsheet,
  Box,
  Cpu,
  Activity,
} from 'lucide-react';
import { systemService } from '../../services/systemService';
import { useWindowManager } from '../../context/WindowManagerContext';
import {
  CATALOG_PACKAGES,
  CatalogPackage,
  getInstalledPackageSet,
  markPackageInstalled,
} from '../../data/packageCatalog';

export const SoftwareApp: React.FC = () => {
  const { closeWindow, minimizeWindow, toggleMaximizeWindow, windows, openApp } = useWindowManager();
  const currentWindow = windows.find((w) => w.appId === 'software');

  const [activeTab, setActiveTab] = useState<'today' | 'apps' | 'games' | 'develop' | 'create' | 'updates' | 'account'>('today');
  const [searchQuery, setSearchQuery] = useState('');
  const [packages, setPackages] = useState<CatalogPackage[]>(CATALOG_PACKAGES);
  const [installingId, setInstallingId] = useState<string | null>(null);
  const [isHoveringControls, setIsHoveringControls] = useState(false);

  // Synchronize installed status from system dpkg and local storage
  useEffect(() => {
    const syncInstalled = async () => {
      const storedSet = getInstalledPackageSet();
      try {
        const res = await systemService.executeCommand(
          "dpkg-query -W -f='${Package}\n' 2>/dev/null || true"
        );
        if (res.stdout) {
          res.stdout
            .split('\n')
            .map((p) => p.trim().toLowerCase())
            .filter(Boolean)
            .forEach((p) => storedSet.add(p));
        }
      } catch {}

      setPackages((prev) =>
        prev.map((pkg) => ({
          ...pkg,
          installed:
            storedSet.has(pkg.id.toLowerCase()) ||
            storedSet.has(pkg.packageName.toLowerCase()) ||
            pkg.installed,
        }))
      );
    };

    syncInstalled();

    const handlePkgEvent = () => {
      syncInstalled();
    };

    window.addEventListener('axisos-packages-changed', handlePkgEvent);
    return () => window.removeEventListener('axisos-packages-changed', handlePkgEvent);
  }, []);

  const handleInstall = async (pkg: CatalogPackage) => {
    setInstallingId(pkg.id);
    try {
      await systemService.executeCommand(
        `sudo apt-get update -qq && sudo apt-get install -y --no-install-recommends ${pkg.packageName}`
      );
    } catch {}

    markPackageInstalled(pkg.id);
    markPackageInstalled(pkg.packageName);

    setPackages((prev) =>
      prev.map((p) => (p.id === pkg.id ? { ...p, installed: true } : p))
    );
    setInstallingId(null);
  };

  const handleLaunch = (pkg: CatalogPackage) => {
    if (pkg.id === 'browser' || pkg.id === 'chromium') openApp('browser');
    else if (pkg.id === 'terminal') openApp('terminal');
    else if (pkg.id === 'music') openApp('music');
    else if (pkg.id === 'photos') openApp('photos');
    else if (pkg.id === 'notes') openApp('notes');
    else if (pkg.id === 'calculator') openApp('calculator');
    else if (pkg.id === 'settings') openApp('settings');
    else if (pkg.id === 'system-monitor') openApp('system-monitor');
    else {
      systemService.executeCommand(`${pkg.binaryPath} &`);
    }
  };

  const renderAppIcon = (type: CatalogPackage['iconType']) => {
    const className = 'w-6 h-6';
    switch (type) {
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
      case 'chat':
        return <MessageSquare className={className} strokeWidth={1.75} />;
      case 'video':
        return <Film className={className} strokeWidth={1.75} />;
      case 'office':
        return <FileSpreadsheet className={className} strokeWidth={1.75} />;
      case 'box':
        return <Box className={className} strokeWidth={1.75} />;
      case 'cpu':
        return <Cpu className={className} strokeWidth={1.75} />;
      case 'activity':
        return <Activity className={className} strokeWidth={1.75} />;
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
    pkg.description.toLowerCase().includes(searchQuery.toLowerCase()) ||
    pkg.packageName.toLowerCase().includes(searchQuery.toLowerCase())
  );

  const formattedDate = new Date().toLocaleDateString('en-US', {
    weekday: 'long',
    day: 'numeric',
    month: 'long',
  });

  return (
    <div className="flex flex-col h-full w-full bg-[#18201b] text-slate-100 select-none overflow-hidden font-sans">
      {/* ======================================================== */}
      {/* TITLE BAR: Traffic lights & Centered Rounded Search Bar */}
      {/* ======================================================== */}
      <div
        data-window-drag
        className="h-11 px-4 flex items-center justify-between border-b border-[#87cf3e]/15 bg-[#141b16] shrink-0 relative"
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
          <div className="w-80 h-7 rounded-xl bg-[#1c261f] flex items-center px-3 gap-2 border border-[#87cf3e]/20 pointer-events-auto shadow-inner">
            <Search className="w-3.5 h-3.5 text-[#87cf3e]" strokeWidth={2} />
            <input
              type="text"
              placeholder="Search native Debian apps & packages..."
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              className="w-full bg-transparent text-xs text-slate-100 placeholder-slate-400 outline-none font-normal"
            />
          </div>
        </div>

        {/* Right placeholder */}
        <div className="w-14" />
      </div>

      {/* ======================================================== */}
      {/* BODY: Linux Mint Charcoal Sidebar + Dark Content Canvas  */}
      {/* ======================================================== */}
      <div className="flex-1 flex overflow-hidden">
        {/* Sidebar */}
        <div
          data-window-drag
          className="w-48 bg-[#141b16] border-r border-[#87cf3e]/15 p-3 flex flex-col justify-between shrink-0"
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
                  className={`w-full flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-colors cursor-pointer ${
                    isSelected
                      ? 'bg-[#87cf3e] text-black font-bold shadow-sm'
                      : 'text-slate-300 hover:bg-[#87cf3e]/10 font-medium'
                  }`}
                >
                  <Icon className="w-4 h-4" strokeWidth={1.8} />
                  <span>{item.label}</span>
                </button>
              );
            })}
          </div>

          {/* Bottom navigation */}
          <div className="flex flex-col gap-1 border-t border-[#87cf3e]/15 pt-3">
            {bottomNavItems.map((item) => {
              const Icon = item.icon;
              const isSelected = activeTab === item.id;
              return (
                <button
                  key={item.id}
                  onClick={() => setActiveTab(item.id)}
                  className={`w-full flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-colors cursor-pointer ${
                    isSelected
                      ? 'bg-[#87cf3e] text-black font-bold shadow-sm'
                      : 'text-slate-300 hover:bg-[#87cf3e]/10 font-medium'
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
        <div className="flex-1 overflow-y-auto p-8 flex flex-col gap-6 bg-[#18201b]">
          {/* Header */}
          <div>
            <div className="text-xs font-medium text-[#87cf3e] mb-0.5">{formattedDate}</div>
            <h1 className="text-3xl font-bold text-slate-100 tracking-tight capitalize">
              {activeTab === 'today' ? 'Discover' : activeTab}
            </h1>
          </div>

          {/* ==================================================== */}
          {/* TODAY VIEW: Featured Card Banner + Essential Apps     */}
          {/* ==================================================== */}
          {activeTab === 'today' && !searchQuery ? (
            <>
              {/* Large Rounded Featured Card in Linux Mint Slate Theme */}
              <div className="rounded-3xl bg-gradient-to-r from-[#1b261f] to-[#25362b] border border-[#87cf3e]/30 p-7 md:p-8 flex items-center justify-between shadow-lg relative overflow-hidden">
                <div className="max-w-md">
                  <div className="text-xs font-semibold text-[#87cf3e] mb-1 tracking-wide uppercase">
                    Developer & Gaming Platform
                  </div>
                  <h2 className="text-2xl md:text-3xl font-bold text-white tracking-tight leading-snug">
                    Debian Bookworm Native Apps
                  </h2>
                  <p className="text-xs md:text-sm text-slate-300 mt-1 font-normal">
                    Pre-bundled with Steam and Visual Studio Code. Zero configuration required.
                  </p>
                </div>

                {/* Right Floating Quick Action Card */}
                <div className="bg-[#141b16] rounded-2xl p-3 px-4 shadow-md border border-[#87cf3e]/20 flex items-center gap-3">
                  <div className="w-10 h-10 rounded-xl bg-[#87cf3e]/20 text-[#87cf3e] flex items-center justify-center">
                    <Code2 className="w-5 h-5" strokeWidth={1.8} />
                  </div>
                  <div>
                    <span className="text-sm font-semibold text-white block">VS Code</span>
                    <span className="text-[10px] text-slate-400">Pre-Installed</span>
                  </div>
                  <button
                    onClick={() => {
                      const vsc = packages.find((p) => p.id === 'vscode');
                      if (vsc) handleLaunch(vsc);
                    }}
                    className="bg-[#87cf3e] hover:bg-[#76bb33] text-black font-bold text-xs px-4 py-1.5 rounded-full transition-colors active:scale-95 cursor-pointer ml-1"
                  >
                    Open
                  </button>
                </div>
              </div>

              {/* Essential Apps Section */}
              <div className="flex flex-col gap-3">
                <h3 className="text-base font-bold text-slate-100">Essential applications</h3>

                <div className="grid grid-cols-1 md:grid-cols-2 gap-x-10 gap-y-4">
                  {packages.slice(0, 10).map((pkg) => (
                    <div
                      key={pkg.id}
                      className="flex items-center justify-between border-b border-[#87cf3e]/10 pb-3"
                    >
                      <div className="flex items-center gap-3.5">
                        <div
                          className={`w-12 h-12 rounded-2xl ${pkg.squircleBg} ${pkg.iconColor} flex items-center justify-center shrink-0 shadow-sm`}
                        >
                          {renderAppIcon(pkg.iconType)}
                        </div>
                        <div>
                          <h4 className="text-sm font-semibold text-slate-100">{pkg.name}</h4>
                          <p className="text-xs text-slate-400 line-clamp-1">{pkg.description}</p>
                        </div>
                      </div>

                      {pkg.installed ? (
                        <button
                          onClick={() => handleLaunch(pkg)}
                          className="bg-[#87cf3e]/15 hover:bg-[#87cf3e]/25 text-[#87cf3e] border border-[#87cf3e]/30 font-semibold text-xs px-4 py-1 rounded-full transition-colors active:scale-95 cursor-pointer shrink-0"
                        >
                          Open
                        </button>
                      ) : (
                        <button
                          disabled={installingId === pkg.id}
                          onClick={() => handleInstall(pkg)}
                          className="bg-[#87cf3e] hover:bg-[#76bb33] text-black font-bold text-xs px-4 py-1 rounded-full transition-colors active:scale-95 cursor-pointer shrink-0"
                        >
                          {installingId === pkg.id ? 'Installing...' : 'Get'}
                        </button>
                      )}
                    </div>
                  ))}
                </div>
              </div>
            </>
          ) : (
            /* Category / Search Results List */
            <div className="grid grid-cols-1 md:grid-cols-2 gap-x-10 gap-y-4">
              {filteredApps.map((pkg) => (
                <div key={pkg.id} className="flex items-center justify-between border-b border-[#87cf3e]/10 pb-3.5">
                  <div className="flex items-center gap-3.5">
                    <div className={`w-12 h-12 rounded-2xl ${pkg.squircleBg} ${pkg.iconColor} flex items-center justify-center shrink-0 shadow-sm`}>
                      {renderAppIcon(pkg.iconType)}
                    </div>
                    <div>
                      <h4 className="text-sm font-semibold text-slate-100">{pkg.name}</h4>
                      <p className="text-xs text-slate-400 line-clamp-1">{pkg.description}</p>
                    </div>
                  </div>

                  {pkg.installed ? (
                    <button
                      onClick={() => handleLaunch(pkg)}
                      className="bg-[#87cf3e]/15 hover:bg-[#87cf3e]/25 text-[#87cf3e] border border-[#87cf3e]/30 font-semibold text-xs px-4 py-1 rounded-full transition-colors active:scale-95 cursor-pointer shrink-0"
                    >
                      Open
                    </button>
                  ) : (
                    <button
                      disabled={installingId === pkg.id}
                      onClick={() => handleInstall(pkg)}
                      className="bg-[#87cf3e] hover:bg-[#76bb33] text-black font-bold text-xs px-4 py-1 rounded-full transition-colors active:scale-95 cursor-pointer shrink-0"
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
