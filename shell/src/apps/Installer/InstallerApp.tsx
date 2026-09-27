import React, { useState } from 'react';
import {
  Sparkles,
  HardDrive,
  User,
  CheckCircle,
  Globe,
  ArrowRight,
  ArrowLeft,
  RotateCcw,
  Check,
  Shield,
  Layers,
  Cpu,
  Monitor,
} from 'lucide-react';
import { useInstaller, INSTALL_STEPS } from '../../context/InstallerContext';
import { useSystemState, ACCENT_COLOR_MAP } from '../../context/SystemStateContext';
import { useWindowManager } from '../../context/WindowManagerContext';

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
    startInstallation,
    resetInstaller,
  } = useInstaller();

  const { accentColor, setIsLiveEnvironment } = useSystemState();
  const { closeWindow, windows } = useWindowManager();
  const accent = ACCENT_COLOR_MAP[accentColor];

  const handleFinish = (action: 'restart' | 'continue') => {
    setIsLiveEnvironment(false);
    const installerWindow = windows.find((w) => w.appId === 'installer');
    if (installerWindow) {
      closeWindow(installerWindow.id);
    }
  };

  return (
    <div className="flex h-full w-full bg-slate-950 text-slate-100 select-none">
      {/* Left Stepper Sidebar */}
      <div className="w-56 bg-slate-900/80 border-r border-white/5 p-5 flex flex-col justify-between">
        <div>
          {/* Logo */}
          <div className="flex items-center space-x-2.5 mb-8">
            <div className="w-8 h-8 rounded-lg bg-gradient-to-tr from-cyan-500 to-blue-600 flex items-center justify-center font-bold text-white shadow-md text-sm">
              ▲
            </div>
            <div>
              <div className="text-sm font-bold tracking-tight">AxisOS</div>
              <div className="text-[10px] text-cyan-400 font-mono">System Installer</div>
            </div>
          </div>

          {/* Stepper Steps */}
          <div className="flex flex-col space-y-2">
            {INSTALL_STEPS.map((step, idx) => {
              const isPast = currentStep > idx;
              const isCurrent = currentStep === idx;

              return (
                <div
                  key={step}
                  className={`flex items-center space-x-3 px-3 py-2 rounded-lg text-xs font-medium transition-all ${
                    isCurrent
                      ? `${accent.bg} ${accent.border} text-white font-semibold border`
                      : isPast
                      ? 'text-emerald-400'
                      : 'text-slate-500'
                  }`}
                >
                  <div
                    className={`w-5 h-5 rounded-full flex items-center justify-center text-[10px] ${
                      isCurrent
                        ? `${accent.primary} text-white`
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
        <div className="text-[10px] text-slate-500 border-t border-white/5 pt-3">
          Kernel 6.12 • EFI Bootloader • Debian Core
        </div>
      </div>

      {/* Main Content Area */}
      <div className="flex-1 flex flex-col justify-between p-8 bg-slate-950/60 overflow-y-auto">
        {/* Step 0: Welcome */}
        {currentStep === 0 && (
          <div className="flex flex-col items-center justify-center text-center my-auto max-w-lg mx-auto">
            <div className="w-16 h-16 rounded-2xl bg-gradient-to-tr from-cyan-500 to-blue-600 flex items-center justify-center text-white shadow-xl shadow-cyan-950/50 mb-4">
              <Sparkles className="w-8 h-8 text-cyan-100" />
            </div>
            <h1 className="text-2xl font-black text-slate-100 tracking-tight">
              Welcome to AxisOS 1.0 "Horizon"
            </h1>
            <p className="text-xs text-slate-400 mt-2 leading-relaxed">
              AxisOS combines the security, power, and stability of the Linux kernel with a fluid,
              glassmorphic modern desktop interface. This installer will guide you through setting up
              AxisOS on your machine.
            </p>

            <div className="mt-8 grid grid-cols-2 gap-3 w-full">
              <div className="p-3.5 rounded-xl bg-white/5 border border-white/10 text-left">
                <div className="text-xs font-semibold text-cyan-400">Wayland Native</div>
                <div className="text-[11px] text-slate-400 mt-1">
                  Built for modern GPU acceleration and tear-free fluid rendering.
                </div>
              </div>
              <div className="p-3.5 rounded-xl bg-white/5 border border-white/10 text-left">
                <div className="text-xs font-semibold text-emerald-400">Btrfs Subvolumes</div>
                <div className="text-[11px] text-slate-400 mt-1">
                  Snapshots and zstd compression enabled by default.
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
              <p className="text-xs text-slate-400 mt-1">Select your preferred system language and layout.</p>
            </div>

            <div className="flex flex-col gap-3">
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

            <div className="flex flex-col gap-3">
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
          <div className="flex flex-col gap-5 max-w-xl mx-auto my-auto w-full">
            <div>
              <h2 className="text-xl font-bold text-slate-100">Storage Destination</h2>
              <p className="text-xs text-slate-400 mt-1">Select the drive where AxisOS will be installed.</p>
            </div>

            {/* Drives List */}
            <div className="flex flex-col gap-2.5">
              {availableDisks.map((disk) => {
                const isSelected = installerData.targetDisk === disk.id;
                return (
                  <button
                    key={disk.id}
                    onClick={() => updateInstallerData({ targetDisk: disk.id })}
                    className={`flex items-center justify-between p-3.5 rounded-xl border text-left transition-all ${
                      isSelected
                        ? `${accent.bg} ${accent.border} text-white`
                        : 'bg-white/5 border-white/5 text-slate-300 hover:bg-white/10'
                    }`}
                  >
                    <div className="flex items-center space-x-3">
                      <div
                        className={`w-9 h-9 rounded-lg flex items-center justify-center ${
                          isSelected ? accent.primary + ' text-white' : 'bg-slate-800 text-slate-400'
                        }`}
                      >
                        <HardDrive className="w-5 h-5" />
                      </div>
                      <div>
                        <div className="text-xs font-semibold">{disk.name}</div>
                        <div className="text-[11px] text-slate-400">
                          {disk.id} • {disk.type}
                        </div>
                      </div>
                    </div>
                    <div className="text-right">
                      <div className="text-xs font-mono font-semibold">{disk.size}</div>
                      <div className="text-[10px] text-emerald-400">{disk.freeSpace}</div>
                    </div>
                  </button>
                );
              })}
            </div>

            {/* Installation Scheme */}
            <div className="p-4 rounded-xl bg-slate-900 border border-white/10 flex flex-col gap-3">
              <label className="flex items-start space-x-3 cursor-pointer">
                <input
                  type="radio"
                  name="installScheme"
                  checked={installerData.eraseDisk}
                  onChange={() => updateInstallerData({ eraseDisk: true })}
                  className="mt-1 accent-cyan-500"
                />
                <div>
                  <div className="text-xs font-semibold text-slate-200">
                    Erase disk and install AxisOS (Recommended)
                  </div>
                  <div className="text-[11px] text-slate-400 mt-0.5">
                    Partitions disk automatically with 1GB ESP (/boot/efi), 4GB Swap, and Btrfs root with subvolumes (@, @home, @snapshots).
                  </div>
                </div>
              </label>
            </div>
          </div>
        )}

        {/* Step 3: User Account & Hostname */}
        {currentStep === 3 && (
          <div className="flex flex-col gap-4 max-w-lg mx-auto my-auto w-full">
            <div>
              <h2 className="text-xl font-bold text-slate-100">User Setup</h2>
              <p className="text-xs text-slate-400 mt-1">Configure your primary administrative user account.</p>
            </div>

            <div className="grid grid-cols-2 gap-3">
              <div className="flex flex-col gap-1.5">
                <label className="text-xs text-slate-300">Your Full Name</label>
                <input
                  type="text"
                  value={installerData.userFullName}
                  onChange={(e) => updateInstallerData({ userFullName: e.target.value })}
                  placeholder="e.g. Alex Hunter"
                  className="px-3 py-2 bg-slate-900 border border-white/10 rounded-xl text-xs text-slate-200 focus:outline-none focus:ring-2 focus:ring-cyan-500"
                />
              </div>

              <div className="flex flex-col gap-1.5">
                <label className="text-xs text-slate-300">Username</label>
                <input
                  type="text"
                  value={installerData.username}
                  onChange={(e) => updateInstallerData({ username: e.target.value })}
                  placeholder="e.g. axis"
                  className="px-3 py-2 bg-slate-900 border border-white/10 rounded-xl text-xs text-slate-200 focus:outline-none focus:ring-2 focus:ring-cyan-500 font-mono"
                />
              </div>
            </div>

            <div className="flex flex-col gap-1.5">
              <label className="text-xs text-slate-300">Computer Name (Hostname)</label>
              <input
                type="text"
                value={installerData.computerName}
                onChange={(e) => updateInstallerData({ computerName: e.target.value })}
                placeholder="axis-pc"
                className="px-3 py-2 bg-slate-900 border border-white/10 rounded-xl text-xs text-slate-200 focus:outline-none focus:ring-2 focus:ring-cyan-500 font-mono"
              />
            </div>

            <div className="grid grid-cols-2 gap-3">
              <div className="flex flex-col gap-1.5">
                <label className="text-xs text-slate-300">Password</label>
                <input
                  type="password"
                  value={installerData.password}
                  onChange={(e) => updateInstallerData({ password: e.target.value })}
                  placeholder="••••••••"
                  className="px-3 py-2 bg-slate-900 border border-white/10 rounded-xl text-xs text-slate-200 focus:outline-none focus:ring-2 focus:ring-cyan-500"
                />
              </div>

              <div className="flex flex-col gap-1.5">
                <label className="text-xs text-slate-300">Auto-login on Boot</label>
                <div className="h-9 flex items-center">
                  <label className="flex items-center space-x-2 cursor-pointer text-xs text-slate-300">
                    <input
                      type="checkbox"
                      checked={installerData.autoLogin}
                      onChange={(e) => updateInstallerData({ autoLogin: e.target.checked })}
                      className="accent-cyan-500 rounded"
                    />
                    <span>Log in automatically</span>
                  </label>
                </div>
              </div>
            </div>
          </div>
        )}

        {/* Step 4: Summary & Ready */}
        {currentStep === 4 && (
          <div className="flex flex-col gap-5 max-w-lg mx-auto my-auto w-full">
            <div>
              <h2 className="text-xl font-bold text-slate-100">Ready to Install</h2>
              <p className="text-xs text-slate-400 mt-1">Review your installation configuration before proceeding.</p>
            </div>

            <div className="bg-slate-900 border border-white/10 rounded-2xl p-4 flex flex-col gap-3 text-xs">
              <div className="flex justify-between py-1.5 border-b border-white/5">
                <span className="text-slate-400">Target Disk</span>
                <span className="font-semibold text-slate-200">{installerData.targetDisk} (All data will be erased)</span>
              </div>
              <div className="flex justify-between py-1.5 border-b border-white/5">
                <span className="text-slate-400">Filesystem</span>
                <span className="font-semibold text-cyan-400">Btrfs (zstd compression)</span>
              </div>
              <div className="flex justify-between py-1.5 border-b border-white/5">
                <span className="text-slate-400">Primary User</span>
                <span className="font-semibold text-slate-200">{installerData.username} ({installerData.userFullName})</span>
              </div>
              <div className="flex justify-between py-1.5 border-b border-white/5">
                <span className="text-slate-400">Hostname</span>
                <span className="font-semibold text-slate-200 font-mono">{installerData.computerName}</span>
              </div>
              <div className="flex justify-between py-1.5">
                <span className="text-slate-400">Language & Keyboard</span>
                <span className="font-semibold text-slate-200">{installerData.language} / {installerData.keyboardLayout}</span>
              </div>
            </div>
          </div>
        )}

        {/* Step 5: Installing Animation & Progress */}
        {currentStep === 5 && (
          <div className="flex flex-col items-center justify-center my-auto max-w-lg mx-auto w-full text-center">
            <div className="w-16 h-16 rounded-2xl bg-gradient-to-tr from-cyan-500 to-blue-600 flex items-center justify-center text-white shadow-xl shadow-cyan-950/50 mb-6 animate-pulse">
              <Cpu className="w-8 h-8 text-cyan-100" />
            </div>

            <h2 className="text-xl font-bold text-slate-100">Installing AxisOS 1.0</h2>
            <p className="text-xs text-slate-400 mt-1 max-w-sm">
              Please wait while the Linux kernel, system packages, and the AxisOS desktop shell are deployed.
            </p>

            {/* Progress Bar */}
            <div className="w-full mt-6 bg-slate-900 border border-white/10 rounded-full h-3 overflow-hidden p-0.5">
              <div
                className="h-full bg-gradient-to-r from-cyan-500 to-blue-500 rounded-full transition-all duration-300"
                style={{ width: `${installProgress}%` }}
              ></div>
            </div>

            <div className="flex items-center justify-between w-full mt-2 text-[11px] font-mono text-slate-400">
              <span className="truncate max-w-[80%] text-left">{installStatusText}</span>
              <span className="font-bold text-cyan-400">{installProgress}%</span>
            </div>
          </div>
        )}

        {/* Step 6: Complete */}
        {currentStep === 6 && (
          <div className="flex flex-col items-center justify-center my-auto max-w-lg mx-auto w-full text-center">
            <div className="w-16 h-16 rounded-2xl bg-gradient-to-tr from-emerald-500 to-teal-600 flex items-center justify-center text-white shadow-xl shadow-emerald-950/50 mb-4">
              <CheckCircle className="w-8 h-8 text-white" />
            </div>

            <h2 className="text-2xl font-black text-slate-100">Installation Finished!</h2>
            <p className="text-xs text-slate-400 mt-2 max-w-md leading-relaxed">
              AxisOS has been successfully installed on <span className="font-semibold text-slate-200">{installerData.targetDisk}</span>. You can now reboot your system into your new operating system or continue testing in the live environment.
            </p>

            <div className="flex items-center gap-3 mt-8">
              <button
                onClick={() => handleFinish('restart')}
                className="px-5 py-2.5 rounded-xl bg-gradient-to-r from-cyan-600 to-blue-600 hover:from-cyan-500 hover:to-blue-500 text-white font-semibold text-xs shadow-lg shadow-cyan-950 transition-all flex items-center gap-2"
              >
                <RotateCcw className="w-3.5 h-3.5" />
                <span>Restart Now</span>
              </button>
              <button
                onClick={() => handleFinish('continue')}
                className="px-5 py-2.5 rounded-xl bg-white/5 hover:bg-white/10 text-slate-300 font-medium text-xs transition-colors"
              >
                Continue Testing
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
              className={`flex items-center space-x-1.5 px-4 py-2 rounded-xl text-xs font-semibold transition-all ${
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
                className="flex items-center space-x-1.5 px-5 py-2 rounded-xl bg-gradient-to-r from-cyan-600 to-blue-600 hover:from-cyan-500 hover:to-blue-500 text-white font-semibold text-xs shadow-lg shadow-cyan-950 transition-all"
              >
                <span>Install AxisOS Now</span>
                <Sparkles className="w-3.5 h-3.5" />
              </button>
            ) : (
              <button
                onClick={goToNextStep}
                className="flex items-center space-x-1.5 px-5 py-2 rounded-xl bg-gradient-to-r from-cyan-600 to-blue-600 hover:from-cyan-500 hover:to-blue-500 text-white font-semibold text-xs shadow-lg shadow-cyan-950 transition-all"
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
