import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'
import tailwindcss from '@tailwindcss/vite'
import { exec } from 'child_process'
import fs from 'fs'
import path from 'path'
import os from 'os'

// Custom plugin to provide real system hardware info and command execution to the shell
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

          const [uname, cpuModel, meminfo, lsblk, lspci, hostname, uptime, osRelease] = await Promise.all([
            runCmd('uname -r'),
            runCmd('lscpu | grep "Model name:" | head -n1 | cut -d: -f2 | xargs'),
            runCmd('cat /proc/meminfo'),
            runCmd('lsblk -d -J -o NAME,SIZE,TYPE,MODEL 2>/dev/null || lsblk -d -o NAME,SIZE,TYPE,MODEL'),
            runCmd('lspci | grep -E "VGA|3D|Display" | head -n1 | cut -d: -f3 | xargs'),
            runCmd('hostname'),
            runCmd('uptime -p 2>/dev/null || uptime'),
            runCmd('cat /etc/os-release'),
          ]);

          // Parse memory
          let totalMemGb = (os.totalmem() / (1024 * 1024 * 1024)).toFixed(1);
          let freeMemGb = (os.freemem() / (1024 * 1024 * 1024)).toFixed(1);
          const memMatch = meminfo.match(/MemTotal:\s+(\d+)\s+kB/);
          if (memMatch) {
            totalMemGb = (parseInt(memMatch[1], 10) / 1024 / 1024).toFixed(1);
          }

          // Parse disks
          let disks: any[] = [];
          try {
            const parsed = JSON.parse(lsblk);
            if (parsed.blockdevices) {
              disks = parsed.blockdevices
                .filter((d: any) => d.type === 'disk')
                .map((d: any) => ({
                  id: `/dev/${d.name}`,
                  name: d.model ? `${d.model} (${d.size})` : `${d.name} (${d.size})`,
                  size: d.size,
                  type: d.name.startsWith('nvme') ? 'NVMe High-Speed Solid State Drive' : 'SATA Solid State Drive',
                  freeSpace: `${d.size} Available`,
                }));
            }
          } catch {
            // Fallback parsing
            disks = [
              { id: '/dev/nvme0n1', name: 'NVMe SSD 256GB', size: '238.5G', type: 'NVMe Solid State Drive', freeSpace: '200G Available' },
              { id: '/dev/sda', name: 'Samsung SSD 120GB', size: '111.8G', type: 'SATA Solid State Drive', freeSpace: '90G Available' },
            ];
          }

          const responseData = {
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

          res.setHeader('Content-Type', 'application/json');
          res.end(JSON.stringify(responseData));
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
            exec(command, { cwd: workingDir, timeout: 10000, maxBuffer: 1024 * 1024 }, (error, stdout, stderr) => {
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
            return res.end(JSON.stringify({ error: 'Path not found' }));
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
          res.end(JSON.stringify({ error: err.message }));
        }
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
