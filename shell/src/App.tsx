import React, { useEffect } from 'react';
import { SystemStateProvider, useSystemState } from './context/SystemStateContext';
import { WindowManagerProvider, useWindowManager } from './context/WindowManagerContext';
import { InstallerProvider } from './context/InstallerContext';
import { Desktop } from './components/Desktop';

const ShellSession: React.FC = () => {
  const { openApp } = useWindowManager();
  const { isLiveEnvironment } = useSystemState();

  // Auto-launch the Installer on initial boot of live session
  useEffect(() => {
    if (isLiveEnvironment) {
      const timer = setTimeout(() => {
        openApp('installer');
      }, 400);
      return () => clearTimeout(timer);
    }
  }, [isLiveEnvironment, openApp]);

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
