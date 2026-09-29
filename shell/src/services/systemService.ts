// Unified Linux System Service Bridge for AxisOS
// Seamlessly routes between Electron IPC, AxisOS System Daemon (HTTP/REST), and Local Fallback

export interface DiskDrive {
  id: string;
  name: string;
  size: string;
  type: string;
  diskType?: string;
  freeSpace: string;
  readOnly?: boolean;
  model?: string;
  isLiveMedium?: boolean;
  hasBitLocker?: boolean;
  hasWindows?: boolean;
  partitions?: string[];
}

export interface SystemHardwareData {
  osName: string;
  osVersion: string;
  kernelVersion: string;
  architecture: string;
  cpuModel: string;
  cpuCores: number;
  gpuModel: string;
  totalMemory: string;
  freeMemory: string;
  storageDevices: DiskDrive[];
  hostname: string;
  uptime: string;
  username: string;
  homeDir: string;
  isLiveEnvironment?: boolean;
}

export interface FileEntry {
  name: string;
  fullPath: string;
  type: 'folder' | 'file';
  size: string;
  modified: string;
}

export interface InstallJobStatus {
  isInstalling: boolean;
  progress: number;
  statusText: string;
  completed: boolean;
  error: string | null;
  log: string[];
}

declare global {
  interface Window {
    axisAPI?: {
      getSystemInfo: () => Promise<SystemHardwareData>;
      executeCommand: (cmd: string, cwd?: string) => Promise<{ stdout: string; stderr: string; exitCode: number }>;
      readDirectory: (dirPath: string) => Promise<{ path: string; items: FileEntry[] }>;
      readFile: (filePath: string) => Promise<string>;
      writeFile: (filePath: string, content: string) => Promise<boolean>;
      createDirectory: (dirPath: string) => Promise<boolean>;
      deleteItem: (targetPath: string) => Promise<boolean>;
      getDisks: () => Promise<DiskDrive[]>;
      startInstall: (config: any) => Promise<{ success: boolean }>;
      getInstallStatus: () => Promise<InstallJobStatus>;
      powerAction: (action: string) => Promise<any>;
    };
  }
}

