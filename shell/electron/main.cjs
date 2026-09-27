const { app, BrowserWindow, Menu, ipcMain } = require('electron');
const path = require('path');
const { exec, spawn } = require('child_process');
const fs = require('fs');
const os = require('os');

let mainWindow;

function runCmd(cmd, cwd) {
  return new Promise((resolve) => {
    exec(cmd, { cwd: cwd || os.homedir(), timeout: 10000, maxBuffer: 4 * 1024 * 1024 }, (err, stdout, stderr) => {
      resolve({
        stdout: stdout ? stdout.trim() : '',
        stderr: stderr ? stderr.trim() : (err ? err.message : ''),
        exitCode: err ? (err.code || 1) : 0,
      });
    });
  });
}

function createWindow() {
  Menu.setApplicationMenu(null);

  const isKiosk = process.argv.includes('--kiosk');

  mainWindow = new BrowserWindow({
    width: 1320,
    height: 840,
    minWidth: 1024,
    minHeight: 640,
    title: 'AxisOS Horizon',
    titleBarStyle: 'hiddenInset',
    backgroundColor: '#020617',
    kiosk: isKiosk,
    autoHideMenuBar: true,
    webPreferences: {
      preload: path.join(__dirname, 'preload.cjs'),
      nodeIntegration: false,
      contextIsolation: true,
      sandbox: false,
    },
  });

  const devUrl = process.env.VITE_DEV_SERVER_URL;
  if (devUrl) {
    mainWindow.loadURL(devUrl);
  } else {
    mainWindow.loadFile(path.join(__dirname, '../dist/index.html'));
  }

  mainWindow.on('closed', () => {
    mainWindow = null;
  });
}

// Active installer job tracking
let installJob = {
  isInstalling: false,
  progress: 0,
  statusText: 'Idle',
  completed: false,
  error: null,
  log: [],
};

// Probe real disk drives
async function getStorageDisks() {
  const res = await runCmd('lsblk -d -J -b -o NAME,SIZE,TYPE,MODEL,TRAN,RO 2>/dev/null');
  if (res.stdout) {
    try {
      const parsed = JSON.parse(res.stdout);
      if (parsed.blockdevices && Array.isArray(parsed.blockdevices)) {
        const disks = parsed.blockdevices
          .filter((d) => d.type === 'disk' && !d.name.startsWith('loop') && !d.name.startsWith('zram'))
          .map((d) => {
            const bytes = parseInt(d.size, 10) || 0;
            const gb = (bytes / (1000 * 1000 * 1000)).toFixed(1);
            let driveType = 'Solid State Drive';
            if (d.name.startsWith('nvme')) driveType = 'NVMe Solid State Drive';
            else if (d.tran === 'usb') driveType = 'USB Removable Drive';
            else if (d.tran === 'sata') driveType = 'SATA Solid State Drive';

            return {
              id: `/dev/${d.name}`,
              name: d.model ? `${d.model} (${gb} GB)` : `${d.name.toUpperCase()} (${gb} GB)`,
              size: `${gb} GB`,
              type: driveType,
              freeSpace: `${gb} GB Available`,
              readOnly: d.ro === true || d.ro === '1',
            };
          });

        if (disks.length > 0) return disks;
      }
    } catch {}
  }

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
}

// IPC Handlers for real Linux hardware & operations
ipcMain.handle('system:getInfo', async () => {
  const [uname, cpuModel, meminfo, lspci, hostname, uptime] = await Promise.all([
    runCmd('uname -r'),
    runCmd('lscpu 2>/dev/null | grep "Model name:" | head -n1 | cut -d: -f2 | xargs'),
    runCmd('cat /proc/meminfo 2>/dev/null'),
    runCmd('lspci 2>/dev/null | grep -E "VGA|3D|Display" | head -n1 | cut -d: -f3 | xargs'),
    runCmd('hostname'),
    runCmd('uptime -p 2>/dev/null || uptime'),
  ]);

  let totalMemGb = (os.totalmem() / (1024 * 1024 * 1024)).toFixed(1);
  let freeMemGb = (os.freemem() / (1024 * 1024 * 1024)).toFixed(1);
  if (meminfo.stdout) {
    const memMatch = meminfo.stdout.match(/MemTotal:\s+(\d+)\s+kB/);
    if (memMatch) {
      totalMemGb = (parseInt(memMatch[1], 10) / 1024 / 1024).toFixed(1);
    }
  }

  const disks = await getStorageDisks();

  return {
    osName: 'AxisOS Linux 1.0',
    osVersion: 'Horizon (Sonoma Edition)',
    kernelVersion: uname.stdout || os.release(),
    architecture: os.arch(),
    cpuModel: cpuModel.stdout || os.cpus()[0]?.model || '64-bit Processor',
    cpuCores: os.cpus().length,
    gpuModel: lspci.stdout || 'Intel Integrated Graphics',
    totalMemory: `${totalMemGb} GB Unified Memory`,
    freeMemory: `${freeMemGb} GB Available`,
    storageDevices: disks,
    hostname: hostname.stdout || os.hostname(),
    uptime: uptime.stdout || 'up 1 hour',
    username: process.env.USER || os.userInfo().username || 'axis',
    homeDir: process.env.HOME || os.homedir(),
  };
});

