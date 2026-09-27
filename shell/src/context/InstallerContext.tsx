import React, { createContext, useContext, useState, useEffect } from 'react';
import { InstallerData } from '../types/os';
import { systemService } from '../services/systemService';

export interface DiskDrive {
  id: string;
  name: string;
  size: string;
  type: string;
  freeSpace: string;
}

export const INSTALL_STEPS = [
  'Welcome',
  'Language',
  'Storage',
  'User Account',
  'Summary',
  'Installing',
  'Complete',
] as const;

export type InstallStepIndex = 0 | 1 | 2 | 3 | 4 | 5 | 6;

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
  startInstallation: () => void;
  resetInstaller: () => void;
}

const defaultInstallerData: InstallerData = {
  language: 'English (United States)',
  keyboardLayout: 'English (US) - Standard',
  targetDisk: '/dev/nvme0n1',
  eraseDisk: true,
  userFullName: 'Jacky Mpoka',
  username: 'jackympoka',
  computerName: 'axis-macbook',
  password: '',
  autoLogin: true,
};

const defaultDisks: DiskDrive[] = [
  {
    id: '/dev/nvme0n1',
    name: 'Wodposit NVMe SSD (238.5 GB)',
    size: '238.5 GB',
    type: 'NVMe High-Speed Solid State Drive',
    freeSpace: '190.2 GB Available',
  },
  {
    id: '/dev/sda',
    name: 'Samsung SSD 750 EVO (120 GB)',
    size: '111.8 GB',
    type: 'SATA Solid State Drive',
    freeSpace: '92.4 GB Available',
  },
];

const InstallerContext = createContext<InstallerContextType | null>(null);

export const InstallerProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [currentStep, setCurrentStep] = useState<InstallStepIndex>(0);
  const [availableDisks, setAvailableDisks] = useState<DiskDrive[]>(defaultDisks);
  const [installerData, setInstallerData] = useState<InstallerData>(defaultInstallerData);
  const [isInstalling, setIsInstalling] = useState<boolean>(false);
  const [installProgress, setInstallProgress] = useState<number>(0);
  const [installStatusText, setInstallStatusText] = useState<string>('Preparing storage...');

  useEffect(() => {
    systemService.getSystemInfo().then((info) => {
      if (info.storageDevices && info.storageDevices.length > 0) {
        setAvailableDisks(info.storageDevices);
        setInstallerData((prev) => ({
          ...prev,
          targetDisk: info.storageDevices[0].id,
          username: info.username || prev.username,
        }));
      }
    });
  }, []);

  const updateInstallerData = (patch: Partial<InstallerData>) => {
    setInstallerData((prev) => ({ ...prev, ...patch }));
  };

  const goToNextStep = () => {
    setCurrentStep((prev) => Math.min(prev + 1, 6) as InstallStepIndex);
  };

  const goToPrevStep = () => {
    setCurrentStep((prev) => Math.max(prev - 1, 0) as InstallStepIndex);
  };

  const jumpToStep = (step: InstallStepIndex) => {
    setCurrentStep(step);
  };

  const resetInstaller = () => {
    setCurrentStep(0);
    setIsInstalling(false);
    setInstallProgress(0);
    setInstallStatusText('Preparing storage...');
  };

  const startInstallation = () => {
    setCurrentStep(5);
    setIsInstalling(true);
    setInstallProgress(0);
  };

  // Simulate installation steps when currentStep is 5 (Installing)
  useEffect(() => {
    if (!isInstalling || currentStep !== 5) return;

    const stages = [
      { progress: 10, text: `Partitioning ${installerData.targetDisk} (GPT, ESP, Btrfs)...` },
      { progress: 25, text: 'Formatting root subvolumes with zstd compression...' },
      { progress: 42, text: 'Unpacking Linux Kernel & base system packages...' },
      { progress: 60, text: 'Installing Wayland graphics stack and hardware drivers...' },
      { progress: 75, text: 'Deploying AxisOS Desktop Shell and macOS UI environment...' },
      { progress: 88, text: 'Configuring user account, hostname, and networking...' },
      { progress: 95, text: 'Generating GRUB bootloader EFI binaries...' },
      { progress: 100, text: 'AxisOS installation complete!' },
    ];

    let currentStageIndex = 0;

    const interval = setInterval(() => {
      currentStageIndex += 1;
      if (currentStageIndex < stages.length) {
        setInstallProgress(stages[currentStageIndex].progress);
        setInstallStatusText(stages[currentStageIndex].text);
      } else {
        clearInterval(interval);
        setIsInstalling(false);
        setCurrentStep(6);
      }
    }, 1800);

    return () => clearInterval(interval);
  }, [isInstalling, currentStep, installerData.targetDisk]);

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
        startInstallation,
        resetInstaller,
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
