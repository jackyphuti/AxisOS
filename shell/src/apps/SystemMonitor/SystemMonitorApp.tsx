import React, { useState, useEffect } from 'react';
import { Activity, Cpu, HardDrive, RefreshCw, XCircle } from 'lucide-react';
import { useSystemState } from '../../context/SystemStateContext';
import { systemService } from '../../services/systemService';

interface ProcessItem {
  pid: number;
  name: string;
  user: string;
  cpu: number;
  mem: string;
  status: 'running' | 'sleeping';
}

export const SystemMonitorApp: React.FC = () => {
  const { systemInfo } = useSystemState();
  const [activeTab, setActiveTab] = useState<'cpu' | 'memory' | 'disk'>('cpu');
  const [cpuUsage, setCpuUsage] = useState(14);
  const [cpuHistory, setCpuHistory] = useState<number[]>([12, 14, 18, 15, 22, 18, 14, 16, 20, 14, 19, 15]);
  const [selectedPid, setSelectedPid] = useState<number | null>(null);
  const [isKilling, setIsKilling] = useState(false);
  const [processes, setProcesses] = useState<ProcessItem[]>([
    { pid: 1, name: 'systemd', user: 'root', cpu: 0.1, mem: '14.2 MB', status: 'sleeping' },
    { pid: 320, name: 'axisos-session', user: systemInfo.username, cpu: 3.2, mem: '140 MB', status: 'running' },
    { pid: 388, name: 'cage-compositor', user: systemInfo.username, cpu: 2.1, mem: '84 MB', status: 'running' },
    { pid: 412, name: 'pipewire', user: systemInfo.username, cpu: 0.4, mem: '28 MB', status: 'sleeping' },
    { pid: 480, name: 'NetworkManager', user: 'root', cpu: 0.2, mem: '34 MB', status: 'sleeping' },
    { pid: 512, name: 'wireplumber', user: systemInfo.username, cpu: 0.3, mem: '22 MB', status: 'sleeping' },
    { pid: 640, name: 'electron (AxisShell)', user: systemInfo.username, cpu: 4.8, mem: '168 MB', status: 'running' },
    { pid: 720, name: 'btrfs-transacti', user: 'root', cpu: 0.0, mem: '0 MB', status: 'sleeping' },
  ]);

  // Real process polling if possible
  useEffect(() => {
    const updateStats = async () => {
      try {
        const res = await systemService.executeCommand('ps -eo pid,user,%cpu,%mem,comm --sort=-%cpu | head -n 12');
        if (res.stdout) {
          const lines = res.stdout.trim().split('\n').slice(1);
          const parsed: ProcessItem[] = lines.map((line) => {
            const parts = line.trim().split(/\s+/);
            return {
              pid: parseInt(parts[0], 10) || 0,
              user: parts[1] || 'root',
              cpu: parseFloat(parts[2]) || 0,
              mem: `${parts[3]}%`,
              name: parts[4] || 'process',
              status: parseFloat(parts[2]) > 0.5 ? 'running' : 'sleeping',
            };
          });
          if (parsed.length > 0) setProcesses(parsed);
        }
      } catch {}

      const nextUsage = Math.floor(12 + Math.random() * 16);
      setCpuUsage(nextUsage);
      setCpuHistory((prev) => [...prev.slice(1), nextUsage]);
    };

    updateStats();
    const interval = setInterval(updateStats, 2000);
    return () => clearInterval(interval);
  }, []);

  const handleKillProcess = async () => {
    if (!selectedPid) return;
    setIsKilling(true);
    try {
      await systemService.executeCommand(`kill -9 ${selectedPid}`);
      setProcesses((prev) => prev.filter((p) => p.pid !== selectedPid));
      setSelectedPid(null);
    } catch {}
    setIsKilling(false);
  };

  return (
    <div className="flex flex-col h-full w-full bg-[#F5F5F7] dark:bg-slate-950 text-slate-900 dark:text-slate-100 select-none">
      {/* Top Activity Monitor Toolbar */}
      <div className="h-10 px-3 flex items-center justify-between border-b border-black/5 dark:border-white/10 bg-white/80 dark:bg-slate-900/60 text-xs">
        {/* Tabs */}
        <div className="flex items-center p-0.5 rounded-lg bg-black/5 dark:bg-white/5 border border-black/5 dark:border-white/10">
          {(['cpu', 'memory', 'disk'] as const).map((tab) => (
            <button
              key={tab}
              onClick={() => setActiveTab(tab)}
              className={`px-3 py-1 rounded-md uppercase font-medium text-[10px] tracking-wider transition-colors ${
                activeTab === tab ? 'bg-white dark:bg-white/20 text-slate-900 dark:text-white shadow-xs' : 'text-slate-500 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white'
              }`}
            >
              {tab}
            </button>
          ))}
        </div>

        <div className="flex items-center space-x-2 text-slate-500 dark:text-slate-400 text-xs">
          {selectedPid && (
            <button
              onClick={handleKillProcess}
              disabled={isKilling}
              className="flex items-center space-x-1 px-2.5 py-1 rounded bg-rose-500/20 hover:bg-rose-500/30 text-rose-600 dark:text-rose-300 border border-rose-500/30 text-[11px] font-semibold transition-colors"
            >
              <XCircle className="w-3.5 h-3.5" />
              <span>{isKilling ? 'Killing...' : `Force Quit (${selectedPid})`}</span>
            </button>
          )}
          <span>{systemInfo.cpuCores} Threads Active</span>
        </div>
      </div>

      {/* Overview Cards */}
      <div className="p-4 grid grid-cols-3 gap-3 border-b border-black/5 dark:border-white/10 bg-black/[0.02] dark:bg-slate-900/40">
        <div className="p-3 rounded-2xl bg-white dark:bg-white/5 border border-slate-200/80 dark:border-white/5 shadow-xs">
          <div className="flex items-center justify-between text-xs text-slate-500 dark:text-slate-400">
            <span>CPU Usage</span>
            <Cpu className="w-4 h-4 text-cyan-500 dark:text-cyan-400" />
          </div>
          <div className="text-xl font-bold text-cyan-600 dark:text-cyan-400 font-mono mt-1">{cpuUsage}%</div>
          <div className="h-6 flex items-end gap-1 mt-2">
            {cpuHistory.map((val, idx) => (
              <div
                key={idx}
                className="flex-1 bg-cyan-500/70 rounded-xs transition-all duration-300"
                style={{ height: `${(val / 40) * 100}%` }}
              ></div>
            ))}
          </div>
        </div>

        <div className="p-3 rounded-2xl bg-white dark:bg-white/5 border border-slate-200/80 dark:border-white/5 shadow-xs">
          <div className="flex items-center justify-between text-xs text-slate-500 dark:text-slate-400">
            <span>Memory Pressure</span>
            <Activity className="w-4 h-4 text-emerald-500 dark:text-emerald-400" />
          </div>
          <div className="text-xl font-bold text-emerald-600 dark:text-emerald-400 font-mono mt-1">
            {systemInfo.totalMemory}
          </div>
          <div className="w-full bg-slate-200 dark:bg-slate-800 rounded-full h-2 mt-4 overflow-hidden">
            <div className="bg-emerald-500 h-full w-[32%]"></div>
          </div>
        </div>

        <div className="p-3 rounded-2xl bg-white dark:bg-white/5 border border-slate-200/80 dark:border-white/5 shadow-xs">
          <div className="flex items-center justify-between text-xs text-slate-500 dark:text-slate-400">
            <span>Primary Disk</span>
            <HardDrive className="w-4 h-4 text-purple-500 dark:text-purple-400" />
          </div>
          <div className="text-base font-bold text-purple-600 dark:text-purple-300 truncate mt-1">
            {systemInfo.storageCapacity}
          </div>
          <div className="w-full bg-slate-200 dark:bg-slate-800 rounded-full h-2 mt-4 overflow-hidden">
            <div className="bg-purple-500 h-full w-[20%]"></div>
          </div>
        </div>
      </div>

      {/* Process Table Header */}
      <div className="px-4 py-1.5 bg-slate-100/90 dark:bg-slate-900 border-b border-black/5 dark:border-white/10 flex text-[11px] font-semibold text-slate-500 dark:text-slate-400 font-mono">
        <span className="w-20">Process Name</span>
        <span className="w-16">PID</span>
        <span className="w-20">User</span>
        <span className="w-20 text-right">% CPU</span>
        <span className="w-24 text-right">Memory</span>
      </div>

      {/* Process Table Body */}
      <div className="flex-1 overflow-y-auto text-xs font-mono">
        {processes.map((proc, i) => (
          <div
            key={i}
            onClick={() => setSelectedPid(proc.pid === selectedPid ? null : proc.pid)}
            className={`px-4 py-1.5 border-b border-slate-200/60 dark:border-white/5 flex items-center transition-colors cursor-pointer ${
              selectedPid === proc.pid ? 'bg-blue-100/70 dark:bg-cyan-500/20 text-blue-950 dark:text-white' : 'hover:bg-black/5 dark:hover:bg-white/5 text-slate-700 dark:text-slate-300'
            }`}
          >
            <span className="w-20 font-sans text-slate-900 dark:text-white font-medium truncate flex items-center gap-1.5">
              <span className={`w-1.5 h-1.5 rounded-full ${proc.status === 'running' ? 'bg-emerald-500' : 'bg-slate-400'}`}></span>
              {proc.name}
            </span>
            <span className="w-16 text-slate-400 dark:text-slate-500">{proc.pid}</span>
            <span className="w-20 text-slate-500 dark:text-slate-400 truncate">{proc.user}</span>
            <span className="w-20 text-right text-cyan-600 dark:text-cyan-400">{proc.cpu}%</span>
            <span className="w-24 text-right text-slate-600 dark:text-slate-300">{proc.mem}</span>
          </div>
        ))}
      </div>
    </div>
  );
};
