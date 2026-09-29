#!/usr/bin/env node
// ==============================================================================
// AxisOS Linux - System Management Daemon & Shell Web Server
// Serves the desktop shell SPA and provides system management REST API
// ==============================================================================

const http = require('http');
const fs = require('fs');
const path = require('path');
const url = require('url');
const os = require('os');
const { exec, spawn } = require('child_process');

const PORT = parseInt(process.env.PORT || '3000', 10);
const HOST = process.env.HOST || '0.0.0.0';

// Resolve distribution root
const DIST_CANDIDATES = [
  '/opt/axisos-shell/dist',
  path.join(__dirname, '../shell/dist'),
  path.join(__dirname, 'dist'),
  path.join(process.cwd(), 'dist'),
  path.join(process.cwd(), 'shell/dist'),
];

let DIST_DIR = DIST_CANDIDATES.find((d) => fs.existsSync(d)) || '/opt/axisos-shell/dist';

const MIME_TYPES = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'application/javascript; charset=utf-8',
  '.mjs': 'application/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.png': 'image/png',
  '.jpg': 'image/jpeg',
  '.jpeg': 'image/jpeg',
  '.gif': 'image/gif',
  '.svg': 'image/svg+xml',
  '.ico': 'image/x-icon',
  '.webp': 'image/webp',
  '.woff': 'font/woff',
  '.woff2': 'font/woff2',
  '.ttf': 'font/ttf',
  '.eot': 'application/vnd.ms-fontobject',
  '.wasm': 'application/wasm',
};

// Installer job state tracker
let installJob = {
  isInstalling: false,
  progress: 0,
  statusText: 'Ready',
  completed: false,
  error: null,
  log: [],
};

// Helper: Run shell command asynchronously
function runCmd(cmd, cwd) {
  return new Promise((resolve) => {
    exec(cmd, { cwd: cwd || os.homedir(), timeout: 15000, maxBuffer: 8 * 1024 * 1024, shell: '/bin/bash' }, (err, stdout, stderr) => {
      resolve({
        stdout: stdout ? stdout.trim() : '',
        stderr: stderr ? stderr.trim() : (err ? err.message : ''),
        exitCode: err ? (err.code || 1) : 0,
      });
    });
  });
}

// Parse request body JSON
function parseJsonBody(req) {
  return new Promise((resolve, reject) => {
    let data = '';
    req.on('data', (chunk) => {
      data += chunk;
      if (data.length > 5 * 1024 * 1024) {
        req.destroy();
        reject(new Error('Payload too large'));
      }
    });
    req.on('end', () => {
      try {
        resolve(data ? JSON.parse(data) : {});
      } catch (err) {
        resolve({});
      }
    });
    req.on('error', reject);
  });
}

// Send JSON response
function sendJson(res, statusCode, data) {
  res.writeHead(statusCode, {
    'Content-Type': 'application/json; charset=utf-8',
    'Access-Control-Allow-Origin': '*',
    'Access-Control-Allow-Methods': 'GET, POST, OPTIONS',
    'Access-Control-Allow-Headers': 'Content-Type',
    'Cache-Control': 'no-cache',
  });
  res.end(JSON.stringify(data));
}

// Detect if running in a Live boot session (USB) or installed disk
function isLiveSession() {
  if (process.platform !== 'linux') return false;
  // 1. Explicit marker file created during installation
  if (fs.existsSync('/etc/axisos-installed')) return false;
  // 2. Live environment marker directory
  if (fs.existsSync('/run/live')) return true;
  // 3. Kernel cmdline check
  try {
    const cmdline = fs.readFileSync('/proc/cmdline', 'utf-8');
    if (cmdline.includes('boot=live')) return true;
  } catch {}
  // 4. Mount overlay check
  try {
    const mounts = fs.readFileSync('/proc/mounts', 'utf-8');
    for (const line of mounts.split('\n')) {
      const parts = line.split(' ');
      if (parts[1] === '/' && (parts[2] === 'overlay' || parts[2] === 'aufs' || parts[2] === 'iso9660')) {
        return true;
      }
    }
  } catch {}
  return false;
}

