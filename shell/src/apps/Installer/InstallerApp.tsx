import React, { useState, useEffect, useRef } from 'react';
import {
  Globe,
  MapPin,
  Keyboard,
  HardDrive,
  User,
  Check,
  CheckCircle2,
  ShieldCheck,
  AlertTriangle,
  Terminal,
  RefreshCw,
  Lock,
  Sparkles,
  ArrowRight,
  ArrowLeft,
  RotateCcw,
  Laptop,
  Eye,
  EyeOff,
  Layers,
  Disc,
} from 'lucide-react';
import { useInstaller, INSTALL_STEPS } from '../../context/InstallerContext';
import { useSystemState, ACCENT_COLOR_MAP } from '../../context/SystemStateContext';
import { useWindowManager } from '../../context/WindowManagerContext';
import { systemService } from '../../services/systemService';
import { AxisLogo } from '../../components/AxisLogo';

// Comprehensive language, location, and keyboard datasets
const LANGUAGE_OPTIONS = [
  { label: 'English (United States)', code: 'en_US.UTF-8', defaultLocation: 'United States', defaultKeymap: 'us' },
  { label: 'English (United Kingdom)', code: 'en_GB.UTF-8', defaultLocation: 'United Kingdom', defaultKeymap: 'gb' },
  { label: 'English (South Africa)', code: 'en_ZA.UTF-8', defaultLocation: 'South Africa', defaultKeymap: 'us' },
  { label: 'English (Canada)', code: 'en_CA.UTF-8', defaultLocation: 'Canada', defaultKeymap: 'us' },
  { label: 'English (Australia)', code: 'en_AU.UTF-8', defaultLocation: 'Australia', defaultKeymap: 'us' },
  { label: 'Español (España)', code: 'es_ES.UTF-8', defaultLocation: 'Spain', defaultKeymap: 'es' },
  { label: 'Español (México)', code: 'es_MX.UTF-8', defaultLocation: 'Mexico', defaultKeymap: 'latam' },
  { label: 'Français (France)', code: 'fr_FR.UTF-8', defaultLocation: 'France', defaultKeymap: 'fr' },
  { label: 'Deutsch (Deutschland)', code: 'de_DE.UTF-8', defaultLocation: 'Germany', defaultKeymap: 'de' },
  { label: 'Português (Brasil)', code: 'pt_BR.UTF-8', defaultLocation: 'Brazil', defaultKeymap: 'br' },
  { label: 'Português (Portugal)', code: 'pt_PT.UTF-8', defaultLocation: 'Portugal', defaultKeymap: 'pt' },
  { label: 'Italiano (Italia)', code: 'it_IT.UTF-8', defaultLocation: 'Italy', defaultKeymap: 'it' },
  { label: 'Nederlands (Nederland)', code: 'nl_NL.UTF-8', defaultLocation: 'Netherlands', defaultKeymap: 'us' },
  { label: 'Polski (Polska)', code: 'pl_PL.UTF-8', defaultLocation: 'Poland', defaultKeymap: 'pl' },
  { label: 'Русский (Россия)', code: 'ru_RU.UTF-8', defaultLocation: 'Russia', defaultKeymap: 'ru' },
  { label: '日本語 (日本)', code: 'ja_JP.UTF-8', defaultLocation: 'Japan', defaultKeymap: 'jp' },
  { label: '中文 (简体, 中国)', code: 'zh_CN.UTF-8', defaultLocation: 'China', defaultKeymap: 'us' },
];

const LOCATION_OPTIONS = [
  { label: 'United States (Pacific / New York)', timezone: 'America/New_York', locale: 'en_US.UTF-8' },
  { label: 'United Kingdom (London)', timezone: 'Europe/London', locale: 'en_GB.UTF-8' },
  { label: 'South Africa (Johannesburg)', timezone: 'Africa/Johannesburg', locale: 'en_ZA.UTF-8' },
  { label: 'Canada (Toronto / Vancouver)', timezone: 'America/Toronto', locale: 'en_CA.UTF-8' },
  { label: 'Australia (Sydney / Melbourne)', timezone: 'Australia/Sydney', locale: 'en_AU.UTF-8' },
  { label: 'Germany (Berlin)', timezone: 'Europe/Berlin', locale: 'de_DE.UTF-8' },
  { label: 'France (Paris)', timezone: 'Europe/Paris', locale: 'fr_FR.UTF-8' },
  { label: 'Spain (Madrid)', timezone: 'Europe/Madrid', locale: 'es_ES.UTF-8' },
  { label: 'Brazil (São Paulo)', timezone: 'America/Sao_Paulo', locale: 'pt_BR.UTF-8' },
  { label: 'Japan (Tokyo)', timezone: 'Asia/Tokyo', locale: 'ja_JP.UTF-8' },
  { label: 'China (Beijing / Shanghai)', timezone: 'Asia/Shanghai', locale: 'zh_CN.UTF-8' },
  { label: 'India (New Delhi / Kolkata)', timezone: 'Asia/Kolkata', locale: 'en_IN.UTF-8' },
  { label: 'Universal Coordinated Time (UTC)', timezone: 'UTC', locale: 'en_US.UTF-8' },
];

