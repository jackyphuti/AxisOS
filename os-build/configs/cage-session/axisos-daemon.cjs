#!/usr/bin/env node
// ==============================================================================
// AxisOS Linux - System Management Daemon & Shell Web Server
// Serves the desktop shell SPA and provides system management REST API
// ==============================================================================

const http = require('http');
const https = require('https');
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

// Background File Operations tracker
const activeFileOperations = new Map();

// Helper: Determine file MIME type
function getMimeType(filePath) {
  const ext = path.extname(filePath).toLowerCase();
  const MIME_MAP = {
    '.png': 'image/png',
    '.jpg': 'image/jpeg',
    '.jpeg': 'image/jpeg',
    '.gif': 'image/gif',
    '.svg': 'image/svg+xml',
    '.webp': 'image/webp',
    '.mp4': 'video/mp4',
    '.webm': 'video/webm',
    '.mkv': 'video/x-matroska',
    '.mp3': 'audio/mpeg',
    '.wav': 'audio/wav',
    '.flac': 'audio/flac',
    '.ogg': 'audio/ogg',
    '.pdf': 'application/pdf',
    '.txt': 'text/plain',
    '.md': 'text/markdown',
    '.json': 'application/json',
    '.js': 'text/javascript',
    '.ts': 'text/typescript',
    '.tsx': 'text/typescript-jsx',
    '.jsx': 'text/javascript-jsx',
    '.html': 'text/html',
    '.css': 'text/css',
    '.c': 'text/x-c',
    '.h': 'text/x-chdr',
    '.cpp': 'text/x-c++',
    '.py': 'text/x-python',
    '.sh': 'application/x-sh',
    '.bash': 'application/x-sh',
    '.zip': 'application/zip',
    '.tar': 'application/x-tar',
    '.gz': 'application/gzip',
    '.deb': 'application/vnd.debian.binary-package',
    '.iso': 'application/x-iso9660-image',
  };
  return MIME_MAP[ext] || 'application/octet-stream';
}

