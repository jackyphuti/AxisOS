const { app, BrowserWindow, Menu, ipcMain } = require('electron');
const path = require('path');
const { exec } = require('child_process');
const fs = require('fs');
const os = require('os');

let mainWindow;

function runCmd(cmd) {
  return new Promise((resolve) => {
    exec(cmd, { timeout: 3000 }, (err, stdout) => {
      resolve(err ? '' : stdout.trim());
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

// IPC Handlers for real Linux hardware & operations
ipcMain.handle('system:getInfo', async () => {
  const [uname, cpuModel, meminfo, lsblk, lspci, hostname, uptime] = await Promise.all([
    runCmd('uname -r'),
    runCmd('lscpu | grep "Model name:" | head -n1 | cut -d: -f2 | xargs'),
    runCmd('cat /proc/meminfo'),
    runCmd('lsblk -d -J -o NAME,SIZE,TYPE,MODEL 2>/dev/null || lsblk -d -o NAME,SIZE,TYPE,MODEL'),
    runCmd('lspci | grep -E "VGA|3D|Display" | head -n1 | cut -d: -f3 | xargs'),
    runCmd('hostname'),
    runCmd('uptime -p 2>/dev/null || uptime'),
  ]);

  let totalMemGb = (os.totalmem() / (1024 * 1024 * 1024)).toFixed(1);
  let freeMemGb = (os.freemem() / (1024 * 1024 * 1024)).toFixed(1);
  const memMatch = meminfo.match(/MemTotal:\s+(\d+)\s+kB/);
  if (memMatch) {
    totalMemGb = (parseInt(memMatch[1], 10) / 1024 / 1024).toFixed(1);
  }

  let disks = [];
  try {
    const parsed = JSON.parse(lsblk);
    if (parsed.blockdevices) {
      disks = parsed.blockdevices
        .filter((d) => d.type === 'disk')
        .map((d) => ({
          id: `/dev/${d.name}`,
          name: d.model ? `${d.model} (${d.size})` : `${d.name} (${d.size})`,
          size: d.size,
          type: d.name.startsWith('nvme') ? 'NVMe Solid State Drive' : 'SATA Solid State Drive',
          freeSpace: `${d.size} Available`,
        }));
    }
  } catch {
    disks = [
      { id: '/dev/nvme0n1', name: 'NVMe SSD 256GB', size: '238.5G', type: 'NVMe Solid State Drive', freeSpace: '200G Available' },
      { id: '/dev/sda', name: 'Samsung SSD 120GB', size: '111.8G', type: 'SATA Solid State Drive', freeSpace: '90G Available' },
    ];
  }

  return {
    osName: 'AxisOS Linux 1.0',
    osVersion: 'Horizon (Sonoma Edition)',
    kernelVersion: uname || os.release(),
    architecture: os.arch(),
    cpuModel: cpuModel || os.cpus()[0]?.model || 'Intel 64-bit Processor',
    cpuCores: os.cpus().length,
    gpuModel: lspci || 'Intel Integrated Graphics',
    totalMemory: `${totalMemGb} GB Unified Memory`,
    freeMemory: `${freeMemGb} GB`,
    storageDevices: disks,
    hostname: hostname || os.hostname(),
    uptime: uptime || 'up 1 hour',
    username: os.userInfo().username || 'axis',
    homeDir: os.homedir(),
  };
});

ipcMain.handle('system:exec', async (event, { cmd, cwd }) => {
  return new Promise((resolve) => {
    exec(cmd, { cwd: cwd || os.homedir(), timeout: 10000, maxBuffer: 1024 * 1024 }, (err, stdout, stderr) => {
      resolve({
        stdout: stdout || '',
        stderr: stderr || (err ? err.message : ''),
        exitCode: err ? (err.code || 1) : 0,
      });
    });
  });
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
  fs.writeFileSync(filePath, content, 'utf-8');
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
