// AxisOS Terminal Emulator & Shell Interface (axis-sh v2.0)
// Complete REPL Architecture, Lexical Analysis, I/O Routing, Signals (SIGINT, SIGTSTP, SIGWINCH),
// ANSI Compliance, Autocompletion, and TTY State Management

import React, { useState, useRef, useEffect, useCallback, useMemo } from 'react';
import { useSystemState } from '../../context/SystemStateContext';
import { systemService } from '../../services/systemService';
import { ShellEngine } from './shellEngine';
import { AnsiRenderer } from './ansiParser';

interface TerminalLine {
  id: string;
  type: 'prompt' | 'stdout' | 'stderr' | 'system' | 'interrupted';
  content: string;
  promptInfo?: {
    user: string;
    host: string;
    cwd: string;
    cmd: string;
    exitCode?: number;
  };
}

export const TerminalApp: React.FC<{ params?: Record<string, any> }> = ({ params }) => {
  const { systemInfo } = useSystemState();
  const engineRef = useRef<ShellEngine | null>(null);

  // Initialize shell engine
  if (!engineRef.current) {
    engineRef.current = new ShellEngine(systemInfo);
  }
  const engine = engineRef.current;

  // Terminal UI state
  const [lines, setLines] = useState<TerminalLine[]>([
    {
      id: 'boot-1',
      type: 'system',
      content: `\x1b[90mLast login: ${new Date().toLocaleString()} on ttys001\x1b[0m`,
    },
    {
      id: 'boot-2',
      type: 'system',
      content: `\x1b[36;1mAxisOS Horizon [Version 2.0.0] - Linux Kernel ${systemInfo.kernelVersion}\x1b[0m`,
    },
    {
      id: 'boot-3',
      type: 'system',
      content: `\x1b[32mType \x1b[1mhelp\x1b[0;32m for commands, \x1b[1maxis-fetch\x1b[0;32m for system telemetry, or run any Linux bash command.\x1b[0m\n`,
    },
  ]);

  const [currentCwd, setCurrentCwd] = useState<string>(params?.cwd || engine.state.cwd);
  const [commandInput, setCommandInput] = useState<string>('');
  const [historyIndex, setHistoryIndex] = useState<number>(-1);
  const [inputDraft, setInputDraft] = useState<string>('');
  const [isExecuting, setIsExecuting] = useState<boolean>(false);
  const [activeRunningCmd, setActiveRunningCmd] = useState<string>('');
  const [lastExitCode, setLastExitCode] = useState<number>(0);

  useEffect(() => {
    if (params?.cwd) {
      setCurrentCwd(params.cwd);
      engine.state.cwd = params.cwd;
      engine.state.env['PWD'] = engine.resolvePath(params.cwd);
    }
  }, [params?.cwd, engine]);

  // TTY Geometry (SIGWINCH emulation)
  const [ttyCols, setTtyCols] = useState<number>(100);
  const [ttyRows, setTtyRows] = useState<number>(32);

  // Autocomplete suggestions
  const [suggestions, setSuggestions] = useState<string[]>([]);

  // DOM Refs
  const containerRef = useRef<HTMLDivElement>(null);
  const inputRef = useRef<HTMLInputElement>(null);
  const bottomRef = useRef<HTMLDivElement>(null);
  const isScrolledToBottomRef = useRef<boolean>(true);

  // Keep engine's systemInfo updated if it changes
  useEffect(() => {
    engine.state.systemInfo = systemInfo;
  }, [systemInfo, engine]);

  // Window Resize (SIGWINCH) Observer
  useEffect(() => {
    const container = containerRef.current;
    if (!container) return;

    const observer = new ResizeObserver((entries) => {
      for (const entry of entries) {
        const { width, height } = entry.contentRect;
        // Monospace approx char box: ~7.6px width, ~18px height
        const cols = Math.max(40, Math.floor(width / 7.6));
        const rows = Math.max(10, Math.floor(height / 18));
        setTtyCols(cols);
        setTtyRows(rows);
        engine.handleResize(cols, rows);
      }
    });

    observer.observe(container);
    return () => observer.disconnect();
  }, [engine]);

  // Auto-scroll handler
  useEffect(() => {
    if (isScrolledToBottomRef.current) {
      bottomRef.current?.scrollIntoView({ behavior: 'auto' });
    }
  }, [lines, isExecuting, suggestions]);

  const handleScroll = () => {
    const el = containerRef.current;
    if (!el) return;
    const isAtBottom = el.scrollHeight - el.scrollTop - el.clientHeight < 40;
    isScrolledToBottomRef.current = isAtBottom;
  };

  // Focus input on viewport click
  const handleContainerClick = () => {
    // Only focus if user didn't make a text selection
    const selection = window.getSelection();
    if (!selection || selection.toString().length === 0) {
      inputRef.current?.focus();
    }
  };

  // Execute command
  const executeCommand = async (rawCmd: string) => {
    const trimmed = rawCmd.trim();
    if (!trimmed) {
      // Empty enter: just print prompt line
      setLines((prev) => [
        ...prev,
        {
          id: `line-${Date.now()}-${Math.random()}`,
          type: 'prompt',
          content: '',
          promptInfo: {
            user: engine.state.env['USER'] || 'axis',
            host: engine.state.env['HOSTNAME'] || 'axis-pc',
            cwd: currentCwd,
            cmd: '',
          },
        },
      ]);
      setCommandInput('');
      setHistoryIndex(-1);
      setInputDraft('');
      setSuggestions([]);
      return;
    }

    // Add prompt line
    const promptLineId = `line-${Date.now()}-${Math.random()}`;
    const promptLine: TerminalLine = {
      id: promptLineId,
      type: 'prompt',
      content: trimmed,
      promptInfo: {
        user: engine.state.env['USER'] || 'axis',
        host: engine.state.env['HOSTNAME'] || 'axis-pc',
        cwd: currentCwd,
        cmd: trimmed,
      },
    };

    setLines((prev) => [...prev, promptLine]);
    setCommandInput('');
    setHistoryIndex(-1);
    setInputDraft('');
    setSuggestions([]);
    setIsExecuting(true);
    setActiveRunningCmd(trimmed);

    try {
      const result = await engine.execute(rawCmd);

      // Handle screen clear
      if (result.shouldClear) {
        setLines([]);
      } else {
        const newLines: TerminalLine[] = [];
        if (result.stdout) {
          newLines.push({
            id: `out-${Date.now()}-${Math.random()}`,
            type: 'stdout',
            content: result.stdout,
          });
        }
        if (result.stderr) {
          newLines.push({
            id: `err-${Date.now()}-${Math.random()}`,
            type: 'stderr',
            content: result.stderr,
          });
        }
        if (newLines.length > 0) {
          setLines((prev) => [...prev, ...newLines]);
        }
      }

      setLastExitCode(result.exitCode);
      setCurrentCwd(engine.state.cwd);
      // Stamp exitCode on prompt line for historical visual reflection
      setLines((prev) =>
        prev.map((l) =>
          l.id === promptLineId && l.promptInfo
            ? { ...l, promptInfo: { ...l.promptInfo, exitCode: result.exitCode } }
            : l
        )
      );
    } catch (err: any) {
      setLines((prev) => [
        ...prev,
        {
          id: `err-${Date.now()}`,
          type: 'stderr',
          content: `\x1b[31;1maxis-sh: internal fatal error: ${err.message || err}\x1b[0m`,
        },
      ]);
      setLastExitCode(1);
      setLines((prev) =>
        prev.map((l) =>
          l.id === promptLineId && l.promptInfo
            ? { ...l, promptInfo: { ...l.promptInfo, exitCode: 1 } }
            : l
        )
      );
    } finally {
      setIsExecuting(false);
      setActiveRunningCmd('');
      setTimeout(() => inputRef.current?.focus(), 10);
    }
  };

  // Autocomplete Tab handler
  const handleTabCompletion = async () => {
    if (!commandInput.trim()) return;

    const parts = commandInput.split(' ');
    const lastPart = parts[parts.length - 1];

    // Case 1: First word completion (commands, builtins, aliases)
    if (parts.length === 1) {
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
        'axis-fetch',
        'neofetch',
        'help',
        'cat',
        'grep',
        'head',
        'tail',
        'wc',
        'exit',
        'axis',
        'axis-update',
      ];
      const commonBins = [
        'ls',
        'cp',
        'mv',
        'rm',
        'mkdir',
        'rmdir',
        'chmod',
        'chown',
        'touch',
        'find',
        'nano',
        'vim',
        'git',
        'python3',
        'node',
        'npm',
        'apt',
        'systemctl',
        'journalctl',
        'ip',
        'ping',
        'curl',
        'wget',
        'htop',
        'top',
        'df',
        'free',
        'dmesg',
        'ps',
      ];
      const aliases = Object.keys(engine.state.aliases);
      const allCandidates = Array.from(new Set([...builtins, ...commonBins, ...aliases]));

      const matches = allCandidates.filter((c) => c.startsWith(lastPart.toLowerCase()));
      if (matches.length === 1) {
        setCommandInput(matches[0] + ' ');
        setSuggestions([]);
      } else if (matches.length > 1) {
        setSuggestions(matches);
      }
      return;
    }

    // Case 2: Environment variable completion ($VAR)
    if (lastPart.startsWith('$')) {
      const varPrefix = lastPart.substring(1).toUpperCase();
      const envKeys = Object.keys(engine.state.env);
      const matches = envKeys.filter((k) => k.startsWith(varPrefix));
      if (matches.length === 1) {
        parts[parts.length - 1] = '$' + matches[0];
        setCommandInput(parts.join(' ') + ' ');
        setSuggestions([]);
      } else if (matches.length > 1) {
        setSuggestions(matches.map((m) => '$' + m));
      }
      return;
    }

    // Case 3: Filesystem path completion
    try {
      const resolvedDir = engine.resolvePath(
        lastPart.includes('/') ? lastPart.substring(0, lastPart.lastIndexOf('/')) : '.'
      );
      const dirContents = await systemService.readDirectory(resolvedDir);
      const searchItem = lastPart.includes('/') ? lastPart.substring(lastPart.lastIndexOf('/') + 1) : lastPart;

      const matches = dirContents.items.filter((item) =>
        item.name.toLowerCase().startsWith(searchItem.toLowerCase())
      );

      if (matches.length === 1) {
        const item = matches[0];
        const dirPrefix = lastPart.includes('/') ? lastPart.substring(0, lastPart.lastIndexOf('/') + 1) : '';
        const completedPath = dirPrefix + item.name + (item.type === 'folder' ? '/' : ' ');
        parts[parts.length - 1] = completedPath;
        setCommandInput(parts.join(' '));
        setSuggestions([]);
      } else if (matches.length > 1) {
        setSuggestions(matches.map((m) => m.name + (m.type === 'folder' ? '/' : '')));
      }
    } catch {}
  };

  // Global Keyboard Navigation & POSIX Signal Trapping
  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    // 1. SIGINT (Ctrl+C)
    if (e.ctrlKey && (e.key === 'c' || e.key === 'C')) {
      e.preventDefault();
      if (isExecuting) {
        engine.sendSigInt();
        setLines((prev) => [
          ...prev,
          {
            id: `sigint-${Date.now()}`,
            type: 'interrupted',
            content: '^C',
          },
        ]);
        setIsExecuting(false);
        setActiveRunningCmd('');
        setLastExitCode(130);
      } else {
        // Idle Ctrl+C: Cancel current input, print ^C, start fresh line
        setLines((prev) => [
          ...prev,
          {
            id: `line-${Date.now()}`,
            type: 'prompt',
            content: commandInput + '^C',
            promptInfo: {
              user: engine.state.env['USER'] || 'axis',
              host: engine.state.env['HOSTNAME'] || 'axis-pc',
              cwd: currentCwd,
              cmd: commandInput + '^C',
            },
          },
        ]);
        setCommandInput('');
        setHistoryIndex(-1);
        setInputDraft('');
        setSuggestions([]);
      }
      return;
    }

    // 2. SIGTSTP (Ctrl+Z)
    if (e.ctrlKey && (e.key === 'z' || e.key === 'Z')) {
      e.preventDefault();
      if (isExecuting) {
        const job = engine.sendSigTstp(activeRunningCmd);
        if (job) {
          setLines((prev) => [
            ...prev,
            {
              id: `sigtstp-${Date.now()}`,
              type: 'system',
              content: `\n[${job.id}]+  Stopped                 ${job.command}`,
            },
          ]);
        }
        setIsExecuting(false);
        setActiveRunningCmd('');
        setLastExitCode(148);
      }
      return;
    }

    // 3. Clear Screen (Ctrl+L)
    if (e.ctrlKey && (e.key === 'l' || e.key === 'L')) {
      e.preventDefault();
      setLines([]);
      setSuggestions([]);
      return;
    }

    // 4. Erase to beginning of line (Ctrl+U)
    if (e.ctrlKey && (e.key === 'u' || e.key === 'U')) {
      e.preventDefault();
      setCommandInput('');
      return;
    }

    // 5. Jump to start (Ctrl+A)
    if (e.ctrlKey && (e.key === 'a' || e.key === 'A')) {
      e.preventDefault();
      if (inputRef.current) {
        inputRef.current.setSelectionRange(0, 0);
      }
      return;
    }

    // 6. Jump to end (Ctrl+E)
    if (e.ctrlKey && (e.key === 'e' || e.key === 'E')) {
      e.preventDefault();
      if (inputRef.current) {
        const len = inputRef.current.value.length;
        inputRef.current.setSelectionRange(len, len);
      }
      return;
    }

    // 7. Delete previous word (Ctrl+W)
    if (e.ctrlKey && (e.key === 'w' || e.key === 'W')) {
      e.preventDefault();
      const trimmed = commandInput.trimEnd();
      const lastSpace = trimmed.lastIndexOf(' ');
      if (lastSpace !== -1) {
        setCommandInput(trimmed.substring(0, lastSpace + 1));
      } else {
        setCommandInput('');
      }
      return;
    }

    // 8. Tab Autocompletion
    if (e.key === 'Tab') {
      e.preventDefault();
      handleTabCompletion();
      return;
    }

    // 9. Enter Execution
    if (e.key === 'Enter') {
      e.preventDefault();
      executeCommand(commandInput);
      return;
    }

    // 10. History Up
    if (e.key === 'ArrowUp') {
      e.preventDefault();
      const hist = engine.state.history;
      if (hist.length > 0) {
        if (historyIndex === -1) {
          setInputDraft(commandInput);
          const newIdx = hist.length - 1;
          setHistoryIndex(newIdx);
          setCommandInput(hist[newIdx]);
        } else if (historyIndex > 0) {
          const newIdx = historyIndex - 1;
          setHistoryIndex(newIdx);
          setCommandInput(hist[newIdx]);
        }
      }
      return;
    }

    // 11. History Down
    if (e.key === 'ArrowDown') {
      e.preventDefault();
      const hist = engine.state.history;
      if (hist.length > 0 && historyIndex !== -1) {
        if (historyIndex < hist.length - 1) {
          const newIdx = historyIndex + 1;
          setHistoryIndex(newIdx);
          setCommandInput(hist[newIdx]);
        } else {
          setHistoryIndex(-1);
          setCommandInput(inputDraft);
        }
      }
      return;
    }
  };

  return (
    <div
      ref={containerRef}
      onClick={handleContainerClick}
      onScroll={handleScroll}
      className="h-full w-full bg-[#0a0e14] text-slate-200 flex flex-col font-mono text-xs select-text overflow-hidden"
      style={{
        fontFamily: 'ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace',
      }}
    >
      {/* TTY Status Bar / Geometry Header */}
      <div className="flex items-center justify-between px-3 py-1.5 bg-[#0f141c] border-b border-slate-800/80 text-[11px] text-slate-400 select-none shrink-0 shadow-sm">
        <div className="flex items-center gap-2">
          <span className="w-2.5 h-2.5 rounded-full bg-[#87cf3e] animate-pulse inline-block shadow-sm shadow-[#87cf3e]/50" />
          <span className="font-semibold text-slate-200">
            {engine.state.env['USER']}@{engine.state.env['HOSTNAME']}
          </span>
          <span className="text-slate-600">:</span>
          <span className="text-[#87cf3e] font-medium">{currentCwd}</span>
        </div>

        <div className="flex items-center gap-3">
          {/* Foreground execution indicator */}
          {isExecuting && (
            <div className="flex items-center gap-1.5 px-2 py-0.5 rounded bg-[#87cf3e]/10 text-[#87cf3e] border border-[#87cf3e]/20 text-[10px]">
              <span className="w-1.5 h-1.5 rounded-full bg-[#87cf3e] animate-ping inline-block" />
              <span>exec: {activeRunningCmd.slice(0, 20)}</span>
            </div>
          )}

          {/* Return code indicator */}
          <div className="flex items-center gap-1 px-1.5 py-0.5 rounded bg-slate-900 border border-slate-800 text-[10px]">
            <span className="text-slate-500">$?</span>
            <span className={lastExitCode === 0 ? 'text-[#87cf3e] font-bold' : 'text-rose-400 font-bold'}>
              {lastExitCode}
            </span>
          </div>

          {/* Window size */}
          <span className="text-slate-500 hover:text-slate-300 transition-colors">
            {ttyCols}x{ttyRows}
          </span>

          <span className="px-1.5 py-0.5 rounded bg-slate-800/60 text-slate-400 text-[10px]">
            axis-sh
          </span>
        </div>
      </div>

      {/* Main Terminal Buffer */}
      <div className="flex-1 p-4 overflow-y-auto space-y-1 cursor-text">
        {lines.map((line) => {
          if (line.type === 'prompt' && line.promptInfo) {
            const hasFailed = line.promptInfo.exitCode !== undefined && line.promptInfo.exitCode !== 0;
            return (
              <div key={line.id} className="leading-relaxed flex items-baseline gap-2 pt-1">
                <span className="text-[#87cf3e] font-bold select-none shrink-0">
                  {line.promptInfo.user}@{line.promptInfo.host}
                </span>
                <span
                  className={
                    hasFailed
                      ? 'text-rose-400 font-bold bg-rose-500/15 px-1.5 py-0.5 rounded border border-rose-500/30 select-none shrink-0'
                      : 'text-[#87cf3e] font-semibold select-none shrink-0'
                  }
                >
                  {line.promptInfo.cwd}
                </span>
                <span className={hasFailed ? 'text-rose-400 font-bold select-none shrink-0' : 'text-slate-500 select-none shrink-0'}>
                  {hasFailed ? `[exit ${line.promptInfo.exitCode}] ✗ %` : '%'}
                </span>
                <span className="text-white font-medium break-all">{line.promptInfo.cmd}</span>
              </div>
            );
          }

          if (line.type === 'interrupted') {
            return (
              <div key={line.id} className="text-rose-400 font-bold py-0.5">
                {line.content}
              </div>
            );
          }

          if (line.type === 'stderr') {
            return (
              <div key={line.id} className="text-rose-400 leading-relaxed">
                <AnsiRenderer text={line.content} />
              </div>
            );
          }

          // stdout or system lines
          return (
            <div key={line.id} className="text-slate-200 leading-relaxed">
              <AnsiRenderer text={line.content} />
            </div>
          );
        })}

        {/* Tab Autocomplete Suggestions Box */}
        {suggestions.length > 0 && (
          <div className="my-2 p-2 rounded bg-slate-900/90 border border-slate-800 text-[11px] grid grid-cols-3 sm:grid-cols-4 md:grid-cols-6 gap-2 text-[#87cf3e]">
            {suggestions.map((s, idx) => (
              <span
                key={idx}
                onClick={() => {
                  const parts = commandInput.split(' ');
                  parts[parts.length - 1] = s;
                  setCommandInput(parts.join(' ') + ' ');
                  setSuggestions([]);
                  inputRef.current?.focus();
                }}
                className="hover:text-white hover:underline cursor-pointer truncate"
              >
                {s}
              </span>
            ))}
          </div>
        )}

        {/* Active Command Prompt Line */}
        <div className="flex items-center gap-2 pt-1 text-xs">
          <span className="text-[#87cf3e] font-bold select-none shrink-0">
            {engine.state.env['USER'] || 'axis'}@{engine.state.env['HOSTNAME'] || 'axis-pc'}
          </span>
          <span
            className={
              lastExitCode === 0
                ? 'text-[#87cf3e] font-semibold select-none shrink-0 transition-colors'
                : 'text-rose-400 font-bold bg-rose-500/15 px-1.5 py-0.5 rounded border border-rose-500/30 select-none shrink-0 shadow-sm shadow-rose-500/10'
            }
          >
            {currentCwd}
          </span>
          <span
            className={
              lastExitCode === 0
                ? 'text-slate-500 select-none shrink-0'
                : 'text-rose-400 font-bold select-none shrink-0'
            }
          >
            {lastExitCode === 0 ? '%' : `[exit ${lastExitCode}] ✗ %`}
          </span>
          <div className="flex-1 relative flex items-center">
            <input
              ref={inputRef}
              type="text"
              value={commandInput}
              onChange={(e) => {
                setCommandInput(e.target.value);
                if (suggestions.length > 0) setSuggestions([]);
              }}
              onKeyDown={handleKeyDown}
              disabled={isExecuting}
              autoFocus
              spellCheck={false}
              autoComplete="off"
              autoCapitalize="off"
              className="w-full bg-transparent border-none outline-none text-white font-mono text-xs p-0 m-0 caret-[#87cf3e]"
            />
          </div>
        </div>

        <div ref={bottomRef} />
      </div>
    </div>
  );
};
