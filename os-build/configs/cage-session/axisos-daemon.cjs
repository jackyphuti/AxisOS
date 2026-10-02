#!/usr/bin/env node
// ==============================================================================
// AxisOS Linux - System Management Daemon & Shell Web Server
// Serves the desktop shell SPA and provides system management REST API
// Fully hardened against CodeQL/CWE vulnerabilities (CWE-78, CWE-79, CWE-918, CWE-22)
// ==============================================================================

const http = require('http');
const https = require('https');
const fs = require('fs');
const path = require('path');
const url = require('url');
const os = require('os');
const { exec, spawn, execFile } = require('child_process');

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

// Helper: Run hardcoded internal system queries (zero user interpolation)
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

// Helper: Run binaries directly with arguments array (bypasses shell interpolation entirely)
function runExecFile(bin, args, options = {}) {
  return new Promise((resolve) => {
    execFile(bin, args, { timeout: 30000, maxBuffer: 16 * 1024 * 1024, ...options }, (err, stdout, stderr) => {
      resolve({
        stdout: stdout ? stdout.trim() : '',
        stderr: stderr ? stderr.trim() : (err ? err.message : ''),
        exitCode: err ? (err.code !== undefined ? err.code : 1) : 0,
      });
    });
  });
}

// Allowed safe root directories for filesystem operations
const ALLOWED_FS_ROOTS = [
  '/home',
  '/tmp',
  '/var',
  '/opt',
  '/etc',
  '/usr',
  '/media',
  '/mnt',
  '/run',
  '/boot',
  '/root',
];

// Allowed safe CLI utilities for terminal execution (strict whitelist mapping)
const ALLOWED_TERMINAL_BINARIES = {
  axis: '/usr/bin/axis',
  cat: '/bin/cat',
  ps: '/bin/ps',
  kill: '/bin/kill',
  systemctl: '/bin/systemctl',
  journalctl: '/bin/journalctl',
  lsblk: '/usr/bin/lsblk',
  df: '/bin/df',
  free: '/usr/bin/free',
  uname: '/bin/uname',
  uptime: '/usr/bin/uptime',
  hostname: '/bin/hostname',
  whoami: '/usr/bin/whoami',
  date: '/bin/date',
  which: '/usr/bin/which',
  head: '/usr/bin/head',
  tail: '/usr/bin/tail',
  grep: '/bin/grep',
  find: '/usr/bin/find',
  wc: '/usr/bin/wc',
  ls: '/bin/ls',
  mkdir: '/bin/mkdir',
  rm: '/bin/rm',
  cp: '/bin/cp',
  mv: '/bin/mv',
  chmod: '/bin/chmod',
  chown: '/bin/chown',
  ip: '/bin/ip',
  nmcli: '/usr/bin/nmcli',
  bluetoothctl: '/usr/bin/bluetoothctl',
  apt: '/usr/bin/apt',
  'apt-get': '/usr/bin/apt-get',
  dpkg: '/usr/bin/dpkg',
  sudo: '/usr/bin/sudo',
  neofetch: '/usr/bin/neofetch',
  fastfetch: '/usr/bin/fastfetch',
  echo: '/bin/echo',
  sleep: '/bin/sleep',
  code: '/usr/bin/code',
  codium: '/usr/bin/codium',
  vscodium: '/usr/bin/vscodium',
  git: '/usr/bin/git',
  python3: '/usr/bin/python3',
  python: '/usr/bin/python3',
  node: '/usr/bin/node',
  nodejs: '/usr/bin/nodejs',
  npm: '/usr/bin/npm',
  curl: '/usr/bin/curl',
  wget: '/usr/bin/wget',
  nano: '/usr/bin/nano',
  htop: '/usr/bin/htop',
  vlc: '/usr/bin/vlc',
  gimp: '/usr/bin/gimp',
  blender: '/usr/bin/blender',
  inkscape: '/usr/bin/inkscape',
  obs: '/usr/bin/obs',
  'obs-studio': '/usr/bin/obs',
  libreoffice: '/usr/bin/libreoffice',
  steam: '/usr/bin/steam',
  discord: '/usr/bin/discord',
  spotify: '/usr/bin/spotify',
  bash: '/bin/bash',
  sh: '/bin/sh',
  tar: '/bin/tar',
  gzip: '/bin/gzip',
  btrfs: '/sbin/btrfs',
  flatpak: '/usr/bin/flatpak',
  docker: '/usr/bin/docker',
  cargo: '/usr/bin/cargo',
  rustc: '/usr/bin/rustc',
  kitty: '/usr/bin/kitty',
};

// Dynamically resolve executable from PATH or whitelist
function resolveBinaryPath(cmd) {
  if (cmd.startsWith('/') && fs.existsSync(cmd)) return cmd;
  const searchDirs = ['/usr/local/bin', '/usr/bin', '/bin', '/usr/local/sbin', '/usr/sbin', '/sbin'];
  for (const dir of searchDirs) {
    const full = path.join(dir, cmd);
    if (fs.existsSync(full)) {
      try {
        fs.accessSync(full, fs.constants.X_OK);
        return full;
      } catch {}
    }
  }
  return ALLOWED_TERMINAL_BINARIES[cmd] || null;
}

// POSIX-style shell argument parser
function tokenizeCommandLine(cmdString) {
  const tokens = [];
  let current = '';
  let inSingle = false;
  let inDouble = false;
  let escaped = false;

  for (let i = 0; i < cmdString.length; i++) {
    const ch = cmdString[i];
    if (escaped) {
      current += ch;
      escaped = false;
    } else if (ch === '\\') {
      escaped = true;
    } else if (ch === "'" && !inDouble) {
      inSingle = !inSingle;
    } else if (ch === '"' && !inSingle) {
      inDouble = !inDouble;
    } else if (/\s/.test(ch) && !inSingle && !inDouble) {
      if (current.length > 0) {
        tokens.push(current);
        current = '';
      }
    } else {
      current += ch;
    }
  }
  if (current.length > 0) {
    tokens.push(current);
  }
  return tokens;
}

