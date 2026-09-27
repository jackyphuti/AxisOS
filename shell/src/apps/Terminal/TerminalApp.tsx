import React, { useState, useRef, useEffect } from 'react';
import { useSystemState } from '../../context/SystemStateContext';
import { systemService } from '../../services/systemService';

interface TerminalLine {
  id: string;
  type: 'input' | 'output';
  content: string | React.ReactNode;
}

export const TerminalApp: React.FC = () => {
  const { systemInfo } = useSystemState();
  const [currentCwd, setCurrentCwd] = useState<string>('~');
  const [history, setHistory] = useState<TerminalLine[]>([
    {
      id: 'boot-1',
      type: 'output',
      content: `Last login: ${new Date().toLocaleString()} on ttys000`,
    },
    {
      id: 'boot-2',
      type: 'output',
      content: `AxisOS Linux 1.0 (Kernel ${systemInfo.kernelVersion}) - Hardware: ${systemInfo.cpuModel}`,
    },
    {
      id: 'boot-3',
      type: 'output',
      content: 'Type "axis-fetch", "uname -a", "ls", "lscpu", or any Linux bash command.',
    },
  ]);
  const [commandInput, setCommandInput] = useState('');
  const [commandHistory, setCommandHistory] = useState<string[]>([]);
  const [historyIndex, setHistoryIndex] = useState<number>(-1);
  const [isExecuting, setIsExecuting] = useState(false);

  const bottomRef = useRef<HTMLDivElement>(null);
  const inputRef = useRef<HTMLInputElement>(null);

  useEffect(() => {
    bottomRef.current?.scrollIntoView({ behavior: 'smooth' });
  }, [history, isExecuting]);

  const handleCommand = async (rawCmd: string) => {
    const cmd = rawCmd.trim();
    if (!cmd) return;

    setCommandHistory((prev) => [...prev, cmd]);
    setHistoryIndex(-1);

    const inputLine: TerminalLine = {
      id: `in-${Date.now()}`,
      type: 'input',
      content: `${systemInfo.username}@${systemInfo.hostname} ${currentCwd} % ${cmd}`,
    };

    setHistory((prev) => [...prev, inputLine]);
    setCommandInput('');

    const args = cmd.split(' ');
    const primary = args[0].toLowerCase();

    // Client-side quick handlers
    if (primary === 'clear') {
      setHistory([]);
      return;
    }

    if (primary === 'axis-fetch' || primary === 'neofetch') {
      const outputLine: TerminalLine = {
        id: `out-${Date.now()}`,
        type: 'output',
        content: (
          <div className="flex gap-6 py-2 text-xs font-mono">
            <div className="text-cyan-400 font-bold select-none leading-none">
              <pre>{`
        /\\
       /  \\
      / /\\ \\
     / /__\\ \\
    / /____\\ \\
   /_/      \\_\\
  ================
     AxisOS 1.0
              `}</pre>
            </div>
            <div className="space-y-1 text-slate-300">
              <div>
                <span className="text-blue-400 font-bold">{systemInfo.username}</span>@
                <span className="text-sky-400 font-bold">{systemInfo.hostname}</span>
              </div>
              <div className="text-slate-500">----------------------------</div>
              <div><span className="text-cyan-400">OS:</span> {systemInfo.osName} ({systemInfo.osVersion})</div>
              <div><span className="text-cyan-400">Host:</span> AxisPC (Sonoma Edition)</div>
              <div><span className="text-cyan-400">Kernel:</span> {systemInfo.kernelVersion}</div>
              <div><span className="text-cyan-400">Uptime:</span> {systemInfo.uptime}</div>
              <div><span className="text-cyan-400">Shell:</span> bash 5.2.32 (AxisShell)</div>
              <div><span className="text-cyan-400">CPU:</span> {systemInfo.cpuModel} ({systemInfo.cpuCores} Threads)</div>
              <div><span className="text-cyan-400">GPU:</span> {systemInfo.gpuModel}</div>
              <div><span className="text-cyan-400">Memory:</span> {systemInfo.totalMemory}</div>
              <div className="flex gap-1.5 pt-2">
                <span className="w-3.5 h-3.5 rounded-full bg-red-500 inline-block"></span>
                <span className="w-3.5 h-3.5 rounded-full bg-amber-500 inline-block"></span>
                <span className="w-3.5 h-3.5 rounded-full bg-emerald-500 inline-block"></span>
                <span className="w-3.5 h-3.5 rounded-full bg-blue-500 inline-block"></span>
                <span className="w-3.5 h-3.5 rounded-full bg-purple-500 inline-block"></span>
              </div>
            </div>
          </div>
        ),
      };
      setHistory((prev) => [...prev, outputLine]);
      return;
    }

    // Execute real command on host machine via systemService
    setIsExecuting(true);
    try {
      const res = await systemService.executeCommand(cmd);
      setIsExecuting(false);

      const combined = [res.stdout, res.stderr].filter(Boolean).join('\n');
      const outputLine: TerminalLine = {
        id: `out-${Date.now()}`,
        type: 'output',
        content: <pre className="whitespace-pre-wrap font-mono text-xs">{combined || '(Done)'}</pre>,
      };
      setHistory((prev) => [...prev, outputLine]);
    } catch (err: any) {
      setIsExecuting(false);
      const outputLine: TerminalLine = {
        id: `out-${Date.now()}`,
        type: 'output',
        content: <span className="text-rose-400">bash: error executing command: {err.message}</span>,
      };
      setHistory((prev) => [...prev, outputLine]);
    }
  };

  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter') {
      handleCommand(commandInput);
    } else if (e.key === 'ArrowUp') {
      if (commandHistory.length > 0) {
        const nextIndex = historyIndex === -1 ? commandHistory.length - 1 : Math.max(0, historyIndex - 1);
        setHistoryIndex(nextIndex);
        setCommandInput(commandHistory[nextIndex]);
      }
    } else if (e.key === 'ArrowDown') {
      if (commandHistory.length > 0 && historyIndex !== -1) {
        const nextIndex = historyIndex + 1;
        if (nextIndex < commandHistory.length) {
          setHistoryIndex(nextIndex);
          setCommandInput(commandHistory[nextIndex]);
        } else {
          setHistoryIndex(-1);
          setCommandInput('');
        }
      }
    }
  };

  return (
    <div
      onClick={() => inputRef.current?.focus()}
      className="h-full w-full bg-[#0d1117] text-slate-200 p-4 font-mono text-xs overflow-y-auto cursor-text select-text"
    >
      <div className="space-y-1.5">
        {history.map((line) => (
          <div key={line.id} className="leading-relaxed">
            {line.type === 'input' ? (
              <div className="flex gap-2 text-sky-400 font-semibold select-none">
                <span>{line.content}</span>
              </div>
            ) : (
              <div className="text-slate-300">{line.content}</div>
            )}
          </div>
        ))}
      </div>

      {isExecuting && (
        <div className="text-slate-500 italic mt-1 font-mono text-xs animate-pulse">
          Executing command...
        </div>
      )}

      {/* Active input line */}
      <div className="flex items-center gap-2 mt-2">
        <span className="text-sky-400 font-bold select-none">
          {systemInfo.username}@{systemInfo.hostname} {currentCwd} %
        </span>
        <input
          ref={inputRef}
          type="text"
          value={commandInput}
          onChange={(e) => setCommandInput(e.target.value)}
          onKeyDown={handleKeyDown}
          autoFocus
          className="flex-1 bg-transparent border-none outline-none text-white font-mono text-xs p-0 m-0"
        />
      </div>
      <div ref={bottomRef} />
    </div>
  );
};