const KEYBOARD_OPTIONS = [
  { label: 'US (QWERTY) - Standard', keymap: 'us' },
  { label: 'United Kingdom (QWERTY)', keymap: 'gb' },
  { label: 'German (QWERTZ)', keymap: 'de' },
  { label: 'French (AZERTY)', keymap: 'fr' },
  { label: 'Spanish (QWERTY)', keymap: 'es' },
  { label: 'Latin American (QWERTY)', keymap: 'latam' },
  { label: 'Portuguese (Brazil ABNT2)', keymap: 'br' },
  { label: 'Italian (QWERTY)', keymap: 'it' },
  { label: 'Japanese', keymap: 'jp' },
  { label: 'US (Dvorak)', keymap: 'dvorak' },
];

export const InstallerApp: React.FC = () => {
  const {
    currentStep,
    goToNextStep,
    goToPrevStep,
    jumpToStep,
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
  const [showPassword, setShowPassword] = useState(false);
  const [confirmModalOpen, setConfirmModalOpen] = useState(false);
  const logContainerRef = useRef<HTMLDivElement>(null);
  const accent = ACCENT_COLOR_MAP[accentColor];

  // Auto-scroll logs
  useEffect(() => {
    if (logContainerRef.current) {
      logContainerRef.current.scrollTop = logContainerRef.current.scrollHeight;
    }
  }, [installLogs]);

  // Handle Language selection change with smart cascade
  const handleLanguageChange = (langLabel: string) => {
    const selected = LANGUAGE_OPTIONS.find((l) => l.label === langLabel);
    if (selected) {
      const matchingLoc = LOCATION_OPTIONS.find((loc) => loc.label.includes(selected.defaultLocation)) || LOCATION_OPTIONS[0];
      const matchingKey = KEYBOARD_OPTIONS.find((k) => k.keymap === selected.defaultKeymap) || KEYBOARD_OPTIONS[0];

      updateInstallerData({
        language: selected.label,
        location: matchingLoc.label,
        keyboardLayout: matchingKey.label,
        locale: selected.code,
        timezone: matchingLoc.timezone,
        keymap: matchingKey.keymap,
      });
    } else {
      updateInstallerData({ language: langLabel });
    }
  };

  const handleLocationChange = (locLabel: string) => {
    const selected = LOCATION_OPTIONS.find((l) => l.label === locLabel);
    if (selected) {
      updateInstallerData({
        location: selected.label,
        timezone: selected.timezone,
        locale: selected.locale,
      });
    } else {
      updateInstallerData({ location: locLabel });
    }
  };

  const handleKeyboardChange = (keyLabel: string) => {
    const selected = KEYBOARD_OPTIONS.find((k) => k.label === keyLabel);
    if (selected) {
      updateInstallerData({
        keyboardLayout: selected.label,
        keymap: selected.keymap,
      });
    } else {
      updateInstallerData({ keyboardLayout: keyLabel });
    }
  };

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
    <div className="flex flex-col h-full w-full bg-slate-950 text-slate-100 select-none overflow-hidden font-sans">
      {/* Windows Setup Header Bar */}
      <div className="h-12 bg-slate-900 border-b border-white/10 px-5 flex items-center justify-between shrink-0">
        <div className="flex items-center space-x-3">
          <div className="w-6 h-6 rounded-lg bg-gradient-to-tr from-[#1b2b20] to-[#87cf3e] flex items-center justify-center text-black shadow-md ring-1 ring-[#87cf3e]/30">
            <AxisLogo size={14} variant="black" />
          </div>
          <span className="text-sm font-semibold tracking-tight text-white">
            AxisOS Setup
          </span>
          <span className="text-xs text-slate-500 font-normal">|</span>
          <span className="text-xs text-[#87cf3e] font-medium">
            {INSTALL_STEPS[currentStep]}
          </span>
        </div>

        {/* Windows-style Step Breadcrumbs */}
        <div className="hidden sm:flex items-center space-x-2 text-xs">
          {INSTALL_STEPS.map((step, idx) => {
            const isPast = currentStep > idx;
            const isCurrent = currentStep === idx;
            return (
              <div key={step} className="flex items-center space-x-1.5">
                <div
                  className={`w-4 h-4 rounded-full flex items-center justify-center text-[9px] font-bold transition-all ${
                    isCurrent
                      ? 'bg-[#87cf3e] text-black font-bold ring-2 ring-[#87cf3e]/40'
                      : isPast
                      ? 'bg-emerald-500/20 text-[#87cf3e]'
                      : 'bg-slate-800 text-slate-500'
                  }`}
                >
                  {isPast ? <Check className="w-2.5 h-2.5" /> : idx + 1}
                </div>
                <span
                  className={`${
                    isCurrent
                      ? 'text-white font-semibold'
                      : isPast
                      ? 'text-[#87cf3e]/90'
                      : 'text-slate-500'
                  }`}
                >
                  {step}
                </span>
                {idx < INSTALL_STEPS.length - 1 && (
                  <span className="text-slate-700 text-[10px]">›</span>
                )}
              </div>
            );
          })}
        </div>
      </div>

      {/* Main Setup Content Area */}
      <div className="flex-1 overflow-y-auto p-6 md:p-10 flex flex-col justify-center items-center">
        {/* ============================================================ */}
        {/* STEP 0: Language, Location & Keyboard (Windows Setup Style) */}
        {/* ============================================================ */}
        {currentStep === 0 && (
          <div className="w-full max-w-xl flex flex-col gap-6 animate-in fade-in duration-200">
            <div className="text-left border-b border-white/10 pb-4">
              <h1 className="text-2xl font-bold tracking-tight text-white flex items-center gap-2.5">
                <Globe className="w-6 h-6 text-[#87cf3e]" />
                <span>Select preferences</span>
              </h1>
              <p className="text-xs text-slate-400 mt-1.5 leading-relaxed">
                Choose the language to install, your time & currency location format, and your keyboard input method.
              </p>
            </div>

            <div className="flex flex-col gap-4">
              {/* Language to install */}
              <div className="flex flex-col gap-1.5">
                <label className="text-xs font-semibold text-slate-300 flex items-center gap-1.5">
                  <Globe className="w-3.5 h-3.5 text-[#87cf3e]" />
                  <span>Language to install:</span>
                </label>
                <select
                  value={installerData.language}
                  onChange={(e) => handleLanguageChange(e.target.value)}
                  className="w-full px-3.5 py-2.5 bg-slate-900 border border-white/15 rounded-xl text-xs text-slate-100 focus:outline-none focus:ring-2 focus:ring-[#87cf3e] shadow-inner"
                >
                  {LANGUAGE_OPTIONS.map((opt) => (
                    <option key={opt.label} value={opt.label}>
                      {opt.label}
                    </option>
                  ))}
                </select>
              </div>

              {/* Time and currency format (Location) */}
              <div className="flex flex-col gap-1.5">
                <label className="text-xs font-semibold text-slate-300 flex items-center gap-1.5">
                  <MapPin className="w-3.5 h-3.5 text-[#87cf3e]" />
                  <span>Time and currency format (Location):</span>
                </label>
                <select
                  value={installerData.location || LOCATION_OPTIONS[0].label}
                  onChange={(e) => handleLocationChange(e.target.value)}
                  className="w-full px-3.5 py-2.5 bg-slate-900 border border-white/15 rounded-xl text-xs text-slate-100 focus:outline-none focus:ring-2 focus:ring-[#87cf3e] shadow-inner"
                >
                  {LOCATION_OPTIONS.map((opt) => (
                    <option key={opt.label} value={opt.label}>
                      {opt.label}
                    </option>
                  ))}
                </select>
              </div>

              {/* Keyboard or input method */}
              <div className="flex flex-col gap-1.5">
                <label className="text-xs font-semibold text-slate-300 flex items-center gap-1.5">
                  <Keyboard className="w-3.5 h-3.5 text-[#87cf3e]" />
                  <span>Keyboard or input method:</span>
                </label>
                <select
                  value={installerData.keyboardLayout}
                  onChange={(e) => handleKeyboardChange(e.target.value)}
                  className="w-full px-3.5 py-2.5 bg-slate-900 border border-white/15 rounded-xl text-xs text-slate-100 focus:outline-none focus:ring-2 focus:ring-[#87cf3e] shadow-inner"
                >
                  {KEYBOARD_OPTIONS.map((opt) => (
                    <option key={opt.label} value={opt.label}>
                      {opt.label}
                    </option>
                  ))}
                </select>
              </div>
            </div>

            {/* Secure Boot & UEFI compatibility note */}
            <div className="p-3.5 rounded-2xl bg-slate-900/60 border border-white/10 flex items-start gap-3">
              <ShieldCheck className="w-5 h-5 text-emerald-400 shrink-0 mt-0.5" />
              <div className="text-[11px] text-slate-300 leading-normal">
                <span className="font-semibold text-white">Microsoft UEFI CA Signed Bootloader: </span>
                AxisOS is certified for modern UEFI firmware with Secure Boot enabled. No BIOS certificates or keys need to be disabled.
              </div>
            </div>
          </div>
        )}

        {/* ============================================================ */}
        {/* STEP 1: "Where do you want to install AxisOS?" (Windows Table) */}
        {/* ============================================================ */}
        {currentStep === 1 && (
          <div className="w-full max-w-2xl flex flex-col gap-5 animate-in fade-in duration-200">
            <div className="flex items-center justify-between border-b border-white/10 pb-4">
              <div>
                <h1 className="text-2xl font-bold tracking-tight text-white flex items-center gap-2.5">
                  <HardDrive className="w-6 h-6 text-[#87cf3e]" />
                  <span>Where do you want to install AxisOS?</span>
                </h1>
                <p className="text-xs text-slate-400 mt-1">
                  Select a drive from the list below. AxisOS will automatically create the UEFI partition and Btrfs filesystem.
                </p>
              </div>
              <button
                onClick={refreshDisks}
                className="px-3 py-1.5 rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-200 transition-colors flex items-center gap-1.5 text-xs font-medium cursor-pointer border border-white/10 shadow-sm"
                title="Refresh connected disks"
              >
                <RefreshCw className="w-3.5 h-3.5" />
                <span>Refresh</span>
              </button>
            </div>

            {/* Windows Setup Style Drives Table */}
            <div className="rounded-2xl border border-white/10 bg-slate-900/80 overflow-hidden shadow-lg">
              <table className="w-full text-left text-xs border-collapse">
                <thead>
                  <tr className="bg-slate-800/80 text-slate-400 font-semibold border-b border-white/10">
                    <th className="py-2.5 px-4 w-12 text-center">Select</th>
                    <th className="py-2.5 px-3">Name / Drive</th>
                    <th className="py-2.5 px-3">Capacity</th>
                    <th className="py-2.5 px-3">Type</th>
                    <th className="py-2.5 px-4">Status</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-white/5">
                  {availableDisks.length === 0 ? (
                    <tr>
                      <td colSpan={5} className="py-8 text-center text-slate-400">
                        <RefreshCw className="w-5 h-5 animate-spin mx-auto mb-2 text-[#87cf3e]" />
                        Scanning connected disks...
                      </td>
                    </tr>
                  ) : (
                    availableDisks.map((disk, idx) => {
                      const isSelected = installerData.targetDisk === disk.id;
                      const isLive = disk.isLiveMedium;
                      const hasWin = disk.hasWindows || disk.hasBitLocker;

                      return (
                        <tr
                          key={disk.id}
                          onClick={() => {
                            if (!isLive) {
                              updateInstallerData({ targetDisk: disk.id });
                            }
                          }}
                          className={`transition-colors cursor-pointer ${
                            isLive
                              ? 'opacity-50 bg-slate-950 cursor-not-allowed'
                              : isSelected
                              ? 'bg-[#87cf3e]/20 text-white font-medium ring-1 ring-inset ring-[#87cf3e]'
                              : 'hover:bg-white/5 text-slate-300'
                          }`}
                        >
                          {/* Radio Selection */}
                          <td className="py-3 px-4 text-center">
                            <input
                              type="radio"
                              name="targetDisk"
                              checked={isSelected}
                              disabled={isLive}
                              onChange={() => !isLive && updateInstallerData({ targetDisk: disk.id })}
                              className="accent-[#87cf3e] cursor-pointer"
                            />
                          </td>

                          {/* Drive Name / Model */}
                          <td className="py-3 px-3">
                            <div className="flex items-center gap-2">
                              <span className="font-semibold text-slate-100">
                                Drive {idx}: {disk.model || disk.name}
                              </span>
                              <span className="text-[10px] text-slate-500 font-mono">
                                ({disk.id})
                              </span>
                            </div>
                          </td>

                          {/* Capacity */}
                          <td className="py-3 px-3 font-mono font-medium text-slate-200">
                            {disk.size}
                          </td>

                          {/* Drive Type */}
                          <td className="py-3 px-3">
                            <span className="px-2 py-0.5 rounded-md text-[10px] uppercase font-bold bg-slate-800 text-slate-300 border border-white/10">
                              {disk.diskType || 'Drive'}
                            </span>
                          </td>

                          {/* Status */}
                          <td className="py-3 px-4">
                            {isLive ? (
                              <span className="inline-flex items-center gap-1 text-[11px] text-amber-400 font-semibold">
                                <Disc className="w-3.5 h-3.5" />
                                <span>Live USB (Protected)</span>
                              </span>
                            ) : hasWin ? (
                              <span className="inline-flex items-center gap-1 text-[11px] text-amber-400 font-semibold">
                                <AlertTriangle className="w-3.5 h-3.5" />
                                <span>{disk.hasBitLocker ? 'BitLocker Windows' : 'Windows OS'}</span>
                              </span>
                            ) : (
                              <span className="inline-flex items-center gap-1 text-[11px] text-emerald-400 font-medium">
                                <Check className="w-3.5 h-3.5" />
                                <span>Ready</span>
                              </span>
                            )}
                          </td>
                        </tr>
                      );
                    })
                  )}
                </tbody>
              </table>
            </div>

            {/* Warning if Windows or BitLocker is on Selected Drive */}
            {selectedDisk && (selectedDisk.hasWindows || selectedDisk.hasBitLocker) && (
              <div className="p-3.5 rounded-2xl bg-amber-500/10 border border-amber-500/30 flex items-start gap-3 text-amber-300 text-xs">
                <AlertTriangle className="w-5 h-5 text-amber-400 shrink-0 mt-0.5" />
                <div className="leading-relaxed">
                  <span className="font-bold text-amber-200">Caution: Existing Windows Installation Detected! </span>
                  Targeting <strong>{selectedDisk.model || selectedDisk.name}</strong> will erase and replace this drive. If you want to keep Windows, ensure you install AxisOS to a separate drive or backup your files first.
                </div>
              </div>
            )}

            {/* Visual Partition Layout Preview */}
            <div className="p-4 rounded-2xl bg-slate-900/60 border border-white/10 flex flex-col gap-2.5">
              <div className="text-xs font-semibold text-slate-300 flex items-center justify-between">
                <span>Automatic Partition Layout Preview:</span>
                <span className="text-[11px] text-[#87cf3e] font-mono">Btrfs Subvolumes + UEFI ESP</span>
              </div>
              <div className="grid grid-cols-12 gap-1.5 text-center text-[10px] font-mono font-medium">
                <div className="col-span-2 p-2 rounded-lg bg-emerald-500/20 text-emerald-300 border border-emerald-500/30 flex flex-col justify-center">
                  <span className="font-bold">ESP (512 MB)</span>
                  <span className="text-[9px] opacity-80">FAT32 /boot/efi</span>
                </div>
                <div className="col-span-2 p-2 rounded-lg bg-amber-500/20 text-amber-300 border border-amber-500/30 flex flex-col justify-center">
                  <span className="font-bold">Swap (4.0 GB)</span>
                  <span className="text-[9px] opacity-80">Linux Swap</span>
                </div>
                <div className="col-span-8 p-2 rounded-lg bg-[#87cf3e]/20 text-[#87cf3e] border border-[#87cf3e]/30 flex flex-col justify-center">
                  <span className="font-bold">AxisOS Root (Remaining Space)</span>
                  <span className="text-[9px] opacity-80">Btrfs @root, @home (zstd)</span>
                </div>
              </div>
            </div>
          </div>
        )}

        {/* ============================================================ */}
        {/* STEP 2: "Who's going to use this PC?" (User Account) */}
        {/* ============================================================ */}
        {currentStep === 2 && (
          <div className="w-full max-w-lg flex flex-col gap-5 animate-in fade-in duration-200">
            <div className="border-b border-white/10 pb-4">
              <h1 className="text-2xl font-bold tracking-tight text-white flex items-center gap-2.5">
                <User className="w-6 h-6 text-[#87cf3e]" />
                <span>Who's going to use this PC?</span>
              </h1>
              <p className="text-xs text-slate-400 mt-1">
                Enter your name and create a password to set up your primary user account.
              </p>
            </div>

            <div className="flex flex-col gap-3.5">
              {/* Full Name */}
              <div className="flex flex-col gap-1">
                <label className="text-xs font-semibold text-slate-300">Your Full Name</label>
                <input
                  type="text"
                  value={installerData.userFullName}
                  onChange={(e) => updateInstallerData({ userFullName: e.target.value })}
                  placeholder="e.g. Jacky Phuti"
                  className="w-full px-3.5 py-2.5 bg-slate-900 border border-white/15 rounded-xl text-xs text-slate-100 focus:outline-none focus:ring-2 focus:ring-[#87cf3e]"
                />
              </div>

              {/* Username & PC Name Row */}
              <div className="grid grid-cols-2 gap-3">
                <div className="flex flex-col gap-1">
                  <label className="text-xs font-semibold text-slate-300">User Account Name</label>
                  <input
                    type="text"
                    value={installerData.username}
                    onChange={(e) => updateInstallerData({ username: e.target.value.toLowerCase().replace(/[^a-z0-9_-]/g, '') })}
                    placeholder="axis"
                    className="w-full px-3.5 py-2.5 bg-slate-900 border border-white/15 rounded-xl text-xs text-slate-100 focus:outline-none focus:ring-2 focus:ring-[#87cf3e] font-mono"
                  />
                </div>
                <div className="flex flex-col gap-1">
                  <label className="text-xs font-semibold text-slate-300">PC / Device Name</label>
                  <input
                    type="text"
                    value={installerData.computerName}
                    onChange={(e) => updateInstallerData({ computerName: e.target.value })}
                    placeholder="axis-pc"
                    className="w-full px-3.5 py-2.5 bg-slate-900 border border-white/15 rounded-xl text-xs text-slate-100 focus:outline-none focus:ring-2 focus:ring-[#87cf3e] font-mono"
                  />
                </div>
              </div>

              {/* Password & Confirm */}
              <div className="grid grid-cols-2 gap-3">
                <div className="flex flex-col gap-1">
                  <label className="text-xs font-semibold text-slate-300">Password</label>
                  <div className="relative">
                    <input
                      type={showPassword ? 'text' : 'password'}
                      value={installerData.password}
                      onChange={(e) => updateInstallerData({ password: e.target.value })}
                      placeholder="Enter password"
                      className="w-full px-3.5 py-2.5 bg-slate-900 border border-white/15 rounded-xl text-xs text-slate-100 focus:outline-none focus:ring-2 focus:ring-[#87cf3e] pr-9"
                    />
                    <button
                      type="button"
                      onClick={() => setShowPassword(!showPassword)}
                      className="absolute right-2.5 top-2.5 text-slate-400 hover:text-white"
                    >
                      {showPassword ? <EyeOff className="w-4 h-4" /> : <Eye className="w-4 h-4" />}
                    </button>
                  </div>
                </div>

                <div className="flex flex-col gap-1">
                  <label className="text-xs font-semibold text-slate-300">Confirm Password</label>
                  <input
                    type={showPassword ? 'text' : 'password'}
                    value={confirmPassword}
                    onChange={(e) => setConfirmPassword(e.target.value)}
                    placeholder="Repeat password"
                    className={`w-full px-3.5 py-2.5 bg-slate-900 border rounded-xl text-xs text-slate-100 focus:outline-none focus:ring-2 focus:ring-[#87cf3e] ${
                      confirmPassword && !passwordsMatch ? 'border-rose-500' : 'border-white/15'
                    }`}
                  />
                </div>
              </div>

              {confirmPassword && !passwordsMatch && (
                <div className="text-[11px] text-rose-400">Passwords do not match. Please verify.</div>
              )}

              {/* Auto-login Toggle */}
              <label className="flex items-center gap-3 p-3 rounded-xl bg-slate-900/60 border border-white/10 cursor-pointer hover:bg-white/5 transition-colors mt-1">
                <input
                  type="checkbox"
                  checked={installerData.autoLogin}
                  onChange={(e) => updateInstallerData({ autoLogin: e.target.checked })}
                  className="accent-[#87cf3e] w-4 h-4 rounded cursor-pointer"
                />
                <span className="text-xs text-slate-200">
                  Sign in automatically without asking for a password on startup
                </span>
              </label>
            </div>
          </div>
        )}

        {/* ============================================================ */}
        {/* STEP 3: "Installing AxisOS" (Windows Setup Progress Screen)  */}
        {/* ============================================================ */}
        {currentStep === 3 && (
          <div className="w-full max-w-xl flex flex-col gap-6 animate-in fade-in duration-200">
            <div className="border-b border-white/10 pb-4 text-center">
              <h1 className="text-3xl font-black tracking-tight text-white">
                Installing AxisOS
              </h1>
              <p className="text-xs text-slate-400 mt-2">
                Your computer will restart once the installation completes. Please keep your PC plugged in.
              </p>
            </div>

            {/* Windows Setup Progress Checklist */}
            <div className="flex flex-col gap-3 py-2 px-6 rounded-2xl bg-slate-900/60 border border-white/10">
              {[
                { title: 'Copying AxisOS system files', minPct: 15 },
                { title: 'Getting files ready for installation', minPct: 45 },
                { title: 'Installing system drivers & hardware firmware', minPct: 75 },
                { title: 'Installing Microsoft-signed UEFI Secure Bootloader', minPct: 90 },
                { title: 'Registering Lenovo / HP / Dell BIOS entries', minPct: 98 },
                { title: 'Finishing up', minPct: 100 },
              ].map((item, idx) => {
                const isDone = installProgress >= item.minPct;
                const isCurrent = !isDone && (idx === 0 || installProgress >= [15, 45, 75, 90, 98, 100][idx - 1]);

                return (
                  <div key={item.title} className="flex items-center gap-3 text-xs">
                    <div className="w-5 h-5 flex items-center justify-center shrink-0">
                      {isDone ? (
                        <Check className="w-4 h-4 text-emerald-400" />
                      ) : isCurrent ? (
                        <RefreshCw className="w-3.5 h-3.5 animate-spin text-[#87cf3e]" />
                      ) : (
                        <div className="w-1.5 h-1.5 rounded-full bg-slate-700" />
                      )}
                    </div>
                    <span
                      className={`${
                        isDone
                          ? 'text-slate-300'
                          : isCurrent
                          ? 'text-white font-semibold'
                          : 'text-slate-600'
                      }`}
                    >
                      {item.title} {isCurrent ? `(${installProgress}%)` : ''}
                    </span>
                  </div>
                );
              })}
            </div>

            {/* Animated Progress Bar */}
            <div className="flex flex-col gap-2">
              <div className="h-3 w-full bg-slate-900 rounded-full overflow-hidden p-0.5 border border-white/10">
                <div
                  className="h-full bg-gradient-to-r from-emerald-600 via-[#87cf3e] to-[#a3e635] rounded-full transition-all duration-300 shadow-lg shadow-[#87cf3e]/30"
                  style={{ width: `${Math.max(installProgress, 4)}%` }}
                />
              </div>
              <div className="flex items-center justify-between text-xs text-slate-400 px-1">
                <span className="truncate max-w-sm">{installStatusText}</span>
                <span className="font-mono font-bold text-white">{installProgress}%</span>
              </div>
            </div>

            {/* Error Banner */}
            {installError && (
              <div className="p-4 rounded-2xl bg-rose-500/10 border border-rose-500/30 flex items-start gap-3 text-rose-300 text-xs">
                <AlertTriangle className="w-5 h-5 text-rose-400 shrink-0 mt-0.5" />
                <div>
                  <div className="font-bold text-rose-200">Installation Error:</div>
                  <div className="mt-1">{installError}</div>
                  <button
                    onClick={resetInstaller}
                    className="mt-3 px-3 py-1.5 rounded-lg bg-rose-600 text-white font-semibold text-xs hover:bg-rose-500 transition-colors cursor-pointer"
                  >
                    Try Again
                  </button>
                </div>
              </div>
            )}

            {/* Expandable Live Log Console */}
            <div className="border border-white/10 rounded-2xl overflow-hidden bg-slate-950">
              <button
                onClick={() => setShowLogConsole(!showLogConsole)}
                className="w-full px-4 py-2 bg-slate-900/60 hover:bg-slate-900 text-slate-400 hover:text-white flex items-center justify-between text-xs transition-colors"
              >
                <div className="flex items-center gap-2">
                  <Terminal className="w-3.5 h-3.5 text-[#87cf3e]" />
                  <span>Installation Log Output</span>
                </div>
                <span className="text-[10px] text-slate-500">
                  {showLogConsole ? 'Hide' : 'Show details'}
                </span>
              </button>

              {showLogConsole && (
                <div
                  ref={logContainerRef}
                  className="p-3 font-mono text-[11px] text-slate-300 max-h-40 overflow-y-auto space-y-1 bg-black/60"
                >
                  {installLogs.map((log, index) => (
                    <div key={index} className="leading-tight text-slate-400">
                      {log}
                    </div>
                  ))}
                </div>
              )}
            </div>
          </div>
        )}

        {/* ============================================================ */}
        {/* STEP 4: "Installation Complete!" (Windows Completion Screen)  */}
        {/* ============================================================ */}
        {currentStep === 4 && (
          <div className="w-full max-w-lg flex flex-col items-center text-center gap-6 animate-in zoom-in-95 duration-300">
            <div className="w-20 h-20 rounded-3xl bg-gradient-to-tr from-[#1b2b20] to-[#87cf3e] flex items-center justify-center text-black shadow-2xl shadow-[#87cf3e]/30 ring-4 ring-[#87cf3e]/20">
              <CheckCircle2 className="w-10 h-10" />
            </div>

            <div>
              <h1 className="text-3xl font-black tracking-tight text-white">
                Installation Complete!
              </h1>
              <p className="text-xs text-slate-400 mt-2 max-w-md leading-relaxed">
                AxisOS has been successfully installed on{' '}
                <strong className="text-slate-200">
                  {selectedDisk ? selectedDisk.model || selectedDisk.id : 'your drive'}
                </strong>
                . Remove your USB flash drive, then restart your PC to boot into AxisOS.
              </p>
            </div>

            <div className="p-4 rounded-2xl bg-amber-500/10 border border-amber-500/30 flex items-start gap-3 text-amber-200 text-xs w-full text-left">
              <AlertTriangle className="w-5 h-5 text-amber-400 shrink-0 mt-0.5" />
              <div>
                <div className="font-bold text-amber-300">Important: Unplug your USB Flash Drive</div>
                <div className="mt-1 text-slate-300 leading-relaxed">
                  Please remove the USB flash drive now. This ensures your computer will boot directly from your drive into your new AxisOS system instead of restarting the installer.
                </div>
              </div>
            </div>

            <div className="p-4 rounded-2xl bg-slate-900/60 border border-white/10 w-full text-left space-y-2 text-xs text-slate-300">
              <div className="flex items-center gap-2">
                <Check className="w-4 h-4 text-[#87cf3e]" />
                <span>Microsoft UEFI CA Signed Bootloader Installed</span>
              </div>
              <div className="flex items-center gap-2">
                <Check className="w-4 h-4 text-[#87cf3e]" />
                <span>Lenovo / HP Universal UEFI Fallback Configured</span>
              </div>
              <div className="flex items-center gap-2">
                <Check className="w-4 h-4 text-[#87cf3e]" />
                <span>Btrfs zstd Transparent Compression Enabled</span>
              </div>
            </div>

            {/* Action Buttons */}
            <div className="flex flex-col sm:flex-row gap-3 w-full">
              <button
                onClick={() => handleFinish('restart')}
                className="flex-1 px-5 py-3 rounded-2xl bg-[#87cf3e] hover:bg-[#76bb33] text-black font-bold text-xs shadow-lg shadow-[#87cf3e]/30 transition-all flex items-center justify-center gap-2 cursor-pointer"
              >
                <RotateCcw className="w-4 h-4" />
                <span>Restart Computer Now</span>
              </button>
              <button
                onClick={() => handleFinish('continue')}
                className="px-5 py-3 rounded-2xl bg-slate-900 hover:bg-slate-800 text-slate-300 font-semibold text-xs border border-white/10 transition-colors cursor-pointer"
              >
                Continue Testing Live Desktop
              </button>
            </div>
          </div>
        )}
      </div>

      {/* Windows Setup Bottom Navigation Bar */}
      {currentStep < 3 && (
        <div className="h-16 bg-slate-900 border-t border-white/10 px-8 flex items-center justify-between shrink-0">
          <div>
            {currentStep === 0 ? (
              <button
                onClick={() => {
                  const win = windows.find((w) => w.appId === 'installer');
                  if (win) closeWindow(win.id);
                }}
                className="px-4 py-2 rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-300 hover:text-white transition-colors text-xs font-semibold cursor-pointer border border-white/10"
              >
                Try AxisOS (Live Desktop)
              </button>
            ) : (
              <button
                onClick={goToPrevStep}
                className="px-4 py-2 rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-300 hover:text-white transition-colors text-xs font-semibold flex items-center gap-1.5 cursor-pointer border border-white/10"
              >
                <ArrowLeft className="w-3.5 h-3.5" />
                <span>Back</span>
              </button>
            )}
          </div>

          <div>
            {currentStep === 0 && (
              <button
                onClick={goToNextStep}
                className="px-6 py-2 rounded-xl bg-[#87cf3e] hover:bg-[#76bb33] text-black font-bold text-xs transition-colors flex items-center gap-1.5 cursor-pointer shadow-md shadow-[#87cf3e]/30"
              >
                <span>Next</span>
                <ArrowRight className="w-3.5 h-3.5" />
              </button>
            )}

            {currentStep === 1 && (
              <button
                disabled={!installerData.targetDisk || !selectedDisk || selectedDisk.isLiveMedium}
                onClick={goToNextStep}
                className={`px-6 py-2 rounded-xl font-bold text-xs transition-colors flex items-center gap-1.5 ${
                  !installerData.targetDisk || !selectedDisk || selectedDisk.isLiveMedium
                    ? 'bg-slate-800 text-slate-500 cursor-not-allowed'
                    : 'bg-[#87cf3e] hover:bg-[#76bb33] text-black cursor-pointer shadow-md shadow-[#87cf3e]/30'
                }`}
              >
                <span>Next</span>
                <ArrowRight className="w-3.5 h-3.5" />
              </button>
            )}

            {currentStep === 2 && (
              <button
                disabled={!passwordsMatch || !installerData.username}
                onClick={() => setConfirmModalOpen(true)}
                className={`px-6 py-2 rounded-xl font-bold text-xs transition-colors flex items-center gap-1.5 ${
                  !passwordsMatch || !installerData.username
                    ? 'bg-slate-800 text-slate-500 cursor-not-allowed'
                    : 'bg-[#87cf3e] hover:bg-[#76bb33] text-black cursor-pointer shadow-lg shadow-[#87cf3e]/30'
                }`}
              >
                <span>Install Now</span>
                <Sparkles className="w-3.5 h-3.5" />
              </button>
            )}
          </div>
        </div>
      )}

      {/* Safety Confirmation Modal Before Partitioning */}
      {confirmModalOpen && (
        <div className="fixed inset-0 z-50 flex items-center justify-center bg-black/70 backdrop-blur-md animate-in fade-in duration-200 p-4">
          <div className="w-full max-w-md rounded-3xl bg-slate-900 border border-white/20 p-6 shadow-2xl flex flex-col gap-4 text-center">
            <div className="w-12 h-12 rounded-2xl bg-amber-500/20 border border-amber-500/30 text-amber-400 flex items-center justify-center mx-auto">
              <AlertTriangle className="w-6 h-6" />
            </div>

            <div>
              <h3 className="text-lg font-bold text-white">Ready to Install AxisOS?</h3>
              <p className="text-xs text-slate-400 mt-1 leading-relaxed">
                The target drive{' '}
                <strong className="text-amber-300">
                  {selectedDisk ? `${selectedDisk.model || selectedDisk.name} (${selectedDisk.id})` : ''}
                </strong>{' '}
                will be formatted. All partitions on this disk will be replaced with AxisOS Btrfs and UEFI bootloaders.
              </p>
            </div>

            <div className="flex gap-3 mt-2">
              <button
                onClick={() => setConfirmModalOpen(false)}
                className="flex-1 py-2.5 rounded-xl bg-slate-800 hover:bg-slate-700 text-slate-300 font-medium text-xs transition-colors cursor-pointer border border-white/10"
              >
                Cancel
              </button>
              <button
                onClick={() => {
                  setConfirmModalOpen(false);
                  startInstallation();
                }}
                className="flex-1 py-2.5 rounded-xl bg-[#87cf3e] hover:bg-[#76bb33] text-black font-bold text-xs transition-colors cursor-pointer shadow-md shadow-[#87cf3e]/30"
              >
                Confirm & Install
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
};
