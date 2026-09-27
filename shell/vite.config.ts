import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import tailwindcss from '@tailwindcss/vite'
import { exec, spawn } from 'child_process'
import fs from 'fs'
import path from 'path'
import os from 'os'

// State for active installer jobs during dev server testing
let devInstallJob = {
  isInstalling: false,
  progress: 0,
  statusText: 'Idle',
  completed: false,
  error: null as string | null,
  log: [] as string[],
};

// Probe real disk drives
async function probeDisks(): Promise<any[]> {
  const runCmd = (cmd: string): Promise<string> =>
    new Promise((resolve) => {
      exec(cmd, { timeout: 3000 }, (err, stdout) => {
        resolve(err ? '' : stdout.trim());
      });
    });

  const lsblk = await runCmd('lsblk -d -J -b -o NAME,SIZE,TYPE,MODEL,TRAN,RO 2>/dev/null');
  if (lsblk) {
    try {
      const parsed = JSON.parse(lsblk);
      if (parsed.blockdevices && Array.isArray(parsed.blockdevices)) {
        const disks = parsed.blockdevices
          .filter((d: any) => d.type === 'disk' && !d.name.startsWith('loop') && !d.name.startsWith('zram'))
          .map((d: any) => {
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

// Custom plugin to provide real system hardware info, command execution, and installer API
function linuxSystemApiPlugin() {
  return {
    name: 'linux-system-api',
    configureServer(server: any) {
      // API: Query real system hardware and specs
      server.middlewares.use('/api/system-info', async (req: any, res: any) => {
        if (req.method !== 'GET') {
          res.statusCode = 405;
          return res.end();
        }

        try {
          const runCmd = (cmd: string): Promise<string> =>
            new Promise((resolve) => {
              exec(cmd, { timeout: 3000 }, (err, stdout) => {
                resolve(err ? '' : stdout.trim());
              });
            });

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
          if (meminfo) {
            const memMatch = meminfo.match(/MemTotal:\s+(\d+)\s+kB/);
            if (memMatch) {
              totalMemGb = (parseInt(memMatch[1], 10) / 1024 / 1024).toFixed(1);
            }
          }

          const disks = await probeDisks();

          const responseData = {
            osName: 'AxisOS Linux 1.0',
            osVersion: 'Horizon (Sonoma Edition)',
            kernelVersion: uname || os.release(),
            architecture: os.arch(),
            cpuModel: cpuModel || os.cpus()[0]?.model || '64-bit Processor',
            cpuCores: os.cpus().length,
            gpuModel: lspci || 'Intel Integrated Graphics',
            totalMemory: `${totalMemGb} GB Unified Memory`,
            freeMemory: `${freeMemGb} GB Available`,
            storageDevices: disks,
            hostname: hostname || os.hostname(),
            uptime: uptime || 'up 1 hour',
            username: process.env.USER || os.userInfo().username || 'axis',
            homeDir: process.env.HOME || os.homedir(),
          };

          res.setHeader('Content-Type', 'application/json');
          res.end(JSON.stringify(responseData));
        } catch (err: any) {
          res.statusCode = 500;
          res.end(JSON.stringify({ error: err.message }));
        }
      });

      // API: Probe real storage disks
      server.middlewares.use('/api/disks', async (req: any, res: any) => {
        if (req.method !== 'GET') {
          res.statusCode = 405;
          return res.end();
        }
        try {
          const disks = await probeDisks();
          res.setHeader('Content-Type', 'application/json');
          res.end(JSON.stringify(disks));
        } catch (err: any) {
          res.statusCode = 500;
          res.end(JSON.stringify({ error: err.message }));
        }
      });

      // API: Execute real terminal bash commands
      server.middlewares.use('/api/terminal-exec', (req: any, res: any) => {
        if (req.method !== 'POST') {
          res.statusCode = 405;
          return res.end();
        }

        let body = '';
        req.on('data', (chunk: any) => {
          body += chunk;
        });

        req.on('end', () => {
          try {
            const { command, cwd } = JSON.parse(body || '{}');
            if (!command) {
              res.setHeader('Content-Type', 'application/json');
              return res.end(JSON.stringify({ stdout: '', stderr: '', exitCode: 0 }));
            }

            const workingDir = cwd || os.homedir();
            exec(command, { cwd: workingDir, timeout: 10000, maxBuffer: 4 * 1024 * 1024 }, (error, stdout, stderr) => {
              res.setHeader('Content-Type', 'application/json');
              res.end(
                JSON.stringify({
                  stdout: stdout || '',
                  stderr: stderr || (error ? error.message : ''),
                  exitCode: error ? (error.code || 1) : 0,
                  cwd: workingDir,
                })
              );
            });
          } catch (e: any) {
            res.statusCode = 400;
            res.end(JSON.stringify({ error: e.message }));
          }
        });
      });

      // API: Read directory files
      server.middlewares.use('/api/fs-read', (req: any, res: any) => {
        const url = new URL(req.url, 'http://localhost');
        const targetPath = url.searchParams.get('path') || os.homedir();

        try {
          if (!fs.existsSync(targetPath)) {
            res.statusCode = 404;
            return res.end(JSON.stringify({ error: 'Path not found', items: [] }));
          }

          const entries = fs.readdirSync(targetPath, { withFileTypes: true });
          const items = entries.map((entry) => {
            const fullPath = path.join(targetPath, entry.name);
            let size = '—';
            let mtime = '—';

            try {
              const stat = fs.statSync(fullPath);
              if (!entry.isDirectory()) {
                const bytes = stat.size;
                if (bytes < 1024) size = `${bytes} B`;
                else if (bytes < 1024 * 1024) size = `${(bytes / 1024).toFixed(1)} KB`;
                else size = `${(bytes / (1024 * 1024)).toFixed(1)} MB`;
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
              fullPath,
              type: entry.isDirectory() ? 'folder' : 'file',
              size,
              modified: mtime,
            };
          });

          res.setHeader('Content-Type', 'application/json');
          res.end(JSON.stringify({ path: targetPath, items }));
        } catch (err: any) {
          res.statusCode = 500;
          res.end(JSON.stringify({ error: err.message, items: [] }));
        }
      });

      // API: Write file
      server.middlewares.use('/api/fs-write', (req: any, res: any) => {
        if (req.method !== 'POST') {
          res.statusCode = 405;
          return res.end();
        }
        let body = '';
        req.on('data', (c: any) => (body += c));
        req.on('end', () => {
          try {
            const { filePath, content } = JSON.parse(body || '{}');
            if (!filePath) throw new Error('filePath required');
            fs.mkdirSync(path.dirname(filePath), { recursive: true });
            fs.writeFileSync(filePath, content || '', 'utf-8');
            res.setHeader('Content-Type', 'application/json');
            res.end(JSON.stringify({ success: true }));
          } catch (err: any) {
            res.statusCode = 500;
            res.end(JSON.stringify({ error: err.message }));
          }
        });
      });

      // API: Mkdir
      server.middlewares.use('/api/fs-mkdir', (req: any, res: any) => {
        if (req.method !== 'POST') {
          res.statusCode = 405;
          return res.end();
        }
        let body = '';
        req.on('data', (c: any) => (body += c));
        req.on('end', () => {
          try {
            const { dirPath } = JSON.parse(body || '{}');
            if (!dirPath) throw new Error('dirPath required');
            fs.mkdirSync(dirPath, { recursive: true });
            res.setHeader('Content-Type', 'application/json');
            res.end(JSON.stringify({ success: true }));
          } catch (err: any) {
            res.statusCode = 500;
            res.end(JSON.stringify({ error: err.message }));
          }
        });
      });

      // API: Delete
      server.middlewares.use('/api/fs-delete', (req: any, res: any) => {
        if (req.method !== 'POST') {
          res.statusCode = 405;
          return res.end();
        }
        let body = '';
        req.on('data', (c: any) => (body += c));
        req.on('end', () => {
          try {
            const { targetPath } = JSON.parse(body || '{}');
            if (!targetPath) throw new Error('targetPath required');
            fs.rmSync(targetPath, { recursive: true, force: true });
            res.setHeader('Content-Type', 'application/json');
            res.end(JSON.stringify({ success: true }));
          } catch (err: any) {
            res.statusCode = 500;
            res.end(JSON.stringify({ error: err.message }));
          }
        });
      });

      // API: Installer Start
      server.middlewares.use('/api/installer/start', (req: any, res: any) => {
        if (req.method !== 'POST') {
          res.statusCode = 405;
          return res.end();
        }
        let body = '';
        req.on('data', (c: any) => (body += c));
        req.on('end', () => {
          try {
            const config = JSON.parse(body || '{}');
            devInstallJob = {
              isInstalling: true,
              progress: 5,
              statusText: 'Initializing installation engine...',
              completed: false,
              error: null,
              log: [`[${new Date().toLocaleTimeString()}] Installation initialized for ${config.targetDisk}`],
            };

            const scriptPath = path.join(process.cwd(), '../os-build/scripts/axisos-install.sh');
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
              devInstallJob.log.push(`[Notice] Running in verified simulation mode (${isLinux ? 'non-root' : process.platform})`);
            }

            const child = spawn('bash', args);

            child.stdout.on('data', (chunk) => {
              const lines = chunk.toString().split('\n');
              for (const line of lines) {
                const trimmed = line.trim();
                if (!trimmed) continue;
                devInstallJob.log.push(trimmed);

                const match = trimmed.match(/^PROGRESS:(\d+):(.*)$/);
                if (match) {
                  devInstallJob.progress = parseInt(match[1], 10);
                  devInstallJob.statusText = match[2];
                }
              }
            });

            child.stderr.on('data', (chunk) => {
              const trimmed = chunk.toString().trim();
              if (trimmed) devInstallJob.log.push(`[err] ${trimmed}`);
            });

            child.on('close', (code) => {
              devInstallJob.isInstalling = false;
              if (code === 0) {
                devInstallJob.progress = 100;
                devInstallJob.statusText = 'Installation completed successfully!';
                devInstallJob.completed = true;
              } else {
                devInstallJob.error = `Installer process exited with code ${code}`;
                devInstallJob.statusText = `Failed: ${devInstallJob.error}`;
              }
            });

            res.setHeader('Content-Type', 'application/json');
            res.end(JSON.stringify({ success: true, message: 'Installer started' }));
          } catch (err: any) {
            devInstallJob.isInstalling = false;
            devInstallJob.error = err.message;
            res.statusCode = 500;
            res.end(JSON.stringify({ error: err.message }));
          }
        });
      });

      // API: Installer Status
      server.middlewares.use('/api/installer/status', (req: any, res: any) => {
        res.setHeader('Content-Type', 'application/json');
        res.end(JSON.stringify(devInstallJob));
      });

      // API: Power Management
      server.middlewares.use('/api/power', (req: any, res: any) => {
        let body = '';
        req.on('data', (c: any) => (body += c));
        req.on('end', () => {
          try {
            const { action } = JSON.parse(body || '{}');
            if (action === 'poweroff') {
              exec('systemctl poweroff || shutdown -h now', () => {});
            } else if (action === 'reboot') {
              exec('systemctl reboot || reboot', () => {});
            }
            res.setHeader('Content-Type', 'application/json');
            res.end(JSON.stringify({ success: true, action }));
          } catch (err: any) {
            res.statusCode = 500;
            res.end(JSON.stringify({ error: err.message }));
          }
        });
      });
    },
  };
}

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [
    react(),
    tailwindcss(),
    linuxSystemApiPlugin(),
  ],
  server: {
    port: 3000,
    host: true,
  },
})
