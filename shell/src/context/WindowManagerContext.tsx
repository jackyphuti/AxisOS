import React, { createContext, useContext, useState, useCallback } from 'react';
import { AppId, AppDefinition, WindowState } from '../types/os';

export const APP_REGISTRY: Record<AppId, AppDefinition> = {
  'file-manager': {
    id: 'file-manager',
    title: 'Axis Files',
    category: 'accessories',
    description: 'Browse local files, directories, drives, and network locations.',
    defaultWidth: 840,
    defaultHeight: 520,
    isPinned: true,
  },
  browser: {
    id: 'browser',
    title: 'Axis Web',
    category: 'utilities',
    description: 'High-performance web browser powered by Google Chromium engine.',
    defaultWidth: 960,
    defaultHeight: 620,
    isPinned: true,
  },
  terminal: {
    id: 'terminal',
    title: 'Axis Console',
    category: 'utilities',
    description: 'Real interactive Linux bash shell with host hardware access.',
    defaultWidth: 780,
    defaultHeight: 480,
    isPinned: true,
  },
  'system-monitor': {
    id: 'system-monitor',
    title: 'Axis Diagnostics',
    category: 'system',
    description: 'Inspect live CPU threads, memory pressure, and Linux processes.',
    defaultWidth: 820,
    defaultHeight: 520,
    isPinned: true,
  },
  'text-editor': {
    id: 'text-editor',
    title: 'Axis Write',
    category: 'accessories',
    description: 'Modern code and text editor with syntax highlighting.',
    defaultWidth: 740,
    defaultHeight: 500,
    isPinned: false,
  },
  music: {
    id: 'music',
    title: 'Axis Audio',
    category: 'accessories',
    description: 'Audio player with real-time synthesizer, spectrum visualizer, and local audio import.',
    defaultWidth: 840,
    defaultHeight: 540,
    isPinned: true,
  },
  photos: {
    id: 'photos',
    title: 'Axis Gallery',
    category: 'accessories',
    description: 'Image gallery, dynamic wallpapers, and media viewer.',
    defaultWidth: 860,
    defaultHeight: 560,
    isPinned: true,
  },
  notes: {
    id: 'notes',
    title: 'Axis Memo',
    category: 'accessories',
    description: 'Quick notes, organized folders, and checkable lists.',
    defaultWidth: 800,
    defaultHeight: 520,
    isPinned: true,
  },
  calculator: {
    id: 'calculator',
    title: 'Axis Calc',
    category: 'accessories',
    description: 'Standard and scientific precision calculator.',
    defaultWidth: 320,
    defaultHeight: 460,
    isPinned: true,
  },
  settings: {
    id: 'settings',
    title: 'Control Center',
    category: 'system',
    description: 'Configure appearance, displays, hardware, and preferences.',
    defaultWidth: 860,
    defaultHeight: 580,
    isPinned: true,
  },
  installer: {
    id: 'installer',
    title: 'Axis Setup',
    category: 'system',
    description: 'System setup and installation wizard for AxisOS.',
    defaultWidth: 880,
    defaultHeight: 600,
    isPinned: true,
  },
  about: {
    id: 'about',
    title: 'About AxisOS',
    category: 'system',
    description: 'Hardware overview, processor, memory, and kernel details.',
    defaultWidth: 600,
    defaultHeight: 420,
    isPinned: false,
  },
  software: {
    id: 'software',
    title: 'Axis Store',
    category: 'utilities',
    description: 'Discover, install, update, and manage native Linux applications & APT packages.',
    defaultWidth: 920,
    defaultHeight: 620,
    isPinned: true,
  },
  clock: {
    id: 'clock',
    title: 'Axis Chrono',
    category: 'accessories',
    description: 'World clock, alarms, precision stopwatch, and countdown timer.',
    defaultWidth: 680,
    defaultHeight: 520,
    isPinned: true,
  },
  weather: {
    id: 'weather',
    title: 'Axis Climate',
    category: 'accessories',
    description: 'Real-time weather forecasts, hourly forecasts, and atmospheric conditions.',
    defaultWidth: 720,
    defaultHeight: 540,
    isPinned: true,
  },
  camera: {
    id: 'camera',
    title: 'Axis Lens',
    category: 'accessories',
    description: 'Capture photos, apply real-time creative filters, and set desktop wallpapers.',
    defaultWidth: 760,
    defaultHeight: 560,
    isPinned: true,
  },
};

interface WindowManagerContextType {
  windows: WindowState[];
  activeWindowId: string | null;
  activeAppId: AppId | null;
  openApp: (appId: AppId, params?: Record<string, any>) => void;
  closeWindow: (id: string) => void;
  closeApp: (appId: AppId) => void;
  minimizeWindow: (id: string) => void;
  toggleMaximizeWindow: (id: string) => void;
  focusWindow: (id: string) => void;
  updateWindowPosition: (id: string, x: number, y: number) => void;
  updateWindowSize: (id: string, width: number, height: number) => void;
  isAppRunning: (appId: AppId) => boolean;
}

const WindowManagerContext = createContext<WindowManagerContextType | null>(null);

let zCounter = 10;

