import React, { createContext, useContext, useState, useEffect, useRef } from 'react';
import { InstallerData } from '../types/os';
import { systemService, DiskDrive } from '../services/systemService';

export const INSTALL_STEPS = [
  'Language & Region',
  'Destination Drive',
  'User Account',
  'Installing',
  'Complete',
] as const;

export type InstallStepIndex = 0 | 1 | 2 | 3 | 4;

interface InstallerContextType {
  currentStep: InstallStepIndex;
  goToNextStep: () => void;
  goToPrevStep: () => void;
  jumpToStep: (step: InstallStepIndex) => void;
  installerData: InstallerData;
  updateInstallerData: (patch: Partial<InstallerData>) => void;
  availableDisks: DiskDrive[];
  isInstalling: boolean;
  installProgress: number;
  installStatusText: string;
  installLogs: string[];
  installError: string | null;
  startInstallation: () => void;
  resetInstaller: () => void;
  refreshDisks: () => void;
}

const defaultInstallerData: InstallerData = {
  language: 'English (United States)',
  location: 'United States',
  keyboardLayout: 'English (US) - Standard',
  targetDisk: '',
  eraseDisk: true,
  userFullName: 'AxisOS User',
  username: 'axis',
  computerName: 'axis-pc',
  password: '',
  autoLogin: true,
  locale: 'en_US.UTF-8',
  timezone: 'UTC',
  keymap: 'us',
};

const InstallerContext = createContext<InstallerContextType | null>(null);

export const InstallerProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [currentStep, setCurrentStep] = useState<InstallStepIndex>(0);
  const [availableDisks, setAvailableDisks] = useState<DiskDrive[]>([]);
  const [installerData, setInstallerData] = useState<InstallerData>(defaultInstallerData);
  const [isInstalling, setIsInstalling] = useState<boolean>(false);
  const [installProgress, setInstallProgress] = useState<number>(0);
  const [installStatusText, setInstallStatusText] = useState<string>('Preparing storage...');
  const [installLogs, setInstallLogs] = useState<string[]>([]);
  const [installError, setInstallError] = useState<string | null>(null);

  const pollIntervalRef = useRef<any>(null);

  const refreshDisks = async () => {
    try {
      const disks = await systemService.getDisks();
      if (disks && disks.length > 0) {
        setAvailableDisks(disks);
        setInstallerData((prev) => ({
          ...prev,
          targetDisk: disks[0].id,
        }));
      }
    } catch {}
  };

  useEffect(() => {
    refreshDisks();
    systemService.getSystemInfo().then((info) => {
      if (info.username && info.username !== 'axis') {
        setInstallerData((prev) => ({
          ...prev,
          username: info.username,
        }));
      }
    });
  }, []);

  const updateInstallerData = (patch: Partial<InstallerData>) => {
    setInstallerData((prev) => ({ ...prev, ...patch }));
  };

  const goToNextStep = () => {
    setCurrentStep((prev) => Math.min(prev + 1, 4) as InstallStepIndex);
  };

  const goToPrevStep = () => {
    setCurrentStep((prev) => Math.max(prev - 1, 0) as InstallStepIndex);
  };

  const jumpToStep = (step: InstallStepIndex) => {
    setCurrentStep(step);
  };

  const resetInstaller = () => {
    if (pollIntervalRef.current) clearInterval(pollIntervalRef.current);
    setCurrentStep(0);
    setIsInstalling(false);
    setInstallProgress(0);
    setInstallStatusText('Preparing storage...');
    setInstallLogs([]);
    setInstallError(null);
  };

  const startInstallation = async () => {
    setCurrentStep(3);
    setIsInstalling(true);
    setInstallProgress(2);
    setInstallStatusText('Connecting to installation engine...');
    setInstallLogs([`Initializing deployment on target device ${installerData.targetDisk}...`]);
    setInstallError(null);

    try {
      const res = await systemService.startInstall(installerData);
      if (!res.success && res.message) {
        setInstallError(res.message);
      }
    } catch (e: any) {
      setInstallError(e.message);
    }

    // Poll real engine progress
    if (pollIntervalRef.current) clearInterval(pollIntervalRef.current);
    pollIntervalRef.current = setInterval(async () => {
      try {
        const status = await systemService.getInstallStatus();
        if (status.log && status.log.length > 0) {
          setInstallLogs(status.log);
        }
        if (status.statusText) {
          setInstallStatusText(status.statusText);
        }
        if (status.progress !== undefined) {
          setInstallProgress(status.progress);
        }

        if (status.completed) {
          clearInterval(pollIntervalRef.current);
          setIsInstalling(false);
          setInstallProgress(100);
          setInstallStatusText('Installation complete!');
          setCurrentStep(4);
        } else if (status.error) {
          clearInterval(pollIntervalRef.current);
          setIsInstalling(false);
          setInstallError(status.error);
        }
      } catch (err) {
        // Continue polling
      }
    }, 800);
  };

  useEffect(() => {
    return () => {
      if (pollIntervalRef.current) clearInterval(pollIntervalRef.current);
    };
  }, []);

  return (
    <InstallerContext.Provider
      value={{
        currentStep,
        goToNextStep,
        goToPrevStep,
        jumpToStep,
        installerData,
        updateInstallerData,
        availableDisks,
        isInstalling,
        installProgress,
        installStatusText,
        installLogs,
        installError,
        startInstallation,
        resetInstaller,
        refreshDisks,
      }}
    >
      {children}
    </InstallerContext.Provider>
  );
};

export const useInstaller = () => {
  const ctx = useContext(InstallerContext);
  if (!ctx) throw new Error('useInstaller must be used within InstallerProvider');
  return ctx;
};
