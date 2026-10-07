import React, { useState } from 'react';
import { useWindowManager } from '../context/WindowManagerContext';
import { useSystemState } from '../context/SystemStateContext';
import { MacIcon } from './MacIcon';
import { AppId } from '../types/os';

interface DockItem {
  id: AppId;
  title: string;
}

const DOCK_APPS: DockItem[] = [
  { id: 'file-manager', title: 'Axis Files' },
  { id: 'browser', title: 'Axis Web' },
  { id: 'terminal', title: 'Axis Console' },
  { id: 'system-monitor', title: 'Axis Diagnostics' },
  { id: 'software', title: 'Axis Store' },
  { id: 'steam', title: 'Steam' },
  { id: 'music', title: 'Axis Audio' },
  { id: 'photos', title: 'Axis Gallery' },
  { id: 'notes', title: 'Axis Memo' },
  { id: 'camera', title: 'Axis Lens' },
  { id: 'clock', title: 'Axis Chrono' },
  { id: 'weather', title: 'Axis Climate' },
  { id: 'calculator', title: 'Axis Calc' },
  { id: 'settings', title: 'Control Center' },
  { id: 'installer', title: 'Axis Setup' },
];

export const DockOrTaskbar: React.FC = () => {
  const { openApp, isAppRunning, activeWindowId, windows, minimizeWindow, focusWindow } =
    useWindowManager();
  const { isLiveEnvironment, isAppMenuOpen, setIsAppMenuOpen } = useSystemState();
  const [hoveredIdx, setHoveredIdx] = useState<number | null>(null);
  const [isDockRevealed, setIsDockRevealed] = useState(false);

  // Auto-hide dock when any focused or active window is maximized
  const hasMaximizedWindow = windows.some(
    (w) => w.isMaximized && !w.isMinimized && (w.id === activeWindowId || windows.filter((x) => !x.isMinimized).every((x) => x.zIndex <= w.zIndex))
  );
  const isDockHidden = hasMaximizedWindow && !isDockRevealed;

  const dockApps = isLiveEnvironment ? DOCK_APPS : DOCK_APPS.filter((a) => a.id !== 'installer');

  const handleAppClick = (appId: AppId) => {
    const runningWindow = windows.find((w) => w.appId === appId);
    if (!runningWindow) {
      openApp(appId);
    } else if (runningWindow.id === activeWindowId && !runningWindow.isMinimized) {
      minimizeWindow(runningWindow.id);
    } else {
      focusWindow(runningWindow.id);
    }
  };

  const getScaleClass = (index: number) => {
    if (hoveredIdx === null) return 'scale-100 relative z-0';
    const dist = Math.abs(hoveredIdx - index);
    if (dist === 0) return 'scale-115 -translate-y-1.5 relative z-20';
    if (dist === 1) return 'scale-105 -translate-y-0.5 relative z-10';
    return 'scale-100 relative z-0';
  };

  return (
    <>
      {/* Bottom edge hover trigger hotspot when in fullscreen/maximized */}
      {hasMaximizedWindow && (
        <div
          onMouseEnter={() => setIsDockRevealed(true)}
          className="fixed bottom-0 left-0 w-full h-2.5 z-40 bg-transparent pointer-events-auto"
        />
      )}

      <div
        onMouseEnter={() => setIsDockRevealed(true)}
        onMouseLeave={() => {
          setHoveredIdx(null);
          setIsDockRevealed(false);
        }}
        className={`fixed bottom-2 left-1/2 -translate-x-1/2 z-40 select-none transition-all duration-300 ease-in-out max-w-[96vw] ${
          isDockHidden
            ? 'translate-y-36 opacity-0 pointer-events-none'
            : 'translate-y-0 opacity-100 pointer-events-auto'
        }`}
      >
        <div
          className="flex items-end gap-1.5 px-3 py-2 rounded-2xl bg-black/90 backdrop-blur-3xl border border-[#87cf3e]/30 shadow-[0_20px_50px_rgba(0,0,0,0.8)] overflow-x-auto scrollbar-none"
        >
          {/* Ubuntu 9-Dots "Show Applications" Launcher */}
          <button
            onClick={() => setIsAppMenuOpen(!isAppMenuOpen)}
            onMouseEnter={() => setHoveredIdx(-1)}
            className="relative group flex flex-col items-center justify-end focus:outline-none transition-all duration-150 ease-out shrink-0"
            title="Show Applications (Super)"
          >
            <div
              className={`w-[44px] h-[44px] rounded-2xl bg-[#141b16] border border-[#87cf3e]/30 flex items-center justify-center transition-all duration-150 ease-out ${
                hoveredIdx === -1 ? 'scale-115 -translate-y-1.5 bg-[#87cf3e]/25 border-[#87cf3e] z-20 relative' : isAppMenuOpen ? 'border-[#87cf3e] bg-[#87cf3e]/20' : 'scale-100'
              }`}
            >
              <div className="grid grid-cols-3 gap-1 p-2">
                {[...Array(9)].map((_, i) => (
                  <div key={i} className="w-1.5 h-1.5 rounded-full bg-[#87cf3e]" />
                ))}
              </div>
            </div>
            <div className="h-1.5 mt-0.5">
              {isAppMenuOpen && (
                <span className="w-1.5 h-1.5 rounded-full bg-[#87cf3e] shadow-[0_0_8px_rgba(135,207,62,0.95)] inline-block" />
              )}
            </div>

            <span className="absolute -top-10 px-2.5 py-1 rounded-md bg-[#101712]/95 text-[11px] text-white font-medium border border-[#87cf3e]/40 shadow-xl opacity-0 group-hover:opacity-100 transition-opacity pointer-events-none whitespace-nowrap backdrop-blur-md z-50">
              Show Applications
            </span>
          </button>

          {/* Vertical divider */}
          <div className="h-8 w-[1px] bg-[#87cf3e]/20 mx-0.5 mb-2 shrink-0" />

          {/* Dock Applications */}
          {dockApps.map((app, idx) => {
            const isRunning = isAppRunning(app.id);
            const runningWindow = windows.find((w) => w.appId === app.id);
            const isActive =
              runningWindow && runningWindow.id === activeWindowId && !runningWindow.isMinimized;

            return (
              <button
                key={app.id}
                onClick={() => handleAppClick(app.id)}
                onMouseEnter={() => setHoveredIdx(idx)}
                className="relative group flex flex-col items-center justify-end focus:outline-none transition-all duration-150 ease-out shrink-0"
              >
                {/* Vector Icon */}
                <div className={`transition-all duration-150 ease-out ${getScaleClass(idx)}`}>
                  <MacIcon id={app.id} size={44} />
                </div>

                {/* Running Status Dot */}
                <div className="h-1.5 flex items-center justify-center mt-0.5">
                  {isRunning && (
                    <span
                      className={`rounded-full transition-all ${
                        isActive
                          ? 'w-1.5 h-1.5 bg-[#87cf3e] shadow-[0_0_8px_rgba(135,207,62,0.95)]'
                          : 'w-1 h-1 bg-[#87cf3e]/60'
                      }`}
                    />
                  )}
                </div>

                {/* Tooltip */}
                <span className="absolute -top-10 px-2.5 py-1 rounded-md bg-[#101712]/95 text-[11px] text-white font-medium border border-[#87cf3e]/40 shadow-xl opacity-0 group-hover:opacity-100 transition-opacity pointer-events-none whitespace-nowrap backdrop-blur-md z-50">
                  {app.title}
                </span>
              </button>
            );
          })}

          {/* Vertical divider */}
          <div className="h-8 w-[1px] bg-[#87cf3e]/20 mx-0.5 mb-2 shrink-0" />

          {/* Trash */}
          <button
            onClick={() => alert('Trash is empty')}
            onMouseEnter={() => setHoveredIdx(99)}
            className="relative group flex flex-col items-center justify-end focus:outline-none transition-all duration-150 ease-out shrink-0"
          >
            <div
              className={`transition-all duration-150 ease-out ${
                hoveredIdx === 99 ? 'scale-115 -translate-y-1.5 relative z-20' : 'scale-100 relative z-0'
              }`}
            >
              <MacIcon id="trash" size={44} />
            </div>
            <div className="h-1.5 mt-0.5" />

            <span className="absolute -top-10 px-2.5 py-1 rounded-md bg-[#101712]/95 text-[11px] text-white font-medium border border-[#87cf3e]/40 shadow-xl opacity-0 group-hover:opacity-100 transition-opacity pointer-events-none whitespace-nowrap backdrop-blur-md z-50">
              Trash
            </span>
          </button>
        </div>
      </div>
    </>
  );
};
