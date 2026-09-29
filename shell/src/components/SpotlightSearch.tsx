import React, { useState, useMemo, useEffect, useRef } from 'react';
import { Search, Calculator, ArrowRight, CornerDownLeft } from 'lucide-react';
import { useSystemState } from '../context/SystemStateContext';
import { useWindowManager, APP_REGISTRY } from '../context/WindowManagerContext';
import { MacIcon } from './MacIcon';
import { AppId } from '../types/os';

export const SpotlightSearch: React.FC = () => {
  const { isSpotlightOpen, setIsSpotlightOpen, isLiveEnvironment } = useSystemState();
  const { openApp } = useWindowManager();
  const [query, setQuery] = useState('');
  const [selectedIndex, setSelectedIndex] = useState(0);
  const inputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    if (isSpotlightOpen) {
      setQuery('');
      setSelectedIndex(0);
      setTimeout(() => inputRef.current?.focus(), 50);
    }
  }, [isSpotlightOpen]);

  // Math calculator evaluation
  const mathResult = useMemo(() => {
    const trimmed = query.trim();
    if (!trimmed || !/^[\d\s+\-*/().%^]+$/.test(trimmed)) return null;
    try {
      // Safe math eval with Function
      const sanitized = trimmed.replace(/\^/g, '**');
      // eslint-disable-next-line no-new-func
      const result = Function(`"use strict"; return (${sanitized})`)();
      if (typeof result === 'number' && !isNaN(result)) {
        return result;
      }
    } catch {}
    return null;
  }, [query]);

  // Filtered Apps
  const matchedApps = useMemo(() => {
    const registryApps = Object.values(APP_REGISTRY).filter((app) => {
      if (app.id === 'installer' && !isLiveEnvironment) return false;
      return true;
    });

    if (!query.trim()) {
      return registryApps.slice(0, 5);
    }
    const q = query.toLowerCase();
    return registryApps.filter(
      (app) =>
        app.title.toLowerCase().includes(q) ||
        app.description.toLowerCase().includes(q)
    );
  }, [query, isLiveEnvironment]);

  if (!isSpotlightOpen) return null;

  const handleSelectApp = (appId: AppId) => {
    openApp(appId);
    setIsSpotlightOpen(false);
  };

  const handleKeyDown = (e: React.KeyboardEvent) => {
    if (e.key === 'Escape') {
      setIsSpotlightOpen(false);
    } else if (e.key === 'ArrowDown') {
      e.preventDefault();
      setSelectedIndex((prev) => (prev + 1) % Math.max(1, matchedApps.length));
    } else if (e.key === 'ArrowUp') {
      e.preventDefault();
      setSelectedIndex((prev) => (prev - 1 + matchedApps.length) % Math.max(1, matchedApps.length));
    } else if (e.key === 'Enter') {
      if (matchedApps[selectedIndex]) {
        handleSelectApp(matchedApps[selectedIndex].id);
      }
    }
  };

  return (
    <div
      className="fixed inset-0 z-50 flex items-start justify-center pt-24 bg-black/40 backdrop-blur-md select-none"
      onClick={() => setIsSpotlightOpen(false)}
    >
      <div
        className="w-full max-w-xl bg-slate-900/90 backdrop-blur-3xl rounded-2xl border border-white/15 shadow-[0_30px_70px_rgba(0,0,0,0.8)] overflow-hidden flex flex-col animate-in zoom-in-95 duration-100"
        onClick={(e) => e.stopPropagation()}
      >
        {/* Search input bar */}
        <div className="flex items-center px-4 py-3.5 border-b border-white/10 gap-3">
          <Search className="w-5 h-5 text-slate-400 shrink-0" />
          <input
            ref={inputRef}
            type="text"
            value={query}
            onChange={(e) => {
              setQuery(e.target.value);
              setSelectedIndex(0);
            }}
            onKeyDown={handleKeyDown}
            placeholder="Spotlight Search"
            className="w-full bg-transparent outline-none text-slate-100 placeholder-slate-400 text-lg font-normal"
          />
          <kbd className="hidden sm:inline-block px-2 py-0.5 text-[10px] font-mono text-slate-400 bg-white/5 rounded border border-white/10">
            esc
          </kbd>
        </div>

        {/* Live Calculation preview */}
        {mathResult !== null && (
          <div className="px-5 py-3 bg-amber-500/10 border-b border-amber-500/20 flex items-center justify-between">
            <div className="flex items-center space-x-2.5">
              <Calculator className="w-4 h-4 text-amber-400" />
              <span className="text-xs text-slate-300 font-mono">{query} =</span>
            </div>
            <span className="text-base font-bold text-amber-300 font-mono">{mathResult}</span>
          </div>
        )}

        {/* Results list */}
        <div className="max-h-72 overflow-y-auto p-2 flex flex-col gap-1">
          <div className="px-3 py-1 text-[11px] font-semibold text-slate-400 uppercase tracking-wider">
            {query.trim() ? 'Applications' : 'Top Suggestions'}
          </div>

          {matchedApps.map((app, idx) => {
            const isSelected = selectedIndex === idx;
            return (
              <button
                key={app.id}
                onClick={() => handleSelectApp(app.id)}
                className={`flex items-center justify-between p-2.5 rounded-xl transition-colors text-left ${
                  isSelected ? 'bg-blue-600 text-white shadow-md' : 'hover:bg-white/5 text-slate-200'
                }`}
              >
                <div className="flex items-center space-x-3">
                  <MacIcon id={app.id} size={32} />
                  <div>
                    <div className="text-xs font-semibold">{app.title}</div>
                    <div className={`text-[10px] ${isSelected ? 'text-blue-100' : 'text-slate-400'}`}>
                      {app.description}
                    </div>
                  </div>
                </div>

                {isSelected && (
                  <CornerDownLeft className="w-4 h-4 text-white shrink-0 mr-2 opacity-80" />
                )}
              </button>
            );
          })}
        </div>
      </div>
    </div>
  );
};