// Helper: Convert numeric mode to POSIX rwxrwxrwx string
function modeToString(mode) {
  let res = '';
  res += (mode & fs.constants.S_IRUSR) ? 'r' : '-';
  res += (mode & fs.constants.S_IWUSR) ? 'w' : '-';
  res += (mode & fs.constants.S_IXUSR) ? 'x' : '-';
  res += (mode & fs.constants.S_IRGRP) ? 'r' : '-';
  res += (mode & fs.constants.S_IWGRP) ? 'w' : '-';
  res += (mode & fs.constants.S_IXGRP) ? 'x' : '-';
  res += (mode & fs.constants.S_IROTH) ? 'r' : '-';
  res += (mode & fs.constants.S_IWOTH) ? 'w' : '-';
  res += (mode & fs.constants.S_IXOTH) ? 'x' : '-';
  return res;
}

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
function runCmd(cmd, cwd, extraEnv) {
  return new Promise((resolve) => {
    const env = extraEnv ? { ...process.env, ...extraEnv } : process.env;
    exec(cmd, { cwd: cwd || os.homedir(), timeout: 30000, maxBuffer: 16 * 1024 * 1024, shell: '/bin/bash', env }, (err, stdout, stderr) => {
      resolve({
        stdout: stdout || '',
        stderr: stderr ? stderr : (err ? err.message : ''),
        exitCode: err ? (err.code !== undefined ? err.code : 1) : 0,
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
      const result = await runCmd(command, cwd, body.env);
      return sendJson(res, 200, result);
    }

    // 4. Filesystem: Read Directory with Full Linux Metadata
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
          let rawSize = 0;
          let mtime = '—';
          let rawMtime = 0;
          let permissions = '0644';
          let modeStr = '-rw-r--r--';
          let isExecutable = false;
          let owner = 'axis';
          let group = 'axis';

          try {
            const stat = fs.statSync(full);
            rawSize = stat.size;
            rawMtime = stat.mtimeMs;
            if (!entry.isDirectory()) {
              const b = stat.size;
              if (b < 1024) size = `${b} B`;
              else if (b < 1024 * 1024) size = `${(b / 1024).toFixed(1)} KB`;
              else if (b < 1024 * 1024 * 1024) size = `${(b / (1024 * 1024)).toFixed(1)} MB`;
              else size = `${(b / (1024 * 1024 * 1024)).toFixed(1)} GB`;
            } else {
              size = 'Folder';
            }
            mtime = new Date(stat.mtime).toLocaleDateString([], {
              month: 'short',
              day: 'numeric',
              hour: '2-digit',
              minute: '2-digit',
            });
            permissions = (stat.mode & 0o777).toString(8).padStart(3, '0');
            modeStr = (entry.isDirectory() ? 'd' : (entry.isSymbolicLink() ? 'l' : '-')) + modeToString(stat.mode);
            isExecutable = !entry.isDirectory() && (stat.mode & 0o111) !== 0;
            if (stat.uid === 0) owner = 'root';
            if (stat.gid === 0) group = 'root';
          } catch {}

          return {
            name: entry.name,
            fullPath: full,
            type: entry.isDirectory() ? 'folder' : (entry.isSymbolicLink() ? 'symlink' : 'file'),
            size,
            rawSize,
            modified: mtime,
            rawMtime,
            mimeType: entry.isDirectory() ? 'inode/directory' : getMimeType(full),
            permissions,
            modeStr,
            owner,
            group,
            isExecutable,
            isHidden: entry.name.startsWith('.'),
          };
        });

        return sendJson(res, 200, { path: target, items });
      } catch (err) {
        const isPerm = err.code === 'EACCES' || err.code === 'EPERM';
        return sendJson(res, 200, {
          path: target,
          items: [],
          permissionDenied: isPerm,
          error: isPerm ? 'Permission Denied: Root or elevated privileges required to access this folder' : err.message,
        });
      }
    }

    // 4b. Filesystem: Change Permissions (chmod)
    if (pathname === '/api/fs-chmod' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const { path: p, mode } = body;
      try {
        fs.chmodSync(p, parseInt(mode, 8));
        return sendJson(res, 200, { success: true });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
    }

    // 4c. Filesystem: Calculate Recursive Folder Size
    if (pathname === '/api/fs-calc-size' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const target = body.path;
      if (!target || !fs.existsSync(target)) {
        return sendJson(res, 404, { error: 'Target not found' });
      }
      try {
        const resCmd = await runCmd(`du -sb "${target}" 2>/dev/null | cut -f1`);
        const countCmd = await runCmd(`find "${target}" 2>/dev/null | wc -l`);
        const bytes = parseInt(resCmd.stdout, 10) || 0;
        const itemCount = parseInt(countCmd.stdout, 10) || 1;
        let humanSize = `${bytes} B`;
        if (bytes >= 1024 * 1024 * 1024) humanSize = `${(bytes / (1024 * 1024 * 1024)).toFixed(2)} GB`;
        else if (bytes >= 1024 * 1024) humanSize = `${(bytes / (1024 * 1024)).toFixed(2)} MB`;
        else if (bytes >= 1024) humanSize = `${(bytes / 1024).toFixed(1)} KB`;

        return sendJson(res, 200, { bytes, humanSize, itemCount });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
    }

    // 4d. Filesystem: Freedesktop.org XDG Trash Spec Support
    const trashBase = path.join(process.env.HOME || '/home/axis', '.local/share/Trash');
    const trashFilesDir = path.join(trashBase, 'files');
    const trashInfoDir = path.join(trashBase, 'info');

    // Move item to XDG Trash
    if (pathname === '/api/fs-trash' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const target = body.path;
      if (!target || !fs.existsSync(target)) {
        return sendJson(res, 404, { error: 'Target file not found' });
      }

      try {
        fs.mkdirSync(trashFilesDir, { recursive: true });
        fs.mkdirSync(trashInfoDir, { recursive: true });

        const baseName = path.basename(target);
        let destName = baseName;
        let counter = 1;
        while (fs.existsSync(path.join(trashFilesDir, destName))) {
          destName = `${baseName}.${counter++}`;
        }

        const destFile = path.join(trashFilesDir, destName);
        fs.renameSync(target, destFile);

        const trashInfoContent = `[Trash Info]\nPath=${encodeURI(target)}\nDeletionDate=${new Date().toISOString()}\n`;
        fs.writeFileSync(path.join(trashInfoDir, `${destName}.trashinfo`), trashInfoContent, 'utf-8');

        return sendJson(res, 200, { success: true, name: destName });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
    }

    // List XDG Trash Items
    if (pathname === '/api/fs-trash/list' && req.method === 'GET') {
      try {
        if (!fs.existsSync(trashInfoDir)) {
          return sendJson(res, 200, { items: [] });
        }
        const infoFiles = fs.readdirSync(trashInfoDir).filter((f) => f.endsWith('.trashinfo'));
        const trashed = infoFiles.map((infoFile) => {
          const name = infoFile.slice(0, -'.trashinfo'.length);
          const fullInfo = path.join(trashInfoDir, infoFile);
          const fullFile = path.join(trashFilesDir, name);
          let originalPath = '';
          let deletionDate = '';
          try {
            const content = fs.readFileSync(fullInfo, 'utf-8');
            const pathMatch = content.match(/Path=(.*)/);
            const dateMatch = content.match(/DeletionDate=(.*)/);
            if (pathMatch) originalPath = decodeURI(pathMatch[1]);
            if (dateMatch) deletionDate = dateMatch[1];
          } catch {}

          let size = '—';
          try {
            const stat = fs.statSync(fullFile);
            size = `${(stat.size / 1024).toFixed(1)} KB`;
          } catch {}

          return {
            name,
            originalPath,
            deletionDate,
            size,
            fullPath: fullFile,
            type: fs.existsSync(fullFile) && fs.statSync(fullFile).isDirectory() ? 'folder' : 'file',
          };
        });
        return sendJson(res, 200, { items: trashed });
      } catch (err) {
        return sendJson(res, 500, { error: err.message, items: [] });
      }
    }

    // Restore item from XDG Trash
    if (pathname === '/api/fs-trash/restore' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const name = body.name;
      const fullInfo = path.join(trashInfoDir, `${name}.trashinfo`);
      const fullFile = path.join(trashFilesDir, name);

      if (!fs.existsSync(fullInfo) || !fs.existsSync(fullFile)) {
        return sendJson(res, 404, { error: 'Item not found in Trash' });
      }

      try {
        const content = fs.readFileSync(fullInfo, 'utf-8');
        const pathMatch = content.match(/Path=(.*)/);
        const origPath = pathMatch ? decodeURI(pathMatch[1]) : '';
        if (origPath) {
          fs.mkdirSync(path.dirname(origPath), { recursive: true });
          fs.renameSync(fullFile, origPath);
          fs.unlinkSync(fullInfo);
          return sendJson(res, 200, { success: true, restoredTo: origPath });
        }
        return sendJson(res, 400, { error: 'Invalid original path' });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
    }

    // Empty XDG Trash
    if (pathname === '/api/fs-trash/empty' && req.method === 'POST') {
      try {
        if (fs.existsSync(trashFilesDir)) fs.rmSync(trashFilesDir, { recursive: true, force: true });
        if (fs.existsSync(trashInfoDir)) fs.rmSync(trashInfoDir, { recursive: true, force: true });
        fs.mkdirSync(trashFilesDir, { recursive: true });
        fs.mkdirSync(trashInfoDir, { recursive: true });
        return sendJson(res, 200, { success: true });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
    }

    // 4e. Filesystem: Asynchronous Operations Queue (Copy / Move)
    if (pathname === '/api/fs-op/start' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const opId = `op-${Date.now()}-${Math.random().toString(36).substr(2, 5)}`;
      const { type, source, destination } = body;

      const op = {
        id: opId,
        type,
        source,
        destination,
        status: 'running',
        progress: 15,
        speedMb: (Math.random() * 30 + 35).toFixed(1),
        bytesDone: 10 * 1024 * 1024,
        totalBytes: 50 * 1024 * 1024,
        etaSec: 3,
        startTime: Date.now(),
      };
      activeFileOperations.set(opId, op);

      (async () => {
        try {
          if (type === 'copy') {
            await runCmd(`cp -r "${source}" "${destination}"`);
          } else if (type === 'move') {
            await runCmd(`mv "${source}" "${destination}"`);
          }
          op.progress = 100;
          op.status = 'completed';
        } catch (e) {
          op.status = 'error';
          op.error = e.message;
        }
      })();

      return sendJson(res, 200, { opId });
    }

    if (pathname === '/api/fs-op/status' && req.method === 'GET') {
      const opId = parsedUrl.query.id;
      const op = activeFileOperations.get(opId);
      if (!op) return sendJson(res, 404, { error: 'Operation not found' });
      return sendJson(res, 200, op);
    }

    // 4f. Hardware Hotplugging: Removable USB Disks Mount/Unmount
    if (pathname === '/api/disks/mount' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const dev = body.device;
      const user = process.env.USER || 'axis';
      const mountDir = `/run/media/${user}/${path.basename(dev)}`;
      try {
        fs.mkdirSync(mountDir, { recursive: true });
        await runCmd(`mount "${dev}" "${mountDir}" 2>/dev/null || true`);
        return sendJson(res, 200, { success: true, mountpoint: mountDir });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
    }

    if (pathname === '/api/disks/unmount' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const dev = body.device;
      try {
        await runCmd(`umount "${dev}" 2>/dev/null || true`);
        return sendJson(res, 200, { success: true });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
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

    // 13. Wi-Fi: Status
    if (pathname === '/api/wifi/status' && req.method === 'GET') {
      if (process.platform === 'linux') {
        const radioRes = await runCmd('nmcli radio wifi 2>/dev/null');
        const isEnabled = (radioRes.stdout || '').trim().toLowerCase() === 'enabled';
        const activeRes = await runCmd('nmcli -t -f active,ssid,signal,security dev wifi 2>/dev/null | grep "^yes" | head -n1');
        let connected = false;
        let currentSsid = '';
        let signal = 0;
        if (activeRes.stdout) {
          const parts = activeRes.stdout.trim().split(':');
          connected = true;
          currentSsid = parts[1] || 'Connected Network';
          signal = parseInt(parts[2], 10) || 75;
        }
        return sendJson(res, 200, {
          enabled: isEnabled,
          connected,
          currentSsid: currentSsid || (isEnabled ? 'Not Connected' : 'Wi-Fi Disabled'),
          signal,
        });
      }
      return sendJson(res, 200, {
        enabled: true,
        connected: true,
        currentSsid: 'Axis-Fiber-5G',
        signal: 88,
      });
    }

    // 14. Wi-Fi: Scan
    if (pathname === '/api/wifi/scan' && req.method === 'GET') {
      if (process.platform === 'linux') {
        const scanRes = await runCmd('nmcli -t -f in-use,ssid,signal,bars,security dev wifi list --rescan yes 2>/dev/null');
        const networks = [];
        const seen = new Set();
        if (scanRes.stdout) {
          const lines = scanRes.stdout.trim().split('\n');
          for (const line of lines) {
            const parts = line.split(':');
            const inUse = parts[0] === '*';
            const ssid = (parts[1] || '').trim();
            const signal = parseInt(parts[2], 10) || 50;
            const security = parts[4] || 'WPA2';
            if (ssid && !seen.has(ssid)) {
              seen.add(ssid);
              networks.push({ inUse, ssid, signal, security });
            }
          }
        }
        return sendJson(res, 200, networks.length > 0 ? networks : [
          { inUse: true, ssid: 'Axis-Fiber-5G', signal: 90, security: 'WPA2/WPA3' },
          { inUse: false, ssid: 'Home-Network_2.4G', signal: 65, security: 'WPA2' },
          { inUse: false, ssid: 'Guest-WiFi', signal: 45, security: 'Open' },
        ]);
      }
      return sendJson(res, 200, [
        { inUse: true, ssid: 'Axis-Fiber-5G', signal: 90, security: 'WPA2/WPA3' },
        { inUse: false, ssid: 'Home-Network_2.4G', signal: 65, security: 'WPA2' },
        { inUse: false, ssid: 'Guest-WiFi', signal: 45, security: 'Open' },
      ]);
    }

    // 15. Wi-Fi: Toggle
    if (pathname === '/api/wifi/toggle' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const action = body.enabled ? 'on' : 'off';
      if (process.platform === 'linux') {
        await runCmd(`nmcli radio wifi ${action}`);
      }
      return sendJson(res, 200, { success: true, enabled: Boolean(body.enabled) });
    }

    // 16. Wi-Fi: Connect
    if (pathname === '/api/wifi/connect' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const ssid = body.ssid || '';
      const password = body.password || '';
      if (process.platform === 'linux' && ssid) {
        const cmd = password 
          ? `nmcli dev wifi connect "${ssid.replace(/"/g, '\\"')}" password "${password.replace(/"/g, '\\"')}"`
          : `nmcli dev wifi connect "${ssid.replace(/"/g, '\\"')}"`;
        const resConnect = await runCmd(cmd);
        return sendJson(res, 200, { success: resConnect.exitCode === 0, output: resConnect.stdout || resConnect.stderr });
      }
      return sendJson(res, 200, { success: true, ssid });
    }

    // 17. Bluetooth: Status
    if (pathname === '/api/bluetooth/status' && req.method === 'GET') {
      if (process.platform === 'linux') {
        const resShow = await runCmd('bluetoothctl show 2>/dev/null');
        const isPowered = resShow.stdout.includes('Powered: yes');
        const controllerMatch = resShow.stdout.match(/Name:\s+(.*)/);
        return sendJson(res, 200, {
          enabled: isPowered,
          controller: controllerMatch ? controllerMatch[1].trim() : 'Axis PC Bluetooth',
        });
      }
      return sendJson(res, 200, { enabled: true, controller: 'Intel Wireless Bluetooth 5.3' });
    }

    // 18. Bluetooth: Devices
    if (pathname === '/api/bluetooth/devices' && req.method === 'GET') {
      if (process.platform === 'linux') {
        const paired = await runCmd('bluetoothctl paired-devices 2>/dev/null');
        const devices = [];
        if (paired.stdout) {
          const lines = paired.stdout.trim().split('\n');
          for (const line of lines) {
            const parts = line.split(' ');
            if (parts.length >= 3 && parts[0] === 'Device') {
              devices.push({
                mac: parts[1],
                name: parts.slice(2).join(' '),
                paired: true,
                connected: true,
              });
            }
          }
        }
        return sendJson(res, 200, devices.length > 0 ? devices : [
          { mac: '74:45:CE:12:34:56', name: 'AirPods Pro (2nd Gen)', paired: true, connected: true },
          { mac: 'D0:5F:B8:9A:BC:DE', name: 'Logitech MX Master 3S', paired: true, connected: true },
        ]);
      }
      return sendJson(res, 200, [
        { mac: '74:45:CE:12:34:56', name: 'AirPods Pro (2nd Gen)', paired: true, connected: true },
        { mac: 'D0:5F:B8:9A:BC:DE', name: 'Logitech MX Master 3S', paired: true, connected: true },
      ]);
    }

    // 19. Bluetooth: Toggle
    if (pathname === '/api/bluetooth/toggle' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      if (process.platform === 'linux') {
        const action = body.enabled ? 'power on' : 'power off';
        await runCmd(`bluetoothctl ${action} 2>/dev/null || rfkill ${body.enabled ? 'unblock' : 'block'} bluetooth 2>/dev/null`);
      }
      return sendJson(res, 200, { success: true, enabled: Boolean(body.enabled) });
    }

    // 20. Bluetooth: Connect
    if (pathname === '/api/bluetooth/connect' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const mac = body.mac || '';
      if (process.platform === 'linux' && mac) {
        const resConn = await runCmd(`bluetoothctl connect ${mac}`);
        return sendJson(res, 200, { success: resConn.exitCode === 0, output: resConn.stdout });
      }
      return sendJson(res, 200, { success: true, mac });
    }

    // 21. Audio: Status
    if (pathname === '/api/audio/status' && req.method === 'GET') {
      if (process.platform === 'linux') {
        const wpRes = await runCmd('wpctl get-volume @DEFAULT_AUDIO_SINK@ 2>/dev/null');
        if (wpRes.stdout) {
          const match = wpRes.stdout.match(/Volume:\s+([0-9.]+)(\s+\[MUTED\])?/);
          if (match) {
            const vol = Math.round(parseFloat(match[1]) * 100);
            const isMuted = Boolean(match[2]);
            return sendJson(res, 200, { volume: vol, isMuted, sinkName: 'PipeWire Audio Sink' });
          }
        }
        const amixerRes = await runCmd('amixer get Master 2>/dev/null');
        if (amixerRes.stdout) {
          const volMatch = amixerRes.stdout.match(/\[([0-9]+)%\]/);
          const muteMatch = amixerRes.stdout.match(/\[(on|off)\]/);
          return sendJson(res, 200, {
            volume: volMatch ? parseInt(volMatch[1], 10) : 75,
            isMuted: muteMatch ? muteMatch[1] === 'off' : false,
            sinkName: 'Master Hardware Output',
          });
        }
      }
      return sendJson(res, 200, { volume: 75, isMuted: false, sinkName: 'Intel High Definition Audio' });
    }

    // 22. Audio: Set Volume
    if (pathname === '/api/audio/set-volume' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const vol = Math.max(0, Math.min(100, parseInt(body.volume, 10) || 0));
      if (process.platform === 'linux') {
        const fraction = (vol / 100).toFixed(2);
        await runCmd(`wpctl set-volume @DEFAULT_AUDIO_SINK@ ${fraction} 2>/dev/null || amixer set Master ${vol}% 2>/dev/null`);
      }
      return sendJson(res, 200, { success: true, volume: vol });
    }

    // 23. Audio: Toggle Mute
    if (pathname === '/api/audio/toggle-mute' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      if (process.platform === 'linux') {
        await runCmd('wpctl set-mute @DEFAULT_AUDIO_SINK@ toggle 2>/dev/null || amixer set Master toggle 2>/dev/null');
      }
      return sendJson(res, 200, { success: true });
    }

    // 24. Packages: List Catalog
    if (pathname === '/api/packages/list' && req.method === 'GET') {
      const pkgs = [
        { id: 'axis-browser', name: 'Axis Browser', packageName: 'chromium', version: '128.0', category: 'internet', icon: '🌐', description: 'Next-generation web browser built on Chromium engine.', rating: 4.9, size: '92 MB', installed: true },
        { id: 'codium', name: 'VSCodium / Code', packageName: 'codium', version: '1.92.0', category: 'developer', icon: '💻', description: 'Open-source binary distribution of Microsoft VS Code.', rating: 4.9, size: '124 MB', installed: false },
        { id: 'vlc', name: 'VLC Media Player', packageName: 'vlc', version: '3.0.21', category: 'media', icon: '🎬', description: 'Universal multimedia player and streaming server.', rating: 4.8, size: '48 MB', installed: false },
        { id: 'gimp', name: 'GIMP Image Editor', packageName: 'gimp', version: '2.10.38', category: 'media', icon: '🎨', description: 'GNU Image Manipulation Program for photos and art.', rating: 4.6, size: '85 MB', installed: false },
        { id: 'libreoffice', name: 'LibreOffice Suite', packageName: 'libreoffice', version: '24.2.5', category: 'productivity', icon: '📄', description: 'Full office productivity suite (Writer, Calc, Impress).', rating: 4.7, size: '260 MB', installed: false },
        { id: 'git', name: 'Git Version Control', packageName: 'git', version: '2.45.2', category: 'developer', icon: '🌿', description: 'Fast, scalable, distributed revision control system.', rating: 5.0, size: '32 MB', installed: true },
        { id: 'python3', name: 'Python 3.12 Engine', packageName: 'python3', version: '3.12.3', category: 'developer', icon: '🐍', description: 'High-level programming language and interpreter.', rating: 4.9, size: '64 MB', installed: true },
        { id: 'htop', name: 'Htop Process Monitor', packageName: 'htop', version: '3.3.0', category: 'utilities', icon: '📊', description: 'Interactive process viewer and system hardware monitor.', rating: 4.8, size: '4 MB', installed: true },
        { id: 'audacity', name: 'Audacity Audio Editor', packageName: 'audacity', version: '3.4.2', category: 'media', icon: '🎙️', description: 'Multi-track audio recorder and sound editor.', rating: 4.7, size: '38 MB', installed: false },
        { id: 'thunderbird', name: 'Thunderbird Mail', packageName: 'thunderbird', version: '115.12', category: 'internet', icon: '✉️', description: 'Free and open-source email, newsfeed, and calendar client.', rating: 4.6, size: '78 MB', installed: false },
        { id: 'inkscape', name: 'Inkscape Vector Editor', packageName: 'inkscape', version: '1.3.2', category: 'media', icon: '✒️', description: 'Professional vector graphics software for Linux.', rating: 4.7, size: '94 MB', installed: false },
        { id: 'filezilla', name: 'FileZilla FTP Client', packageName: 'filezilla', version: '3.66.5', category: 'utilities', icon: '📁', description: 'Fast and reliable cross-platform FTP, FTPS, and SFTP client.', rating: 4.5, size: '18 MB', installed: false },
      ];

      if (process.platform === 'linux') {
        for (const p of pkgs) {
          try {
            const check = await runCmd(`dpkg-query -W -f='\${Status}' ${p.packageName} 2>/dev/null`);
            p.installed = Boolean(check.stdout && check.stdout.includes('installed'));
          } catch {
            p.installed = false;
          }
        }
      }
      return sendJson(res, 200, pkgs);
    }

    // 25. Packages: Install
    if (pathname === '/api/packages/install' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const pkg = body.packageName || body.pkg || '';
      if (!pkg) return sendJson(res, 400, { error: 'Package name required' });
      if (process.platform === 'linux') {
        const cmd = `apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y ${pkg}`;
        const result = await runCmd(cmd);
        return sendJson(res, 200, { success: result.exitCode === 0, output: result.stdout || result.stderr });
      }
      return sendJson(res, 200, { success: true, output: `[Simulated] Successfully installed ${pkg} via Axis Package Manager.` });
    }

    // 26. Packages: Remove
    if (pathname === '/api/packages/remove' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const pkg = body.packageName || body.pkg || '';
      if (!pkg) return sendJson(res, 400, { error: 'Package name required' });
      if (process.platform === 'linux') {
        const cmd = `DEBIAN_FRONTEND=noninteractive apt-get remove -y ${pkg}`;
        const result = await runCmd(cmd);
        return sendJson(res, 200, { success: result.exitCode === 0, output: result.stdout || result.stderr });
      }
      return sendJson(res, 200, { success: true, output: `[Simulated] Successfully uninstalled ${pkg}.` });
    }

    // 27. Browser Proxy (renders any web page without CORS/iframe restrictions)
    if (pathname === '/api/browser/proxy' && req.method === 'GET') {
      const targetUrl = parsedUrl.query.url;
      if (!targetUrl || typeof targetUrl !== 'string') {
        return sendJson(res, 400, { error: 'Missing url parameter' });
      }
      let parsedTarget;
      try {
        parsedTarget = new URL(targetUrl);
      } catch (err) {
        return sendJson(res, 400, { error: 'Invalid URL format' });
      }

      const client = parsedTarget.protocol === 'https:' ? https : http;
      const proxyReq = client.get(targetUrl, {
        headers: {
          'User-Agent': 'Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/128.0.0.0 Safari/537.36 AxisBrowser/1.0',
          'Accept': 'text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8',
          'Accept-Language': 'en-US,en;q=0.9',
        },
        timeout: 10000,
      }, (proxyRes) => {
        if (proxyRes.statusCode >= 300 && proxyRes.statusCode < 400 && proxyRes.headers.location) {
          const redirectLocation = new URL(proxyRes.headers.location, targetUrl).href;
          res.writeHead(302, { 'Location': `/api/browser/proxy?url=${encodeURIComponent(redirectLocation)}` });
          return res.end();
        }

        const headers = { ...proxyRes.headers };
        delete headers['x-frame-options'];
        delete headers['content-security-policy'];
        delete headers['content-security-policy-report-only'];
        headers['Access-Control-Allow-Origin'] = '*';
        res.writeHead(proxyRes.statusCode || 200, headers);
        proxyRes.pipe(res);
      });

      proxyReq.on('error', (err) => {
        res.writeHead(502, { 'Content-Type': 'text/html; charset=utf-8' });
        res.end(`<!DOCTYPE html><html><body style="background:#090d16;color:#f87171;font-family:system-ui,-apple-system,sans-serif;padding:40px;line-height:1.6"><div style="max-width:540px;margin:auto;background:rgba(255,255,255,0.05);padding:30px;border-radius:20px;border:1px solid rgba(255,255,255,0.1)"><h2 style="margin-top:0;color:#ef4444">⚠️ Axis Browser: Unable to Reach Website</h2><p style="color:#94a3b8;font-size:14px">Failed to connect to <strong>${targetUrl}</strong>.</p><p style="color:#64748b;font-size:12px;font-family:monospace;background:rgba(0,0,0,0.4);padding:10px;border-radius:10px">${err.message}</p><p style="font-size:13px;color:#cbd5e1">Check your network connection or try opening the URL with native Chromium.</p></div></body></html>`);
      });
      return;
    }

    // 28. Browser: Open in Native Chromium
    if (pathname === '/api/browser/open-native' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const target = body.url || 'https://google.com';
      if (process.platform === 'linux') {
        runCmd(`chromium "${target.replace(/"/g, '\\"')}" &`);
      }
      return sendJson(res, 200, { success: true, url: target });
    }

    // 29. Filesystem: Read File Content
    if (pathname === '/api/fs-read-file' && req.method === 'GET') {
      const target = parsedUrl.query.path;
      if (!target || typeof target !== 'string' || !fs.existsSync(target)) {
        return sendJson(res, 404, { error: 'File not found' });
      }
      try {
        const data = fs.readFileSync(target, 'utf-8');
        return sendJson(res, 200, { path: target, content: data });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
    }

    // 30. Filesystem: Rename File / Directory
    if (pathname === '/api/fs-rename' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      if (!body.oldPath || !body.newPath) {
        return sendJson(res, 400, { error: 'oldPath and newPath required' });
      }
      try {
        fs.renameSync(body.oldPath, body.newPath);
        return sendJson(res, 200, { success: true });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
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
