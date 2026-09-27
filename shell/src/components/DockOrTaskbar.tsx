import React, { useState } from 'react';
import { useWindowManager } from '../context/WindowManagerContext';
import { MacIcon } from './MacIcon';
import { AppId } from '../types/os';

interface DockItem {
  id: AppId;
  title: string;
}

const DOCK_APPS: DockItem[] = [
  { id: 'file-manager', title: 'Finder' },
  { id: 'browser', title: 'Safari' },
  { id: 'terminal', title: 'Terminal' },
  { id: 'system-monitor', title: 'Activity Monitor' },
  { id: 'text-editor', title: 'Notes' },
  { id: 'calculator', title: 'Calculator' },
  { id: 'settings', title: 'System Settings' },
  { id: 'installer', title: 'Install AxisOS' },
];

export const DockOrTaskbar: React.FC = () => {
  const { openApp, isAppRunning, activeWindowId, windows, minimizeWindow, focusWindow } =
    useWindowManager();
  const [hoveredIdx, setHoveredIdx] = useState<number | null>(null);

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
    if (hoveredIdx === null) return 'scale-100';
    const dist = Math.abs(hoveredIdx - index);
    if (dist === 0) return 'scale-135 -translate-y-2.5';
    if (dist === 1) return 'scale-118 -translate-y-1.5';
    if (dist === 2) return 'scale-106 -translate-y-0.5';
    return 'scale-100';
  };

  return (
    <div className="fixed bottom-2.5 left-1/2 -translate-x-1/2 z-40 select-none">
      <div
        onMouseLeave={() => setHoveredIdx(null)}
        className="flex items-end space-x-2.5 px-3.5 py-2.5 rounded-2xl bg-white/10 dark:bg-slate-900/40 backdrop-blur-3xl border border-white/20 dark:border-white/15 shadow-[0_20px_50px_rgba(0,0,0,0.6)]"
      >
        {/* Dock Applications */}
        {DOCK_APPS.map((app, idx) => {
          const isRunning = isAppRunning(app.id);
          const runningWindow = windows.find((w) => w.appId === app.id);
          const isActive =
            runningWindow && runningWindow.id === activeWindowId && !runningWindow.isMinimized;

          return (
            <button
              key={app.id}
              onClick={() => handleAppClick(app.id)}
              onMouseEnter={() => setHoveredIdx(idx)}
              className="relative group flex flex-col items-center justify-end focus:outline-none transition-all duration-150 ease-out"
            >
              {/* Animated macOS Squircle Icon */}
              <div className={`transition-all duration-150 ease-out ${getScaleClass(idx)}`}>
                <MacIcon id={app.id} size={50} />
              </div>

              {/* Running Status Dot */}
              <div className="h-1.5 flex items-center justify-center mt-1">
                {isRunning && (
                  <span
                    className={`rounded-full transition-all ${
                      isActive
                        ? 'w-1.5 h-1.5 bg-white shadow-[0_0_6px_rgba(255,255,255,0.9)]'
                        : 'w-1 h-1 bg-white/50'
                    }`}
                  />
                )}
              </div>

              {/* macOS Tooltip */}
              <span className="absolute -top-10 px-2.5 py-1 rounded-md bg-slate-900/90 text-[11px] text-white font-medium border border-white/10 shadow-lg opacity-0 group-hover:opacity-100 transition-opacity pointer-events-none whitespace-nowrap backdrop-blur-md">
                {app.title}
              </span>
            </button>
          );
        })}

        {/* Vertical divider */}
        <div className="h-10 w-[1px] bg-white/20 mx-1 mb-1"></div>

        {/* Trash */}
        <button
          onClick={() => alert('Trash is empty')}
          onMouseEnter={() => setHoveredIdx(99)}
          className="relative group flex flex-col items-center justify-end focus:outline-none transition-all duration-150 ease-out"
        >
          <div
            className={`transition-all duration-150 ease-out ${
              hoveredIdx === 99 ? 'scale-125 -translate-y-2' : 'scale-100'
            }`}
          >
            <MacIcon id="trash" size={48} />
          </div>
          <div className="h-1.5 mt-1" />

          <span className="absolute -top-10 px-2.5 py-1 rounded-md bg-slate-900/90 text-[11px] text-white font-medium border border-white/10 shadow-lg opacity-0 group-hover:opacity-100 transition-opacity pointer-events-none whitespace-nowrap backdrop-blur-md">
            Trash
          </span>
        </button>
      </div>
    </div>
  );
};
