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
  { id: 'file-manager', title: 'Finder' },
  { id: 'browser', title: 'Axis Browser' },
  { id: 'music', title: 'Music' },
  { id: 'photos', title: 'Photos' },
  { id: 'notes', title: 'Notes' },
  { id: 'camera', title: 'Photo Booth' },
  { id: 'software', title: 'Axis Store' },
  { id: 'clock', title: 'Clock' },
  { id: 'weather', title: 'Weather' },
  { id: 'terminal', title: 'Terminal' },
  { id: 'system-monitor', title: 'Activity Monitor' },
  { id: 'calculator', title: 'Calculator' },
  { id: 'settings', title: 'System Settings' },
  { id: 'installer', title: 'Install AxisOS' },
];

export const DockOrTaskbar: React.FC = () => {
  const { openApp, isAppRunning, activeWindowId, windows, minimizeWindow, focusWindow } =
    useWindowManager();
  const { isLiveEnvironment } = useSystemState();
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
    if (hoveredIdx === null) return 'scale-100';
    const dist = Math.abs(hoveredIdx - index);
    if (dist === 0) return 'scale-135 -translate-y-2.5';
    if (dist === 1) return 'scale-118 -translate-y-1.5';
    if (dist === 2) return 'scale-106 -translate-y-0.5';
    return 'scale-100';
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
        className={`fixed bottom-2.5 left-1/2 -translate-x-1/2 z-40 select-none transition-all duration-300 ease-in-out ${
          isDockHidden
            ? 'translate-y-36 opacity-0 pointer-events-none'
            : 'translate-y-0 opacity-100 pointer-events-auto'
        }`}
      >
        <div
          className="flex items-end space-x-2.5 px-3.5 py-2.5 rounded-2xl bg-white/40 dark:bg-[#18201b]/75 backdrop-blur-3xl border border-black/10 dark:border-[#87cf3e]/25 shadow-[0_20px_50px_rgba(0,0,0,0.25)] dark:shadow-[0_20px_50px_rgba(0,0,0,0.7)]"
        >
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
                        ? 'w-1.5 h-1.5 bg-[#87cf3e] shadow-[0_0_8px_rgba(135,207,62,0.95)]'
                        : 'w-1 h-1 bg-[#87cf3e]/60'
                    }`}
                  />
                )}
              </div>

              {/* macOS Tooltip */}
              <span className="absolute -top-10 px-2.5 py-1 rounded-md bg-[#18201b]/95 text-[11px] text-white font-medium border border-[#87cf3e]/30 shadow-lg opacity-0 group-hover:opacity-100 transition-opacity pointer-events-none whitespace-nowrap backdrop-blur-md">
                {app.title}
              </span>
            </button>
          );
        })}

        {/* Vertical divider */}
        <div className="h-10 w-[1px] bg-[#87cf3e]/20 mx-1 mb-1"></div>

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

          <span className="absolute -top-10 px-2.5 py-1 rounded-md bg-[#18201b]/95 text-[11px] text-white font-medium border border-[#87cf3e]/30 shadow-lg opacity-0 group-hover:opacity-100 transition-opacity pointer-events-none whitespace-nowrap backdrop-blur-md">
            Trash
          </span>
        </button>
      </div>
    </div>
  </>
);
};
