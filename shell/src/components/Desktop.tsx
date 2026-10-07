import React, { useState } from 'react';
import { Sparkles, Terminal as TerminalIcon, Folder, Settings, Palette, Info, Monitor, HardDrive } from 'lucide-react';
import { useSystemState } from '../context/SystemStateContext';
import { useWindowManager } from '../context/WindowManagerContext';
import { WindowFrame } from './WindowFrame';
import { TopBar } from './TopBar';
import { DockOrTaskbar } from './DockOrTaskbar';
import { QuickSettings } from './QuickSettings';
import { SpotlightSearch } from './SpotlightSearch';
import { PowerModal } from './PowerModal';
import { LiveWelcomeModal } from './LiveWelcomeModal';
import { MacIcon } from './MacIcon';
import { AppMenu } from './AppMenu';
import { LockScreen } from './LockScreen';
import { BootSplashScreen } from './BootSplashScreen';

// Apps
import { InstallerApp } from '../apps/Installer/InstallerApp';
import { SettingsApp } from '../apps/Settings/SettingsApp';
import { TerminalApp } from '../apps/Terminal/TerminalApp';
import { FileManagerApp } from '../apps/FileManager/FileManagerApp';
import { TextEditorApp } from '../apps/TextEditor/TextEditorApp';
import { SystemMonitorApp } from '../apps/SystemMonitor/SystemMonitorApp';
import { BrowserApp } from '../apps/Browser/BrowserApp';
import { CalculatorApp } from '../apps/Calculator/CalculatorApp';
import { AboutApp } from '../apps/About/AboutApp';
import { MusicApp } from '../apps/Music/MusicApp';
import { PhotosApp } from '../apps/Photos/PhotosApp';
import { NotesApp } from '../apps/Notes/NotesApp';
import { SoftwareApp } from '../apps/Software/SoftwareApp';
import { ClockApp } from '../apps/Clock/ClockApp';
import { WeatherApp } from '../apps/Weather/WeatherApp';
import { CameraApp } from '../apps/Camera/CameraApp';
import { AppId } from '../types/os';

