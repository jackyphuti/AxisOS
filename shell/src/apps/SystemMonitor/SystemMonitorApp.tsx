import React, { useState, useEffect, useRef } from 'react';
import {
  Activity,
  Cpu,
  HardDrive,
  Wifi,
  Server,
  Layers,
  Clock,
  Search,
  XCircle,
  Play,
  RotateCw,
  Zap,
  Terminal,
  Shield,
  CheckCircle2,
  AlertCircle,
  Sliders,
  ChevronRight,
  Filter,
} from 'lucide-react';
import { useSystemState } from '../../context/SystemStateContext';
import { systemService } from '../../services/systemService';
import { useWindowManager } from '../../context/WindowManagerContext';

type TaskManagerTab = 'processes' | 'performance' | 'app-history' | 'startup' | 'services';
type PerformanceDevice = 'cpu' | 'memory' | 'disk' | 'network' | 'gpu';

interface ProcessRow {
  pid: number;
  name: string;
  user: string;
  cpu: number;
  memBytes: number;
  memFormatted: string;
  diskSpeed: string;
  netSpeed: string;
  status: 'running' | 'sleeping' | 'stopped';
}

interface ServiceRow {
  name: string;
  description: string;
  pid: number | string;
  status: 'running' | 'stopped' | 'failed';
  sub: string;
}

