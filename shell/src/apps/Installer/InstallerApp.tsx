import React, { useState } from 'react';
import {
  Sparkles,
  HardDrive,
  User,
  CheckCircle,
  ArrowRight,
  ArrowLeft,
  RotateCcw,
  Check,
  Shield,
  Layers,
  Cpu,
  AlertTriangle,
  Terminal,
  RefreshCw,
  Lock,
  ShieldCheck,
  Disc,
  Info,
  CheckCircle2,
} from 'lucide-react';
import { useInstaller, INSTALL_STEPS } from '../../context/InstallerContext';
import { useSystemState, ACCENT_COLOR_MAP } from '../../context/SystemStateContext';
import { useWindowManager } from '../../context/WindowManagerContext';
import { systemService } from '../../services/systemService';

export const InstallerApp: React.FC = () => {
  const {
    currentStep,
    goToNextStep,
    goToPrevStep,
    installerData,
    updateInstallerData,
    availableDisks,
    isInstalling,
    installProgress,
    installStatusText,
    installLogs,
    installError,
    startInstallation,
    resetInstaller,
    refreshDisks,
  } = useInstaller();

  const { accentColor, setIsLiveEnvironment } = useSystemState();
  const { closeWindow, windows } = useWindowManager();
  const [showLogConsole, setShowLogConsole] = useState(false);
  const [confirmPassword, setConfirmPassword] = useState('');
  const accent = ACCENT_COLOR_MAP[accentColor];

  const handleFinish = async (action: 'restart' | 'continue') => {
    setIsLiveEnvironment(false);
    const installerWindow = windows.find((w) => w.appId === 'installer');
    if (installerWindow) {
      closeWindow(installerWindow.id);
    }
    if (action === 'restart') {
      await systemService.reboot();
    }
  };

  const selectedDisk = availableDisks.find((d) => d.id === installerData.targetDisk);
  const passwordsMatch = !installerData.password || installerData.password === confirmPassword;

  return (
    <div className="flex h-full w-full bg-slate-950 text-slate-100 select-none">
      {/* Left Stepper Sidebar */}
      <div className="w-56 bg-slate-900/90 border-r border-white/5 p-5 flex flex-col justify-between">
        <div>
          {/* Logo */}
          <div className="flex items-center space-x-2.5 mb-8">
            <div className="w-8 h-8 rounded-xl bg-gradient-to-tr from-cyan-500 to-blue-600 flex items-center justify-center font-bold text-white shadow-md text-sm ring-2 ring-white/10">
              ▲
            </div>
            <div>
              <div className="text-sm font-bold tracking-tight">AxisOS</div>
              <div className="text-[10px] text-cyan-400 font-mono">System Installer</div>
            </div>
          </div>

          {/* Stepper Steps */}
          <div className="flex flex-col space-y-1.5">
            {INSTALL_STEPS.map((step, idx) => {
              const isPast = currentStep > idx;
              const isCurrent = currentStep === idx;

              return (
                <div
                  key={step}
                  className={`flex items-center space-x-3 px-3 py-2 rounded-xl text-xs font-medium transition-all ${
                    isCurrent
                      ? `${accent.bg} ${accent.border} text-white font-semibold border shadow-sm`
                      : isPast
                      ? 'text-emerald-400'
                      : 'text-slate-500'
                  }`}
                >
                  <div
                    className={`w-5 h-5 rounded-full flex items-center justify-center text-[10px] font-bold ${
                      isCurrent
                        ? `${accent.primary} text-white shadow-sm`
                        : isPast
                        ? 'bg-emerald-500/20 text-emerald-400'
                        : 'bg-slate-800 text-slate-500'
                    }`}
                  >
                    {isPast ? <Check className="w-3 h-3" /> : idx + 1}
                  </div>
                  <span>{step}</span>
                </div>
              );
            })}
          </div>
        </div>

        {/* Footer Note */}
        <div className="space-y-1.5 border-t border-white/5 pt-3">
          <div className="flex items-center gap-1.5 text-[10px] text-emerald-400 font-medium">
            <ShieldCheck className="w-3.5 h-3.5" />
            <span>Microsoft UEFI Signed</span>
          </div>
          <div className="text-[10px] text-slate-500 font-mono">
            EFI Bootloader • Btrfs Root
          </div>
        </div>
      </div>

      {/* Main Content Area */}
      <div className="flex-1 flex flex-col justify-between p-8 bg-slate-950/60 overflow-y-auto">
        {/* Step 0: Welcome */}
        {currentStep === 0 && (
          <div className="flex flex-col items-center justify-center text-center my-auto max-w-lg mx-auto">
            <div className="w-16 h-16 rounded-2xl bg-gradient-to-tr from-cyan-500 to-blue-600 flex items-center justify-center text-white shadow-xl shadow-cyan-950/50 mb-4 ring-4 ring-white/10">
              <Sparkles className="w-8 h-8 text-cyan-100" />
            </div>
            <h1 className="text-2xl font-black text-slate-100 tracking-tight">
              Install AxisOS 1.0 "Horizon"
            </h1>
            <p className="text-xs text-slate-400 mt-2 leading-relaxed">
              This automated wizard will guide you through setting up your language, picking your storage drive,
              configuring your user account, and deploying the complete AxisOS operating system.
            </p>

            <div className="mt-8 grid grid-cols-2 gap-3 w-full text-left">
              <div className="p-3.5 rounded-xl bg-white/5 border border-white/10">
                <div className="flex items-center gap-1.5 text-xs font-semibold text-emerald-400">
                  <ShieldCheck className="w-4 h-4" />
                  <span>Secure Boot Certified</span>
                </div>
                <div className="text-[11px] text-slate-400 mt-1">
                  Includes Microsoft-signed Shim and Debian-signed GRUB for out-of-the-box hardware trust.
                </div>
              </div>
              <div className="p-3.5 rounded-xl bg-white/5 border border-white/10">
                <div className="flex items-center gap-1.5 text-xs font-semibold text-cyan-400">
                  <Layers className="w-4 h-4" />
                  <span>Modern Btrfs Subvolumes</span>
                </div>
                <div className="text-[11px] text-slate-400 mt-1">
                  Transparent zstd compression with isolated @root, @home, and @snapshots subvolumes.
                </div>
              </div>
            </div>
          </div>
        )}

        {/* Step 1: Language & Keyboard */}
        {currentStep === 1 && (
          <div className="flex flex-col gap-5 max-w-lg mx-auto my-auto w-full">
            <div>
              <h2 className="text-xl font-bold text-slate-100">Language & Keyboard</h2>
              <p className="text-xs text-slate-400 mt-1">Select your preferred system language and keyboard layout.</p>
            </div>

            <div className="flex flex-col gap-2">
              <label className="text-xs text-slate-300 font-medium">System Language</label>
              <select
                value={installerData.language}
                onChange={(e) => updateInstallerData({ language: e.target.value })}
                className="w-full px-3 py-2.5 bg-slate-900 border border-white/10 rounded-xl text-xs text-slate-200 focus:outline-none focus:ring-2 focus:ring-cyan-500"
              >
                <option value="English (United States)">English (United States)</option>
                <option value="English (United Kingdom)">English (United Kingdom)</option>
                <option value="Français (France)">Français (France)</option>
                <option value="Deutsch (Deutschland)">Deutsch (Deutschland)</option>
                <option value="Español (España)">Español (España)</option>
                <option value="日本語 (Japan)">日本語 (Japan)</option>
              </select>
            </div>

            <div className="flex flex-col gap-2">
              <label className="text-xs text-slate-300 font-medium">Keyboard Layout</label>
              <select
                value={installerData.keyboardLayout}
                onChange={(e) => updateInstallerData({ keyboardLayout: e.target.value })}
                className="w-full px-3 py-2.5 bg-slate-900 border border-white/10 rounded-xl text-xs text-slate-200 focus:outline-none focus:ring-2 focus:ring-cyan-500"
              >
                <option value="English (US) - Standard">English (US) - Standard</option>
                <option value="English (US) - Dvorak">English (US) - Dvorak</option>
                <option value="English (UK) - Standard">English (UK) - Standard</option>
                <option value="German - QWERTZ">German - QWERTZ</option>
                <option value="French - AZERTY">French - AZERTY</option>
              </select>
            </div>
          </div>
        )}

        {/* Step 2: Storage & Partitioning */}
        {currentStep === 2 && (
          <div className="flex flex-col gap-4 max-w-xl mx-auto my-auto w-full">
            <div className="flex items-center justify-between">
              <div>
                <h2 className="text-xl font-bold text-slate-100">Select Target Drive</h2>
                <p className="text-xs text-slate-400 mt-0.5">
                  Choose the drive where you want to install AxisOS.
                </p>
              </div>
              <button
                onClick={refreshDisks}
                className="px-2.5 py-1.5 rounded-lg bg-white/5 hover:bg-white/10 text-slate-300 transition-colors flex items-center gap-1.5 text-xs font-medium cursor-pointer"
                title="Rescan drives"
              >
                <RefreshCw className="w-3.5 h-3.5" />
                <span>Rescan</span>
              </button>
            </div>

            {/* Drives List */}
            <div className="flex flex-col gap-2.5 max-h-64 overflow-y-auto pr-1">
              {availableDisks.length === 0 ? (
                <div className="p-6 rounded-2xl bg-slate-900 border border-white/10 text-center text-xs text-slate-400 flex flex-col items-center gap-2">
                  <RefreshCw className="w-5 h-5 animate-spin text-cyan-400" />
                  <span>Scanning connected storage devices...</span>
                </div>
              ) : (
                availableDisks.map((disk) => {
                  const isSelected = installerData.targetDisk === disk.id;
                  const isLive = disk.isLiveMedium;

                  return (
                    <button
                      key={disk.id}
                      disabled={isLive}
                      onClick={() => !isLive && updateInstallerData({ targetDisk: disk.id })}
                      className={`flex flex-col p-3.5 rounded-2xl border text-left transition-all ${
                        isLive
                          ? 'bg-slate-900/40 border-white/5 opacity-60 cursor-not-allowed'
                          : isSelected
                          ? `${accent.bg} ${accent.border} text-white ring-2 ring-cyan-500/40 cursor-pointer shadow-lg`
                          : 'bg-white/5 border-white/5 text-slate-300 hover:bg-white/10 hover:border-white/15 cursor-pointer'
                      }`}
                    >
                      <div className="flex items-center justify-between">
                        <div className="flex items-center space-x-3">
                          <div
                            className={`w-10 h-10 rounded-xl flex items-center justify-center shrink-0 ${
                              isSelected ? accent.primary + ' text-white shadow-md' : 'bg-slate-800 text-slate-400'
                            }`}
                          >
                            <HardDrive className="w-5 h-5" />
                          </div>
                          <div>
                            <div className="text-xs font-bold text-slate-100 flex items-center gap-2">
                              <span>{disk.model || disk.name}</span>
                              {isLive && (
                                <span className="px-2 py-0.5 rounded-full bg-amber-500/20 text-amber-300 text-[10px] font-semibold border border-amber-500/30">
                                  Live USB Installer (Protected)
                                </span>
                              )}
                              {disk.hasBitLocker && (
                                <span className="px-2 py-0.5 rounded-full bg-rose-500/20 text-rose-300 text-[10px] font-semibold border border-rose-500/30">
                                  BitLocker Encrypted
                                </span>
                              )}
                              {disk.hasWindows && (
                                <span className="px-2 py-0.5 rounded-full bg-blue-500/20 text-blue-300 text-[10px] font-semibold border border-blue-500/30">
                                  Windows Drive
                                </span>
                              )}
                            </div>
                            <div className="text-[11px] text-slate-400 mt-0.5 flex items-center gap-2 font-mono">
                              <span>{disk.id}</span>
                              <span>•</span>
                              <span>{disk.type}</span>
                            </div>
                          </div>
                        </div>

                        <div className="text-right">
                          <div className="text-xs font-mono font-bold text-slate-200">{disk.size}</div>
                          <div className="text-[10px] text-emerald-400">{disk.freeSpace}</div>
                        </div>
                      </div>

                      {/* Partition preview if available */}
                      {disk.partitions && disk.partitions.length > 0 && (
                        <div className="mt-2.5 pt-2 border-t border-white/5 flex flex-wrap gap-1.5 text-[10px] text-slate-400 font-mono">
                          <span className="text-slate-500">Partitions:</span>
                          {disk.partitions.map((p, i) => (
                            <span key={i} className="px-1.5 py-0.5 rounded bg-black/40 text-slate-300">
                              {p}
                            </span>
                          ))}
                        </div>
                      )}
                    </button>
                  );
                })
              )}
            </div>

            {/* Target Layout Scheme Card */}
            {selectedDisk && (
              <div className="p-3.5 rounded-xl bg-slate-900 border border-white/10 flex flex-col gap-2">
                <div className="text-xs font-bold text-slate-200 flex items-center gap-1.5">
                  <CheckCircle2 className="w-4 h-4 text-cyan-400" />
                  <span>Target Partition Layout (Automatic GPT)</span>
                </div>
                <div className="grid grid-cols-12 gap-1 text-center font-mono text-[10px] mt-1">
                  <div className="col-span-2 p-1.5 rounded-lg bg-emerald-500/20 border border-emerald-500/40 text-emerald-300">
                    <div className="font-bold">ESP 512 MB</div>
                    <div className="text-[9px] text-emerald-400/80">/boot/efi (FAT32)</div>
                  </div>
                  <div className="col-span-2 p-1.5 rounded-lg bg-purple-500/20 border border-purple-500/40 text-purple-300">
                    <div className="font-bold">Swap 4 GB</div>
                    <div className="text-[9px] text-purple-400/80">Linux Swap</div>
                  </div>
                  <div className="col-span-8 p-1.5 rounded-lg bg-cyan-500/20 border border-cyan-500/40 text-cyan-300">
                    <div className="font-bold">AxisOS Root (Remaining Space)</div>
                    <div className="text-[9px] text-cyan-400/80">Btrfs Subvolumes (@, @home, @snapshots, @var_log)</div>
                  </div>
                </div>
              </div>
            )}
          </div>
        )}

        {/* Step 3: User Account & Hostname */}
        {currentStep === 3 && (
          <div className="flex flex-col gap-4 max-w-lg mx-auto my-auto w-full">
            <div>
              <h2 className="text-xl font-bold text-slate-100">User Setup</h2>
              <p className="text-xs text-slate-400 mt-1">Configure your administrator profile and computer name.</p>
            </div>

            <div className="grid grid-cols-2 gap-3">
              <div className="flex flex-col gap-1.5">
                <label className="text-xs text-slate-300 font-medium">Your Full Name</label>
                <input
                  type="text"
                  value={installerData.userFullName}
                  onChange={(e) => updateInstallerData({ userFullName: e.target.value })}
                  placeholder="e.g. Jacky"
                  className="px-3 py-2 bg-slate-900 border border-white/10 rounded-xl text-xs text-slate-200 focus:outline-none focus:ring-2 focus:ring-cyan-500"
                />
              </div>

              <div className="flex flex-col gap-1.5">
                <label className="text-xs text-slate-300 font-medium">Username</label>
                <input
                  type="text"
                  value={installerData.username}
                  onChange={(e) => updateInstallerData({ username: e.target.value.toLowerCase().replace(/[^a-z0-9_-]/g, '') })}
                  placeholder="axis"
                  className="px-3 py-2 bg-slate-900 border border-white/10 rounded-xl text-xs text-slate-200 focus:outline-none focus:ring-2 focus:ring-cyan-500 font-mono"
                />
              </div>
            </div>

            <div className="flex flex-col gap-1.5">
              <label className="text-xs text-slate-300 font-medium">Computer Name (Hostname)</label>
              <input
                type="text"
                value={installerData.computerName}
                onChange={(e) => updateInstallerData({ computerName: e.target.value.toLowerCase().replace(/[^a-z0-9-]/g, '') })}
                placeholder="axis-pc"
                className="px-3 py-2 bg-slate-900 border border-white/10 rounded-xl text-xs text-slate-200 focus:outline-none focus:ring-2 focus:ring-cyan-500 font-mono"
              />
            </div>

            <div className="grid grid-cols-2 gap-3">
              <div className="flex flex-col gap-1.5">
                <label className="text-xs text-slate-300 font-medium">Password</label>
                <input
                  type="password"
                  value={installerData.password}
                  onChange={(e) => updateInstallerData({ password: e.target.value })}
                  placeholder="Enter password"
                  className="px-3 py-2 bg-slate-900 border border-white/10 rounded-xl text-xs text-slate-200 focus:outline-none focus:ring-2 focus:ring-cyan-500"
                />
              </div>

              <div className="flex flex-col gap-1.5">
                <label className="text-xs text-slate-300 font-medium">Confirm Password</label>
                <input
                  type="password"
                  value={confirmPassword}
                  onChange={(e) => setConfirmPassword(e.target.value)}
                  placeholder="Confirm password"
                  className={`px-3 py-2 bg-slate-900 border rounded-xl text-xs text-slate-200 focus:outline-none focus:ring-2 ${
                    !passwordsMatch && confirmPassword ? 'border-rose-500 focus:ring-rose-500' : 'border-white/10 focus:ring-cyan-500'
                  }`}
                />
              </div>
            </div>

            {!passwordsMatch && confirmPassword && (
              <div className="text-[11px] text-rose-400 font-medium">Passwords do not match.</div>
            )}

            <div className="pt-2">
              <label className="flex items-center space-x-2.5 cursor-pointer text-xs text-slate-300">
                <input
                  type="checkbox"
                  checked={installerData.autoLogin}
                  onChange={(e) => updateInstallerData({ autoLogin: e.target.checked })}
                  className="w-4 h-4 accent-cyan-500 rounded"
                />
                <span>Log in automatically without asking for password on startup</span>
              </label>
            </div>
          </div>
        )}

        {/* Step 4: Summary & Ready */}
        {currentStep === 4 && (
          <div className="flex flex-col gap-4 max-w-lg mx-auto my-auto w-full">
            <div>
              <h2 className="text-xl font-bold text-slate-100">Ready to Install</h2>
              <p className="text-xs text-slate-400 mt-1">Review the deployment configuration below.</p>
            </div>

            <div className="bg-slate-900 border border-white/10 rounded-2xl p-4 flex flex-col gap-2.5 text-xs">
              <div className="flex justify-between py-1.5 border-b border-white/5">
                <span className="text-slate-400">Target Disk</span>
                <span className="font-semibold text-slate-200 font-mono text-right">
                  {selectedDisk ? `${selectedDisk.model || selectedDisk.name} (${selectedDisk.id})` : installerData.targetDisk}
                </span>
              </div>
              <div className="flex justify-between py-1.5 border-b border-white/5">
                <span className="text-slate-400">Bootloader Security</span>
                <span className="font-semibold text-emerald-400 flex items-center gap-1">
                  <ShieldCheck className="w-3.5 h-3.5" />
                  <span>Microsoft UEFI CA Signed Shim (Secure Boot ON)</span>
                </span>
              </div>
              <div className="flex justify-between py-1.5 border-b border-white/5">
                <span className="text-slate-400">UEFI Visibility</span>
                <span className="font-semibold text-cyan-400">
                  NVRAM Entry + Universal /EFI/BOOT/BOOTX64.EFI Fallback
                </span>
              </div>
              <div className="flex justify-between py-1.5 border-b border-white/5">
                <span className="text-slate-400">Root Filesystem</span>
                <span className="font-semibold text-slate-200">Btrfs (zstd:3 subvolumes @, @home, @snapshots)</span>
              </div>
              <div className="flex justify-between py-1.5 border-b border-white/5">
                <span className="text-slate-400">Primary Account</span>
                <span className="font-semibold text-slate-200">
                  {installerData.username} ({installerData.userFullName})
                </span>
              </div>
              <div className="flex justify-between py-1.5">
                <span className="text-slate-400">Auto-login</span>
                <span className="font-semibold text-emerald-400">{installerData.autoLogin ? 'Enabled' : 'Disabled'}</span>
              </div>
            </div>

            <div className="p-3.5 bg-amber-500/10 border border-amber-500/25 rounded-2xl flex items-center gap-3 text-amber-300 text-xs">
              <AlertTriangle className="w-5 h-5 shrink-0 text-amber-400" />
              <span>
                Warning: Proceeding will erase all data and format{' '}
                <strong className="text-amber-200">{installerData.targetDisk}</strong>. Make sure you selected the correct drive!
              </span>
            </div>
          </div>
        )}

        {/* Step 5: Real Installing Progress & Live Log */}
        {currentStep === 5 && (
          <div className="flex flex-col items-center justify-center my-auto max-w-lg mx-auto w-full text-center">
            <div className="w-16 h-16 rounded-2xl bg-gradient-to-tr from-cyan-500 to-blue-600 flex items-center justify-center text-white shadow-xl shadow-cyan-950/50 mb-6 animate-pulse ring-4 ring-white/10">
              <Cpu className="w-8 h-8 text-cyan-100" />
            </div>

            <h2 className="text-xl font-bold text-slate-100">Installing AxisOS 1.0</h2>
            <p className="text-xs text-slate-400 mt-1 max-w-sm">
              Deploying kernel, Btrfs subvolumes, base system, and Microsoft-signed bootloader.
            </p>

            {/* Error Message if any */}
            {installError && (
              <div className="mt-4 p-3.5 bg-rose-500/10 border border-rose-500/30 rounded-2xl text-rose-300 text-xs text-left w-full">
                <div className="font-bold flex items-center gap-1.5">
                  <AlertTriangle className="w-4 h-4 text-rose-400" />
                  <span>Installation Error</span>
                </div>
                <div className="mt-1 font-mono text-[11px] break-all">{installError}</div>
                <button
                  onClick={resetInstaller}
                  className="mt-3 px-3 py-1.5 bg-white/10 hover:bg-white/20 rounded-lg text-slate-200 text-xs font-semibold cursor-pointer"
                >
                  Restart Installer
                </button>
              </div>
            )}

            {/* Progress Bar */}
            <div className="w-full mt-6 bg-slate-900 border border-white/10 rounded-full h-3.5 overflow-hidden p-0.5">
              <div
                className="h-full bg-gradient-to-r from-cyan-500 via-blue-500 to-indigo-500 rounded-full transition-all duration-300 shadow-sm"
                style={{ width: `${installProgress}%` }}
              />
            </div>

            <div className="flex items-center justify-between w-full mt-2.5 text-[11px] font-mono text-slate-400">
              <span className="truncate max-w-[80%] text-left">{installStatusText}</span>
              <span className="font-bold text-cyan-400">{installProgress}%</span>
            </div>

            {/* Live Log Console Toggle */}
            <div className="w-full mt-5 flex flex-col items-start">
              <button
                onClick={() => setShowLogConsole(!showLogConsole)}
                className="flex items-center gap-1.5 text-[11px] text-slate-400 hover:text-cyan-400 transition-colors cursor-pointer"
              >
                <Terminal className="w-3.5 h-3.5" />
                <span>{showLogConsole ? 'Hide Console Output' : 'View Live Installation Logs'}</span>
              </button>

              {showLogConsole && (
                <div className="w-full h-44 mt-2 p-3.5 bg-black/85 border border-white/10 rounded-2xl overflow-y-auto font-mono text-[10px] text-slate-300 text-left space-y-1">
                  {installLogs.map((log, i) => (
                    <div key={i} className="leading-tight break-all font-mono">
                      {log}
                    </div>
                  ))}
                </div>
              )}
            </div>
          </div>
        )}

        {/* Step 6: Complete */}
        {currentStep === 6 && (
          <div className="flex flex-col items-center justify-center my-auto max-w-lg mx-auto w-full text-center">
            <div className="w-16 h-16 rounded-2xl bg-gradient-to-tr from-emerald-500 to-teal-600 flex items-center justify-center text-white shadow-xl shadow-emerald-950/50 mb-4 ring-4 ring-emerald-500/20">
              <CheckCircle className="w-8 h-8 text-white" />
            </div>

            <h2 className="text-2xl font-black text-slate-100">Installation Finished!</h2>
            <p className="text-xs text-slate-400 mt-2 max-w-md leading-relaxed">
              AxisOS has been successfully installed to <span className="font-bold text-slate-200">{installerData.targetDisk}</span>.
              The Microsoft-signed UEFI bootloader is ready and registered in your computer's motherboard.
            </p>

            <div className="p-3.5 mt-5 rounded-2xl bg-emerald-500/10 border border-emerald-500/20 text-emerald-300 text-xs text-left max-w-md">
              <div className="font-bold flex items-center gap-1.5">
                <CheckCircle2 className="w-4 h-4" />
                <span>Ready to Boot</span>
              </div>
              <p className="mt-1 text-[11px] text-emerald-400/90 leading-normal">
                Please remove your USB installer drive after rebooting. Your computer will automatically detect AxisOS in the boot menu!
              </p>
            </div>

            <div className="flex items-center gap-3 mt-7">
              <button
                onClick={() => handleFinish('restart')}
                className="px-6 py-2.5 rounded-xl bg-gradient-to-r from-cyan-600 to-blue-600 hover:from-cyan-500 hover:to-blue-500 text-white font-bold text-xs shadow-lg shadow-cyan-950 transition-all flex items-center gap-2 cursor-pointer"
              >
                <RotateCcw className="w-3.5 h-3.5" />
                <span>Restart Computer Now</span>
              </button>
              <button
                onClick={() => handleFinish('continue')}
                className="px-5 py-2.5 rounded-xl bg-white/5 hover:bg-white/10 text-slate-300 font-medium text-xs transition-colors cursor-pointer"
              >
                Continue Live Session
              </button>
            </div>
          </div>
        )}

        {/* Bottom Step Navigation Bar */}
        {currentStep < 5 && (
          <div className="flex items-center justify-between pt-6 border-t border-white/5">
            <button
              onClick={goToPrevStep}
              disabled={currentStep === 0}
              className={`flex items-center space-x-1.5 px-4 py-2 rounded-xl text-xs font-semibold transition-all cursor-pointer ${
                currentStep === 0
                  ? 'opacity-0 pointer-events-none'
                  : 'bg-white/5 hover:bg-white/10 text-slate-300'
              }`}
            >
              <ArrowLeft className="w-3.5 h-3.5" />
              <span>Back</span>
            </button>

            {currentStep === 4 ? (
              <button
                onClick={startInstallation}
                className="flex items-center space-x-1.5 px-6 py-2.5 rounded-xl bg-gradient-to-r from-cyan-600 to-blue-600 hover:from-cyan-500 hover:to-blue-500 text-white font-bold text-xs shadow-lg shadow-cyan-950 transition-all cursor-pointer"
              >
                <span>Install AxisOS Now</span>
                <Sparkles className="w-3.5 h-3.5" />
              </button>
            ) : (
              <button
                onClick={goToNextStep}
                disabled={currentStep === 3 && (!passwordsMatch || !installerData.username)}
                className={`flex items-center space-x-1.5 px-5 py-2 rounded-xl bg-gradient-to-r from-cyan-600 to-blue-600 hover:from-cyan-500 hover:to-blue-500 text-white font-semibold text-xs shadow-lg shadow-cyan-950 transition-all cursor-pointer ${
                  currentStep === 3 && (!passwordsMatch || !installerData.username)
                    ? 'opacity-50 pointer-events-none'
                    : ''
                }`}
              >
                <span>Next</span>
                <ArrowRight className="w-3.5 h-3.5" />
              </button>
            )}
          </div>
        )}
      </div>
    </div>
  );
};
