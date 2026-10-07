import React, { useState, useMemo, useEffect, useRef } from 'react';
import {
  Search,
  X,
  Settings as SettingsIcon,
  Terminal as TerminalIcon,
  Power,
  Lock,
  Grid,
  Clock,
  Sparkles,
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
    lockSession,
    accentColor,
    isLiveEnvironment,
  } = useSystemState();

  const [searchQuery, setSearchQuery] = useState('');
  const [activeTab, setActiveTab] = useState<'all' | 'frequent' | 'system' | 'utilities' | 'accessories'>('all');
  const searchInputRef = useRef<HTMLInputElement>(null);

  const accent = ACCENT_COLOR_MAP[accentColor];

  // Auto-focus search input when menu opens
  useEffect(() => {
    if (isAppMenuOpen) {
      setSearchQuery('');
      const timer = setTimeout(() => {
        searchInputRef.current?.focus();
      }, 50);
      return () => clearTimeout(timer);
    }
  }, [isAppMenuOpen]);

  // Frequent apps list
  const frequentAppIds: AppId[] = [
    'browser',
    'terminal',
    'file-manager',
    'software',
    'steam',
    'system-monitor',
    'settings',
    'text-editor',
    'music',
  ];

  const appList = useMemo(() => {
    return Object.values(APP_REGISTRY).filter((app) => {
      if (app.id === 'installer' && !isLiveEnvironment) return false;

      const matchesSearch =
        app.title.toLowerCase().includes(searchQuery.toLowerCase()) ||
        app.description.toLowerCase().includes(searchQuery.toLowerCase());

      if (!matchesSearch) return false;

      if (activeTab === 'frequent') {
        return frequentAppIds.includes(app.id);
      }
      if (activeTab === 'all') {
        return true;
      }
      return app.category === activeTab;
    });
  }, [searchQuery, activeTab, isLiveEnvironment]);

  if (!isAppMenuOpen) return null;

  const handleLaunchApp = (appId: AppId) => {
    openApp(appId);
    setIsAppMenuOpen(false);
  };

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Escape') {
      setIsAppMenuOpen(false);
    } else if (e.key === 'Enter' && appList.length > 0) {
      handleLaunchApp(appList[0].id);
    }
  };

  return (
    <div
      id="app-menu-panel"
      className="fixed inset-0 z-50 flex flex-col justify-between bg-black/70 backdrop-blur-3xl select-none animate-in fade-in duration-200 p-6 sm:p-10"
      onClick={() => setIsAppMenuOpen(false)}
      onKeyDown={handleKeyDown}
    >
      {/* ======================================================== */}
      {/* TOP: Ubuntu Centered Search Bar & Category Navigation     */}
      {/* ======================================================== */}
      <div
        className="w-full max-w-2xl mx-auto flex flex-col items-center gap-4 pt-2"
        onClick={(e) => e.stopPropagation()}
      >
        {/* Ubuntu Pill Search Bar */}
        <div className="relative w-full flex items-center">
          <Search className="w-5 h-5 absolute left-4 text-slate-400 pointer-events-none" />
          <input
            ref={searchInputRef}
            type="text"
            placeholder="Type to search..."
            value={searchQuery}
            onChange={(e) => setSearchQuery(e.target.value)}
            className="w-full pl-12 pr-11 py-3 bg-[#18231c]/90 hover:bg-[#1f2b23]/95 border border-[#87cf3e]/30 focus:border-[#87cf3e] rounded-full text-slate-100 placeholder-slate-400 focus:outline-none focus:ring-4 focus:ring-[#87cf3e]/20 text-sm shadow-xl shadow-black/40 transition-all font-sans"
          />
          {searchQuery && (
            <button
              onClick={() => setSearchQuery('')}
              className="absolute right-4 p-1 rounded-full hover:bg-white/10 text-slate-400 hover:text-white transition-colors"
            >
              <X className="w-4 h-4" />
            </button>
          )}
        </div>

        {/* Ubuntu Category Tabs */}
        <div className="flex items-center gap-1.5 p-1 rounded-full bg-[#141c16]/80 border border-white/10 text-xs shadow-lg backdrop-blur-md">
          {[
            { id: 'all', label: 'All', icon: <Grid className="w-3.5 h-3.5" /> },
            { id: 'frequent', label: 'Frequent', icon: <Clock className="w-3.5 h-3.5" /> },
            { id: 'system', label: 'System', icon: null },
            { id: 'utilities', label: 'Utilities', icon: null },
            { id: 'accessories', label: 'Accessories', icon: null },
          ].map((tab) => {
            const isActive = activeTab === tab.id;
            return (
              <button
                key={tab.id}
                onClick={() => setActiveTab(tab.id as any)}
                className={`px-4 py-1.5 rounded-full capitalize transition-all font-medium flex items-center gap-1.5 cursor-pointer ${
                  isActive
                    ? 'bg-[#87cf3e] text-black font-semibold shadow-md shadow-[#87cf3e]/20'
                    : 'text-slate-300 hover:text-white hover:bg-white/10'
                }`}
              >
                {tab.icon}
                <span>{tab.label}</span>
              </button>
            );
          })}
        </div>
      </div>

      {/* ======================================================== */}
      {/* CENTER: Ubuntu Expansive Application Grid                 */}
      {/* ======================================================== */}
      <div
        className="flex-1 w-full max-w-5xl mx-auto my-6 overflow-y-auto px-4 py-2 flex items-center justify-center"
        onClick={(e) => e.stopPropagation()}
      >
        {appList.length === 0 ? (
          <div className="flex flex-col items-center justify-center gap-3 text-slate-400 py-16">
            <Search className="w-12 h-12 stroke-[1.5] text-slate-500" />
            <div className="text-base font-semibold text-slate-300">No applications found</div>
            <div className="text-xs text-slate-500">Try searching for a different name or description</div>
          </div>
        ) : (
          <div className="grid grid-cols-3 sm:grid-cols-4 md:grid-cols-5 lg:grid-cols-6 gap-6 sm:gap-8 w-full">
            {appList.map((app) => (
              <button
                key={app.id}
                onClick={() => handleLaunchApp(app.id)}
                className="group flex flex-col items-center justify-center p-3.5 rounded-2xl hover:bg-white/10 hover:shadow-xl hover:shadow-black/30 transition-all duration-150 cursor-pointer text-center relative focus:outline-none focus:ring-2 focus:ring-[#87cf3e]/40"
              >
                {/* App Vector Icon with Ubuntu hover bounce */}
                <div className="transform group-hover:scale-110 group-active:scale-95 transition-transform duration-150 drop-shadow-md">
                  <MacIcon id={app.id} size={54} />
                </div>

                {/* App Label */}
                <span className="text-xs font-medium text-slate-200 group-hover:text-white mt-2.5 truncate max-w-[100px] leading-tight">
                  {app.title}
                </span>

                {/* Description Pill on Hover */}
                <span className="text-[10px] text-slate-400 group-hover:text-slate-300 truncate max-w-[110px] mt-0.5 opacity-80">
                  {app.category}
                </span>
              </button>
            ))}
          </div>
        )}
      </div>

      {/* ======================================================== */}
      {/* BOTTOM: Ubuntu System Bar & Quick Power Controls          */}
      {/* ======================================================== */}
      <div
        className="w-full max-w-5xl mx-auto flex items-center justify-between pt-4 border-t border-white/10 text-xs"
        onClick={(e) => e.stopPropagation()}
      >
        {/* User Identity Chip */}
        <div className="flex items-center space-x-3">
          <div className="w-9 h-9 rounded-full bg-gradient-to-tr from-[#76bb33] to-[#87cf3e] flex items-center justify-center font-bold text-black text-xs shadow-md">
            AX
          </div>
          <div>
            <div className="font-semibold text-slate-200">Axis User</div>
            <div className="text-[10px] text-slate-400 font-mono">axis-pc • Debian 12 / Linux 6.12</div>
          </div>
        </div>

        {/* Ubuntu Application Page Dots */}
        <div className="hidden sm:flex items-center space-x-2">
          <span className="w-2.5 h-2.5 rounded-full bg-[#87cf3e] shadow-[0_0_8px_rgba(135,207,62,0.8)]" />
          <span className="w-2 h-2 rounded-full bg-white/30" />
          <span className="w-2 h-2 rounded-full bg-white/30" />
        </div>

        {/* Quick System Action Buttons */}
        <div className="flex items-center space-x-2">
          <button
            onClick={() => handleLaunchApp('terminal')}
            className="p-2 rounded-xl bg-white/5 hover:bg-white/10 text-slate-300 border border-white/10 flex items-center gap-1.5 transition-colors"
            title="Launch Terminal"
          >
            <TerminalIcon className="w-3.5 h-3.5 text-emerald-400" />
            <span className="hidden md:inline">Terminal</span>
          </button>
          <button
            onClick={() => handleLaunchApp('settings')}
            className="p-2 rounded-xl bg-white/5 hover:bg-white/10 text-slate-300 border border-white/10 flex items-center gap-1.5 transition-colors"
            title="System Settings"
          >
            <SettingsIcon className="w-3.5 h-3.5 text-slate-300" />
            <span className="hidden md:inline">Settings</span>
          </button>
          <button
            onClick={() => {
              setIsAppMenuOpen(false);
              lockSession();
            }}
            className="p-2 rounded-xl bg-white/5 hover:bg-white/10 text-slate-300 border border-white/10 flex items-center gap-1.5 transition-colors"
            title="Lock Session"
          >
            <Lock className="w-3.5 h-3.5 text-amber-400" />
            <span className="hidden md:inline">Lock</span>
          </button>
          <button
            onClick={() => {
              setIsAppMenuOpen(false);
              setPowerModalOpen(true);
            }}
            className="p-2 rounded-xl bg-rose-500/15 hover:bg-rose-500/25 text-rose-300 border border-rose-500/25 flex items-center gap-1.5 font-semibold transition-colors"
            title="Power Off / Restart"
          >
            <Power className="w-3.5 h-3.5 text-rose-400" />
            <span>Power</span>
          </button>
        </div>
      </div>
    </div>
  );
};