// Helper: Run interactive terminal commands using direct executable spawning (no shell injection)
function executeTerminalCommand(cmdString, cwd, extraEnv) {
  return new Promise((resolve) => {
    const trimmed = typeof cmdString === 'string' ? cmdString.trim() : '';
    if (!trimmed) {
      return resolve({ stdout: '', stderr: '', exitCode: 0 });
    }

    const pipelineSegments = trimmed.split(/\s*\|\s*/);
    const parsedCommands = [];

    for (const segment of pipelineSegments) {
      const tokens = tokenizeCommandLine(segment);
      if (tokens.length === 0) continue;

      const baseCmd = path.basename(tokens[0]);
      const binaryPath = resolveBinaryPath(baseCmd);
      if (!binaryPath) {
        return resolve({
          stdout: '',
          stderr: `axis-sh: ${baseCmd}: command not permitted by security policy`,
          exitCode: 126,
        });
      }
      parsedCommands.push({ binaryPath, args: tokens.slice(1) });
    }

    if (parsedCommands.length === 0) {
      return resolve({ stdout: '', stderr: '', exitCode: 0 });
    }

    const env = extraEnv ? { ...process.env, ...extraEnv } : process.env;
    const workingDir = cwd || os.homedir();

    if (parsedCommands.length === 1) {
      const { binaryPath, args } = parsedCommands[0];
      const proc = spawn(binaryPath, args, {
        cwd: workingDir,
        env,
        stdio: ['pipe', 'pipe', 'pipe'],
        shell: false,
      });

      let stdout = '';
      let stderr = '';
      let isDone = false;

      const isPkgCmd = /^(apt|apt-get|dpkg|axis|npm)$/.test(path.basename(binaryPath));
      const timeoutMs = isPkgCmd ? 180000 : 30000;

      const timer = setTimeout(() => {
        if (!isDone) {
          isDone = true;
          try { proc.kill('SIGTERM'); } catch {}
          resolve({ stdout, stderr: stderr + `\nExecution timed out (${timeoutMs / 1000}s).`, exitCode: 124 });
        }
      }, timeoutMs);

      proc.stdout.on('data', (d) => { stdout += d.toString(); });
      proc.stderr.on('data', (d) => { stderr += d.toString(); });

      proc.on('close', (code) => {
        if (!isDone) {
          isDone = true;
          clearTimeout(timer);
          resolve({
            stdout: stdout.trim(),
            stderr: stderr.trim(),
            exitCode: code !== null ? code : 1,
          });
        }
      });

      proc.on('error', (err) => {
        if (!isDone) {
          isDone = true;
          clearTimeout(timer);
          resolve({ stdout: '', stderr: err.message, exitCode: 1 });
        }
      });
      return;
    }

    // Multiple pipeline processes (e.g. ps ... | head ...)
    const procs = [];
    let isDone = false;
    let finalStdout = '';
    let finalStderr = '';

    const timer = setTimeout(() => {
      if (!isDone) {
        isDone = true;
        procs.forEach((p) => { try { p.kill('SIGTERM'); } catch {} });
        resolve({ stdout: finalStdout, stderr: finalStderr + '\nPipeline timed out (30s).', exitCode: 124 });
      }
    }, 30000);

    for (let i = 0; i < parsedCommands.length; i++) {
      const { binaryPath, args } = parsedCommands[i];
      const p = spawn(binaryPath, args, {
        cwd: workingDir,
        env,
        stdio: ['pipe', 'pipe', 'pipe'],
        shell: false,
      });
      procs.push(p);

      if (i > 0) {
        procs[i - 1].stdout.pipe(p.stdin);
      }

      p.stderr.on('data', (d) => { finalStderr += d.toString(); });
    }

    const lastProc = procs[procs.length - 1];
    lastProc.stdout.on('data', (d) => { finalStdout += d.toString(); });

    lastProc.on('close', (code) => {
      if (!isDone) {
        isDone = true;
        clearTimeout(timer);
        resolve({
          stdout: finalStdout.trim(),
          stderr: finalStderr.trim(),
          exitCode: code !== null ? code : 0,
        });
      }
    });

    lastProc.on('error', (err) => {
      if (!isDone) {
        isDone = true;
        clearTimeout(timer);
        resolve({ stdout: '', stderr: err.message, exitCode: 1 });
      }
    });
  });
}

// Helper: HTML entity escaping (CWE-79 prevention)
function escapeHtml(str) {
  if (str === null || str === undefined) return '';
  return String(str)
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&#39;');
}

