import React, { useState, useEffect, useRef } from 'react';
import {
  Globe,
  AlarmClock,
  Timer as TimerIcon,
  Play,
  Pause,
  RotateCcw,
  Plus,
  Trash2,
  Bell,
  Clock as ClockIcon,
} from 'lucide-react';

type Tab = 'world' | 'alarm' | 'stopwatch' | 'timer';

interface CityClock {
  id: string;
  name: string;
  country: string;
  timeZone: string;
}

interface AlarmItem {
  id: string;
  time: string; // "07:30"
  label: string;
  enabled: boolean;
}

const DEFAULT_CITIES: CityClock[] = [
  { id: 'cupertino', name: 'Cupertino', country: 'United States', timeZone: 'America/Los_Angeles' },
  { id: 'london', name: 'London', country: 'United Kingdom', timeZone: 'Europe/London' },
  { id: 'paris', name: 'Paris', country: 'France', timeZone: 'Europe/Paris' },
  { id: 'tokyo', name: 'Tokyo', country: 'Japan', timeZone: 'Asia/Tokyo' },
  { id: 'sydney', name: 'Sydney', country: 'Australia', timeZone: 'Australia/Sydney' },
  { id: 'johannesburg', name: 'Johannesburg', country: 'South Africa', timeZone: 'Africa/Johannesburg' },
];

