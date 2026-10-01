import React, { useEffect } from 'react';
import { SystemStateProvider, useSystemState } from './context/SystemStateContext';
import { WindowManagerProvider, useWindowManager } from './context/WindowManagerContext';
import { InstallerProvider } from './context/InstallerContext';
import { Desktop } from './components/Desktop';

const ShellSession: React.FC = () => {
  const { openApp } = useWindowManager();
  const { isLiveEnvironment, autoInstall } = useSystemState();

  // Auto-launch the Installer ONLY when booted with explicit autoinstall parameter
  useEffect(() => {
    if (isLiveEnvironment && autoInstall) {
      const timer = setTimeout(() => {
        openApp('installer');
      }, 400);
      return () => clearTimeout(timer);
    }
  }, [isLiveEnvironment, autoInstall, openApp]);

  return <Desktop />;
};

export const App: React.FC = () => {
  return (
    <SystemStateProvider>
      <WindowManagerProvider>
        <InstallerProvider>
          <ShellSession />
        </InstallerProvider>
      </WindowManagerProvider>
    </SystemStateProvider>
  );
};

export default App;