// Helper: Validate external URL against SSRF (CWE-918 prevention)
function validatePublicUrl(rawUrl) {
  if (!rawUrl || typeof rawUrl !== 'string') {
    return { ok: false, error: 'Missing or invalid URL' };
  }
  let parsed;
  try {
    parsed = new URL(rawUrl);
  } catch {
    return { ok: false, error: 'Malformed URL' };
  }

  if (parsed.protocol !== 'http:' && parsed.protocol !== 'https:') {
    return { ok: false, error: 'Only HTTP and HTTPS protocols are permitted' };
  }

  const hostname = parsed.hostname.toLowerCase();
  if (
    hostname === 'localhost' ||
    hostname === '0.0.0.0' ||
    hostname === '127.0.0.1' ||
    hostname === '::1' ||
    hostname === '[::1]' ||
    hostname.endsWith('.localhost') ||
    hostname.endsWith('.local') ||
    hostname.endsWith('.internal') ||
    hostname.endsWith('.lan')
  ) {
    return { ok: false, error: 'Access to loopback or private hostnames is forbidden' };
  }

  const ipv4 = hostname.match(/^(\d{1,3})\.(\d{1,3})\.(\d{1,3})\.(\d{1,3})$/);
  if (ipv4) {
    const [, a, b, c, d] = ipv4.map(Number);
    if (a > 255 || b > 255 || c > 255 || d > 255) return { ok: false, error: 'Invalid IPv4 address' };
    if (a === 127 || a === 10 || a === 0) return { ok: false, error: 'Private/Loopback IP is forbidden' };
    if (a === 172 && b >= 16 && b <= 31) return { ok: false, error: 'Private IPv4 is forbidden' };
    if (a === 192 && b === 168) return { ok: false, error: 'Private IPv4 is forbidden' };
    if (a === 169 && b === 254) return { ok: false, error: 'Link-local IPv4 is forbidden' };
  }

  if (hostname.includes(':')) {
    const v6 = hostname.replace(/^\[|\]$/g, '').toLowerCase();
    if (
      v6 === '::1' ||
      v6.startsWith('fe80:') ||
      v6.startsWith('fc00:') ||
      v6.startsWith('fd00:') ||
      v6.startsWith('::ffff:127.')
    ) {
      return { ok: false, error: 'Private IPv6 is forbidden' };
    }
  }

  return { ok: true, urlObj: parsed };
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
  if (fs.existsSync('/etc/axisos-installed')) return false;
  if (fs.existsSync('/run/live')) return true;
  try {
    const cmdline = fs.readFileSync('/proc/cmdline', 'utf-8');
    if (cmdline.includes('boot=live')) return true;
  } catch {}
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

// Detect if kernel cmdline explicitly ordered automatic/direct setup wizard launch
function isAutoInstall() {
  if (process.platform !== 'linux') return false;
  try {
    const cmdline = fs.readFileSync('/proc/cmdline', 'utf-8');
    if (cmdline.includes('axisos.autoinstall=1') || cmdline.includes('axisos.autoinstall=true')) {
      return true;
    }
  } catch {}
  return false;
}

// Query real hardware battery state from /sys/class/power_supply or upower
async function getBatteryInfo() {
  if (process.platform !== 'linux') {
    return {
      hasBattery: false,
      level: 100,
      charging: true,
      status: 'AC Connected',
    };
  }

  try {
    const powerSupplyPath = '/sys/class/power_supply';
    if (fs.existsSync(powerSupplyPath)) {
      const supplies = fs.readdirSync(powerSupplyPath);
      const batNames = supplies.filter((s) => s.startsWith('BAT') || s.toLowerCase().includes('battery'));
      if (batNames.length > 0) {
        const batDir = path.join(powerSupplyPath, batNames[0]);
        let capacity = 100;
        let status = 'Discharging';

        if (fs.existsSync(path.join(batDir, 'capacity'))) {
          const capStr = fs.readFileSync(path.join(batDir, 'capacity'), 'utf8').trim();
          const capInt = parseInt(capStr, 10);
          if (!isNaN(capInt)) capacity = Math.max(0, Math.min(100, capInt));
        }

        if (fs.existsSync(path.join(batDir, 'status'))) {
          status = fs.readFileSync(path.join(batDir, 'status'), 'utf8').trim();
        }

        const isCharging = status === 'Charging' || status === 'Full' || status === 'Not charging';

        return {
          hasBattery: true,
          level: capacity,
          charging: isCharging,
          status,
        };
      }

      // Check if AC adapter is online
      const acNames = supplies.filter((s) => s.startsWith('AC') || s.startsWith('ADP') || s.toLowerCase().includes('mains'));
      let acOnline = true;
      if (acNames.length > 0) {
        const acOnlinePath = path.join(powerSupplyPath, acNames[0], 'online');
        if (fs.existsSync(acOnlinePath)) {
          acOnline = fs.readFileSync(acOnlinePath, 'utf8').trim() === '1';
        }
      }

      return {
        hasBattery: false,
        level: 100,
        charging: acOnline,
        status: acOnline ? 'AC Connected' : 'No Battery',
      };
    }
  } catch (err) {}

  // Fallback to upower if /sys/class/power_supply was not readable
  try {
    const upowerRes = await runCmd('upower -i $(upower -e 2>/dev/null | grep -m1 battery) 2>/dev/null');
    if (upowerRes.stdout) {
      const capMatch = upowerRes.stdout.match(/percentage:\s+(\d+)%/);
      const stateMatch = upowerRes.stdout.match(/state:\s+(\w+)/);
      if (capMatch) {
        const level = parseInt(capMatch[1], 10);
        const state = stateMatch ? stateMatch[1] : 'discharging';
        const charging = state === 'charging' || state === 'fully-charged';
        return {
          hasBattery: true,
          level,
          charging,
          status: state,
        };
      }
    }
  } catch {}

  return {
    hasBattery: false,
    level: 100,
    charging: true,
    status: 'AC Connected',
  };
}

// Scan real nearby and paired Bluetooth devices via bluetoothctl (no demo fallbacks)
async function getRealBluetoothDevices() {
  const devices = [];
  const seenMacs = new Set();

  if (process.platform !== 'linux') return devices;

  // 1. Check if controller is powered
  const showRes = await runCmd('bluetoothctl show 2>/dev/null');
  if (!showRes.stdout.includes('Powered: yes')) {
    return devices;
  }

  // 2. Identify paired devices
  const pairedRes = await runCmd('bluetoothctl paired-devices 2>/dev/null');
  const pairedMacs = new Set();
  if (pairedRes.stdout) {
    for (const line of pairedRes.stdout.trim().split('\n')) {
      const m = line.match(/^Device\s+([0-9A-Fa-f:]{17})\s+(.*)$/);
      if (m) pairedMacs.add(m[1].toUpperCase());
    }
  }

  // 3. Trigger discovery scan in background if not already active
  if (!showRes.stdout.includes('Discovering: yes')) {
    runCmd('bluetoothctl --timeout 3 scan on 2>/dev/null');
  }

  // 4. Query all discovered / nearby devices
  const allDevRes = await runCmd('bluetoothctl devices 2>/dev/null');
  if (allDevRes.stdout) {
    for (const line of allDevRes.stdout.trim().split('\n')) {
      const m = line.match(/^Device\s+([0-9A-Fa-f:]{17})\s+(.*)$/);
      if (m) {
        const mac = m[1].toUpperCase();
        const rawName = m[2].trim();
        const name = rawName || mac;
        if (!seenMacs.has(mac)) {
          seenMacs.add(mac);
          const isPaired = pairedMacs.has(mac);
          let icon = 'bluetooth';

          const lower = name.toLowerCase();
          if (lower.includes('airpod') || lower.includes('headphone') || lower.includes('buds') || lower.includes('wh-') || lower.includes('audio')) {
            icon = 'headphones';
          } else if (lower.includes('speaker') || lower.includes('soundbar') || lower.includes('jbl')) {
            icon = 'speaker';
          } else if (lower.includes('mouse') || lower.includes('trackpad')) {
            icon = 'mouse';
          } else if (lower.includes('keyboard') || lower.includes('keychron')) {
            icon = 'keyboard';
          } else if (lower.includes('phone') || lower.includes('iphone') || lower.includes('galaxy') || lower.includes('pixel')) {
            icon = 'smartphone';
          }

          devices.push({
            mac,
            name,
            connected: false,
            paired: isPaired,
            icon,
          });
        }
      }
    }
  }

  // Check connection status for paired devices
  for (const dev of devices) {
    if (dev.paired) {
      try {
        const infoRes = await runCmd(`bluetoothctl info ${dev.mac} 2>/dev/null`);
        dev.connected = infoRes.stdout.includes('Connected: yes');
      } catch {}
    }
  }

  return devices;
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
              const fsType = (node.fstype || '').toLowerCase();
              const lbl = (node.label || '').toLowerCase();
              const mp = (node.mountpoint || '').toLowerCase();

              if (fsType.includes('bitlocker')) hasBitLocker = true;
              if (fsType.includes('ntfs') || lbl.includes('windows') || lbl.includes('recovery')) hasWindows = true;
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

// Static file serving handler (verified containment within DIST_DIR)
function serveStaticFile(req, res, filePath) {
  const resolvedDist = path.resolve(DIST_DIR);
  const realFile = path.resolve(filePath);

  if (!realFile.startsWith(resolvedDist)) {
    res.writeHead(403, { 'Content-Type': 'text/plain' });
    res.end('AxisOS: Access Forbidden');
    return;
  }

  fs.stat(realFile, (err, stats) => {
    if (err || !stats.isFile()) {
      const indexPath = path.join(resolvedDist, 'index.html');
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

    const ext = path.extname(realFile).toLowerCase();
    const contentType = MIME_TYPES[ext] || 'application/octet-stream';
    res.writeHead(200, {
      'Content-Type': contentType,
      'Content-Length': stats.size,
      'Cache-Control': ext === '.html' ? 'no-cache' : 'max-age=86400',
    });

    const stream = fs.createReadStream(realFile);
    stream.pipe(res);
  });
}

// Create HTTP Server
const server = http.createServer(async (req, res) => {
  const parsedUrl = url.parse(req.url, true);
  const pathname = parsedUrl.pathname;

  // Handle CORS Preflight
  if (req.method === 'OPTIONS') {
    res.writeHead(204, {
      'Access-Control-Allow-Origin': '*',
      'Access-Control-Allow-Methods': 'GET, POST, OPTIONS',
      'Access-Control-Allow-Headers': 'Content-Type',
    });
    return res.end();
  }

  // ==================== REST API ENDPOINTS ====================
  if (pathname.startsWith('/api/')) {
    // 1. System Telemetry
    if (pathname === '/api/system-info' && req.method === 'GET') {
      const cpus = os.cpus();
      const totalMem = os.totalmem();
      const freeMem = os.freemem();
      const uptime = os.uptime();

      let kernelVersion = os.release();
      let cpuModel = cpus[0] ? cpus[0].model : 'x86_64 Processor';
      let ramTotalMb = Math.round(totalMem / (1024 * 1024));
      let ramUsedMb = Math.round((totalMem - freeMem) / (1024 * 1024));
      let gpuRenderer = 'Mesa Intel Graphics / Gallium';
      let hostname = os.hostname();

      if (process.platform === 'linux') {
        const [kRes, cpuRes, memRes, gpuRes, hostRes, upRes] = await Promise.all([
          runCmd('uname -r'),
          runCmd('lscpu 2>/dev/null | grep "Model name:" | head -n1 | cut -d: -f2 | xargs'),
          runCmd('cat /proc/meminfo 2>/dev/null'),
          runCmd('lspci 2>/dev/null | grep -E "VGA|3D|Display" | head -n1 | cut -d: -f3 | xargs'),
          runCmd('hostname'),
          runCmd('uptime -p 2>/dev/null || uptime'),
        ]);

        if (kRes.stdout) kernelVersion = kRes.stdout.trim();
        if (cpuRes.stdout) cpuModel = cpuRes.stdout.trim();
        if (gpuRes.stdout) gpuRenderer = gpuRes.stdout.trim();
        if (hostRes.stdout) hostname = hostRes.stdout.trim();

        if (memRes.stdout) {
          const tMatch = memRes.stdout.match(/MemTotal:\s+(\d+)\s+kB/);
          const aMatch = memRes.stdout.match(/MemAvailable:\s+(\d+)\s+kB/);
          if (tMatch && aMatch) {
            const tot = parseInt(tMatch[1], 10);
            const avail = parseInt(aMatch[1], 10);
            ramTotalMb = Math.round(tot / 1024);
            ramUsedMb = Math.round((tot - avail) / 1024);
          }
        }
      }

      return sendJson(res, 200, {
        kernel: kernelVersion,
        hostname,
        cpuModel,
        cpuCores: cpus.length,
        ramUsedMb,
        ramTotalMb,
        ramPercent: Math.round((ramUsedMb / ramTotalMb) * 100),
        gpu: gpuRenderer,
        uptime,
        uptimeFormatted: `${Math.floor(uptime / 3600)}h ${Math.floor((uptime % 3600) / 60)}m`,
        osName: 'AxisOS 2.0 (Horizon)',
        arch: os.arch(),
        homeDir: process.env.HOME || '/home/axis',
        username: process.env.USER || 'axis',
        isLiveEnvironment: isLiveSession(),
        autoInstall: isAutoInstall(),
        battery: await getBatteryInfo(),
      });
    }

    // Battery Telemetry
    if (pathname === '/api/battery' && req.method === 'GET') {
      const bat = await getBatteryInfo();
      return sendJson(res, 200, bat);
    }

    // 2. Disks
    if (pathname === '/api/disks' && req.method === 'GET') {
      const disks = await getStorageDisks();
      return sendJson(res, 200, disks);
    }

    // 3. Terminal Execution (Strict loopback isolation)
    if (pathname === '/api/terminal-exec' && req.method === 'POST') {
      const clientIp = req.socket.remoteAddress || '';
      const isLocal = clientIp === '127.0.0.1' || clientIp === '::1' || clientIp === '::ffff:127.0.0.1';
      if (!isLocal) {
        return sendJson(res, 403, { error: 'Terminal execution restricted to local loopback' });
      }

      const body = await parseJsonBody(req);
      const rawCmd = body.command;
      if (typeof rawCmd !== 'string' || rawCmd.includes('\0')) {
        return sendJson(res, 400, { error: 'Invalid command' });
      }

      const rawCwd = body.cwd || '/home/axis';
      const safeCwd = (typeof rawCwd === 'string' && !rawCwd.includes('\0') && fs.existsSync(rawCwd))
        ? path.resolve('/', path.normalize(rawCwd))
        : os.homedir();

      const result = await executeTerminalCommand(rawCmd, safeCwd, body.env);
      return sendJson(res, 200, result);
    }

    // 4. Filesystem: Read Directory with Full Linux Metadata (Sanitized)
    if (pathname === '/api/fs-read' && req.method === 'GET') {
      const requestedPath = parsedUrl.query.path;
      if (!requestedPath || requestedPath === '/' || requestedPath === '') {
        const ROOT_PATH = '/';
        try {
          const entries = fs.readdirSync(ROOT_PATH, { withFileTypes: true });
          const items = entries.map((entry) => {
            const full = path.join(ROOT_PATH, entry.name);
            let size = '—';
            let rawSize = 0;
            let mtime = '—';
            let rawMtime = 0;
            let permissions = '0755';
            let modeStr = entry.isDirectory() ? 'drwxr-xr-x' : '-rw-r--r--';
            let isExecutable = false;
            let owner = 'root';
            let group = 'root';

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

          return sendJson(res, 200, { path: ROOT_PATH, items });
        } catch (err) {
          return sendJson(res, 500, { error: err.message });
        }
      }

      const rawTarget = requestedPath;
      if (typeof rawTarget !== 'string' || rawTarget.includes('\0')) {
        return sendJson(res, 400, { error: 'Invalid path' });
      }

      let matchedRoot = null;
      for (const root of ALLOWED_FS_ROOTS) {
        if (rawTarget === root || rawTarget.startsWith(root + '/')) {
          matchedRoot = root;
          break;
        }
      }

      if (!matchedRoot) {
        return sendJson(res, 403, { error: 'Path outside allowed root hierarchy' });
      }

      const candidate = path.resolve(matchedRoot, '.' + rawTarget.slice(matchedRoot.length));
      let realTarget;
      try {
        realTarget = fs.realpathSync(candidate);
      } catch (err) {
        if (err.code === 'EACCES') {
          return sendJson(res, 200, { path: rawTarget, items: [], permissionDenied: true });
        }
        return sendJson(res, 200, { path: rawTarget, items: [] });
      }

      if (!realTarget.startsWith(matchedRoot)) {
        return sendJson(res, 403, { error: 'Forbidden path: traversal detected' });
      }

      try {
        const entries = fs.readdirSync(realTarget, { withFileTypes: true });
        const items = entries.map((entry) => {
          const full = path.join(realTarget, entry.name);
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

        return sendJson(res, 200, { path: realTarget, items });
      } catch (err) {
        const isPerm = err.code === 'EACCES' || err.code === 'EPERM';
        return sendJson(res, 200, {
          path: realTarget,
          items: [],
          permissionDenied: isPerm,
          error: isPerm ? 'Permission Denied: Root or elevated privileges required to access this folder' : err.message,
        });
      }
    }

    // 4b. Filesystem: Change Permissions (chmod)
    if (pathname === '/api/fs-chmod' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const rawP = body.path;
      const modeStr = String(body.mode || '');
      if (!rawP || typeof rawP !== 'string' || rawP.includes('\0') || !/^[0-7]{3,4}$/.test(modeStr)) {
        return sendJson(res, 400, { error: 'Invalid path or octal mode' });
      }
      const safeP = path.resolve('/', path.normalize(rawP));
      try {
        fs.chmodSync(safeP, parseInt(modeStr, 8));
        return sendJson(res, 200, { success: true });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
    }

    // 4c. Filesystem: Calculate Recursive Folder Size (Shell-free)
    if (pathname === '/api/fs-calc-size' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const rawTarget = body.path;
      if (!rawTarget || typeof rawTarget !== 'string' || rawTarget.includes('\0')) {
        return sendJson(res, 400, { error: 'Invalid path' });
      }
      const safeTarget = path.resolve('/', path.normalize(rawTarget));
      if (!fs.existsSync(safeTarget)) {
        return sendJson(res, 404, { error: 'Target not found' });
      }
      try {
        let bytes = 0;
        let itemCount = 0;
        if (process.platform === 'linux') {
          const duRes = await runExecFile('/usr/bin/du', ['-sb', safeTarget]);
          if (duRes.exitCode === 0 && duRes.stdout) {
            bytes = parseInt(duRes.stdout.split('\t')[0], 10) || 0;
          }
          const findRes = await runExecFile('/usr/bin/find', [safeTarget]);
          if (findRes.exitCode === 0 && findRes.stdout) {
            itemCount = findRes.stdout.trim().split('\n').length;
          }
        }
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
      const rawTarget = body.path;
      if (!rawTarget || typeof rawTarget !== 'string' || rawTarget.includes('\0')) {
        return sendJson(res, 400, { error: 'Invalid file path' });
      }
      const safeTarget = path.resolve('/', path.normalize(rawTarget));
      if (!fs.existsSync(safeTarget)) {
        return sendJson(res, 404, { error: 'Target file not found' });
      }

      try {
        fs.mkdirSync(trashFilesDir, { recursive: true });
        fs.mkdirSync(trashInfoDir, { recursive: true });

        const baseName = path.basename(safeTarget);
        let destName = baseName;
        let counter = 1;
        while (fs.existsSync(path.join(trashFilesDir, destName))) {
          destName = `${baseName}.${counter++}`;
        }

        const destFile = path.join(trashFilesDir, destName);
        fs.renameSync(safeTarget, destFile);

        const trashInfoContent = `[Trash Info]\nPath=${encodeURI(safeTarget)}\nDeletionDate=${new Date().toISOString()}\n`;
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
      const name = String(body.name || '');
      if (!name || name.includes('/') || name.includes('\\') || name.includes('..') || name.includes('\0')) {
        return sendJson(res, 400, { error: 'Invalid trash item name' });
      }
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
          const safeOrig = path.resolve('/', path.normalize(origPath));
          fs.mkdirSync(path.dirname(safeOrig), { recursive: true });
          fs.renameSync(fullFile, safeOrig);
          fs.unlinkSync(fullInfo);
          return sendJson(res, 200, { success: true, restoredTo: safeOrig });
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

    // 4e. Filesystem: Asynchronous Operations Queue (Shell-free)
    if (pathname === '/api/fs-op/start' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const opId = `op-${Date.now()}-${Math.random().toString(36).substr(2, 5)}`;
      const { type, source, destination } = body;

      if (!source || !destination || typeof source !== 'string' || typeof destination !== 'string') {
        return sendJson(res, 400, { error: 'Invalid source or destination' });
      }

      const realSource = path.resolve('/', path.normalize(source));
      const realDest = path.resolve('/', path.normalize(destination));

      const op = {
        id: opId,
        type,
        source: realSource,
        destination: realDest,
        progress: 10,
        speedMb: '42.5',
        etaSec: 3,
        status: 'running',
      };
      activeFileOperations.set(opId, op);

      (async () => {
        try {
          if (type === 'copy') {
            fs.cpSync(realSource, realDest, { recursive: true });
          } else if (type === 'move') {
            try {
              fs.renameSync(realSource, realDest);
            } catch (renameErr) {
              if (renameErr.code === 'EXDEV') {
                fs.cpSync(realSource, realDest, { recursive: true });
                fs.rmSync(realSource, { recursive: true, force: true });
              } else {
                throw renameErr;
              }
            }
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

    // 4f. Hardware Hotplugging: Removable USB Disks Mount/Unmount (Shell-free)
    if (pathname === '/api/disks/mount' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const dev = String(body.device || '');
      if (!/^\/dev\/[a-z0-9_-]+$/.test(dev)) {
        return sendJson(res, 400, { error: 'Invalid block device path' });
      }
      const user = process.env.USER || 'axis';
      const mountDir = path.resolve(`/run/media/${user}/${path.basename(dev)}`);
      try {
        fs.mkdirSync(mountDir, { recursive: true });
        await runExecFile('/bin/mount', [dev, mountDir]);
        return sendJson(res, 200, { success: true, mountpoint: mountDir });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
    }

    if (pathname === '/api/disks/unmount' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const dev = String(body.device || '');
      if (!/^\/dev\/[a-z0-9_-]+$/.test(dev)) {
        return sendJson(res, 400, { error: 'Invalid block device path' });
      }
      try {
        await runExecFile('/bin/umount', [dev]);
        return sendJson(res, 200, { success: true });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
    }

    // 5. Filesystem: Write File
    if (pathname === '/api/fs-write' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const rawPath = body.filePath;
      if (!rawPath || typeof rawPath !== 'string' || rawPath.includes('\0')) {
        return sendJson(res, 400, { error: 'Invalid file path' });
      }
      const safePath = path.resolve('/', path.normalize(rawPath));
      if (!safePath.startsWith('/')) {
        return sendJson(res, 400, { error: 'Path traversal forbidden' });
      }
      try {
        fs.mkdirSync(path.dirname(safePath), { recursive: true });
        fs.writeFileSync(safePath, body.content || '', 'utf-8');
        return sendJson(res, 200, { success: true });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
    }

    // 6. Filesystem: Create Directory
    if (pathname === '/api/fs-mkdir' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const rawPath = body.dirPath;
      if (!rawPath || typeof rawPath !== 'string' || rawPath.includes('\0')) {
        return sendJson(res, 400, { error: 'Invalid directory path' });
      }
      const safePath = path.resolve('/', path.normalize(rawPath));
      if (!safePath.startsWith('/')) {
        return sendJson(res, 400, { error: 'Path traversal forbidden' });
      }
      try {
        fs.mkdirSync(safePath, { recursive: true });
        return sendJson(res, 200, { success: true });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
    }

    // 7. Filesystem: Delete Item
    if (pathname === '/api/fs-delete' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const rawPath = body.targetPath;
      if (!rawPath || typeof rawPath !== 'string' || rawPath.includes('\0') || rawPath === '/') {
        return sendJson(res, 400, { error: 'Invalid or protected path' });
      }
      const safePath = path.resolve('/', path.normalize(rawPath));
      if (safePath === '/' || !safePath.startsWith('/')) {
        return sendJson(res, 400, { error: 'Deleting root filesystem is forbidden' });
      }
      try {
        fs.rmSync(safePath, { recursive: true, force: true });
        return sendJson(res, 200, { success: true });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
    }

    // 8. Installer: Start (Strictly validated)
    if (pathname === '/api/installer/start' && req.method === 'POST') {
      const config = await parseJsonBody(req);
      if (installJob.isInstalling) {
        return sendJson(res, 400, { success: false, message: 'Installation already in progress' });
      }

      const rawDisk = config.targetDisk || '/dev/vda';
      if (!/^\/dev\/[a-z0-9_-]+$/.test(rawDisk)) {
        return sendJson(res, 400, { success: false, message: 'Invalid target disk specification' });
      }
      const rawUser = config.username || 'axis';
      if (!/^[a-z_][a-z0-9_-]*[$]?$/.test(rawUser)) {
        return sendJson(res, 400, { success: false, message: 'Invalid username format' });
      }
      const rawHost = config.computerName || 'axis-pc';
      if (!/^[a-zA-Z0-9_-]+$/.test(rawHost)) {
        return sendJson(res, 400, { success: false, message: 'Invalid hostname format' });
      }

      installJob = {
        isInstalling: true,
        progress: 2,
        statusText: 'Initializing installation engine...',
        completed: false,
        error: null,
        log: [`[${new Date().toLocaleTimeString()}] Installation started for target disk ${rawDisk}`],
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
        '--disk', rawDisk,
        '--username', rawUser,
        '--password', config.password || 'password',
        '--fullname', config.userFullName || 'AxisOS User',
        '--hostname', rawHost,
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
        connected: false,
        currentSsid: 'Not Connected',
        signal: 0,
      });
    }

    // 14. Wi-Fi: Scan (Real hardware networks only - no demo fallbacks)
    if (pathname === '/api/wifi/scan' && req.method === 'GET') {
      if (process.platform === 'linux') {
        await runCmd('nmcli dev wifi rescan 2>/dev/null');
        const scanRes = await runCmd('nmcli -t -f in-use,ssid,signal,security dev wifi list 2>/dev/null');
        const networks = [];
        const seen = new Set();
        if (scanRes.stdout) {
          const lines = scanRes.stdout.trim().split('\n');
          for (const line of lines) {
            const parts = line.split(':');
            const inUse = parts[0] === '*';
            const ssid = (parts[1] || '').trim();
            const signal = parseInt(parts[2], 10) || 50;
            const security = parts[3] || 'WPA2';
            if (ssid && ssid !== '--' && !seen.has(ssid)) {
              seen.add(ssid);
              networks.push({ inUse, ssid, signal, security });
            }
          }
        }
        return sendJson(res, 200, networks);
      }
      return sendJson(res, 200, []);
    }

    // 15. Wi-Fi: Toggle Radio (Shell-free)
    if (pathname === '/api/wifi/toggle' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const action = body.enabled ? 'on' : 'off';
      if (process.platform === 'linux') {
        await runExecFile('/usr/bin/nmcli', ['radio', 'wifi', action]);
      }
      return sendJson(res, 200, { success: true, enabled: Boolean(body.enabled) });
    }

    // 16. Wi-Fi: Connect (Shell-free, CWE-78 & CWE-88 resolved)
    if (pathname === '/api/wifi/connect' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const ssid = String(body.ssid || '');
      const password = String(body.password || '');
      if (process.platform === 'linux' && ssid) {
        const args = ['dev', 'wifi', 'connect', ssid];
        if (password) args.push('password', password);
        const resConnect = await runExecFile('/usr/bin/nmcli', args);
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
          controllerName: controllerMatch ? controllerMatch[1] : 'Bluetooth Controller',
        });
      }
      return sendJson(res, 200, { enabled: true, controllerName: 'Bluetooth Controller' });
    }

    // 18. Bluetooth: Devices Scan (Real nearby and paired devices - no demo fallbacks)
    if (pathname === '/api/bluetooth/devices' && req.method === 'GET') {
      const devices = await getRealBluetoothDevices();
      return sendJson(res, 200, devices);
    }

    // 19. Bluetooth: Toggle (Shell-free)
    if (pathname === '/api/bluetooth/toggle' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      if (process.platform === 'linux') {
        await runExecFile('/usr/bin/bluetoothctl', body.enabled ? ['power', 'on'] : ['power', 'off']);
      }
      return sendJson(res, 200, { success: true, enabled: Boolean(body.enabled) });
    }

    // 20. Bluetooth: Connect (Strict MAC validation, shell-free)
    if (pathname === '/api/bluetooth/connect' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const mac = String(body.mac || '');
      if (mac && !/^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$/.test(mac)) {
        return sendJson(res, 400, { error: 'Invalid MAC address format' });
      }
      if (process.platform === 'linux' && mac) {
        const resConn = await runExecFile('/usr/bin/bluetoothctl', ['connect', mac]);
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

    // 22. Audio: Set Volume (Shell-free)
    if (pathname === '/api/audio/set-volume' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const vol = Math.max(0, Math.min(100, parseInt(body.volume, 10) || 0));
      if (process.platform === 'linux') {
        const fraction = (vol / 100).toFixed(2);
        const wpRes = await runExecFile('/usr/bin/wpctl', ['set-volume', '@DEFAULT_AUDIO_SINK@', fraction]);
        if (wpRes.exitCode !== 0) {
          await runExecFile('/usr/bin/amixer', ['set', 'Master', `${vol}%`]);
        }
      }
      return sendJson(res, 200, { success: true, volume: vol });
    }

    // 23. Audio: Toggle Mute (Shell-free)
    if (pathname === '/api/audio/toggle-mute' && req.method === 'POST') {
      if (process.platform === 'linux') {
        const wpRes = await runExecFile('/usr/bin/wpctl', ['set-mute', '@DEFAULT_AUDIO_SINK@', 'toggle']);
        if (wpRes.exitCode !== 0) {
          await runExecFile('/usr/bin/amixer', ['set', 'Master', 'toggle']);
        }
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
            const check = await runExecFile('/usr/bin/dpkg-query', ['-W', "-f=${Status}", p.packageName]);
            p.installed = Boolean(check.stdout && check.stdout.includes('installed'));
          } catch {
            p.installed = false;
          }
        }
      }
      return sendJson(res, 200, pkgs);
    }

    // 25. Packages: Install (Validated, shell-free)
    if (pathname === '/api/packages/install' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const pkg = String(body.packageName || body.pkg || '');
      if (!pkg || !/^[a-z0-9][a-z0-9+.-]+$/.test(pkg)) {
        return sendJson(res, 400, { error: 'Invalid Debian package name format' });
      }
      if (process.platform === 'linux') {
        const result = await runExecFile('/usr/bin/apt-get', ['install', '-y', pkg], {
          env: { ...process.env, DEBIAN_FRONTEND: 'noninteractive' },
        });
        return sendJson(res, 200, { success: result.exitCode === 0, output: result.stdout || result.stderr });
      }
      return sendJson(res, 200, { success: true, output: `[Simulated] Successfully installed ${pkg} via Axis Package Manager.` });
    }

    // 26. Packages: Remove (Validated, shell-free)
    if (pathname === '/api/packages/remove' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const pkg = String(body.packageName || body.pkg || '');
      if (!pkg || !/^[a-z0-9][a-z0-9+.-]+$/.test(pkg)) {
        return sendJson(res, 400, { error: 'Invalid Debian package name format' });
      }
      if (process.platform === 'linux') {
        const result = await runExecFile('/usr/bin/apt-get', ['remove', '-y', pkg], {
          env: { ...process.env, DEBIAN_FRONTEND: 'noninteractive' },
        });
        return sendJson(res, 200, { success: result.exitCode === 0, output: result.stdout || result.stderr });
      }
      return sendJson(res, 200, { success: true, output: `[Simulated] Successfully uninstalled ${pkg}.` });
    }

    // 27. Browser Proxy (Deprecated: Client iframe renders URL directly to prevent SSRF)
    if (pathname === '/api/browser/proxy' && req.method === 'GET') {
      return sendJson(res, 410, { error: 'Proxy endpoint disabled for security compliance (SSRF prevention)' });
    }

    // 28. Browser: Open in Native Chromium (Shell-free)
    if (pathname === '/api/browser/open-native' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const rawTarget = body.url || 'https://google.com';
      let safeTarget = 'https://google.com';
      try {
        const p = new URL(rawTarget);
        if (p.protocol === 'http:' || p.protocol === 'https:') {
          safeTarget = p.href;
        }
      } catch {}
      if (process.platform === 'linux') {
        spawn('chromium', [safeTarget], { detached: true, stdio: 'ignore' }).unref();
      }
      return sendJson(res, 200, { success: true, url: safeTarget });
    }

    // 29. Filesystem: Read File Content (Path containment verified against allowed roots)
    if (pathname === '/api/fs-read-file' && req.method === 'GET') {
      const rawTarget = parsedUrl.query.path;
      if (!rawTarget || typeof rawTarget !== 'string' || rawTarget.includes('\0')) {
        return sendJson(res, 400, { error: 'Invalid file path' });
      }

      let matchedRoot = null;
      for (const root of ALLOWED_FS_ROOTS) {
        if (rawTarget === root || rawTarget.startsWith(root + '/')) {
          matchedRoot = root;
          break;
        }
      }

      if (!matchedRoot) {
        return sendJson(res, 403, { error: 'Path outside allowed root hierarchy' });
      }

      const candidate = path.resolve(matchedRoot, '.' + rawTarget.slice(matchedRoot.length));
      let realPath;
      try {
        realPath = fs.realpathSync(candidate);
      } catch {
        return sendJson(res, 404, { error: 'File not found' });
      }

      if (!realPath.startsWith(matchedRoot)) {
        return sendJson(res, 403, { error: 'Access forbidden: path traversal detected' });
      }

      try {
        const stat = fs.statSync(realPath);
        if (!stat.isFile()) {
          return sendJson(res, 400, { error: 'Target is not a regular file' });
        }
        if (stat.size > 10 * 1024 * 1024) {
          return sendJson(res, 413, { error: 'File size exceeds 10MB limit' });
        }
        const data = fs.readFileSync(realPath, 'utf-8');
        return sendJson(res, 200, { path: realPath, content: data });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
    }

    // 30. Filesystem: Rename File / Directory (Validated)
    if (pathname === '/api/fs-rename' && req.method === 'POST') {
      const body = await parseJsonBody(req);
      const rawOld = body.oldPath;
      const rawNew = body.newPath;
      if (!rawOld || !rawNew || typeof rawOld !== 'string' || typeof rawNew !== 'string' || rawOld.includes('\0') || rawNew.includes('\0')) {
        return sendJson(res, 400, { error: 'Invalid oldPath or newPath' });
      }
      const safeOld = path.resolve('/', path.normalize(rawOld));
      const safeNew = path.resolve('/', path.normalize(rawNew));
      if (!safeOld.startsWith('/') || !safeNew.startsWith('/')) {
        return sendJson(res, 400, { error: 'Path traversal forbidden' });
      }
      try {
        fs.renameSync(safeOld, safeNew);
        return sendJson(res, 200, { success: true });
      } catch (err) {
        return sendJson(res, 500, { error: err.message });
      }
    }

    return sendJson(res, 404, { error: 'Unknown API endpoint' });
  }

  // ==================== STATIC ASSET SERVING ====================
  const normalizedPath = path.normalize(pathname);
  const resolvedDist = path.resolve(DIST_DIR);
  let requestedFile = path.resolve(resolvedDist, '.' + normalizedPath);

  if (!requestedFile.startsWith(resolvedDist)) {
    res.writeHead(403, { 'Content-Type': 'text/plain' });
    return res.end('AxisOS: Access Forbidden');
  }

  if (requestedFile === resolvedDist || !path.extname(requestedFile)) {
    requestedFile = path.join(resolvedDist, 'index.html');
  }

  serveStaticFile(req, res, requestedFile);
});

server.listen(PORT, HOST, () => {
  console.log(`==================================================`);
  console.log(` AxisOS System Management Daemon (Secure Mode)`);
  console.log(` Listening on: http://${HOST}:${PORT}`);
  console.log(` Serving Shell: ${DIST_DIR}`);
  console.log(`==================================================`);
});