export const WindowManagerProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [windows, setWindows] = useState<WindowState[]>([]);
  const [activeWindowId, setActiveWindowId] = useState<string | null>(null);

  const activeWindow = windows.find((w) => w.id === activeWindowId);
  const activeAppId = activeWindow ? activeWindow.appId : null;

  const focusWindow = useCallback((id: string) => {
    zCounter += 1;
    setActiveWindowId(id);
    setWindows((prev) =>
      prev.map((win) => (win.id === id ? { ...win, isMinimized: false, zIndex: zCounter } : win))
    );
  }, []);

  const openApp = useCallback(
    (appId: AppId, params?: Record<string, any>) => {
      // Check if already open
      const existing = windows.find((w) => w.appId === appId);
      if (existing) {
        zCounter += 1;
        setWindows((prev) =>
          prev.map((w) =>
            w.id === existing.id
              ? {
                  ...w,
                  isMinimized: false,
                  animating: 'restoring',
                  zIndex: zCounter,
                  params: params ? { ...w.params, ...params } : w.params,
                }
              : w
          )
        );
        setActiveWindowId(existing.id);
        setTimeout(() => {
          setWindows((prev) =>
            prev.map((w) => (w.id === existing.id ? { ...w, animating: undefined } : w))
          );
        }, 220);
        return;
      }

      const appDef = APP_REGISTRY[appId];
      if (!appDef) return;

      zCounter += 1;
      const screenWidth = typeof window !== 'undefined' ? window.innerWidth : 1280;
      const screenHeight = typeof window !== 'undefined' ? window.innerHeight : 800;

      const offset = (windows.length % 5) * 28;
      const initialX = Math.max(40, (screenWidth - appDef.defaultWidth) / 2 + offset);
      const initialY = Math.max(50, (screenHeight - appDef.defaultHeight) / 2 - 30 + offset);

      const newWindow: WindowState = {
        id: `${appId}-${Date.now()}`,
        appId,
        title: appDef.title,
        x: initialX,
        y: initialY,
        width: Math.min(appDef.defaultWidth, screenWidth - 60),
        height: Math.min(appDef.defaultHeight, screenHeight - 100),
        isMinimized: false,
        isMaximized: false,
        animating: 'restoring',
        zIndex: zCounter,
        params,
      };

      setWindows((prev) => [...prev, newWindow]);
      setActiveWindowId(newWindow.id);
      setTimeout(() => {
        setWindows((prev) =>
          prev.map((w) => (w.id === newWindow.id ? { ...w, animating: undefined } : w))
        );
      }, 220);
    },
    [windows, focusWindow]
  );

  const closeWindow = useCallback((id: string) => {
    setWindows((prev) =>
      prev.map((w) => (w.id === id ? { ...w, animating: 'closing' } : w))
    );
    setTimeout(() => {
      setWindows((prev) => prev.filter((w) => w.id !== id));
      setActiveWindowId((current) => (current === id ? null : current));
    }, 180);
  }, []);

  const closeApp = useCallback((appId: AppId) => {
    setWindows((prev) =>
      prev.map((w) => (w.appId === appId ? { ...w, animating: 'closing' } : w))
    );
    setTimeout(() => {
      setWindows((prev) => prev.filter((w) => w.appId !== appId));
      setActiveWindowId((current) => {
        const closing = windows.find((w) => w.appId === appId);
        return closing && closing.id === current ? null : current;
      });
    }, 180);
  }, [windows]);

  const minimizeWindow = useCallback((id: string) => {
    setWindows((prev) =>
      prev.map((w) => (w.id === id ? { ...w, animating: 'minimizing' } : w))
    );
    setActiveWindowId((current) => (current === id ? null : current));
    setTimeout(() => {
      setWindows((prev) =>
        prev.map((w) => (w.id === id ? { ...w, isMinimized: true, animating: undefined } : w))
      );
    }, 200);
  }, []);

  const toggleMaximizeWindow = useCallback((id: string) => {
    setWindows((prev) =>
      prev.map((w) => (w.id === id ? { ...w, isMaximized: !w.isMaximized } : w))
    );
  }, []);

  const updateWindowPosition = useCallback((id: string, x: number, y: number) => {
    setWindows((prev) =>
      prev.map((w) => (w.id === id ? { ...w, x, y } : w))
    );
  }, []);

  const updateWindowSize = useCallback((id: string, width: number, height: number) => {
    setWindows((prev) =>
      prev.map((w) => (w.id === id ? { ...w, width, height } : w))
    );
  }, []);

  const isAppRunning = useCallback(
    (appId: AppId) => windows.some((w) => w.appId === appId),
    [windows]
  );

  return (
    <WindowManagerContext.Provider
      value={{
        windows,
        activeWindowId,
        activeAppId,
        openApp,
        closeWindow,
        closeApp,
        minimizeWindow,
        toggleMaximizeWindow,
        focusWindow,
        updateWindowPosition,
        updateWindowSize,
        isAppRunning,
      }}
    >
      {children}
    </WindowManagerContext.Provider>
  );
};

export const useWindowManager = () => {
  const ctx = useContext(WindowManagerContext);
  if (!ctx) throw new Error('useWindowManager must be used within WindowManagerProvider');
  return ctx;
};