export const Desktop: React.FC = () => {
  const { wallpaper, isQuickSettingsOpen, isLiveEnvironment, brightness, nightLight } = useSystemState();
  const { windows, openApp } = useWindowManager();
  const [contextMenu, setContextMenu] = useState<{ x: number; y: number } | null>(null);

  const handleContextMenu = (e: React.MouseEvent) => {
    // Only trigger if clicking directly on the desktop background
    if (
      (e.target as HTMLElement).closest('.window-frame') ||
      (e.target as HTMLElement).closest('header') ||
      (e.target as HTMLElement).closest('button')
    ) {
      return;
    }
    e.preventDefault();
    setContextMenu({ x: e.clientX, y: e.clientY });
  };

  const closeContextMenu = () => {
    if (contextMenu) setContextMenu(null);
  };

  const renderAppContent = (appId: AppId, params?: Record<string, any>) => {
    switch (appId) {
      case 'installer':
        return <InstallerApp />;
      case 'settings':
        return <SettingsApp params={params} />;
      case 'terminal':
        return <TerminalApp params={params} />;
      case 'file-manager':
        return <FileManagerApp params={params} />;
      case 'browser':
        return <BrowserApp />;
      case 'music':
        return <MusicApp />;
      case 'photos':
        return <PhotosApp />;
      case 'notes':
        return <NotesApp />;
      case 'software':
        return <SoftwareApp />;
      case 'clock':
        return <ClockApp />;
      case 'weather':
        return <WeatherApp />;
      case 'camera':
        return <CameraApp />;
      case 'calculator':
        return <CalculatorApp />;
      case 'text-editor':
        return <TextEditorApp params={params} />;
      case 'system-monitor':
        return <SystemMonitorApp />;
      case 'about':
        return <AboutApp />;
      default:
        return <div className="p-4 text-xs text-slate-400">Application not loaded.</div>;
    }
  };

  return (
    <div
      onClick={closeContextMenu}
      onContextMenu={handleContextMenu}
      className="relative h-screen w-screen overflow-hidden select-none bg-cover bg-center transition-all duration-700"
      style={{ background: wallpaper.gradient }}
    >
      {/* Screen Brightness Overlay (Physical Display Dimmer) */}
      <div
        id="screen-brightness-overlay"
        className="pointer-events-none fixed inset-0 z-[9999] transition-opacity duration-150"
        style={{
          backgroundColor: '#000000',
          opacity: Math.max(0, Math.min(0.85, ((100 - brightness) / 100) * 0.82)),
        }}
      />

      {/* Screen Night Light (Warm Color Temperature Overlay) */}
      <div
        id="screen-nightlight-overlay"
        className="pointer-events-none fixed inset-0 z-[9998] transition-opacity duration-300"
        style={{
          backgroundColor: '#ff9800',
          opacity: nightLight ? 0.2 : 0,
          mixBlendMode: 'multiply',
        }}
      />

      {/* Boot Splash Screen */}
      <BootSplashScreen />

      {/* iOS Lock Screen */}
      <LockScreen />

      {/* macOS / Debian App Launcher Menu (Triggered by Windows Key or Menu Bar) */}
      <AppMenu />

      {/* macOS Menu Bar */}
      <TopBar />

      {/* Control Center Dropdown */}
      {isQuickSettingsOpen && <QuickSettings />}

      {/* macOS Spotlight Search Modal */}
      <SpotlightSearch />

      {/* Power Off / Restart Modal */}
      <PowerModal />

      {/* Live Mode Welcome & Guided Installer Modal */}
      <LiveWelcomeModal />

      {/* Desktop Drive & Shortcuts (Top Right in true macOS fashion!) */}
      <div className="absolute top-10 right-4 flex flex-col items-center gap-5 z-10">
        {/* Axis System HD / Root Drive */}
        <button
          onDoubleClick={() => openApp('file-manager')}
          className="flex flex-col items-center gap-1 p-2 rounded-xl hover:bg-white/15 active:bg-blue-600/30 group cursor-pointer w-22 transition-all text-center"
        >
          <div className="w-13 h-13 rounded-2xl bg-white/10 backdrop-blur-md border border-white/20 flex items-center justify-center text-slate-100 shadow-xl group-hover:scale-105 transition-transform">
            <HardDrive className="w-7 h-7 text-cyan-300 drop-shadow" />
          </div>
          <span className="text-[11px] font-medium text-white drop-shadow-[0_1px_2px_rgba(0,0,0,0.8)] leading-tight">
            Axis System HD
          </span>
        </button>

        {/* Live Installer Shortcut */}
        {isLiveEnvironment && (
          <button
            onClick={() => openApp('installer')}
            onDoubleClick={() => openApp('installer')}
            className="flex flex-col items-center gap-1 p-2 rounded-xl hover:bg-white/15 active:bg-blue-600/30 group cursor-pointer w-22 transition-all text-center"
          >
            <div className="transition-transform group-hover:scale-105 drop-shadow-xl">
              <MacIcon id="installer" size={50} />
            </div>
            <span className="text-[11px] font-medium text-white drop-shadow-[0_1px_2px_rgba(0,0,0,0.8)] leading-tight">
              Install AxisOS
            </span>
          </button>
        )}
      </div>

      {/* Desktop Context Menu (Right Click) */}
      {contextMenu && (
        <div
          className="fixed z-50 w-52 bg-slate-900/90 backdrop-blur-3xl border border-white/15 rounded-xl p-1.5 shadow-2xl shadow-black text-xs text-slate-200 flex flex-col gap-0.5 animate-in fade-in duration-100"
          style={{ top: contextMenu.y, left: contextMenu.x }}
        >
          <button
            onClick={() => {
              openApp('file-manager');
              closeContextMenu();
            }}
            className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-blue-600 hover:text-white transition-colors text-left"
          >
            <Folder className="w-3.5 h-3.5 text-sky-400" />
            <span>New Folder</span>
          </button>
          <button
            onClick={() => {
              openApp('terminal');
              closeContextMenu();
            }}
            className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-blue-600 hover:text-white transition-colors text-left"
          >
            <TerminalIcon className="w-3.5 h-3.5 text-emerald-400" />
            <span>Open Terminal Here</span>
          </button>
          <div className="my-1 border-t border-white/10"></div>
          <button
            onClick={() => {
              openApp('settings', { tab: 'displays' });
              closeContextMenu();
            }}
            className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-blue-600 hover:text-white transition-colors text-left"
          >
            <Monitor className="w-3.5 h-3.5 text-cyan-400" />
            <span>Display Settings...</span>
          </button>
          <button
            onClick={() => {
              openApp('settings', { tab: 'wallpaper' });
              closeContextMenu();
            }}
            className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-blue-600 hover:text-white transition-colors text-left"
          >
            <Palette className="w-3.5 h-3.5 text-blue-400" />
            <span>Change Wallpaper...</span>
          </button>
          <button
            onClick={() => {
              openApp('settings', { tab: 'general' });
              closeContextMenu();
            }}
            className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-blue-600 hover:text-white transition-colors text-left"
          >
            <Settings className="w-3.5 h-3.5 text-slate-300" />
            <span>System Settings...</span>
          </button>
          <button
            onClick={() => {
              openApp('about');
              closeContextMenu();
            }}
            className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-blue-600 hover:text-white transition-colors text-left"
          >
            <Info className="w-3.5 h-3.5 text-slate-400" />
            <span>About This AxisPC</span>
          </button>
        </div>
      )}

      {/* Open Windows */}
      {windows.map((win) => (
        <WindowFrame key={win.id} window={win}>
          {renderAppContent(win.appId, win.params)}
        </WindowFrame>
      ))}

      {/* macOS Curved Glass Dock */}
      <DockOrTaskbar />
    </div>
  );
};
