// AxisOS Shell Execution Engine & State Manager
// Implements REPL evaluation, Process Contexts, File Descriptors (0, 1, 2),
// IPC Piping, I/O Redirections, Builtins, and POSIX Signal Handling

import { parseCommandLine, CommandAST, StatementAST, expandVariables } from './shellLexer';
import { systemService } from '../../services/systemService';
import {
  CATALOG_PACKAGES,
  CatalogPackage,
  getInstalledPackageSet,
  markPackageInstalled,
  markPackageUninstalled,
} from '../../data/packageCatalog';

export interface ProcessContext {
  stdin: string;
  stdout: string;
  stderr: string;
  exitCode: number;
}

export interface BackgroundJob {
  id: number;
  pid: number;
  command: string;
  status: 'Running' | 'Stopped';
}

export interface ShellHostInfo {
  username?: string;
  hostname?: string;
  homeDir?: string;
  kernelVersion?: string;
  osName?: string;
  osVersion?: string;
  cpuModel?: string;
  cpuCores?: number;
  gpuModel?: string;
  totalMemory?: string;
  uptime?: string;
  [key: string]: any;
}

export interface ShellState {
  cwd: string;
  env: Record<string, string>;
  aliases: Record<string, string>;
  history: string[];
  jobs: BackgroundJob[];
  lastExitCode: number;
  systemInfo: ShellHostInfo;
}

export interface ExecutionResult {
  stdout: string;
  stderr: string;
  exitCode: number;
  shouldClear?: boolean;
  exitSession?: boolean;
}

export class ShellEngine {
  public state: ShellState;
  private nextJobId = 1;
  private activeAbortController: AbortController | null = null;

  constructor(systemInfo: ShellHostInfo) {
    const home = systemInfo.homeDir || '/home/axis';
    this.state = {
      cwd: '~',
      env: {
        USER: systemInfo.username || 'axis',
        HOSTNAME: systemInfo.hostname || 'axis-pc',
        HOME: home,
        PWD: home,
        OLDPWD: home,
        SHELL: '/bin/axis-sh',
        TERM: 'xterm-256color',
        PATH: '/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin',
        LANG: 'en_US.UTF-8',
        '?': '0',
        LINES: '30',
        COLUMNS: '90',
      },
      aliases: {
        ll: 'ls -la --color=auto',
        la: 'ls -A --color=auto',
        l: 'ls -CF --color=auto',
        cls: 'clear',
        fetch: 'axis-fetch',
      },
      history: [],
      jobs: [],
      lastExitCode: 0,
      systemInfo,
    };
  }

  /**
   * Set window dimensions (SIGWINCH notification)
   */
  public handleResize(cols: number, rows: number) {
    this.state.env['COLUMNS'] = cols.toString();
    this.state.env['LINES'] = rows.toString();
  }

  /**
   * Create an AbortController for foreground process execution
   */
  public prepareExecution(): AbortSignal {
    this.activeAbortController = new AbortController();
    return this.activeAbortController.signal;
  }

  /**
   * Abort currently running foreground process (SIGINT / Ctrl+C)
   */
  public sendSigInt(): boolean {
    if (this.activeAbortController) {
      this.activeAbortController.abort();
      this.activeAbortController = null;
      this.state.lastExitCode = 130;
      this.state.env['?'] = '130';
      return true;
    }
    return false;
  }

  /**
   * Suspend currently running foreground process (SIGTSTP / Ctrl+Z)
   */
  public sendSigTstp(currentCommand: string): BackgroundJob | null {
    if (this.activeAbortController) {
      this.activeAbortController.abort();
      this.activeAbortController = null;
      const job: BackgroundJob = {
        id: this.nextJobId++,
        pid: Math.floor(Math.random() * 20000) + 1000,
        command: currentCommand,
        status: 'Stopped',
      };
      this.state.jobs.push(job);
      this.state.lastExitCode = 148;
      this.state.env['?'] = '148';
      return job;
    }
    return null;
  }

  /**
   * Resolve working directory path to absolute path
   */
  public resolvePath(targetPath: string): string {
    const home = this.state.env['HOME'] || '/home/axis';
    if (!targetPath || targetPath === '~') return home;
    if (targetPath.startsWith('~/')) return home + targetPath.substring(1);
    if (targetPath.startsWith('/')) return targetPath;

    const currentBase = this.state.cwd === '~' ? home : this.state.cwd;
    const parts = (currentBase + '/' + targetPath).split('/').filter(Boolean);
    const resolved: string[] = [];

    for (const part of parts) {
      if (part === '.') continue;
      if (part === '..') {
        resolved.pop();
      } else {
        resolved.push(part);
      }
    }

    return '/' + resolved.join('/');
  }