// Real disk detection via lsblk with BitLocker, Windows, and Live Medium flags
async function getStorageDisks() {
  if (process.platform === 'linux') {
    const res = await runCmd('lsblk -J -b -o NAME,SIZE,TYPE,MOUNTPOINT,MODEL,TRAN,RO,FSTYPE,LABEL,ROTA 2>/dev/null');
    try {
      const parsed = JSON.parse(res.stdout);
      if (parsed.blockdevices && Array.isArray(parsed.blockdevices)) {
        const disks = parsed.blockdevices
          .filter((d) => d.type === 'disk' && !d.name.startsWith('loop') && !d.name.startsWith('zram'))
          .map((d) => {
            const bytes = parseInt(d.size, 10) || 0;
            const gb = (bytes / (1000 * 1000 * 1000)).toFixed(1);

            let hasBitLocker = false;
            let hasWindows = false;
            let isLiveMedium = false;
            const partitions = [];

            const inspect = (node) => {
              const fs = (node.fstype || '').toLowerCase();
              const lbl = (node.label || '').toLowerCase();
              const mp = (node.mountpoint || '').toLowerCase();

              if (fs.includes('bitlocker')) hasBitLocker = true;
              if (fs.includes('ntfs') || lbl.includes('windows') || lbl.includes('recovery')) hasWindows = true;
              if (mp.includes('live') || mp.includes('medium') || mp === '/run/live/medium' || mp === '/lib/live/mount/medium') {
                isLiveMedium = true;
              }

              if (node.name && node.size) {
                const partGb = (parseInt(node.size, 10) / (1000 * 1000 * 1000)).toFixed(1);
                partitions.push(`${node.name} (${partGb} GB, ${node.fstype || 'raw'})`);
              }

              if (node.children && Array.isArray(node.children)) {
                node.children.forEach(inspect);
              }
            };

            inspect(d);

            let driveType = 'Solid State Drive (SSD)';
            if (d.name.startsWith('nvme')) driveType = 'NVMe Solid State Drive';
            else if (d.tran === 'usb') driveType = isLiveMedium ? 'Live USB Installer Drive' : 'External USB Drive';
            else if (d.rota === true || d.rota === '1' || d.rota === 1) driveType = 'Traditional Hard Disk Drive (HDD)';
            else if (d.name.startsWith('vd')) driveType = 'VirtIO Virtual Disk';
            else if (d.name.startsWith('sd')) driveType = 'SATA Storage Disk';

            const cleanModel = (d.model || '').trim();
            const displayName = cleanModel ? `${cleanModel} (${gb} GB)` : `${d.name.toUpperCase()} (${gb} GB)`;

            return {
              id: `/dev/${d.name}`,
              name: displayName,
              model: cleanModel || d.name.toUpperCase(),
              size: `${gb} GB`,
              bytes,
              type: driveType,
              freeSpace: `${gb} GB Total`,
              readOnly: d.ro === true || d.ro === '1',
              isLiveMedium,
              hasBitLocker,
              hasWindows,
              partitions,
            };
          });

        if (disks.length > 0) return disks;
      }
    } catch {}
  }

  return [
    {
      id: '/dev/sda',
      name: 'Toshiba MQ01ABD100 (1000.2 GB)',
      model: 'Toshiba MQ01ABD100',
      size: '931.5 GB',
      type: 'Traditional Hard Disk Drive (HDD)',
      freeSpace: '931.5 GB Total',
      readOnly: false,
      isLiveMedium: false,
      hasBitLocker: false,
      hasWindows: false,
      partitions: ['sda1 (0.5 GB, vfat)', 'sda2 (4.0 GB, swap)', 'sda3 (927.0 GB, btrfs)'],
    },
    {
      id: '/dev/vda',
      name: 'VirtIO Virtual Disk (20 GB)',
      model: 'VirtIO Virtual Disk',
      size: '20.0 GB',
      type: 'VirtIO Virtual Disk',
      freeSpace: '19.5 GB Total',
      readOnly: false,
      isLiveMedium: false,
      hasBitLocker: false,
      hasWindows: false,
      partitions: [],
    },
  ];
}

// Static file serving handler
function serveStaticFile(req, res, filePath) {
  fs.stat(filePath, (err, stats) => {
    if (err || !stats.isFile()) {
      // Fallback to index.html for client-side SPA routing
      const indexPath = path.join(DIST_DIR, 'index.html');
      fs.readFile(indexPath, (readErr, content) => {
        if (readErr) {
          res.writeHead(404, { 'Content-Type': 'text/plain' });
          res.end('AxisOS Shell: 404 Not Found');
          return;
        }
        res.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8' });
        res.end(content);
      });
      return;
    }

    const ext = path.extname(filePath).toLowerCase();
    const contentType = MIME_TYPES[ext] || 'application/octet-stream';
    res.writeHead(200, {
      'Content-Type': contentType,
      'Content-Length': stats.size,
      'Cache-Control': ext === '.html' ? 'no-cache' : 'max-age=86400',
    });

    const stream = fs.createReadStream(filePath);
    stream.pipe(res);
  });
}