ipcMain.handle('system:exec', async (event, { cmd, cwd }) => {
  return runCmd(cmd, cwd);
});

ipcMain.handle('system:power', async (event, action) => {
  if (action === 'poweroff') {
    return runCmd('systemctl poweroff || shutdown -h now');
  } else if (action === 'reboot') {
    return runCmd('systemctl reboot || reboot');
  }
  return { stdout: 'Lock screen', stderr: '', exitCode: 0 };
});

ipcMain.handle('installer:getDisks', async () => {
  return getStorageDisks();
});

ipcMain.handle('installer:start', async (event, config) => {
  if (installJob.isInstalling) {
    throw new Error('Installation already in progress');
  }

  installJob = {
    isInstalling: true,
    progress: 2,
    statusText: 'Initializing installation engine...',
    completed: false,
    error: null,
    log: [`[${new Date().toLocaleTimeString()}] Installation started for ${config.targetDisk}`],
  };

  const scriptCandidates = [
    '/usr/local/bin/axisos-installer.sh',
    path.join(__dirname, '../../os-build/scripts/axisos-install.sh'),
    path.join(__dirname, '../os-build/scripts/axisos-install.sh'),
    path.join(process.cwd(), 'os-build/scripts/axisos-install.sh'),
  ];
  const scriptPath = scriptCandidates.find((s) => fs.existsSync(s));

  if (!scriptPath) {
    throw new Error('Installer script axisos-install.sh not found');
  }

  const args = [
    scriptPath,
    '--disk', config.targetDisk,
    '--username', config.username || 'axis',
    '--password', config.password || 'password',
    '--fullname', config.userFullName || 'AxisOS User',
    '--hostname', config.computerName || 'axis-pc',
    '--autologin', config.autoLogin ? 'true' : 'false',
    '--fs', 'btrfs',
  ];

  const isLinux = process.platform === 'linux';
  const isRoot = process.getuid ? process.getuid() === 0 : false;
  if (!isLinux || !isRoot) {
    args.push('--dry-run');
    installJob.log.push(`[Notice] Running in verified simulation mode (${isLinux ? 'non-root' : process.platform})`);
  }

  const child = spawn('bash', args);

  child.stdout.on('data', (chunk) => {
    const lines = chunk.toString().split('\n');
    for (const line of lines) {
      const trimmed = line.trim();
      if (!trimmed) continue;
      installJob.log.push(trimmed);

      const match = trimmed.match(/^PROGRESS:(\d+):(.*)$/);
      if (match) {
        installJob.progress = parseInt(match[1], 10);
        installJob.statusText = match[2];
      }
    }
  });

  child.stderr.on('data', (chunk) => {
    const trimmed = chunk.toString().trim();
    if (trimmed) installJob.log.push(`[err] ${trimmed}`);
  });

  child.on('close', (code) => {
    installJob.isInstalling = false;
    if (code === 0) {
      installJob.progress = 100;
      installJob.statusText = 'Installation completed successfully!';
      installJob.completed = true;
    } else {
      installJob.error = `Installer exited with code ${code}`;
      installJob.statusText = `Failed: ${installJob.error}`;
    }
  });

  return { success: true };
});

ipcMain.handle('installer:status', async () => {
  return installJob;
});

ipcMain.handle('fs:readDir', async (event, dirPath) => {
  const target = dirPath || os.homedir();
  if (!fs.existsSync(target)) return { error: 'Path not found', items: [] };

  const entries = fs.readdirSync(target, { withFileTypes: true });
  const items = entries.map((entry) => {
    const full = path.join(target, entry.name);
    let size = '—';
    let mtime = '—';
    try {
      const stat = fs.statSync(full);
      if (!entry.isDirectory()) {
        const b = stat.size;
        if (b < 1024) size = `${b} B`;
        else if (b < 1024 * 1024) size = `${(b / 1024).toFixed(1)} KB`;
        else size = `${(b / (1024 * 1024)).toFixed(1)} MB`;
      } else {
        size = 'Folder';
      }
      mtime = new Date(stat.mtime).toLocaleDateString([], {
        month: 'short',
        day: 'numeric',
        hour: '2-digit',
        minute: '2-digit',
      });
    } catch {}

    return {
      name: entry.name,
      fullPath: full,
      type: entry.isDirectory() ? 'folder' : 'file',
      size,
      modified: mtime,
    };
  });

  return { path: target, items };
});

ipcMain.handle('fs:readFile', async (event, filePath) => {
  if (!fs.existsSync(filePath)) throw new Error('File not found');
  return fs.readFileSync(filePath, 'utf-8');
});

ipcMain.handle('fs:writeFile', async (event, { filePath, content }) => {
  fs.mkdirSync(path.dirname(filePath), { recursive: true });
  fs.writeFileSync(filePath, content || '', 'utf-8');
  return true;
});

ipcMain.handle('fs:createDir', async (event, dirPath) => {
  fs.mkdirSync(dirPath, { recursive: true });
  return true;
});

ipcMain.handle('fs:deleteFile', async (event, targetPath) => {
  fs.rmSync(targetPath, { recursive: true, force: true });
  return true;
});

app.whenReady().then(() => {
  createWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on('window-all-closed', () => {
  if (process.platform !== 'darwin') app.quit();
});
