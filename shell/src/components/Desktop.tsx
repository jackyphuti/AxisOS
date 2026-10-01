import React, { useState } from 'react';
import { Terminal as TerminalIcon, Folder, Settings, Palette, Info, Monitor } from 'lucide-react';
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

const APP_CONTENT_RENDERERS: Partial<Record<AppId, (params?: Record<string, any>) => React.ReactNode>> = {
  installer: () => <InstallerApp />,
  settings: (params) => <SettingsApp params={params} />,
  terminal: (params) => <TerminalApp params={params} />,
  'file-manager': (params) => <FileManagerApp params={params} />,
  browser: () => <BrowserApp />,
  music: () => <MusicApp />,
  photos: () => <PhotosApp />,
  notes: () => <NotesApp />,
  software: () => <SoftwareApp />,
  clock: () => <ClockApp />,
  weather: () => <WeatherApp />,
  camera: () => <CameraApp />,
  calculator: () => <CalculatorApp />,
  'text-editor': (params) => <TextEditorApp params={params} />,
  'system-monitor': () => <SystemMonitorApp />,
  about: () => <AboutApp />,
};

const DesktopShortcut: React.FC<{ label: string; emoji?: string; onOpen: () => void; icon?: React.ReactNode }> = ({
  label,
  emoji,
  onOpen,
  icon,
}) => (
  <button
    onClick={onOpen}
    onDoubleClick={onOpen}
    className="flex flex-col items-center gap-1 p-2 rounded-xl hover:bg-white/15 active:bg-blue-600/30 group cursor-pointer w-22 transition-all text-center"
  >
    <div className="w-13 h-13 rounded-2xl flex items-center justify-center transition-transform group-hover:scale-105 drop-shadow-lg">
      {icon ?? <span className="text-4xl">{emoji ?? '💾'}</span>}
    </div>
    <span className="text-[11px] font-medium text-white drop-shadow-[0_1px_2px_rgba(0,0,0,0.8)] leading-tight">
      {label}
    </span>
  </button>
);

const DesktopContextMenu: React.FC<{
  contextMenu: { x: number; y: number } | null;
  openApp: (appId: AppId, params?: Record<string, any>) => void;
  closeContextMenu: () => void;
}> = ({ contextMenu, openApp, closeContextMenu }) => {
  if (!contextMenu) return null;

  return (
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
  );
};

export const Desktop: React.FC = () => {
  const { wallpaper, isQuickSettingsOpen, isLiveEnvironment } = useSystemState();
  const { windows, openApp } = useWindowManager();
  const [contextMenu, setContextMenu] = useState<{ x: number; y: number } | null>(null);

  const handleContextMenu = (e: React.MouseEvent) => {
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
    const renderer = APP_CONTENT_RENDERERS[appId];
    if (renderer) return renderer(params);

    return <div className="p-4 text-xs text-slate-400">Application not loaded.</div>;
  };

  return (
    <div
      onClick={closeContextMenu}
      onContextMenu={handleContextMenu}
      className="relative h-screen w-screen overflow-hidden select-none bg-cover bg-center transition-all duration-700"
      style={{ background: wallpaper.gradient }}
    >
      <TopBar />

      {isQuickSettingsOpen && <QuickSettings />}
      <SpotlightSearch />
      <PowerModal />
      <LiveWelcomeModal />

      <div className="absolute top-10 right-4 flex flex-col items-center gap-5 z-10">
        <DesktopShortcut label="Macintosh HD" emoji="💾" onOpen={() => openApp('file-manager')} />

        {isLiveEnvironment && (
          <DesktopShortcut
            label="Install AxisOS"
            onOpen={() => openApp('installer')}
            icon={<MacIcon id="installer" size={50} />}
          />
        )}
      </div>

      <DesktopContextMenu contextMenu={contextMenu} openApp={openApp} closeContextMenu={closeContextMenu} />

      {windows.map((win) => (
        <WindowFrame key={win.id} window={win}>
          {renderAppContent(win.appId, win.params)}
        </WindowFrame>
      ))}

      <DockOrTaskbar />
    </div>
  );
};