// Create HTTP Server
const server = http.createServer(async (req, res) => {
  const parsedUrl = url.parse(req.url, true);
  const pathname = parsedUrl.pathname || '/';

  // Handle CORS preflight
  if (req.method === 'OPTIONS') {
    res.writeHead(204, {
      'Access-Control-Allow-Origin': '*',
      'Access-Control-Allow-Methods': 'GET, POST, OPTIONS',
      'Access-Control-Allow-Headers': 'Content-Type',
    });
    res.end();
    return;
  }

  // ==================== REST API ENDPOINTS ====================
  if (pathname.startsWith('/api/')) {
    // 1. System Info
    if (pathname === '/api/system-info' && req.method === 'GET') {
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
        const freeMatch = meminfo.stdout.match(/MemAvailable:\s+(\d+)\s+kB/);
        if (memMatch) totalMemGb = (parseInt(memMatch[1], 10) / 1024 / 1024).toFixed(1);
        if (freeMatch) freeMemGb = (parseInt(freeMatch[1], 10) / 1024 / 1024).toFixed(1);
      }

      const disks = await getStorageDisks();

      return sendJson(res, 200, {
        osName: 'AxisOS Linux 1.0',
        osVersion: 'Horizon (Sonoma Edition)',
        kernelVersion: uname.stdout || os.release(),
        architecture: os.arch(),
        cpuModel: cpuModel.stdout || os.cpus()[0]?.model || '64-bit Processor',
        cpuCores: os.cpus().length,
        gpuModel: lspci.stdout || 'VirtIO GPU / Mesa Hardware Acceleration',
        totalMemory: `${totalMemGb} GB Unified Memory`,
        freeMemory: `${freeMemGb} GB Available`,
        storageDevices: disks,
        hostname: hostname.stdout || os.hostname(),
        uptime: uptime.stdout || 'up recently',
        username: process.env.USER || 'axis',
        homeDir: process.env.HOME || '/home/axis',
        isLiveEnvironment: isLiveSession(),
      });
    }

    // 2. Disks
    if (pathname === '/api/disks' && req.method === 'GET') {
      const disks = await getStorageDisks();
      return sendJson(res, 200, disks);
    }

    // 3. Terminal Execution
    if (pathname === '/api/terminal-exec' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const command = body.command || '';
      const cwd = body.cwd || '/home/axis';
      const result = await runCmd(command, cwd);
      return sendJson(res, 200, result);
    }

    // 4. Filesystem: Read Directory
    if (pathname === '/api/fs-read' && req.method === 'GET') {
      const target = (parsedUrl.query.path && typeof parsedUrl.query.path === 'string') 
        ? parsedUrl.query.path 
        : (process.env.HOME || '/home/axis');

      if (!fs.existsSync(target)) {
        return sendJson(res, 200, { path: target, items: [] });
      }

      try {
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

        return sendJson(res, 200, { path: target, items });
      } catch (err) {
        return sendJson(res, 500, { error: err.message, path: target, items: [] });
      }
    }

    // 5. Filesystem: Write File
    if (pathname === '/api/fs-write' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      try {
        fs.mkdirSync(path.dirname(body.filePath), { recursive: true });
        fs.writeFileSync(body.filePath, body.content || '', 'utf-8');
        return sendJson(res, 200, { success: true });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
    }

    // 6. Filesystem: Create Directory
    if (pathname === '/api/fs-mkdir' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      try {
        fs.mkdirSync(body.dirPath, { recursive: true });
        return sendJson(res, 200, { success: true });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
    }

    // 7. Filesystem: Delete Item
    if (pathname === '/api/fs-delete' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      try {
        fs.rmSync(body.targetPath, { recursive: true, force: true });
        return sendJson(res, 200, { success: true });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
    }

    // 8. Installer: Start
    if (pathname === '/api/installer/start' && req.method === 'POST') {
      const config = await parseJsonBody(req);
      if (installJob.isInstalling) {
        return sendJson(res, 400, { success: false, message: 'Installation already in progress' });
      }

      installJob = {
        isInstalling: true,
        progress: 2,
        statusText: 'Initializing installation engine...',
        completed: false,
        error: null,
        log: [`[${new Date().toLocaleTimeString()}] Installation started for target disk ${config.targetDisk}`],
      };

      const scriptCandidates = [
        '/usr/local/bin/axisos-installer.sh',
        path.join(__dirname, '../../os-build/scripts/axisos-install.sh'),
        path.join(process.cwd(), 'os-build/scripts/axisos-install.sh'),
      ];
      const scriptPath = scriptCandidates.find((s) => fs.existsSync(s));

      if (!scriptPath) {
        installJob.isInstalling = false;
        installJob.error = 'Installer script not found';
        return sendJson(res, 500, { success: false, message: 'Installer script axisos-installer.sh not found' });
      }

      const args = [
        scriptPath,
        '--disk', config.targetDisk || '/dev/vda',
        '--username', config.username || 'axis',
        '--password', config.password || 'password',
        '--fullname', config.userFullName || 'AxisOS User',
        '--hostname', config.computerName || 'axis-pc',
        '--autologin', config.autoLogin ? 'true' : 'false',
        '--locale', config.locale || 'en_US.UTF-8',
        '--timezone', config.timezone || 'UTC',
        '--keymap', config.keymap || 'us',
        '--fs', 'btrfs',
      ];

      const isLinux = process.platform === 'linux';
      const isRoot = process.getuid ? process.getuid() === 0 : false;
      let cmd = 'bash';
      let cmdArgs = args;

      if (isLinux) {
        if (!isRoot) {
          cmd = 'sudo';
          cmdArgs = [scriptPath, ...args.slice(1)];
          installJob.log.push('[System] Elevating installer process with sudo');
        }
      } else {
        args.push('--dry-run');
        installJob.log.push(`[Notice] Running in verified simulation mode (${process.platform})`);
      }

      const child = spawn(cmd, cmdArgs);

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

      return sendJson(res, 200, { success: true });
    }

    // 9. Installer: Status
    if (pathname === '/api/installer/status' && req.method === 'GET') {
      return sendJson(res, 200, installJob);
    }

    // 10. Power Controls
    if (pathname === '/api/power' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      if (body.action === 'poweroff') {
        runCmd('sync; systemctl poweroff || shutdown -h now');
        return sendJson(res, 200, { success: true, action: 'poweroff' });
      } else if (body.action === 'reboot') {
        runCmd('sync; systemctl reboot || reboot');
        return sendJson(res, 200, { success: true, action: 'reboot' });
      }
      return sendJson(res, 200, { success: true, action: 'lock' });
    }

    // 11. System Updates: Check
    if (pathname === '/api/updates/check' && req.method === 'GET') {
      const uname = await runCmd('uname -r');
      const aptCheck = await runCmd('apt list --upgradable 2>/dev/null | grep -v "Listing..." | head -n 30');
      const pkgs = aptCheck.stdout ? aptCheck.stdout.split('\n').filter(Boolean) : [];

      return sendJson(res, 200, {
        upToDate: pkgs.length === 0,
        packagesCount: pkgs.length,
        packages: pkgs,
        kernelUpgradeAvailable: pkgs.some((p) => p.includes('linux-image')),
        autoUpdatesEnabled: true,
        currentKernel: uname.stdout || '6.1.0-axisos',
        message: pkgs.length === 0 ? 'All packages and kernel are up to date.' : `${pkgs.length} update(s) available.`,
      });
    }

    // 12. System Updates: Apply
    if (pathname === '/api/updates/apply' && req.method === 'POST') {
      runCmd('unattended-upgrade || (apt-get update && DEBIAN_FRONTEND=noninteractive apt-get -y upgrade)');
      return sendJson(res, 200, { success: true, message: 'Update process initiated in background.' });
    }

    return sendJson(res, 404, { error: 'Unknown API endpoint' });
  }

  // ==================== STATIC ASSET SERVING ====================
  let safePath = path.normalize(pathname).replace(/^(\.\.[\/\\])+/, '');
  if (safePath === '/' || safePath === '\\') safePath = '/index.html';

  const requestedFile = path.join(DIST_DIR, safePath);
  serveStaticFile(req, res, requestedFile);
});

server.listen(PORT, HOST, () => {
  console.log(`==================================================`);
  console.log(` AxisOS System Management Daemon`);
  console.log(` Listening on: http://${HOST}:${PORT}`);
  console.log(` Serving Shell: ${DIST_DIR}`);
  console.log(`==================================================`);
});
