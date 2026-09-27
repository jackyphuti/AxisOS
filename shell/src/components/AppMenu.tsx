import React, { useState, useMemo } from 'react';
import {
  Search,
  Sparkles,
  Settings as SettingsIcon,
  Terminal as TerminalIcon,
  Folder,
  FileText,
  Activity,
  Info,
  Power,
  Lock,
} from 'lucide-react';
import { useWindowManager, APP_REGISTRY } from '../context/WindowManagerContext';
import { useSystemState, ACCENT_COLOR_MAP } from '../context/SystemStateContext';
import { MacIcon } from './MacIcon';
import { AppId } from '../types/os';

export const AppMenu: React.FC = () => {
  const { openApp } = useWindowManager();
  const {
    isAppMenuOpen,
    setIsAppMenuOpen,
    setPowerModalOpen,
    accentColor,
  } = useSystemState();

  const [searchQuery, setSearchQuery] = useState('');
  const [selectedCategory, setSelectedCategory] = useState<'all' | 'system' | 'utilities' | 'accessories'>('all');

  const accent = ACCENT_COLOR_MAP[accentColor];

  const appList = useMemo(() => {
    return Object.values(APP_REGISTRY).filter((app) => {
      const matchesSearch =
        app.title.toLowerCase().includes(searchQuery.toLowerCase()) ||
        app.description.toLowerCase().includes(searchQuery.toLowerCase());
      const matchesCategory =
        selectedCategory === 'all' || app.category === selectedCategory;
      return matchesSearch && matchesCategory;
    });
  }, [searchQuery, selectedCategory]);

  if (!isAppMenuOpen) return null;

  const handleLaunchApp = (appId: AppId) => {
    openApp(appId);
    setIsAppMenuOpen(false);
  };

  return (
    <div
      id="app-menu-panel"
      className="fixed inset-0 z-50 flex items-center justify-center bg-black/40 backdrop-blur-md select-none"
      onClick={() => setIsAppMenuOpen(false)}
    >
      <div
        className="w-full max-w-2xl bg-slate-950/90 backdrop-blur-2xl rounded-2xl border border-white/10 shadow-2xl shadow-black/90 p-6 flex flex-col gap-5 animate-in zoom-in-95 duration-150"
        onClick={(e) => e.stopPropagation()}
      >
        {/* Search Bar */}
        <div className="relative flex items-center">
          <Search className="w-5 h-5 absolute left-4 text-slate-400" />
          <input
            type="text"
            placeholder="Type to search apps, files, or settings..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            autoFocus
            className="w-full pl-12 pr-4 py-3 bg-white/5 border border-white/10 rounded-xl text-slate-100 placeholder-slate-400 focus:outline-none focus:ring-2 focus:ring-cyan-500/50 text-sm"
          />
        </div>

        {/* Categories Bar */}
        <div className="flex items-center space-x-2 border-b border-white/10 pb-3 text-xs">
          {(['all', 'system', 'utilities', 'accessories'] as const).map((cat) => (
            <button
              key={cat}
              onClick={() => setSelectedCategory(cat)}
              className={`px-3 py-1.5 rounded-lg capitalize transition-colors ${
                selectedCategory === cat
                  ? `${accent.bg} ${accent.border} text-white font-semibold`
                  : 'text-slate-400 hover:text-white hover:bg-white/5'
              }`}
            >
              {cat}
            </button>
          ))}
          <div className="ml-auto text-[11px] text-slate-500 font-mono">
            {appList.length} {appList.length === 1 ? 'app' : 'apps'}
          </div>
        </div>

        {/* App Grid */}
        <div className="grid grid-cols-2 sm:grid-cols-3 gap-3 max-h-80 overflow-y-auto pr-1">
          {appList.map((app) => (
            <button
              key={app.id}
              onClick={() => handleLaunchApp(app.id)}
              className="group flex items-start space-x-3 p-3 rounded-xl bg-white/5 hover:bg-white/10 border border-white/5 hover:border-white/15 transition-all text-left"
            >
              <div className="shrink-0 group-hover:scale-105 transition-transform">
                <MacIcon id={app.id} size={42} />
              </div>
              <div className="overflow-hidden">
                <div className="text-xs font-semibold text-slate-200 group-hover:text-white truncate">
                  {app.title}
                </div>
                <div className="text-[11px] text-slate-400 line-clamp-2 mt-0.5 leading-snug">
                  {app.description}
                </div>
              </div>
            </button>
          ))}
        </div>

        {/* Bottom User Bar & Power Controls */}
        <div className="flex items-center justify-between pt-3 border-t border-white/10">
          <div className="flex items-center space-x-3">
            <div className="w-8 h-8 rounded-full bg-gradient-to-tr from-cyan-600 to-blue-500 flex items-center justify-center font-bold text-white text-xs">
              AX
            </div>
            <div>
              <div className="text-xs font-semibold text-slate-200">Axis User</div>
              <div className="text-[10px] text-slate-400">axis-pc • Debian Core / Linux 6.12</div>
            </div>
          </div>

          <div className="flex items-center space-x-2">
            <button
              onClick={() => {
                setIsAppMenuOpen(false);
                setPowerModalOpen(true);
              }}
              className="p-2 rounded-xl bg-rose-500/10 hover:bg-rose-500/20 text-rose-300 border border-rose-500/20 flex items-center gap-1.5 text-xs font-medium transition-colors"
            >
              <Power className="w-3.5 h-3.5" />
              <span>Power</span>
            </button>
          </div>
        </div>
      </div>
    </div>
  );
};