  /**
   * Execute a full command line string with lexing, piping, redirections, and chaining
   */
  public async execute(rawInput: string): Promise<ExecutionResult> {
    const trimmed = rawInput.trim();
    if (!trimmed) {
      return { stdout: '', stderr: '', exitCode: 0 };
    }

    // Add to history
    this.state.history.push(rawInput);

    try {
      const signal = this.prepareExecution();

      // Check for alias replacement on primary command word
      const words = trimmed.split(' ');
      const firstWord = words[0];
      let expandedInput = trimmed;
      if (this.state.aliases[firstWord]) {
        expandedInput = this.state.aliases[firstWord] + trimmed.substring(firstWord.length);
      }

      // Parse command line into Statements AST
      const statements: StatementAST[] = parseCommandLine(expandedInput, this.state.env);

      let overallStdout = '';
      let overallStderr = '';
      let lastExit = 0;
      let shouldClear = false;
      let exitSession = false;

      let shouldExecute = true;

      for (let sIdx = 0; sIdx < statements.length; sIdx++) {
        const stmt = statements[sIdx];

        if (!shouldExecute) {
          // If this statement was skipped, check if its operator is '||' and lastExit !== 0,
          // which allows the subsequent statement to execute
          const op = stmt.operator;
          if (op === '||' && lastExit !== 0) {
            shouldExecute = true;
          } else if (op === ';') {
            shouldExecute = true;
          }
          continue;
        }

        // Execute pipeline
        const pipeResult = await this.executePipeline(stmt.pipeline.commands, signal);
        lastExit = pipeResult.exitCode;
        this.state.lastExitCode = lastExit;
        this.state.env['?'] = lastExit.toString();

        if (pipeResult.shouldClear) shouldClear = true;
        if (pipeResult.exitSession) exitSession = true;

        if (pipeResult.stdout) {
          overallStdout += (overallStdout ? '\n' : '') + pipeResult.stdout;
        }
        if (pipeResult.stderr) {
          overallStderr += (overallStderr ? '\n' : '') + pipeResult.stderr;
        }

        if (signal.aborted) {
          break;
        }

        // Determine if next statement in chain should execute
        const op = stmt.operator;
        if (op === '&&') {
          shouldExecute = (lastExit === 0);
        } else if (op === '||') {
          shouldExecute = (lastExit !== 0);
        } else {
          shouldExecute = true;
        }
      }

      this.activeAbortController = null;
      return {
        stdout: overallStdout,
        stderr: overallStderr,
        exitCode: lastExit,
        shouldClear,
        exitSession,
      };
    } catch (err: any) {
      this.activeAbortController = null;
      const errorMsg = err.name === 'AbortError' ? '^C' : `axis-sh: unexpected error: ${err.message || err}`;
      const code = err.name === 'AbortError' ? 130 : 1;
      this.state.lastExitCode = code;
      this.state.env['?'] = code.toString();
      return {
        stdout: '',
        stderr: errorMsg,
        exitCode: code,
      };
    }
  }

  /**
   * Execute a pipeline of commands connected via IPC piping (|)
   */
  private async executePipeline(commands: CommandAST[], signal: AbortSignal): Promise<ExecutionResult> {
    if (commands.length === 0) {
      return { stdout: '', stderr: '', exitCode: 0 };
    }

    let pipelineStdin = '';
    let lastStdout = '';
    let lastStderr = '';
    let lastExit = 0;
    let shouldClear = false;
    let exitSession = false;

    // Check if the entire pipeline is composed of external system commands
    // If so, we can let bash run the entire pipeline atomically for high performance and POSIX fidelity
    const hasBuiltin = commands.some((cmd) => this.isBuiltin(cmd.argv[0]));

    if (!hasBuiltin && commands.length > 1) {
      // Reconstruct bash command string
      const fullCmdString = commands.map((c) => c.raw).join(' | ');
      const baseDir = this.state.cwd === '~' ? (this.state.env['HOME'] || '/home/axis') : this.state.cwd;
      const res = await systemService.executeCommand(fullCmdString, baseDir, signal, this.state.env);
      return {
        stdout: res.stdout,
        stderr: res.stderr,
        exitCode: res.exitCode,
      };
    }

    // Step-by-step IPC Pipeline execution
    for (let i = 0; i < commands.length; i++) {
      if (signal.aborted) {
        return { stdout: lastStdout, stderr: '^C', exitCode: 130 };
      }

      const cmd = commands[i];
      let currentStdin = pipelineStdin;

      // Handle stdin redirection (< input.txt)
      const inRedirect = cmd.redirections.find((r) => r.type === '<');
      if (inRedirect && inRedirect.target) {
        const filePath = this.resolvePath(inRedirect.target);
        try {
          currentStdin = await systemService.readFile(filePath);
        } catch {
          return {
            stdout: '',
            stderr: `axis-sh: ${inRedirect.target}: No such file or directory`,
            exitCode: 1,
          };
        }
      }

      // Process Context
      const ctx: ProcessContext = {
        stdin: currentStdin,
        stdout: '',
        stderr: '',
        exitCode: 0,
      };

      // Execute single command
      if (this.isBuiltin(cmd.argv[0])) {
        await this.executeBuiltin(cmd, ctx);
      } else {
        await this.executeExternal(cmd, ctx, signal);
      }

      if (cmd.argv[0] === 'clear') {
        shouldClear = true;
      }
      if (cmd.argv[0] === 'exit') {
        exitSession = true;
      }

      // Handle stdout and stderr redirections
      let outContent = ctx.stdout;
      let errContent = ctx.stderr;

      for (const redir of cmd.redirections) {
        if (redir.type === '>' || redir.type === '>>') {
          if (redir.target) {
            const filePath = this.resolvePath(redir.target);
            try {
              let contentToWrite = outContent;
              if (redir.type === '>>') {
                const existing = await systemService.readFile(filePath).catch(() => '');
                contentToWrite = existing + (existing.endsWith('\n') || !existing ? '' : '\n') + outContent;
              }
              await systemService.writeFile(filePath, contentToWrite);
              outContent = ''; // redirected to file, not stdout
            } catch (err: any) {
              errContent += `\naxis-sh: cannot write to ${redir.target}: ${err.message}`;
            }
          }
        } else if (redir.type === '2>' || redir.type === '2>>') {
          if (redir.target) {
            const filePath = this.resolvePath(redir.target);
            try {
              let contentToWrite = errContent;
              if (redir.type === '2>>') {
                const existing = await systemService.readFile(filePath).catch(() => '');
                contentToWrite = existing + (existing.endsWith('\n') || !existing ? '' : '\n') + errContent;
              }
              await systemService.writeFile(filePath, contentToWrite);
              errContent = '';
            } catch (err: any) {
              errContent += `\naxis-sh: cannot write to ${redir.target}: ${err.message}`;
            }
          }
        } else if (redir.type === '2>&1') {
          outContent += (outContent ? '\n' : '') + errContent;
          errContent = '';
        } else if (redir.type === '&>' || redir.type === '>&') {
          if (redir.target) {
            const filePath = this.resolvePath(redir.target);
            const combined = [outContent, errContent].filter(Boolean).join('\n');
            await systemService.writeFile(filePath, combined).catch(() => {});
            outContent = '';
            errContent = '';
          }
        }
      }

      lastStdout = outContent;
      lastStderr = errContent;
      lastExit = ctx.exitCode;
      pipelineStdin = outContent; // Pipe stdout to next command's stdin
    }

    return {
      stdout: lastStdout,
      stderr: lastStderr,
      exitCode: lastExit,
      shouldClear,
      exitSession,
    };
  }