export const ClockApp: React.FC = () => {
  const [activeTab, setActiveTab] = useState<Tab>('world');
  const [now, setNow] = useState(new Date());

  // Web Audio Synthesizer for Chimes/Alarms
  const playChime = () => {
    try {
      const AudioCtx = window.AudioContext || (window as any).webkitAudioContext;
      if (!AudioCtx) return;
      const ctx = new AudioCtx();
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();
      osc.type = 'sine';
      osc.frequency.setValueAtTime(880, ctx.currentTime); // A5
      osc.frequency.exponentialRampToValueAtTime(440, ctx.currentTime + 0.8);
      gain.gain.setValueAtTime(0.3, ctx.currentTime);
      gain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + 0.8);
      osc.connect(gain);
      gain.connect(ctx.destination);
      osc.start();
      osc.stop(ctx.currentTime + 0.8);
    } catch {}
  };

  // Clock Ticker
  useEffect(() => {
    const timer = setInterval(() => setNow(new Date()), 1000);
    return () => clearInterval(timer);
  }, []);

  // --- Stopwatch State ---
  const [swTime, setSwTime] = useState(0); // in ms
  const [swRunning, setSwRunning] = useState(false);
  const [laps, setLaps] = useState<number[]>([]);
  const swRef = useRef<number | null>(null);

  useEffect(() => {
    if (swRunning) {
      const startTime = Date.now() - swTime;
      swRef.current = window.setInterval(() => {
        setSwTime(Date.now() - startTime);
      }, 10);
    } else if (swRef.current) {
      clearInterval(swRef.current);
    }
    return () => {
      if (swRef.current) clearInterval(swRef.current);
    };
  }, [swRunning]);

  const handleLap = () => {
    setLaps([swTime, ...laps]);
  };

  const handleResetSw = () => {
    setSwRunning(false);
    setSwTime(0);
    setLaps([]);
  };

  // --- Timer State ---
  const [timerDuration, setTimerDuration] = useState(300); // 5 mins in seconds
  const [timerRemaining, setTimerRemaining] = useState(300);
  const [timerRunning, setTimerRunning] = useState(false);
  const timerRef = useRef<number | null>(null);

  useEffect(() => {
    if (timerRunning && timerRemaining > 0) {
      timerRef.current = window.setInterval(() => {
        setTimerRemaining((prev) => {
          if (prev <= 1) {
            setTimerRunning(false);
            playChime();
            return 0;
          }
          return prev - 1;
        });
      }, 1000);
    } else if (timerRef.current) {
      clearInterval(timerRef.current);
    }
    return () => {
      if (timerRef.current) clearInterval(timerRef.current);
    };
  }, [timerRunning, timerRemaining]);

  // --- Alarms State ---
  const [alarms, setAlarms] = useState<AlarmItem[]>([
    { id: '1', time: '07:00', label: 'Morning Standup', enabled: true },
    { id: '2', time: '08:30', label: 'Workout', enabled: false },
  ]);
  const [newAlarmTime, setNewAlarmTime] = useState('09:00');
  const [newAlarmLabel, setNewAlarmLabel] = useState('New Alarm');
  const [showAddAlarm, setShowAddAlarm] = useState(false);

  // Helper to format Stopwatch ms
  const formatStopwatch = (ms: number) => {
    const mins = Math.floor(ms / 60000);
    const secs = Math.floor((ms % 60000) / 1000);
    const centis = Math.floor((ms % 1000) / 10);
    return `${String(mins).padStart(2, '0')}:${String(secs).padStart(2, '0')}.${String(centis).padStart(2, '0')}`;
  };

  // Helper to format Timer seconds
  const formatTimer = (seconds: number) => {
    const hrs = Math.floor(seconds / 3600);
    const mins = Math.floor((seconds % 3600) / 60);
    const secs = seconds % 60;
    if (hrs > 0) {
      return `${String(hrs).padStart(2, '0')}:${String(mins).padStart(2, '0')}:${String(secs).padStart(2, '0')}`;
    }
    return `${String(mins).padStart(2, '0')}:${String(secs).padStart(2, '0')}`;
  };

  // Analog Clock SVG Renderer
  const renderAnalogClock = (date: Date, timeZone?: string) => {
    const timeStr = timeZone
      ? date.toLocaleTimeString('en-US', { timeZone, hour12: false })
      : date.toLocaleTimeString('en-US', { hour12: false });
    const [h, m, s] = timeStr.split(':').map(Number);

    const secDeg = (s / 60) * 360;
    const minDeg = ((m + s / 60) / 60) * 360;
    const hrDeg = (((h % 12) + m / 60) / 12) * 360;

    return (
      <div className="relative w-28 h-28 rounded-full bg-slate-100 dark:bg-slate-900 border-2 border-slate-300 dark:border-slate-700/80 shadow-inner flex items-center justify-center">
        {/* Ticks */}
        {[0, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300, 330].map((deg) => (
          <div
            key={deg}
            className={`absolute w-0.5 rounded-full ${deg % 90 === 0 ? 'h-2 bg-slate-500 dark:bg-slate-400' : 'h-1 bg-slate-300 dark:bg-slate-600'}`}
            style={{
              transform: `rotate(${deg}deg) translateY(-46px)`,
            }}
          />
        ))}

        {/* Hour Hand */}
        <div
          className="absolute w-1 bg-slate-800 dark:bg-white rounded-full origin-bottom shadow-sm"
          style={{
            height: '24px',
            transform: `translateY(-12px) rotate(${hrDeg}deg)`,
          }}
        />

        {/* Minute Hand */}
        <div
          className="absolute w-0.75 bg-slate-600 dark:bg-slate-300 rounded-full origin-bottom shadow-sm"
          style={{
            height: '34px',
            transform: `translateY(-17px) rotate(${minDeg}deg)`,
          }}
        />

        {/* Second Hand (Orange/Accent) */}
        <div
          className="absolute w-0.5 bg-orange-500 rounded-full origin-bottom"
          style={{
            height: '40px',
            transform: `translateY(-18px) rotate(${secDeg}deg)`,
          }}
        />

        {/* Center Pivot */}
        <div className="w-2 h-2 rounded-full bg-orange-500 z-10 border border-white" />
      </div>
    );
  };

  return (
    <div className="flex flex-col h-full w-full bg-[#F5F5F7] dark:bg-[#1E1E1E] text-slate-900 dark:text-slate-100 select-none overflow-hidden font-sans">
      {/* Top Segmented Controls */}
      <div className="h-12 border-b border-black/5 dark:border-white/10 px-4 bg-white/80 dark:bg-slate-900/60 flex items-center justify-center">
        <div className="flex bg-black/5 dark:bg-slate-800/80 p-1 rounded-xl border border-black/5 dark:border-white/10 space-x-1">
          {[
            { id: 'world', label: 'World Clock', icon: Globe },
            { id: 'alarm', label: 'Alarm', icon: AlarmClock },
            { id: 'stopwatch', label: 'Stopwatch', icon: ClockIcon },
            { id: 'timer', label: 'Timer', icon: TimerIcon },
          ].map((tab) => {
            const Icon = tab.icon;
            const active = activeTab === tab.id;
            return (
              <button
                key={tab.id}
                onClick={() => setActiveTab(tab.id as Tab)}
                className={`flex items-center space-x-1.5 px-3 py-1 rounded-lg text-xs font-medium transition-all ${
                  active
                    ? 'bg-[#007AFF] text-white shadow-xs'
                    : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white hover:bg-black/5 dark:hover:bg-white/5'
                }`}
              >
                <Icon className="w-3.5 h-3.5" />
                <span>{tab.label}</span>
              </button>
            );
          })}
        </div>
      </div>

      {/* Main Tab Content */}
      <div className="flex-1 overflow-y-auto p-5">
        {/* WORLD CLOCK */}
        {activeTab === 'world' && (
          <div className="space-y-4">
            <div className="flex items-center justify-between">
              <h2 className="text-xs font-semibold uppercase tracking-wider text-slate-500 dark:text-slate-400">
                World Clocks
              </h2>
              <span className="text-xs text-slate-500">
                Local: {now.toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' })}
              </span>
            </div>

            <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
              {DEFAULT_CITIES.map((city) => {
                const cityTime = now.toLocaleTimeString('en-US', {
                  timeZone: city.timeZone,
                  hour: '2-digit',
                  minute: '2-digit',
                  hour12: true,
                });
                return (
                  <div
                    key={city.id}
                    className="p-4 rounded-2xl bg-white dark:bg-slate-900/50 border border-black/5 dark:border-white/10 shadow-xs hover:shadow-md flex items-center justify-between transition-all"
                  >
                    <div>
                      <h3 className="text-sm font-semibold text-slate-800 dark:text-white">{city.name}</h3>
                      <p className="text-[11px] text-slate-500 dark:text-slate-400">{city.country}</p>
                      <div className="mt-2 text-2xl font-light tracking-tight text-slate-900 dark:text-white font-mono">
                        {cityTime}
                      </div>
                    </div>
                    {renderAnalogClock(now, city.timeZone)}
                  </div>
                );
              })}
            </div>
          </div>
        )}

        {/* ALARM */}
        {activeTab === 'alarm' && (
          <div className="max-w-xl mx-auto space-y-4">
            <div className="flex items-center justify-between">
              <h2 className="text-xs font-semibold uppercase tracking-wider text-slate-500 dark:text-slate-400">
                Alarms
              </h2>
              <button
                onClick={() => setShowAddAlarm(!showAddAlarm)}
                className="flex items-center space-x-1.5 px-3 py-1 rounded-xl bg-[#007AFF] hover:bg-blue-600 text-xs font-medium text-white shadow-xs transition-all"
              >
                <Plus className="w-3.5 h-3.5" />
                <span>Add Alarm</span>
              </button>
            </div>

            {showAddAlarm && (
              <div className="p-4 rounded-2xl bg-white dark:bg-slate-900 border border-blue-500/30 space-y-3 shadow-md animate-in fade-in duration-200">
                <div className="flex items-center space-x-3">
                  <input
                    type="time"
                    value={newAlarmTime}
                    onChange={(e) => setNewAlarmTime(e.target.value)}
                    className="bg-black/5 dark:bg-slate-800 border border-black/10 dark:border-white/10 rounded-xl px-3 py-2 text-sm text-slate-900 dark:text-white font-mono"
                  />
                  <input
                    type="text"
                    placeholder="Alarm Label"
                    value={newAlarmLabel}
                    onChange={(e) => setNewAlarmLabel(e.target.value)}
                    className="flex-1 bg-black/5 dark:bg-slate-800 border border-black/10 dark:border-white/10 rounded-xl px-3 py-2 text-sm text-slate-900 dark:text-white placeholder-slate-400"
                  />
                </div>
                <div className="flex justify-end space-x-2">
                  <button
                    onClick={() => setShowAddAlarm(false)}
                    className="px-3 py-1.5 rounded-xl hover:bg-black/5 dark:hover:bg-white/10 text-xs text-slate-500 dark:text-slate-400"
                  >
                    Cancel
                  </button>
                  <button
                    onClick={() => {
                      setAlarms([
                        ...alarms,
                        {
                          id: Date.now().toString(),
                          time: newAlarmTime,
                          label: newAlarmLabel || 'Alarm',
                          enabled: true,
                        },
                      ]);
                      setShowAddAlarm(false);
                    }}
                    className="px-4 py-1.5 rounded-xl bg-[#007AFF] text-xs font-semibold text-white"
                  >
                    Save
                  </button>
                </div>
              </div>
            )}

            <div className="space-y-2.5">
              {alarms.map((alarm) => (
                <div
                  key={alarm.id}
                  className="p-4 rounded-2xl bg-white dark:bg-slate-900/50 border border-black/5 dark:border-white/10 shadow-xs hover:shadow-md flex items-center justify-between transition-all"
                >
                  <div className="flex items-center space-x-3">
                    <Bell
                      className={`w-5 h-5 ${alarm.enabled ? 'text-blue-500' : 'text-slate-400 dark:text-slate-600'}`}
                    />
                    <div>
                      <div className="text-2xl font-light font-mono text-slate-900 dark:text-white tracking-tight">
                        {alarm.time}
                      </div>
                      <div className="text-xs text-slate-500 dark:text-slate-400">{alarm.label}</div>
                    </div>
                  </div>

                  <div className="flex items-center space-x-3">
                    <button
                      onClick={() =>
                        setAlarms((prev) =>
                          prev.map((a) => (a.id === alarm.id ? { ...a, enabled: !a.enabled } : a))
                        )
                      }
                      className={`w-11 h-6 rounded-full transition-colors relative ${
                        alarm.enabled ? 'bg-emerald-500' : 'bg-slate-300 dark:bg-slate-700'
                      }`}
                    >
                      <div
                        className={`w-4 h-4 rounded-full bg-white absolute top-1 transition-transform ${
                          alarm.enabled ? 'translate-x-6' : 'translate-x-1'
                        }`}
                      />
                    </button>
                    <button
                      onClick={() => setAlarms(alarms.filter((a) => a.id !== alarm.id))}
                      className="p-1.5 text-slate-400 hover:text-red-500 hover:bg-red-500/10 rounded-lg transition-colors"
                    >
                      <Trash2 className="w-4 h-4" />
                    </button>
                  </div>
                </div>
              ))}
            </div>
          </div>
        )}

        {/* STOPWATCH */}
        {activeTab === 'stopwatch' && (
          <div className="max-w-md mx-auto flex flex-col items-center justify-center space-y-6 pt-4">
            <div className="text-6xl font-light font-mono tracking-tight text-slate-900 dark:text-white py-6">
              {formatStopwatch(swTime)}
            </div>

            <div className="flex items-center space-x-6">
              <button
                onClick={handleLap}
                disabled={!swRunning}
                className="w-16 h-16 rounded-full bg-white dark:bg-slate-800 border border-black/10 dark:border-white/10 hover:bg-black/5 dark:hover:bg-slate-700 active:scale-95 text-xs font-semibold text-slate-700 dark:text-slate-300 shadow-xs disabled:opacity-40 transition-all"
              >
                Lap
              </button>
              <button
                onClick={() => setSwRunning(!swRunning)}
                className={`w-20 h-20 rounded-full flex items-center justify-center shadow-lg active:scale-95 transition-all text-white ${
                  swRunning
                    ? 'bg-red-600 hover:bg-red-500 shadow-red-600/30'
                    : 'bg-emerald-600 hover:bg-emerald-500 shadow-emerald-600/30'
                }`}
              >
                {swRunning ? <Pause className="w-7 h-7" /> : <Play className="w-7 h-7 ml-1" />}
              </button>
              <button
                onClick={handleResetSw}
                className="w-16 h-16 rounded-full bg-white dark:bg-slate-800 border border-black/10 dark:border-white/10 hover:bg-black/5 dark:hover:bg-slate-700 active:scale-95 flex items-center justify-center text-slate-700 dark:text-slate-300 shadow-xs transition-all"
              >
                <RotateCcw className="w-5 h-5" />
              </button>
            </div>

            {/* Laps List */}
            {laps.length > 0 && (
              <div className="w-full border-t border-black/5 dark:border-white/10 pt-4 max-h-56 overflow-y-auto space-y-1.5">
                {laps.map((lapMs, idx) => (
                  <div
                    key={idx}
                    className="flex items-center justify-between px-3 py-1.5 rounded-lg bg-white dark:bg-slate-900/60 border border-black/5 dark:border-white/5 text-xs font-mono shadow-2xs"
                  >
                    <span className="text-slate-500 dark:text-slate-400">Lap {laps.length - idx}</span>
                    <span className="text-slate-800 dark:text-white font-medium">{formatStopwatch(lapMs)}</span>
                  </div>
                ))}
              </div>
            )}
          </div>
        )}

        {/* TIMER */}
        {activeTab === 'timer' && (
          <div className="max-w-md mx-auto flex flex-col items-center justify-center space-y-6 pt-4">
            {/* Circular Timer Ring */}
            <div className="relative w-56 h-56 flex items-center justify-center">
              <svg className="w-full h-full -rotate-90">
                <circle
                  cx="112"
                  cy="112"
                  r="95"
                  className="stroke-slate-200 dark:stroke-slate-800"
                  strokeWidth="8"
                  fill="none"
                />
                <circle
                  cx="112"
                  cy="112"
                  r="95"
                  className="stroke-orange-500 transition-all duration-300"
                  strokeWidth="8"
                  fill="none"
                  strokeDasharray={2 * Math.PI * 95}
                  strokeDashoffset={
                    (2 * Math.PI * 95 * (timerDuration - timerRemaining)) / (timerDuration || 1)
                  }
                  strokeLinecap="round"
                />
              </svg>
              <div className="absolute flex flex-col items-center">
                <div className="text-4xl font-light font-mono text-slate-900 dark:text-white tracking-tight">
                  {formatTimer(timerRemaining)}
                </div>
                <div className="text-[11px] text-slate-500 dark:text-slate-400 mt-1">Remaining</div>
              </div>
            </div>

            {/* Quick Presets */}
            <div className="flex items-center space-x-2">
              {[
                { label: '1m', sec: 60 },
                { label: '5m', sec: 300 },
                { label: '10m', sec: 600 },
                { label: '15m', sec: 900 },
                { label: '30m', sec: 1800 },
              ].map((preset) => (
                <button
                  key={preset.sec}
                  onClick={() => {
                    setTimerDuration(preset.sec);
                    setTimerRemaining(preset.sec);
                    setTimerRunning(false);
                  }}
                  className={`px-3 py-1 rounded-xl text-xs font-semibold border transition-all ${
                    timerDuration === preset.sec
                      ? 'bg-orange-500/15 text-orange-600 dark:text-orange-400 border-orange-500/50 shadow-xs'
                      : 'bg-white dark:bg-slate-800/80 text-slate-700 dark:text-slate-300 border-black/10 dark:border-white/10 hover:bg-black/5 dark:hover:bg-slate-800 shadow-xs'
                  }`}
                >
                  {preset.label}
                </button>
              ))}
            </div>

            {/* Controls */}
            <div className="flex items-center space-x-4">
              <button
                onClick={() => {
                  setTimerRunning(!timerRunning);
                }}
                className={`w-14 h-14 rounded-full flex items-center justify-center text-white shadow-lg active:scale-95 transition-all ${
                  timerRunning
                    ? 'bg-red-600 hover:bg-red-500 shadow-red-600/30'
                    : 'bg-orange-600 hover:bg-orange-500 shadow-orange-600/30'
                }`}
              >
                {timerRunning ? <Pause className="w-5 h-5" /> : <Play className="w-5 h-5 ml-0.5" />}
              </button>
              <button
                onClick={() => {
                  setTimerRunning(false);
                  setTimerRemaining(timerDuration);
                }}
                className="w-12 h-12 rounded-full bg-white dark:bg-slate-800 border border-black/10 dark:border-white/10 hover:bg-black/5 dark:hover:bg-slate-700 active:scale-95 flex items-center justify-center text-slate-700 dark:text-slate-300 shadow-xs transition-all"
              >
                <RotateCcw className="w-4 h-4" />
              </button>
            </div>
          </div>
        )}
      </div>
    </div>
  );
};