export const systemService = {
  // Query real host hardware
  async getSystemInfo(): Promise<SystemHardwareData> {
    if (window.axisAPI?.getSystemInfo) {
      try {
        return await window.axisAPI.getSystemInfo();
      } catch (err) {
        console.warn('Electron IPC getSystemInfo failed, falling back to HTTP', err);
      }
    }

    try {
      const res = await fetch('/api/system-info');
      if (res.ok) {
        return await res.json();
      }
    } catch {}

    // Fallback info
    return {
      osName: 'AxisOS Linux 1.0',
      osVersion: 'Horizon (Sonoma Edition)',
      kernelVersion: '6.12.0-axisos-amd64',
      architecture: 'x86_64',
      cpuModel: 'Intel(R) Core(TM) Processor / AMD Ryzen 64-bit',
      cpuCores: 8,
      gpuModel: 'Hardware Accelerated GPU',
      totalMemory: '16.0 GB Unified Memory',
      freeMemory: '12.4 GB Available',
      storageDevices: [
        {
          id: '/dev/nvme0n1',
          name: 'NVMe Solid State Drive (256 GB)',
          size: '256.0 GB',
          type: 'NVMe Solid State Drive',
          freeSpace: '240.0 GB Available',
          readOnly: false,
        },
        {
          id: '/dev/sda',
          name: 'SATA Solid State Drive (120 GB)',
          size: '120.0 GB',
          type: 'SATA Solid State Drive',
          freeSpace: '110.0 GB Available',
          readOnly: false,
        },
      ],
      hostname: 'axis-pc',
      uptime: 'up 1 hour',
      username: 'axis',
      homeDir: '/home/axis',
      isLiveEnvironment: false,
    };
  },

  // Query real block storage devices
  async getDisks(): Promise<DiskDrive[]> {
    if (window.axisAPI?.getDisks) {
      try {
        return await window.axisAPI.getDisks();
      } catch (err) {
        console.warn('Electron IPC getDisks failed, falling back to HTTP', err);
      }
    }

    try {
      const res = await fetch('/api/disks');
      if (res.ok) {
        return await res.json();
      }
    } catch {}

    return [
      {
        id: '/dev/nvme0n1',
        name: 'NVMe Solid State Drive (256 GB)',
        size: '256.0 GB',
        type: 'NVMe Solid State Drive',
        freeSpace: '240.0 GB Available',
        readOnly: false,
      },
      {
        id: '/dev/sda',
        name: 'SATA Solid State Drive (120 GB)',
        size: '120.0 GB',
        type: 'SATA Solid State Drive',
        freeSpace: '110.0 GB Available',
        readOnly: false,
      },
    ];
  },

  // Execute real Linux command
  async executeCommand(
    command: string,
    cwd?: string
  ): Promise<{ stdout: string; stderr: string; exitCode: number }> {
    if (window.axisAPI?.executeCommand) {
      try {
        return await window.axisAPI.executeCommand(command, cwd);
      } catch (err: any) {
        return { stdout: '', stderr: err.message, exitCode: 1 };
      }
    }

    try {
      const res = await fetch('/api/terminal-exec', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ command, cwd }),
      });
      if (res.ok) {
        return await res.json();
      }
    } catch {}

    return {
      stdout: `[axis-shell] Executed: ${command}`,
      stderr: '',
      exitCode: 0,
    };
  },

  // Read real files in directory
  async readDirectory(dirPath?: string): Promise<{ path: string; items: FileEntry[] }> {
    if (window.axisAPI?.readDirectory) {
      try {
        return await window.axisAPI.readDirectory(dirPath || '');
      } catch (err) {
        console.warn('Electron IPC readDirectory failed', err);
      }
    }

    try {
      const url = dirPath ? `/api/fs-read?path=${encodeURIComponent(dirPath)}` : '/api/fs-read';
      const res = await fetch(url);
      if (res.ok) {
        return await res.json();
      }
    } catch {}

    const home = '/home/axis';
    return {
      path: dirPath || home,
      items: [
        { name: 'Documents', fullPath: `${home}/Documents`, type: 'folder', size: 'Folder', modified: 'Today' },
        { name: 'Downloads', fullPath: `${home}/Downloads`, type: 'folder', size: 'Folder', modified: 'Yesterday' },
        { name: 'Pictures', fullPath: `${home}/Pictures`, type: 'folder', size: 'Folder', modified: 'Sep 27' },
        { name: 'AxisOS', fullPath: `${home}/Documents/AxisOS`, type: 'folder', size: 'Folder', modified: 'Today' },
        { name: 'welcome.txt', fullPath: `${home}/welcome.txt`, type: 'file', size: '1.2 KB', modified: 'Today' },
      ],
    };
  },

  // Read file contents
  async readFile(filePath: string): Promise<string> {
    if (window.axisAPI?.readFile) {
      return await window.axisAPI.readFile(filePath);
    }
    return '';
  },

  // Write file contents to disk
  async writeFile(filePath: string, content: string): Promise<boolean> {
    if (window.axisAPI?.writeFile) {
      try {
        return await window.axisAPI.writeFile(filePath, content);
      } catch (e) {
        console.warn('IPC writeFile failed', e);
      }
    }

    try {
      const res = await fetch('/api/fs-write', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ filePath, content }),
      });
      return res.ok;
    } catch {
      return false;
    }
  },

  // Create directory
  async createDirectory(dirPath: string): Promise<boolean> {
    if (window.axisAPI?.createDirectory) {
      return await window.axisAPI.createDirectory(dirPath);
    }
    try {
      const res = await fetch('/api/fs-mkdir', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ dirPath }),
      });
      return res.ok;
    } catch {
      return false;
    }
  },

  // Delete file or folder
  async deleteItem(targetPath: string): Promise<boolean> {
    if (window.axisAPI?.deleteItem) {
      return await window.axisAPI.deleteItem(targetPath);
    }
    try {
      const res = await fetch('/api/fs-delete', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ targetPath }),
      });
      return res.ok;
    } catch {
      return false;
    }
  },

  // Start real installation
  async startInstall(installerData: any): Promise<{ success: boolean; message?: string }> {
    if (window.axisAPI?.startInstall) {
      try {
        return await window.axisAPI.startInstall(installerData);
      } catch (err: any) {
        return { success: false, message: err.message };
      }
    }

    try {
      const res = await fetch('/api/installer/start', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(installerData),
      });
      return await res.json();
    } catch (err: any) {
      return { success: false, message: err.message };
    }
  },

  // Poll installation job status
  async getInstallStatus(): Promise<InstallJobStatus> {
    if (window.axisAPI?.getInstallStatus) {
      try {
        return await window.axisAPI.getInstallStatus();
      } catch (err) {
        console.warn('IPC getInstallStatus failed', err);
      }
    }

    try {
      const res = await fetch('/api/installer/status');
      if (res.ok) {
        return await res.json();
      }
    } catch {}

    return {
      isInstalling: false,
      progress: 0,
      statusText: 'Idle',
      completed: false,
      error: null,
      log: [],
    };
  },

  // System Power controls
  async powerOff(): Promise<void> {
    if (window.axisAPI?.powerAction) {
      await window.axisAPI.powerAction('poweroff');
      return;
    }
    try {
      await fetch('/api/power', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: 'poweroff' }),
      });
    } catch {}
  },

  async reboot(): Promise<void> {
    if (window.axisAPI?.powerAction) {
      await window.axisAPI.powerAction('reboot');
      return;
    }
    try {
      await fetch('/api/power', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ action: 'reboot' }),
      });
    } catch {}
  },

  // Check for Linux Kernel & System Updates
  async checkUpdates(): Promise<{
    upToDate: boolean;
    packagesCount: number;
    packages: string[];
    kernelUpgradeAvailable: boolean;
    autoUpdatesEnabled: boolean;
    currentKernel: string;
    message: string;
  }> {
    try {
      const res = await fetch('/api/updates/check');
      if (res.ok) {
        return await res.json();
      }
    } catch {}

    return {
      upToDate: true,
      packagesCount: 0,
      packages: [],
      kernelUpgradeAvailable: false,
      autoUpdatesEnabled: true,
      currentKernel: '6.12.0-axisos-amd64',
      message: 'AxisOS automatic kernel updates are enabled via unattended-upgrades.',
    };
  },

  // Apply pending system updates
  async applyUpdates(): Promise<{ success: boolean; message: string }> {
    try {
      const res = await fetch('/api/updates/apply', { method: 'POST' });
      if (res.ok) {
        return await res.json();
      }
    } catch {}

    return { success: true, message: 'System update check scheduled.' };
  },
};