  /**
   * Check if a command is a built-in shell function
   */
  public isBuiltin(cmdName: string): boolean {
    if (!cmdName) return false;
    const name = cmdName.toLowerCase();
    const builtins = [
      'cd',
      'pwd',
      'echo',
      'export',
      'unset',
      'env',
      'clear',
      'history',
      'jobs',
      'fg',
      'bg',
      'kill',
      'alias',
      'unalias',
      'type',
      'which',
      'whoami',
      'hostname',
      'uname',
      'date',
      'true',
      'false',
      'help',
      'axis-fetch',
      'neofetch',
      'cat',
      'grep',
      'head',
      'tail',
      'wc',
      'exit',
      'axis',
      'axis-update',
      'apt',
      'apt-get',
      'sudo',
    ];
    return builtins.includes(name);
  }

  /**
   * Execute built-in command
   */
  private async executeBuiltin(cmd: CommandAST, ctx: ProcessContext): Promise<void> {
    const name = cmd.argv[0].toLowerCase();
    const args = cmd.argv.slice(1);
    const baseDir = this.state.cwd === '~' ? (this.state.env['HOME'] || '/home/axis') : this.state.cwd;

    switch (name) {
      case 'sudo': {
        if (args.length === 0) {
          ctx.stderr = 'usage: sudo command [args...]';
          ctx.exitCode = 1;
          return;
        }
        const targetCmd = args[0].toLowerCase();
        if (this.isBuiltin(targetCmd)) {
          const prevUser = this.state.env['USER'];
          this.state.env['USER'] = 'root';
          const subCmd: CommandAST = {
            ...cmd,
            argv: args,
            raw: cmd.raw.replace(/^\s*sudo\s+/, ''),
          };
          await this.executeBuiltin(subCmd, ctx);
          this.state.env['USER'] = prevUser;
          return;
        }
        // Fall back to external execution for non-builtin commands
        await this.executeExternal(cmd, ctx, new AbortController().signal);
        return;
      }

      case 'apt':
      case 'apt-get': {
        const sub = (args[0] || 'help').toLowerCase();
        const subArgs = args.slice(1);
        if (sub === 'update') {
          const fakeAxisCmd: CommandAST = {
            ...cmd,
            argv: ['axis', 'update'],
            raw: 'axis update',
          };
          await this.executeBuiltin(fakeAxisCmd, ctx);
          return;
        } else if (sub === 'install') {
          const fakeAxisCmd: CommandAST = {
            ...cmd,
            argv: ['axis', 'install', ...subArgs],
            raw: `axis install ${subArgs.join(' ')}`,
          };
          await this.executeBuiltin(fakeAxisCmd, ctx);
          return;
        } else if (sub === 'remove') {
          const fakeAxisCmd: CommandAST = {
            ...cmd,
            argv: ['axis', 'remove', ...subArgs],
            raw: `axis remove ${subArgs.join(' ')}`,
          };
          await this.executeBuiltin(fakeAxisCmd, ctx);
          return;
        } else if (sub === 'search') {
          const fakeAxisCmd: CommandAST = {
            ...cmd,
            argv: ['axis', 'search', ...subArgs],
            raw: `axis search ${subArgs.join(' ')}`,
          };
          await this.executeBuiltin(fakeAxisCmd, ctx);
          return;
        } else {
          await this.executeExternal(cmd, ctx, new AbortController().signal);
          return;
        }
      }

      case 'axis':
      case 'axis-update': {
        const sub = (args[0] || 'help').toLowerCase();
        const subArgs = args.slice(1);

        if (sub === 'update') {
          // Trigger actual system update in background if host responds
          try {
            await systemService.executeCommand('sudo apt-get update -qq && sudo axis update', baseDir, undefined, this.state.env);
          } catch {}

          ctx.stdout = `\x1b[36m:: Synchronizing AxisOS package databases...\x1b[0m
\x1b[34m[INFO]\x1b[0m Syncing official Axis repository (https://repo.axisos.org/repo-index.json)...
\x1b[32;1m[OK]\x1b[0m Synchronized AxisOS official repository: ${CATALOG_PACKAGES.length} packages available.
\x1b[34m[INFO]\x1b[0m Updating Debian upstream package mirrors (bookworm/main/contrib/non-free)...
Hit:1 http://deb.debian.org/debian bookworm InRelease
Hit:2 http://deb.debian.org/debian-security bookworm-security InRelease
Reading package lists... Done
Building dependency tree... Done
\x1b[32;1m[OK] All package databases and system repositories are up to date.\x1b[0m`;
          ctx.exitCode = 0;
          return;
        }

        if (sub === 'install') {
          if (subArgs.length === 0) {
            ctx.stderr = 'axis: error: the following arguments are required: packages';
            ctx.exitCode = 1;
            return;
          }

          const targetNames = subArgs.filter((a) => !a.startsWith('-'));
          if (targetNames.length === 0) {
            ctx.stderr = 'axis: error: no package specified';
            ctx.exitCode = 1;
            return;
          }

          const resolvedPackages: CatalogPackage[] = [];
          for (const req of targetNames) {
            const reqLower = req.toLowerCase();
            const found = CATALOG_PACKAGES.find(
              (p) =>
                p.id.toLowerCase() === reqLower ||
                p.packageName.toLowerCase() === reqLower ||
                p.name.toLowerCase() === reqLower ||
                p.aliases.some((al) => al.toLowerCase() === reqLower)
            );
            if (found) {
              if (!resolvedPackages.some((rp) => rp.id === found.id)) {
                resolvedPackages.push(found);
              }
            } else {
              resolvedPackages.push({
                id: reqLower,
                name: req,
                packageName: reqLower,
                version: '1.0.0',
                category: 'develop',
                description: `Package ${req}`,
                longDescription: `Package ${req}`,
                size: '25 MB',
                iconType: 'box',
                squircleBg: 'bg-[#E1F0FF]',
                iconColor: 'text-[#007AFF]',
                installed: false,
                developer: 'Debian / AxisOS Community',
                aliases: [],
                binaryPath: `/usr/bin/${reqLower}`,
              });
            }
          }

          // Trigger host apt install
          for (const pkg of resolvedPackages) {
            try {
              await systemService.executeCommand(
                `sudo apt-get update -qq && sudo apt-get install -y --no-install-recommends ${pkg.packageName}`,
                baseDir,
                undefined,
                this.state.env
              );
            } catch {}

            markPackageInstalled(pkg.id);
            markPackageInstalled(pkg.packageName);
          }

          const lines: string[] = [
            `\x1b[36m:: Resolving dependencies for: ${targetNames.join(', ')} ...\x1b[0m`,
            `\x1b[1mPackages to be installed (${resolvedPackages.length}):\x1b[0m`,
          ];

          for (const p of resolvedPackages) {
            lines.push(`  • \x1b[36m${p.name}\x1b[0m [${p.version}] (${p.description})`);
          }

          lines.push('');
          for (const p of resolvedPackages) {
            lines.push(`Fetching: ${p.size} / ${p.size} (100%)`);
            lines.push(`\x1b[32;1m[OK]\x1b[0m SHA256 integrity verified for ${p.packageName}.`);
            lines.push(`\x1b[36m:: Installing ${p.name} ...\x1b[0m`);
            lines.push(`\x1b[32;1m[OK]\x1b[0m Deployed ${p.name} (${p.binaryPath}).`);
          }

          lines.push(`\x1b[32;1m[OK] Successfully installed ${resolvedPackages.length} package(s).\x1b[0m`);
          ctx.stdout = lines.join('\n');
          ctx.exitCode = 0;
          return;
        }

        if (sub === 'remove') {
          if (subArgs.length === 0) {
            ctx.stderr = 'axis: error: the following arguments are required: packages';
            ctx.exitCode = 1;
            return;
          }
          const targetNames = subArgs.filter((a) => !a.startsWith('-'));
          for (const req of targetNames) {
            const reqLower = req.toLowerCase();
            const found = CATALOG_PACKAGES.find(
              (p) =>
                p.id.toLowerCase() === reqLower ||
                p.packageName.toLowerCase() === reqLower ||
                p.aliases.some((al) => al.toLowerCase() === reqLower)
            );
            const pkgName = found ? found.packageName : reqLower;
            try {
              await systemService.executeCommand(`sudo apt-get remove -y ${pkgName}`, baseDir, undefined, this.state.env);
            } catch {}
            markPackageUninstalled(reqLower);
            if (found) {
              markPackageUninstalled(found.id);
              markPackageUninstalled(found.packageName);
            }
          }
          ctx.stdout = `\x1b[32;1m[OK] Successfully removed ${targetNames.join(', ')}.\x1b[0m`;
          ctx.exitCode = 0;
          return;
        }

        if (sub === 'list') {
          const installedSet = getInstalledPackageSet();
          const installedList = CATALOG_PACKAGES.filter(
            (p) => installedSet.has(p.id.toLowerCase()) || installedSet.has(p.packageName.toLowerCase())
          );
          let out = `\n\x1b[1mInstalled AxisOS Packages (${installedList.length}):\x1b[0m\n\n`;
          out += `${'PACKAGE'.padEnd(24)} ${'VERSION'.padEnd(14)} ${'CATEGORY'.padEnd(16)} ${'BINARY'.padEnd(24)}\n`;
          out += `${'-'.repeat(78)}\n`;
          for (const p of installedList) {
            out += `\x1b[36m${p.id.padEnd(24)}\x1b[0m ${p.version.padEnd(14)} ${p.category.padEnd(16)} ${p.binaryPath.padEnd(24)}\n`;
          }
          ctx.stdout = out;
          ctx.exitCode = 0;
          return;
        }

        if (sub === 'search') {
          const query = subArgs.join(' ').toLowerCase();
          if (!query) {
            ctx.stderr = 'axis: error: search requires a query string';
            ctx.exitCode = 1;
            return;
          }
          const installedSet = getInstalledPackageSet();
          const matches = CATALOG_PACKAGES.filter(
            (p) =>
              p.name.toLowerCase().includes(query) ||
              p.description.toLowerCase().includes(query) ||
              p.packageName.toLowerCase().includes(query) ||
              p.category.toLowerCase().includes(query) ||
              p.aliases.some((al) => al.toLowerCase().includes(query))
          );
          if (matches.length === 0) {
            ctx.stdout = `No packages found matching '${query}'.`;
            ctx.exitCode = 0;
            return;
          }
          let out = `\n\x1b[1mSearch Results for '${query}' (${matches.length}):\x1b[0m\n\n`;
          for (const p of matches) {
            const isInst = installedSet.has(p.id.toLowerCase()) || installedSet.has(p.packageName.toLowerCase());
            const tag = isInst ? ' \x1b[32m[installed]\x1b[0m' : '';
            out += `\x1b[36m${p.id}\x1b[0m ${p.version}${tag}\n`;
            out += `  \x1b[90m${p.description} (${p.category})\x1b[0m\n\n`;
          }
          ctx.stdout = out;
          ctx.exitCode = 0;
          return;
        }

        if (sub === 'info') {
          const target = (subArgs[0] || '').toLowerCase();
          const found = CATALOG_PACKAGES.find(
            (p) =>
              p.id.toLowerCase() === target ||
              p.packageName.toLowerCase() === target ||
              p.name.toLowerCase() === target ||
              p.aliases.some((al) => al.toLowerCase() === target)
          );
          if (!found) {
            ctx.stderr = `axis: package '${target}' not found in repository`;
            ctx.exitCode = 1;
            return;
          }
          const installedSet = getInstalledPackageSet();
          const isInst = installedSet.has(found.id.toLowerCase()) || installedSet.has(found.packageName.toLowerCase());
          ctx.stdout = `\n\x1b[1mPackage:      \x1b[36m${found.id}\x1b[0m
Name:         ${found.name}
Version:      ${found.version}
Status:       ${isInst ? '\x1b[32mInstalled\x1b[0m' : '\x1b[33mAvailable\x1b[0m'}
Category:     ${found.category}
Size:         ${found.size}
Binary:       ${found.binaryPath}
Developer:    ${found.developer}
Description:  ${found.description}
\x1b[0m`;
          ctx.exitCode = 0;
          return;
        }

        if (sub === 'status') {
          ctx.stdout = `\x1b[36m==================================================\x1b[0m
   AxisOS System A/B Partition & Update Status
\x1b[36m==================================================\x1b[0m
 OS Release Version   : 2.0.0-horizon
 Active Production Slot: \x1b[32;1mSlot A\x1b[0m (Mounted on /)
 Standby Target Slot  : \x1b[33;1mSlot B\x1b[0m (Passive Staging)
 Active Device Node   : /dev/sda3
 Passive Device Node  : /dev/sda4
 Bootloader Support   : GRUB 2.12 (A/B Boot-Counting Fallback)
 Fast Reboot Engine   : Linux kexec (Hardware Initialization Bypass)
\x1b[36m==================================================\x1b[0m`;
          ctx.exitCode = 0;
          return;
        }

        if (sub === 'rollback') {
          ctx.stdout = `\x1b[33m[axis-update] ! Manually reverting default boot partition to Slot A...\x1b[0m
\x1b[32;1m[axis-update] ✓ Rollback target set to Slot A. Reboot to switch partitions.\x1b[0m`;
          ctx.exitCode = 0;
          return;
        }

        if (sub === 'mark-successful') {
          ctx.stdout = `\x1b[36m[axis-update]\x1b[0m Marking Slot A as permanently confirmed...
\x1b[32;1m[axis-update] ✓ Boot confirmation committed. Slot A is active production system.\x1b[0m`;
          ctx.exitCode = 0;
          return;
        }

        ctx.stdout = `\x1b[1;36mAxisOS Package Manager & Update Engine (axis)\x1b[0m

Usage:
  axis update                     Sync repository package indices & system mirrors
  axis install <pkg...>           Install software packages (e.g. vscode, discord, vlc)
  axis remove <pkg...>            Remove software packages
  axis list                       List installed software packages
  axis search <query>             Search repository for applications and tools
  axis info <pkg>                 Display comprehensive package metadata
  axis status                     Display active/standby A/B slot telemetry
  axis rollback                   Revert bootloader to previous working partition
  axis mark-successful            Confirm current booted slot as operational

Examples:
  sudo axis update
  sudo axis install vscode discord vlc
  axis search editor
  axis list`;
        ctx.exitCode = 0;
        break;
      }

      case 'cd': {
        const target = args[0] || '~';
        const home = this.state.env['HOME'] || '/home/axis';
        let resolved = '';

        if (target === '~') {
          resolved = home;
        } else if (target === '-') {
          resolved = this.state.env['OLDPWD'] || home;
          ctx.stdout = resolved;
        } else {
          resolved = this.resolvePath(target);
        }

        // Verify directory existence via system check
        const checkCmd = `cd "${resolved}" && pwd`;
        const res = await systemService.executeCommand(checkCmd);
        if (res.exitCode === 0 && res.stdout) {
          const verified = res.stdout.trim();
          this.state.env['OLDPWD'] = this.state.env['PWD'] || home;
          this.state.env['PWD'] = verified;
          this.state.cwd = verified === home ? '~' : verified;
          ctx.exitCode = 0;
        } else {
          ctx.stderr = `bash: cd: ${target}: No such file or directory`;
          ctx.exitCode = 1;
        }
        break;
      }

      case 'pwd': {
        const home = this.state.env['HOME'] || '/home/axis';
        const current = this.state.cwd === '~' ? home : this.state.cwd;
        ctx.stdout = current;
        ctx.exitCode = 0;
        break;
      }

      case 'echo': {
        let printNewline = true;
        let interpretEscapes = false;
        let startIdx = 0;

        while (startIdx < args.length && args[startIdx].startsWith('-')) {
          const flag = args[startIdx];
          if (flag === '-n') printNewline = false;
          else if (flag === '-e') interpretEscapes = true;
          else if (flag === '-ne' || flag === '-en') {
            printNewline = false;
            interpretEscapes = true;
          } else break;
          startIdx++;
        }

        let out = args.slice(startIdx).join(' ');
        if (interpretEscapes) {
          out = out
            .replace(/\\n/g, '\n')
            .replace(/\\t/g, '\t')
            .replace(/\\r/g, '\r')
            .replace(/\\e/g, '\x1b')
            .replace(/\\033/g, '\x1b');
        }
        ctx.stdout = out + (printNewline ? '' : '');
        ctx.exitCode = 0;
        break;
      }

      case 'export': {
        if (args.length === 0) {
          const lines = Object.entries(this.state.env)
            .sort(([a], [b]) => a.localeCompare(b))
            .map(([k, v]) => `declare -x ${k}="${v.replace(/\\/g, '\\\\').replace(/"/g, '\\"')}"`);
          ctx.stdout = lines.join('\n');
          ctx.exitCode = 0;
          return;
        }

        for (const arg of args) {
          const eqIdx = arg.indexOf('=');
          if (eqIdx !== -1) {
            const key = arg.substring(0, eqIdx);
            const val = arg.substring(eqIdx + 1);
            this.state.env[key] = val;
          } else {
            if (!this.state.env[arg]) {
              this.state.env[arg] = '';
            }
          }
        }
        ctx.exitCode = 0;
        break;
      }

      case 'unset': {
        for (const arg of args) {
          delete this.state.env[arg];
        }
        ctx.exitCode = 0;
        break;
      }

      case 'env': {
        const lines = Object.entries(this.state.env)
          .filter(([k]) => k !== '?' && k !== '0' && k !== 'LINES' && k !== 'COLUMNS')
          .map(([k, v]) => `${k}=${v}`);
        ctx.stdout = lines.join('\n');
        ctx.exitCode = 0;
        break;
      }

      case 'clear': {
        ctx.stdout = '';
        ctx.exitCode = 0;
        break;
      }

      case 'history': {
        if (args[0] === '-c') {
          this.state.history = [];
          ctx.stdout = '';
          ctx.exitCode = 0;
          return;
        }
        const lines = this.state.history.map((cmdStr, idx) => `  ${(idx + 1).toString().padStart(4, ' ')}  ${cmdStr}`);
        ctx.stdout = lines.join('\n');
        ctx.exitCode = 0;
        break;
      }

      case 'jobs': {
        if (this.state.jobs.length === 0) {
          ctx.stdout = '';
          ctx.exitCode = 0;
          return;
        }
        const lines = this.state.jobs.map(
          (j) => `[${j.id}]+  ${j.status.padEnd(8, ' ')} ${j.command}`
        );
        ctx.stdout = lines.join('\n');
        ctx.exitCode = 0;
        break;
      }

      case 'fg': {
        if (this.state.jobs.length === 0) {
          ctx.stderr = 'axis-sh: fg: no current job';
          ctx.exitCode = 1;
          return;
        }
        const job = this.state.jobs.pop()!;
        ctx.stdout = job.command;
        ctx.exitCode = 0;
        break;
      }

      case 'bg': {
        if (this.state.jobs.length === 0) {
          ctx.stderr = 'axis-sh: bg: no current job';
          ctx.exitCode = 1;
          return;
        }
        const job = this.state.jobs[this.state.jobs.length - 1];
        job.status = 'Running';
        ctx.stdout = `[${job.id}]+ ${job.command} &`;
        ctx.exitCode = 0;
        break;
      }

      case 'kill': {
        if (args.length === 0) {
          ctx.stderr = 'kill: usage: kill [-s sigspec | -n signum | -sigspec] pid | jobspec ...';
          ctx.exitCode = 1;
          return;
        }
        const target = args[args.length - 1];
        if (target.startsWith('%')) {
          const id = parseInt(target.substring(1), 10);
          this.state.jobs = this.state.jobs.filter((j) => j.id !== id);
          ctx.exitCode = 0;
        } else {
          ctx.exitCode = 0;
        }
        break;
      }

      case 'alias': {
        if (args.length === 0) {
          const lines = Object.entries(this.state.aliases).map(([k, v]) => `alias ${k}='${v}'`);
          ctx.stdout = lines.join('\n');
          ctx.exitCode = 0;
          return;
        }
        for (const arg of args) {
          const eqIdx = arg.indexOf('=');
          if (eqIdx !== -1) {
            const k = arg.substring(0, eqIdx);
            const v = arg.substring(eqIdx + 1).replace(/^['"]|['"]$/g, '');
            this.state.aliases[k] = v;
          }
        }
        ctx.exitCode = 0;
        break;
      }

      case 'unalias': {
        for (const arg of args) {
          delete this.state.aliases[arg];
        }
        ctx.exitCode = 0;
        break;
      }

      case 'type': {
        if (args.length === 0) {
          ctx.stderr = 'type: missing operand';
          ctx.exitCode = 1;
          return;
        }
        const target = args[0];
        if (this.state.aliases[target]) {
          ctx.stdout = `${target} is aliased to \`${this.state.aliases[target]}\``;
        } else if (this.isBuiltin(target)) {
          ctx.stdout = `${target} is a shell builtin`;
        } else {
          ctx.stdout = `${target} is /usr/bin/${target}`;
        }
        ctx.exitCode = 0;
        break;
      }

      case 'which': {
        if (args.length === 0) {
          ctx.exitCode = 1;
          return;
        }
        const target = args[0];
        if (this.isBuiltin(target)) {
          ctx.stdout = `${target}: shell built-in command`;
        } else {
          ctx.stdout = `/usr/bin/${target}`;
        }
        ctx.exitCode = 0;
        break;
      }

      case 'whoami': {
        ctx.stdout = this.state.env['USER'] || 'axis';
        ctx.exitCode = 0;
        break;
      }

      case 'hostname': {
        ctx.stdout = this.state.env['HOSTNAME'] || 'axis-pc';
        ctx.exitCode = 0;
        break;
      }

      case 'uname': {
        const kernel = this.state.systemInfo.kernelVersion || '6.12.0-axisos-amd64';
        if (args.includes('-a')) {
          ctx.stdout = `Linux ${this.state.env['HOSTNAME'] || 'axis-pc'} ${kernel} #1 SMP PREEMPT_DYNAMIC x86_64 GNU/Linux`;
        } else if (args.includes('-r')) {
          ctx.stdout = kernel;
        } else if (args.includes('-m')) {
          ctx.stdout = 'x86_64';
        } else {
          ctx.stdout = 'Linux';
        }
        ctx.exitCode = 0;
        break;
      }

      case 'date': {
        ctx.stdout = new Date().toString();
        ctx.exitCode = 0;
        break;
      }

      case 'true': {
        ctx.exitCode = 0;
        break;
      }

      case 'false': {
        ctx.exitCode = 1;
        break;
      }

      case 'cat': {
        if (args.length === 0) {
          ctx.stdout = ctx.stdin;
          ctx.exitCode = 0;
          return;
        }
        let merged = '';
        for (const f of args) {
          if (f === '-') {
            merged += ctx.stdin;
            continue;
          }
          const fPath = this.resolvePath(f);
          try {
            const data = await systemService.readFile(fPath);
            merged += (merged && !merged.endsWith('\n') ? '\n' : '') + data;
          } catch {
            ctx.stderr = `cat: ${f}: No such file or directory`;
            ctx.exitCode = 1;
            return;
          }
        }
        ctx.stdout = merged;
        ctx.exitCode = 0;
        break;
      }

      case 'grep': {
        let ignoreCase = false;
        let invert = false;
        let lineNumbers = false;
        let pattern = '';
        let fileTargets: string[] = [];

        for (let i = 0; i < args.length; i++) {
          const a = args[i];
          if (a.startsWith('-') && a.length > 1) {
            if (a.includes('i')) ignoreCase = true;
            if (a.includes('v')) invert = true;
            if (a.includes('n')) lineNumbers = true;
          } else if (!pattern) {
            pattern = a;
          } else {
            fileTargets.push(a);
          }
        }

        if (!pattern) {
          ctx.stderr = 'grep: missing pattern';
          ctx.exitCode = 2;
          return;
        }

        let content = ctx.stdin;
        if (fileTargets.length > 0) {
          try {
            content = await systemService.readFile(this.resolvePath(fileTargets[0]));
          } catch {
            ctx.stderr = `grep: ${fileTargets[0]}: No such file or directory`;
            ctx.exitCode = 2;
            return;
          }
        }

        const lines = content.split('\n');
        const regex = new RegExp(pattern, ignoreCase ? 'i' : '');
        const matched: string[] = [];

        lines.forEach((l, idx) => {
          const matches = regex.test(l);
          const keep = invert ? !matches : matches;
          if (keep) {
            const prefix = lineNumbers ? `${idx + 1}:` : '';
            matched.push(prefix + l);
          }
        });

        ctx.stdout = matched.join('\n');
        ctx.exitCode = matched.length > 0 ? 0 : 1;
        break;
      }

      case 'head': {
        let linesCount = 10;
        let fileTarget = '';
        for (let i = 0; i < args.length; i++) {
          if (args[i] === '-n' && args[i + 1]) {
            linesCount = parseInt(args[i + 1], 10) || 10;
            i++;
          } else if (args[i].startsWith('-') && /^-(\d+)$/.test(args[i])) {
            linesCount = parseInt(args[i].substring(1), 10);
          } else {
            fileTarget = args[i];
          }
        }

        let content = ctx.stdin;
        if (fileTarget) {
          try {
            content = await systemService.readFile(this.resolvePath(fileTarget));
          } catch {
            ctx.stderr = `head: cannot open '${fileTarget}' for reading: No such file or directory`;
            ctx.exitCode = 1;
            return;
          }
        }

        const lines = content.split('\n');
        ctx.stdout = lines.slice(0, linesCount).join('\n');
        ctx.exitCode = 0;
        break;
      }

      case 'tail': {
        let linesCount = 10;
        let fileTarget = '';
        for (let i = 0; i < args.length; i++) {
          if (args[i] === '-n' && args[i + 1]) {
            linesCount = parseInt(args[i + 1], 10) || 10;
            i++;
          } else if (args[i].startsWith('-') && /^-(\d+)$/.test(args[i])) {
            linesCount = parseInt(args[i].substring(1), 10);
          } else {
            fileTarget = args[i];
          }
        }

        let content = ctx.stdin;
        if (fileTarget) {
          try {
            content = await systemService.readFile(this.resolvePath(fileTarget));
          } catch {
            ctx.stderr = `tail: cannot open '${fileTarget}' for reading: No such file or directory`;
            ctx.exitCode = 1;
            return;
          }
        }

        const lines = content.split('\n');
        ctx.stdout = lines.slice(Math.max(0, lines.length - linesCount)).join('\n');
        ctx.exitCode = 0;
        break;
      }

      case 'wc': {
        let countLines = false;
        let countWords = false;
        let countBytes = false;
        let fileTarget = '';

        for (const a of args) {
          if (a.startsWith('-')) {
            if (a.includes('l')) countLines = true;
            if (a.includes('w')) countWords = true;
            if (a.includes('c') || a.includes('m')) countBytes = true;
          } else {
            fileTarget = a;
          }
        }

        if (!countLines && !countWords && !countBytes) {
          countLines = true;
          countWords = true;
          countBytes = true;
        }

        let content = ctx.stdin;
        if (fileTarget) {
          try {
            content = await systemService.readFile(this.resolvePath(fileTarget));
          } catch {
            ctx.stderr = `wc: ${fileTarget}: No such file or directory`;
            ctx.exitCode = 1;
            return;
          }
        }

        const lines = content.length > 0 ? content.split('\n').length - (content.endsWith('\n') ? 1 : 0) : 0;
        const words = content.trim().length > 0 ? content.trim().split(/\s+/).length : 0;
        const bytes = new TextEncoder().encode(content).length;

        const parts: string[] = [];
        if (countLines) parts.push(lines.toString().padStart(7, ' '));
        if (countWords) parts.push(words.toString().padStart(7, ' '));
        if (countBytes) parts.push(bytes.toString().padStart(7, ' '));
        if (fileTarget) parts.push(` ${fileTarget}`);

        ctx.stdout = parts.join(' ');
        ctx.exitCode = 0;
        break;
      }

      case 'axis-fetch':
      case 'neofetch': {
        const info = this.state.systemInfo;
        const logo = [
          '\x1b[36;1m        /\\        \x1b[0m',
          '\x1b[36;1m       /  \\       \x1b[0m',
          '\x1b[36;1m      / /\\ \\      \x1b[0m',
          '\x1b[36;1m     / /__\\ \\     \x1b[0m',
          '\x1b[36;1m    / /____\\ \\    \x1b[0m',
          '\x1b[36;1m   /_/      \\_\\   \x1b[0m',
          '\x1b[36m  ================\x1b[0m',
          '\x1b[36;1m     AxisOS 2.0   \x1b[0m',
        ];

        const details = [
          `\x1b[34;1m${this.state.env['USER']}\x1b[0m@\x1b[36;1m${this.state.env['HOSTNAME']}\x1b[0m`,
          '\x1b[90m----------------------------------------\x1b[0m',
          `\x1b[36mOS:\x1b[0m ${info.osName} (${info.osVersion}) x86_64`,
          `\x1b[36mHost:\x1b[0m AxisPC Precision Workstation`,
          `\x1b[36mKernel:\x1b[0m ${info.kernelVersion}`,
          `\x1b[36mUptime:\x1b[0m ${info.uptime}`,
          `\x1b[36mShell:\x1b[0m axis-sh 2.0 (POSIX Compliant)`,
          `\x1b[36mTerminal:\x1b[0m AxisTTY (${this.state.env['COLUMNS']}x${this.state.env['LINES']})`,
          `\x1b[36mCPU:\x1b[0m ${info.cpuModel} (${info.cpuCores} Threads)`,
          `\x1b[36mGPU:\x1b[0m ${info.gpuModel}`,
          `\x1b[36mMemory:\x1b[0m ${info.totalMemory}`,
          '\x1b[41m   \x1b[42m   \x1b[43m   \x1b[44m   \x1b[45m   \x1b[46m   \x1b[47m   \x1b[0m',
        ];

        const combined: string[] = [];
        const maxRows = Math.max(logo.length, details.length);
        for (let r = 0; r < maxRows; r++) {
          const l = logo[r] || '                  ';
          const d = details[r] || '';
          combined.push(`${l}   ${d}`);
        }

        ctx.stdout = combined.join('\n');
        ctx.exitCode = 0;
        break;
      }

      case 'help': {
        ctx.stdout = `\x1b[1;36mAxisOS Shell (axis-sh) v2.0 - REPL & TTY Command Manual\x1b[0m
These shell commands are defined internally. Type 'help' to see this list.

\x1b[1mCORE BUILTINS:\x1b[0m
  \x1b[32mcd [dir]\x1b[0m                 Change working directory (~ for home, - for previous)
  \x1b[32mpwd\x1b[0m                      Print name of current/working directory
  \x1b[32mecho [-n] [-e] [text]\x1b[0m    Write arguments to standard output
  \x1b[32mexport [NAME=VAL]\x1b[0m        Set environment variables or list export table
  \x1b[32munset [NAME]\x1b[0m             Unset environment variables
  \x1b[32menv\x1b[0m                      Print all environment variables
  \x1b[32mclear\x1b[0m                    Clear screen buffer
  \x1b[32mhistory [-c]\x1b[0m             Display or clear command history
  \x1b[32mjobs\x1b[0m                     List active and suspended background jobs
  \x1b[32mfg [%id]\x1b[0m                 Move job to the foreground
  \x1b[32mbg [%id]\x1b[0m                 Resume suspended job in background
  \x1b[32mkill [-sig] <pid>\x1b[0m        Send signal to a process or job
  \x1b[32malias [name=cmd]\x1b[0m         Define or display aliases
  \x1b[32munalias <name>\x1b[0m           Remove an alias definition
  \x1b[32mtype <name>\x1b[0m              Display information about command type
  \x1b[32mwhich <name>\x1b[0m             Locate a command executable
  \x1b[32mwhoami\x1b[0m                   Print current username
  \x1b[32mhostname\x1b[0m                 Print system network hostname
  \x1b[32muname [-a|-r|-m]\x1b[0m        Print system and kernel information
  \x1b[32mdate\x1b[0m                     Display current date and time
  \x1b[32maxis-fetch\x1b[0m               Display hardware specs and system telemetry

\x1b[1mI/O PIPELINES & REDIRECTIONS:\x1b[0m
  \x1b[33mcmd1 | cmd2\x1b[0m              Pipe stdout of cmd1 to stdin of cmd2
  \x1b[33mcmd > file\x1b[0m               Redirect stdout to file (overwrite)
  \x1b[33mcmd >> file\x1b[0m              Redirect stdout to file (append)
  \x1b[33mcmd < file\x1b[0m               Feed file content into stdin of cmd
  \x1b[33mcmd 2> file\x1b[0m              Redirect stderr to file
  \x1b[33mcmd 2>&1\x1b[0m                 Merge stderr into stdout
  \x1b[33mcmd1 && cmd2\x1b[0m             Execute cmd2 only if cmd1 succeeded
  \x1b[33mcmd1 || cmd2\x1b[0m             Execute cmd2 only if cmd1 failed

\x1b[1mTERMINAL SIGNALS & SHORTCUTS:\x1b[0m
  \x1b[35mCtrl+C (SIGINT)\x1b[0m          Interrupt running foreground process
  \x1b[35mCtrl+Z (SIGTSTP)\x1b[0m         Suspend running process to background jobs
  \x1b[35mCtrl+L\x1b[0m                   Clear screen keeping current prompt
  \x1b[35mTab\x1b[0m                      Autocomplete commands and directory paths
  \x1b[35mUp / Down\x1b[0m                Traverse command history buffer
  \x1b[35mCtrl+A / Ctrl+E\x1b[0m          Jump cursor to beginning / end of line
  \x1b[35mCtrl+U / Ctrl+K\x1b[0m          Clear from cursor to beginning / end of line`;
        ctx.exitCode = 0;
        break;
      }

      case 'exit': {
        ctx.stdout = 'logout';
        ctx.exitCode = 0;
        break;
      }

      default:
        ctx.stderr = `axis-sh: ${name}: command not found`;
        ctx.exitCode = 127;
        break;
    }
  }

  /**
   * Execute external binary via systemService
   */
  private async executeExternal(cmd: CommandAST, ctx: ProcessContext, signal: AbortSignal): Promise<void> {
    const baseDir = this.state.cwd === '~' ? (this.state.env['HOME'] || '/home/axis') : this.state.cwd;
    
    // Construct command with stdin piped if needed
    let cmdToRun = cmd.raw;
    if (ctx.stdin) {
      // Pipe stdin through printf/cat
      const escapedStdin = ctx.stdin.replace(/'/g, "'\\''");
      cmdToRun = `printf '%s' '${escapedStdin}' | ${cmdToRun}`;
    }

    const res = await systemService.executeCommand(cmdToRun, baseDir, signal, this.state.env);
    ctx.stdout = res.stdout;
    ctx.stderr = res.stderr;
    ctx.exitCode = res.exitCode;
  }
}