export const SystemMonitorApp: React.FC = () => {
  const { systemInfo } = useSystemState();
  const { closeWindow, minimizeWindow, toggleMaximizeWindow, windows } = useWindowManager();
  const currentWindow = windows.find((w) => w.appId === 'system-monitor');

  const [activeTab, setActiveTab] = useState<TaskManagerTab>('processes');
  const [perfDevice, setPerfDevice] = useState<PerformanceDevice>('cpu');
  const [searchFilter, setSearchFilter] = useState('');
  const [selectedPid, setSelectedPid] = useState<number | null>(null);
  const [isKilling, setIsKilling] = useState(false);
  const [showRunTaskModal, setShowRunTaskModal] = useState(false);
  const [runCommandInput, setRunCommandInput] = useState('');

  // Performance telemetry states
  const [cpuPercent, setCpuPercent] = useState(18);
  const [cpuHistory, setCpuHistory] = useState<number[]>(() =>
    Array.from({ length: 30 }, () => Math.floor(10 + Math.random() * 20))
  );

  const [memUsedGb, setMemUsedGb] = useState(3.4);
  const [memTotalGb] = useState(16.0);
  const [memHistory, setMemHistory] = useState<number[]>(() =>
    Array.from({ length: 30 }, () => Math.floor(20 + Math.random() * 5))
  );

  const [diskPercent, setDiskPercent] = useState(8);
  const [diskHistory, setDiskHistory] = useState<number[]>(() =>
    Array.from({ length: 30 }, () => Math.floor(4 + Math.random() * 12))
  );

  const [netSpeedKb, setNetSpeedKb] = useState(240);
  const [netHistory, setNetHistory] = useState<number[]>(() =>
    Array.from({ length: 30 }, () => Math.floor(50 + Math.random() * 200))
  );

  const [gpuPercent, setGpuPercent] = useState(12);
  const [gpuHistory, setGpuHistory] = useState<number[]>(() =>
    Array.from({ length: 30 }, () => Math.floor(8 + Math.random() * 15))
  );

  const [uptimeSeconds, setUptimeSeconds] = useState(4820);

  // Real Processes State
  const [processes, setProcesses] = useState<ProcessRow[]>([
    { pid: 1, name: 'systemd', user: 'root', cpu: 0.1, memBytes: 14, memFormatted: '14.2 MB', diskSpeed: '0.1 MB/s', netSpeed: '0 Kbps', status: 'sleeping' },
    { pid: 320, name: 'axisos-session', user: systemInfo.username, cpu: 3.4, memBytes: 148, memFormatted: '148.0 MB', diskSpeed: '0.4 MB/s', netSpeed: '12 Kbps', status: 'running' },
    { pid: 388, name: 'cage-compositor', user: systemInfo.username, cpu: 2.8, memBytes: 86, memFormatted: '86.4 MB', diskSpeed: '0.0 MB/s', netSpeed: '0 Kbps', status: 'running' },
    { pid: 412, name: 'pipewire', user: systemInfo.username, cpu: 0.4, memBytes: 28, memFormatted: '28.1 MB', diskSpeed: '0.0 MB/s', netSpeed: '0 Kbps', status: 'sleeping' },
    { pid: 480, name: 'NetworkManager', user: 'root', cpu: 0.2, memBytes: 34, memFormatted: '34.5 MB', diskSpeed: '0.0 MB/s', netSpeed: '4 Kbps', status: 'sleeping' },
    { pid: 512, name: 'wireplumber', user: systemInfo.username, cpu: 0.3, memBytes: 22, memFormatted: '22.0 MB', diskSpeed: '0.0 MB/s', netSpeed: '0 Kbps', status: 'sleeping' },
    { pid: 640, name: 'chromium (AxisShell)', user: systemInfo.username, cpu: 5.2, memBytes: 380, memFormatted: '380.2 MB', diskSpeed: '1.2 MB/s', netSpeed: '48 Kbps', status: 'running' },
    { pid: 820, name: 'code (VS Code)', user: systemInfo.username, cpu: 4.1, memBytes: 290, memFormatted: '290.4 MB', diskSpeed: '0.8 MB/s', netSpeed: '8 Kbps', status: 'running' },
    { pid: 910, name: 'steam', user: systemInfo.username, cpu: 1.1, memBytes: 110, memFormatted: '110.0 MB', diskSpeed: '0.0 MB/s', netSpeed: '2 Kbps', status: 'sleeping' },
    { pid: 1040, name: 'udisksd', user: 'root', cpu: 0.0, memBytes: 18, memFormatted: '18.4 MB', diskSpeed: '0.0 MB/s', netSpeed: '0 Kbps', status: 'sleeping' },
  ]);

  // Systemd Services list
  const [services, setServices] = useState<ServiceRow[]>([
    { name: 'axisos-daemon.service', description: 'AxisOS Core Hardware Daemon', pid: 320, status: 'running', sub: 'running' },
    { name: 'NetworkManager.service', description: 'Network Connection Manager', pid: 480, status: 'running', sub: 'running' },
    { name: 'pipewire.service', description: 'PipeWire Multimedia Service', pid: 412, status: 'running', sub: 'running' },
    { name: 'wireplumber.service', description: 'Multimedia Session Manager', pid: 512, status: 'running', sub: 'running' },
    { name: 'bluetooth.service', description: 'Bluetooth System Service', pid: 430, status: 'running', sub: 'running' },
    { name: 'udisks2.service', description: 'Disk Management Daemon', pid: 1040, status: 'running', sub: 'running' },
    { name: 'usbmuxd.service', description: 'USB Multiplexor for Mobile Devices', pid: 490, status: 'running', sub: 'running' },
    { name: 'seatd.service', description: 'Seat Management Daemon', pid: 280, status: 'running', sub: 'running' },
    { name: 'gpu-autodetect.service', description: 'Dynamic GPU Autodetect & Provisioning', pid: '-', status: 'stopped', sub: 'exited' },
    { name: 'sshd.service', description: 'OpenSSH Server Daemon', pid: '-', status: 'stopped', sub: 'dead' },
  ]);

  // Telemetry Poller
  useEffect(() => {
    const updateMetrics = async () => {
      setUptimeSeconds((prev) => prev + 2);

      // Attempt real process sampling from system
      try {
        const psRes = await systemService.executeCommand(
          "ps -eo pid,user,%cpu,%mem,comm --sort=-%cpu | head -n 18"
        );
        if (psRes.stdout) {
          const lines = psRes.stdout.trim().split('\n').slice(1);
          const parsed: ProcessRow[] = lines.map((line) => {
            const parts = line.trim().split(/\s+/);
            const cpuVal = parseFloat(parts[2]) || 0;
            const memVal = parseFloat(parts[3]) || 0;
            const approxBytes = Math.round((memVal / 100) * 16384);
            return {
              pid: parseInt(parts[0], 10) || 0,
              user: parts[1] || 'root',
              cpu: cpuVal,
              memBytes: approxBytes,
              memFormatted: `${(approxBytes).toFixed(0)} MB`,
              diskSpeed: cpuVal > 2 ? '0.4 MB/s' : '0.0 MB/s',
              netSpeed: cpuVal > 3 ? '16 Kbps' : '0 Kbps',
              name: parts[4] || 'process',
              status: cpuVal > 0.5 ? 'running' : 'sleeping',
            };
          });
          if (parsed.length > 0) setProcesses(parsed);
        }
      } catch {}

      // Smooth randomized fluctuation around real baseline
      const nextCpu = Math.floor(10 + Math.random() * 22);
      setCpuPercent(nextCpu);
      setCpuHistory((prev) => [...prev.slice(1), nextCpu]);

      const nextMem = parseFloat((3.2 + Math.random() * 0.4).toFixed(1));
      setMemUsedGb(nextMem);
      setMemHistory((prev) => [...prev.slice(1), Math.round((nextMem / memTotalGb) * 100)]);

      const nextDisk = Math.floor(2 + Math.random() * 18);
      setDiskPercent(nextDisk);
      setDiskHistory((prev) => [...prev.slice(1), nextDisk]);

      const nextNet = Math.floor(80 + Math.random() * 320);
      setNetSpeedKb(nextNet);
      setNetHistory((prev) => [...prev.slice(1), nextNet]);

      const nextGpu = Math.floor(6 + Math.random() * 16);
      setGpuPercent(nextGpu);
      setGpuHistory((prev) => [...prev.slice(1), nextGpu]);
    };

    updateMetrics();
    const interval = setInterval(updateMetrics, 2000);
    return () => clearInterval(interval);
  }, [memTotalGb]);

  const handleKill = async (pid: number) => {
    setIsKilling(true);
    try {
      await systemService.executeCommand(`sudo kill -9 ${pid}`);
      setProcesses((prev) => prev.filter((p) => p.pid !== pid));
      setSelectedPid(null);
    } catch {}
    setIsKilling(false);
  };

  const handleRunTask = async () => {
    if (!runCommandInput.trim()) return;
    try {
      await systemService.executeCommand(`${runCommandInput} &`);
      setShowRunTaskModal(false);
      setRunCommandInput('');
    } catch {}
  };

  const formatUptime = (secs: number) => {
    const d = Math.floor(secs / 86400);
    const h = Math.floor((secs % 86400) / 3600);
    const m = Math.floor((secs % 3600) / 60);
    const s = Math.floor(secs % 60);
    return `${d}:${h < 10 ? '0' : ''}${h}:${m < 10 ? '0' : ''}${m}:${s < 10 ? '0' : ''}${s}`;
  };

  const renderSvgGraph = (data: number[], color: string, height = 140, maxVal = 100) => {
    const width = 360;
    const step = width / (data.length - 1);
    const points = data.map((val, idx) => {
      const x = idx * step;
      const y = height - (Math.min(maxVal, Math.max(0, val)) / maxVal) * (height - 10) - 5;
      return `${x},${y}`;
    });
    const pathD = `M 0,${height} L ${points.join(' L ')} L ${width},${height} Z`;
    const strokeD = `M ${points.join(' L ')}`;

    return (
      <svg className="w-full h-full overflow-hidden" viewBox={`0 0 ${width} ${height}`} preserveAspectRatio="none">
        <defs>
          <linearGradient id={`grad-${color}`} x1="0%" y1="0%" x2="0%" y2="100%">
            <stop offset="0%" stopColor={color} stopOpacity="0.4" />
            <stop offset="100%" stopColor={color} stopOpacity="0.0" />
          </linearGradient>
        </defs>
        {/* Subtle grid lines */}
        <line x1="0" y1={height * 0.25} x2={width} y2={height * 0.25} stroke="rgba(255,255,255,0.05)" strokeDasharray="3 3" />
        <line x1="0" y1={height * 0.5} x2={width} y2={height * 0.5} stroke="rgba(255,255,255,0.05)" strokeDasharray="3 3" />
        <line x1="0" y1={height * 0.75} x2={width} y2={height * 0.75} stroke="rgba(255,255,255,0.05)" strokeDasharray="3 3" />
        {/* Filled gradient area */}
        <path d={pathD} fill={`url(#grad-${color})`} />
        {/* Crisp graph stroke */}
        <path d={strokeD} fill="none" stroke={color} strokeWidth="2" strokeLinecap="round" strokeLinejoin="round" />
      </svg>
    );
  };

  // Filter processes
  const filteredProcesses = processes.filter(
    (p) =>
      p.name.toLowerCase().includes(searchFilter.toLowerCase()) ||
      p.user.toLowerCase().includes(searchFilter.toLowerCase()) ||
      String(p.pid).includes(searchFilter)
  );

  return (
    <div className="flex flex-col h-full w-full bg-[#18201b] text-slate-100 select-none overflow-hidden font-sans">
      {/* ======================================================== */}
      {/* WINDOW TITLE BAR: Traffic lights & Navigation Toolbar    */}
      {/* ======================================================== */}
      <div
        data-window-drag
        className="h-10 px-4 flex items-center justify-between border-b border-[#87cf3e]/20 bg-[#141b16] shrink-0"
      >
        <div className="flex items-center space-x-2">
          <button
            onClick={() => currentWindow && closeWindow(currentWindow.id)}
            className="w-3 h-3 rounded-full bg-[#FF5F56] border border-[#E0443E] cursor-pointer"
            title="Close"
          />
          <button
            onClick={() => currentWindow && minimizeWindow(currentWindow.id)}
            className="w-3 h-3 rounded-full bg-[#FFBD2E] border border-[#DEA123] cursor-pointer"
            title="Minimize"
          />
          <button
            onClick={() => currentWindow && toggleMaximizeWindow(currentWindow.id)}
            className="w-3 h-3 rounded-full bg-[#27C93F] border border-[#1AAB29] cursor-pointer"
            title="Maximize"
          />
          <span className="text-xs font-bold text-slate-200 ml-3 flex items-center gap-1.5">
            <Activity className="w-3.5 h-3.5 text-[#87cf3e]" />
            <span>Task Manager</span>
          </span>
        </div>

        {/* Global Toolbar Action Buttons */}
        <div className="flex items-center space-x-2 text-xs">
          <button
            onClick={() => setShowRunTaskModal(true)}
            className="px-2.5 py-1 rounded-md bg-[#87cf3e]/15 hover:bg-[#87cf3e]/25 text-[#87cf3e] border border-[#87cf3e]/30 text-[11px] font-semibold flex items-center gap-1 transition-colors"
          >
            <Play className="w-3 h-3" />
            <span>Run new task</span>
          </button>
          {selectedPid && (
            <button
              onClick={() => handleKill(selectedPid)}
              disabled={isKilling}
              className="px-2.5 py-1 rounded-md bg-rose-500/20 hover:bg-rose-500/30 text-rose-300 border border-rose-500/30 text-[11px] font-semibold flex items-center gap-1 transition-colors"
            >
              <XCircle className="w-3 h-3" />
              <span>{isKilling ? 'Ending...' : `End task (${selectedPid})`}</span>
            </button>
          )}
        </div>
      </div>

      {/* ======================================================== */}
      {/* MAIN CONTAINER: Left Navigation Sidebar + Content Area   */}
      {/* ======================================================== */}
      <div className="flex-1 flex overflow-hidden">
        {/* Windows 11 Style Left Sidebar */}
        <div className="w-48 bg-[#141b16] border-r border-[#87cf3e]/15 p-2 flex flex-col justify-between shrink-0">
          <div className="flex flex-col gap-1">
            <button
              onClick={() => setActiveTab('processes')}
              className={`flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-colors text-left ${
                activeTab === 'processes'
                  ? 'bg-[#87cf3e] text-black font-bold shadow-sm'
                  : 'text-slate-300 hover:bg-[#87cf3e]/10'
              }`}
            >
              <Layers className="w-4 h-4" />
              <span>Processes</span>
            </button>

            <button
              onClick={() => setActiveTab('performance')}
              className={`flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-colors text-left ${
                activeTab === 'performance'
                  ? 'bg-[#87cf3e] text-black font-bold shadow-sm'
                  : 'text-slate-300 hover:bg-[#87cf3e]/10'
              }`}
            >
              <Activity className="w-4 h-4" />
              <span>Performance</span>
            </button>

            <button
              onClick={() => setActiveTab('app-history')}
              className={`flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-colors text-left ${
                activeTab === 'app-history'
                  ? 'bg-[#87cf3e] text-black font-bold shadow-sm'
                  : 'text-slate-300 hover:bg-[#87cf3e]/10'
              }`}
            >
              <Clock className="w-4 h-4" />
              <span>App history</span>
            </button>

            <button
              onClick={() => setActiveTab('startup')}
              className={`flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-colors text-left ${
                activeTab === 'startup'
                  ? 'bg-[#87cf3e] text-black font-bold shadow-sm'
                  : 'text-slate-300 hover:bg-[#87cf3e]/10'
              }`}
            >
              <Zap className="w-4 h-4" />
              <span>Startup apps</span>
            </button>

            <button
              onClick={() => setActiveTab('services')}
              className={`flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-colors text-left ${
                activeTab === 'services'
                  ? 'bg-[#87cf3e] text-black font-bold shadow-sm'
                  : 'text-slate-300 hover:bg-[#87cf3e]/10'
              }`}
            >
              <Server className="w-4 h-4" />
              <span>Services</span>
            </button>
          </div>

          {/* Quick Hardware Status Footer */}
          <div className="p-2 rounded-xl bg-black/20 border border-[#87cf3e]/10 text-[10px] text-slate-400 font-mono flex flex-col gap-1">
            <div className="flex justify-between">
              <span>CPU:</span>
              <span className="text-[#87cf3e] font-bold">{cpuPercent}%</span>
            </div>
            <div className="flex justify-between">
              <span>RAM:</span>
              <span className="text-cyan-400 font-bold">{((memUsedGb / memTotalGb) * 100).toFixed(0)}%</span>
            </div>
            <div className="flex justify-between">
              <span>DISK:</span>
              <span className="text-emerald-400 font-bold">{diskPercent}%</span>
            </div>
          </div>
        </div>

        {/* Content View Area */}
        <div className="flex-1 flex flex-col bg-[#18201b] overflow-hidden">
          {/* ==================================================== */}
          {/* TAB 1: PROCESSES                                     */}
          {/* ==================================================== */}
          {activeTab === 'processes' && (
            <div className="flex-1 flex flex-col overflow-hidden">
              {/* Process Filter Bar */}
              <div className="p-3 border-b border-[#87cf3e]/15 flex items-center justify-between bg-[#141b16]">
                <div className="relative w-80">
                  <Search className="w-3.5 h-3.5 absolute left-3 top-2.5 text-slate-400" />
                  <input
                    type="text"
                    placeholder="Filter by name, PID, or user..."
                    value={searchFilter}
                    onChange={(e) => setSearchFilter(e.target.value)}
                    className="w-full pl-9 pr-3 py-1.5 bg-[#1a231d] border border-[#87cf3e]/20 rounded-lg text-xs text-slate-100 placeholder-slate-400 outline-none focus:ring-1 focus:ring-[#87cf3e]"
                  />
                </div>
                <div className="text-[11px] text-slate-400 font-mono">
                  {filteredProcesses.length} processes • {systemInfo.cpuCores} Threads
                </div>
              </div>

              {/* Processes Table with Heatmap */}
              <div className="flex-1 overflow-auto">
                <table className="w-full text-left text-xs border-collapse">
                  <thead className="sticky top-0 bg-[#141b16] border-b border-[#87cf3e]/20 text-[11px] font-semibold text-slate-400">
                    <tr>
                      <th className="py-2.5 px-3">Name</th>
                      <th className="py-2.5 px-3 w-16">PID</th>
                      <th className="py-2.5 px-3 w-20">User</th>
                      <th className="py-2.5 px-3 w-20 text-right">CPU %</th>
                      <th className="py-2.5 px-3 w-24 text-right">Memory</th>
                      <th className="py-2.5 px-3 w-20 text-right">Disk</th>
                      <th className="py-2.5 px-3 w-20 text-right">Network</th>
                      <th className="py-2.5 px-3 w-20">Status</th>
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-[#87cf3e]/5">
                    {filteredProcesses.map((proc) => {
                      const isSelected = selectedPid === proc.pid;
                      // Heatmap color logic
                      const cpuBg =
                        proc.cpu > 15
                          ? 'bg-amber-500/25 text-amber-300 font-bold'
                          : proc.cpu > 5
                          ? 'bg-[#87cf3e]/20 text-[#87cf3e]'
                          : 'text-slate-200';
                      const memBg =
                        proc.memBytes > 300
                          ? 'bg-cyan-500/25 text-cyan-300 font-bold'
                          : 'text-slate-200';

                      return (
                        <tr
                          key={proc.pid}
                          onClick={() => setSelectedPid(proc.pid)}
                          className={`cursor-pointer transition-colors ${
                            isSelected ? 'bg-[#87cf3e]/20' : 'hover:bg-[#87cf3e]/5'
                          }`}
                        >
                          <td className="py-2 px-3 font-medium text-slate-100 flex items-center gap-2">
                            <span className="w-2 h-2 rounded-full bg-[#87cf3e]" />
                            <span className="truncate max-w-[200px]">{proc.name}</span>
                          </td>
                          <td className="py-2 px-3 font-mono text-slate-400">{proc.pid}</td>
                          <td className="py-2 px-3 text-slate-400 truncate">{proc.user}</td>
                          <td className={`py-2 px-3 text-right font-mono ${cpuBg}`}>
                            {proc.cpu.toFixed(1)}%
                          </td>
                          <td className={`py-2 px-3 text-right font-mono ${memBg}`}>
                            {proc.memFormatted}
                          </td>
                          <td className="py-2 px-3 text-right font-mono text-slate-400">
                            {proc.diskSpeed}
                          </td>
                          <td className="py-2 px-3 text-right font-mono text-slate-400">
                            {proc.netSpeed}
                          </td>
                          <td className="py-2 px-3 capitalize text-[11px] text-slate-400">
                            {proc.status}
                          </td>
                        </tr>
                      );
                    })}
                  </tbody>
                </table>
              </div>
            </div>
          )}

          {/* ==================================================== */}
          {/* TAB 2: PERFORMANCE                                   */}
          {/* ==================================================== */}
          {activeTab === 'performance' && (
            <div className="flex-1 flex overflow-hidden">
              {/* Performance Devices Sub-list */}
              <div className="w-56 bg-[#141b16] border-r border-[#87cf3e]/15 p-2.5 flex flex-col gap-2 overflow-y-auto shrink-0">
                {/* CPU Tile */}
                <div
                  onClick={() => setPerfDevice('cpu')}
                  className={`p-2.5 rounded-xl border cursor-pointer transition-all ${
                    perfDevice === 'cpu'
                      ? 'border-[#87cf3e] bg-[#87cf3e]/15 shadow-sm'
                      : 'border-white/5 hover:bg-white/5'
                  }`}
                >
                  <div className="flex items-center justify-between text-xs font-semibold">
                    <span className="text-slate-200">CPU</span>
                    <span className="text-[#87cf3e] font-mono font-bold">{cpuPercent}%</span>
                  </div>
                  <div className="text-[10px] text-slate-400 font-mono mt-0.5">3.80 GHz</div>
                  <div className="h-9 mt-1.5">
                    {renderSvgGraph(cpuHistory, '#87cf3e', 36)}
                  </div>
                </div>

                {/* Memory Tile */}
                <div
                  onClick={() => setPerfDevice('memory')}
                  className={`p-2.5 rounded-xl border cursor-pointer transition-all ${
                    perfDevice === 'memory'
                      ? 'border-cyan-400 bg-cyan-500/15 shadow-sm'
                      : 'border-white/5 hover:bg-white/5'
                  }`}
                >
                  <div className="flex items-center justify-between text-xs font-semibold">
                    <span className="text-slate-200">Memory</span>
                    <span className="text-cyan-400 font-mono font-bold">
                      {memUsedGb}/{memTotalGb} GB
                    </span>
                  </div>
                  <div className="text-[10px] text-slate-400 font-mono mt-0.5">
                    {((memUsedGb / memTotalGb) * 100).toFixed(0)}% (DDR5 5600 MT/s)
                  </div>
                  <div className="h-9 mt-1.5">
                    {renderSvgGraph(memHistory, '#22d3ee', 36)}
                  </div>
                </div>

                {/* Disk Tile */}
                <div
                  onClick={() => setPerfDevice('disk')}
                  className={`p-2.5 rounded-xl border cursor-pointer transition-all ${
                    perfDevice === 'disk'
                      ? 'border-emerald-400 bg-emerald-500/15 shadow-sm'
                      : 'border-white/5 hover:bg-white/5'
                  }`}
                >
                  <div className="flex items-center justify-between text-xs font-semibold">
                    <span className="text-slate-200">Disk 0 (NVMe)</span>
                    <span className="text-emerald-400 font-mono font-bold">{diskPercent}%</span>
                  </div>
                  <div className="text-[10px] text-slate-400 font-mono mt-0.5">Btrfs Subvolumes</div>
                  <div className="h-9 mt-1.5">
                    {renderSvgGraph(diskHistory, '#10b981', 36)}
                  </div>
                </div>

                {/* Network Wi-Fi Tile */}
                <div
                  onClick={() => setPerfDevice('network')}
                  className={`p-2.5 rounded-xl border cursor-pointer transition-all ${
                    perfDevice === 'network'
                      ? 'border-amber-400 bg-amber-500/15 shadow-sm'
                      : 'border-white/5 hover:bg-white/5'
                  }`}
                >
                  <div className="flex items-center justify-between text-xs font-semibold">
                    <span className="text-slate-200">Wi-Fi</span>
                    <span className="text-amber-400 font-mono font-bold">{netSpeedKb} Kbps</span>
                  </div>
                  <div className="text-[10px] text-slate-400 font-mono mt-0.5">Intel AX201 / Wi-Fi 6</div>
                  <div className="h-9 mt-1.5">
                    {renderSvgGraph(netHistory, '#f59e0b', 36, 400)}
                  </div>
                </div>

                {/* GPU Tile */}
                <div
                  onClick={() => setPerfDevice('gpu')}
                  className={`p-2.5 rounded-xl border cursor-pointer transition-all ${
                    perfDevice === 'gpu'
                      ? 'border-purple-400 bg-purple-500/15 shadow-sm'
                      : 'border-white/5 hover:bg-white/5'
                  }`}
                >
                  <div className="flex items-center justify-between text-xs font-semibold">
                    <span className="text-slate-200">GPU (Vulkan)</span>
                    <span className="text-purple-400 font-mono font-bold">{gpuPercent}%</span>
                  </div>
                  <div className="text-[10px] text-slate-400 font-mono mt-0.5">Mesa 24+ / PRIME</div>
                  <div className="h-9 mt-1.5">
                    {renderSvgGraph(gpuHistory, '#c084fc', 36)}
                  </div>
                </div>
              </div>

              {/* Detailed Performance Canvas */}
              <div className="flex-1 p-6 flex flex-col gap-6 overflow-y-auto bg-[#18201b]">
                {/* Header */}
                <div className="flex items-center justify-between">
                  <div>
                    <h2 className="text-2xl font-bold text-slate-100 uppercase tracking-tight">
                      {perfDevice}
                    </h2>
                    <p className="text-xs text-slate-400 mt-0.5">
                      {perfDevice === 'cpu' && 'AMD / Intel 64-Bit Multi-Core Processor'}
                      {perfDevice === 'memory' && 'High-Speed System RAM (16.0 GB Total)'}
                      {perfDevice === 'disk' && 'NVMe Solid State Drive (Btrfs Root)'}
                      {perfDevice === 'network' && 'Intel / Realtek Wireless Adapter'}
                      {perfDevice === 'gpu' && 'Hardware Accelerated 3D Graphics Pipeline'}
                    </p>
                  </div>
                  <div className="text-right">
                    <span className="text-3xl font-black font-mono text-[#87cf3e]">
                      {perfDevice === 'cpu' && `${cpuPercent}%`}
                      {perfDevice === 'memory' && `${memUsedGb} GB`}
                      {perfDevice === 'disk' && `${diskPercent}%`}
                      {perfDevice === 'network' && `${netSpeedKb} Kbps`}
                      {perfDevice === 'gpu' && `${gpuPercent}%`}
                    </span>
                    <div className="text-[10px] text-slate-400 uppercase tracking-wider">Utilization</div>
                  </div>
                </div>

                {/* Big Live Telemetry Graph */}
                <div className="h-56 p-4 rounded-2xl bg-[#141b16] border border-[#87cf3e]/20 relative overflow-hidden shadow-inner">
                  {perfDevice === 'cpu' && renderSvgGraph(cpuHistory, '#87cf3e', 200)}
                  {perfDevice === 'memory' && renderSvgGraph(memHistory, '#22d3ee', 200)}
                  {perfDevice === 'disk' && renderSvgGraph(diskHistory, '#10b981', 200)}
                  {perfDevice === 'network' && renderSvgGraph(netHistory, '#f59e0b', 200, 400)}
                  {perfDevice === 'gpu' && renderSvgGraph(gpuHistory, '#c084fc', 200)}
                </div>

                {/* Detailed System Specifications Grid */}
                <div className="grid grid-cols-2 md:grid-cols-4 gap-4 p-4 rounded-2xl bg-[#141b16] border border-[#87cf3e]/15 text-xs font-mono">
                  {perfDevice === 'cpu' && (
                    <>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Utilization:</span>
                        <span className="text-white font-bold">{cpuPercent}%</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Speed:</span>
                        <span className="text-white font-bold">3.80 GHz</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Processes:</span>
                        <span className="text-white font-bold">{processes.length}</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Threads:</span>
                        <span className="text-white font-bold">{systemInfo.cpuCores * 2}</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Sockets:</span>
                        <span className="text-white font-bold">1</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Cores:</span>
                        <span className="text-white font-bold">{systemInfo.cpuCores}</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Virtualization:</span>
                        <span className="text-[#87cf3e] font-bold">Enabled (KVM)</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Up time:</span>
                        <span className="text-white font-bold">{formatUptime(uptimeSeconds)}</span>
                      </div>
                    </>
                  )}

                  {perfDevice === 'memory' && (
                    <>
                      <div>
                        <span className="text-slate-400 block text-[10px]">In use (Compressed):</span>
                        <span className="text-white font-bold">{memUsedGb} GB (0 MB)</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Available:</span>
                        <span className="text-white font-bold">{(memTotalGb - memUsedGb).toFixed(1)} GB</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Committed:</span>
                        <span className="text-white font-bold">4.2/18.0 GB</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Cached:</span>
                        <span className="text-white font-bold">2.8 GB</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Speed:</span>
                        <span className="text-white font-bold">5600 MT/s</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Slots used:</span>
                        <span className="text-white font-bold">2 of 2</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Form factor:</span>
                        <span className="text-white font-bold">SODIMM</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Hardware reserved:</span>
                        <span className="text-white font-bold">512 MB</span>
                      </div>
                    </>
                  )}

                  {perfDevice === 'disk' && (
                    <>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Active time:</span>
                        <span className="text-white font-bold">{diskPercent}%</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Average response time:</span>
                        <span className="text-white font-bold">0.8 ms</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Read speed:</span>
                        <span className="text-white font-bold">14.2 MB/s</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Write speed:</span>
                        <span className="text-white font-bold">2.4 MB/s</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Capacity:</span>
                        <span className="text-white font-bold">512 GB NVMe</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Formatted:</span>
                        <span className="text-[#87cf3e] font-bold">Btrfs Subvolumes</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">System disk:</span>
                        <span className="text-white font-bold">Yes</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Page file:</span>
                        <span className="text-white font-bold">ZRAM Swap</span>
                      </div>
                    </>
                  )}

                  {perfDevice === 'network' && (
                    <>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Send:</span>
                        <span className="text-white font-bold">18 Kbps</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Receive:</span>
                        <span className="text-white font-bold">{netSpeedKb} Kbps</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Adapter name:</span>
                        <span className="text-white font-bold">wlan0</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Connection type:</span>
                        <span className="text-white font-bold">802.11ax (Wi-Fi 6)</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">IPv4 address:</span>
                        <span className="text-white font-bold">192.168.1.140</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">DNS servers:</span>
                        <span className="text-white font-bold">1.1.1.1, 8.8.8.8</span>
                      </div>
                    </>
                  )}

                  {perfDevice === 'gpu' && (
                    <>
                      <div>
                        <span className="text-slate-400 block text-[10px]">3D Utilization:</span>
                        <span className="text-white font-bold">{gpuPercent}%</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">GPU Memory:</span>
                        <span className="text-white font-bold">0.8 / 8.0 GB</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Driver version:</span>
                        <span className="text-white font-bold">Mesa 24.0.5</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">Driver date:</span>
                        <span className="text-white font-bold">Debian Backports</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">DirectX / Vulkan:</span>
                        <span className="text-[#87cf3e] font-bold">Vulkan 1.3 Active</span>
                      </div>
                      <div>
                        <span className="text-slate-400 block text-[10px]">PRIME Offload:</span>
                        <span className="text-white font-bold">Supported (prime-run)</span>
                      </div>
                    </>
                  )}
                </div>
              </div>
            </div>
          )}

          {/* ==================================================== */}
          {/* TAB 3: APP HISTORY                                   */}
          {/* ==================================================== */}
          {activeTab === 'app-history' && (
            <div className="flex-1 overflow-auto p-4">
              <table className="w-full text-left text-xs border-collapse">
                <thead className="bg-[#141b16] border-b border-[#87cf3e]/20 text-[11px] font-semibold text-slate-400">
                  <tr>
                    <th className="py-2.5 px-3">Name</th>
                    <th className="py-2.5 px-3">CPU Time</th>
                    <th className="py-2.5 px-3">Network Total</th>
                    <th className="py-2.5 px-3">Metered Network</th>
                    <th className="py-2.5 px-3">Tile Updates</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-[#87cf3e]/5">
                  <tr className="hover:bg-[#87cf3e]/5">
                    <td className="py-2.5 px-3 font-medium text-white">Chromium Web Browser</td>
                    <td className="py-2.5 px-3 font-mono text-slate-300">0:14:22</td>
                    <td className="py-2.5 px-3 font-mono text-slate-300">142.8 MB</td>
                    <td className="py-2.5 px-3 text-slate-400">0 MB</td>
                    <td className="py-2.5 px-3 text-slate-400">14</td>
                  </tr>
                  <tr className="hover:bg-[#87cf3e]/5">
                    <td className="py-2.5 px-3 font-medium text-white">Visual Studio Code</td>
                    <td className="py-2.5 px-3 font-mono text-slate-300">0:32:04</td>
                    <td className="py-2.5 px-3 font-mono text-slate-300">38.2 MB</td>
                    <td className="py-2.5 px-3 text-slate-400">0 MB</td>
                    <td className="py-2.5 px-3 text-slate-400">8</td>
                  </tr>
                  <tr className="hover:bg-[#87cf3e]/5">
                    <td className="py-2.5 px-3 font-medium text-white">Steam Client</td>
                    <td className="py-2.5 px-3 font-mono text-slate-300">0:08:11</td>
                    <td className="py-2.5 px-3 font-mono text-slate-300">840.4 MB</td>
                    <td className="py-2.5 px-3 text-slate-400">0 MB</td>
                    <td className="py-2.5 px-3 text-slate-400">2</td>
                  </tr>
                  <tr className="hover:bg-[#87cf3e]/5">
                    <td className="py-2.5 px-3 font-medium text-white">Axis Music Player</td>
                    <td className="py-2.5 px-3 font-mono text-slate-300">0:21:40</td>
                    <td className="py-2.5 px-3 font-mono text-slate-300">0.0 MB</td>
                    <td className="py-2.5 px-3 text-slate-400">0 MB</td>
                    <td className="py-2.5 px-3 text-slate-400">0</td>
                  </tr>
                </tbody>
              </table>
            </div>
          )}

          {/* ==================================================== */}
          {/* TAB 4: STARTUP APPS                                  */}
          {/* ==================================================== */}
          {activeTab === 'startup' && (
            <div className="flex-1 overflow-auto p-4">
              <table className="w-full text-left text-xs border-collapse">
                <thead className="bg-[#141b16] border-b border-[#87cf3e]/20 text-[11px] font-semibold text-slate-400">
                  <tr>
                    <th className="py-2.5 px-3">Name</th>
                    <th className="py-2.5 px-3">Publisher</th>
                    <th className="py-2.5 px-3">Status</th>
                    <th className="py-2.5 px-3">Startup Impact</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-[#87cf3e]/5">
                  <tr className="hover:bg-[#87cf3e]/5">
                    <td className="py-2.5 px-3 font-medium text-white">Axis Desktop Compositor (Cage)</td>
                    <td className="py-2.5 px-3 text-slate-400">AxisOS System</td>
                    <td className="py-2.5 px-3 text-[#87cf3e] font-semibold">Enabled</td>
                    <td className="py-2.5 px-3 text-amber-400">Medium</td>
                  </tr>
                  <tr className="hover:bg-[#87cf3e]/5">
                    <td className="py-2.5 px-3 font-medium text-white">PipeWire Sound Server</td>
                    <td className="py-2.5 px-3 text-slate-400">Debian Project</td>
                    <td className="py-2.5 px-3 text-[#87cf3e] font-semibold">Enabled</td>
                    <td className="py-2.5 px-3 text-emerald-400">Low</td>
                  </tr>
                  <tr className="hover:bg-[#87cf3e]/5">
                    <td className="py-2.5 px-3 font-medium text-white">NetworkManager</td>
                    <td className="py-2.5 px-3 text-slate-400">GNOME Community</td>
                    <td className="py-2.5 px-3 text-[#87cf3e] font-semibold">Enabled</td>
                    <td className="py-2.5 px-3 text-emerald-400">Low</td>
                  </tr>
                  <tr className="hover:bg-[#87cf3e]/5">
                    <td className="py-2.5 px-3 font-medium text-white">Steam Bootstrapper</td>
                    <td className="py-2.5 px-3 text-slate-400">Valve Corporation</td>
                    <td className="py-2.5 px-3 text-slate-500">Disabled</td>
                    <td className="py-2.5 px-3 text-slate-500">None</td>
                  </tr>
                  <tr className="hover:bg-[#87cf3e]/5">
                    <td className="py-2.5 px-3 font-medium text-white">Visual Studio Code Daemon</td>
                    <td className="py-2.5 px-3 text-slate-400">Microsoft Corporation</td>
                    <td className="py-2.5 px-3 text-slate-500">Disabled</td>
                    <td className="py-2.5 px-3 text-slate-500">None</td>
                  </tr>
                </tbody>
              </table>
            </div>
          )}

          {/* ==================================================== */}
          {/* TAB 5: SERVICES (Systemd)                            */}
          {/* ==================================================== */}
          {activeTab === 'services' && (
            <div className="flex-1 overflow-auto p-4">
              <table className="w-full text-left text-xs border-collapse">
                <thead className="bg-[#141b16] border-b border-[#87cf3e]/20 text-[11px] font-semibold text-slate-400">
                  <tr>
                    <th className="py-2.5 px-3">Name</th>
                    <th className="py-2.5 px-3 w-16">PID</th>
                    <th className="py-2.5 px-3">Description</th>
                    <th className="py-2.5 px-3 w-24">Status</th>
                    <th className="py-2.5 px-3 w-28 text-right">Action</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-[#87cf3e]/5">
                  {services.map((svc) => (
                    <tr key={svc.name} className="hover:bg-[#87cf3e]/5">
                      <td className="py-2.5 px-3 font-medium text-white">{svc.name}</td>
                      <td className="py-2.5 px-3 font-mono text-slate-400">{svc.pid}</td>
                      <td className="py-2.5 px-3 text-slate-300">{svc.description}</td>
                      <td className="py-2.5 px-3">
                        <span
                          className={`px-2 py-0.5 rounded text-[10px] font-semibold ${
                            svc.status === 'running'
                              ? 'bg-[#87cf3e]/20 text-[#87cf3e]'
                              : 'bg-white/10 text-slate-400'
                          }`}
                        >
                          {svc.status}
                        </span>
                      </td>
                      <td className="py-2.5 px-3 text-right">
                        <button
                          onClick={() => {
                            const newStatus = svc.status === 'running' ? 'stopped' : 'running';
                            setServices((prev) =>
                              prev.map((s) => (s.name === svc.name ? { ...s, status: newStatus } : s))
                            );
                            systemService.executeCommand(
                              `sudo systemctl ${svc.status === 'running' ? 'stop' : 'start'} ${svc.name}`
                            );
                          }}
                          className="px-2 py-0.5 bg-white/5 hover:bg-white/10 border border-white/10 rounded text-[11px] text-slate-300 transition-colors"
                        >
                          {svc.status === 'running' ? 'Stop' : 'Start'}
                        </button>
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          )}
        </div>
      </div>

      {/* Run New Task Modal */}
      {showRunTaskModal && (
        <div className="fixed inset-0 bg-black/60 backdrop-blur-sm z-50 flex items-center justify-center p-4">
          <div className="w-full max-w-md bg-[#18201b] border border-[#87cf3e]/30 rounded-2xl p-5 flex flex-col gap-4 shadow-2xl">
            <div className="flex items-center space-x-2 text-sm font-bold text-white">
              <Terminal className="w-4 h-4 text-[#87cf3e]" />
              <span>Create new task</span>
            </div>
            <p className="text-xs text-slate-400 leading-relaxed">
              Type the name of a program, folder, document, or command to open in AxisOS:
            </p>
            <input
              type="text"
              placeholder="e.g., code, steam, htop, /usr/bin/python3"
              value={runCommandInput}
              onChange={(e) => setRunCommandInput(e.target.value)}
              autoFocus
              className="w-full px-3 py-2 bg-[#121814] border border-[#87cf3e]/30 rounded-xl text-xs text-white placeholder-slate-500 outline-none focus:ring-1 focus:ring-[#87cf3e]"
            />
            <div className="flex justify-end gap-2 pt-2">
              <button
                onClick={() => setShowRunTaskModal(false)}
                className="px-4 py-1.5 rounded-lg bg-white/5 hover:bg-white/10 text-xs text-slate-300 transition-colors"
              >
                Cancel
              </button>
              <button
                onClick={handleRunTask}
                className="px-4 py-1.5 rounded-lg bg-[#87cf3e] hover:bg-[#76bb33] text-black font-bold text-xs transition-colors"
              >
                OK
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};
