import React, { useState } from 'react';
import { Save, FileText, Check, FolderOpen } from 'lucide-react';
import { systemService } from '../../services/systemService';
import { useSystemState } from '../../context/SystemStateContext';

export const TextEditorApp: React.FC = () => {
  const { systemInfo } = useSystemState();
  const defaultDir = systemInfo.homeDir || `/home/${systemInfo.username || 'axis'}`;
  const [filePath, setFilePath] = useState<string>(`${defaultDir}/welcome.txt`);
  const [content, setContent] = useState<string>(
`# Welcome to AxisOS
# ===================
# This is a live file on your operating system.
# AxisOS pairs the Linux kernel with a hardware-accelerated Wayland desktop shell.

def init_axis_kernel():
    print("Virtual filesystems mounted (/proc, /sys, /dev)...")
    print("systemd init and Cage Wayland compositor active.")
    print("Welcome to AxisOS!")

init_axis_kernel()
`
  );
  const [isSaved, setIsSaved] = useState(true);
  const [isSaving, setIsSaving] = useState(false);

  const lines = content.split('\n').length;
  const chars = content.length;

  const handleSave = async () => {
    setIsSaving(true);
    try {
      const ok = await systemService.writeFile(filePath, content);
      if (ok) {
        setIsSaved(true);
      }
    } catch {}
    setIsSaving(false);
  };

  return (
    <div className="flex flex-col h-full w-full bg-slate-950 text-slate-100 select-none">
      {/* Top toolbar */}
      <div className="h-9 px-3 flex items-center justify-between bg-slate-900 border-b border-white/5 text-xs">
        <div className="flex items-center space-x-2">
          <FileText className="w-4 h-4 text-cyan-400" />
          <input
            type="text"
            value={filePath}
            onChange={(e) => {
              setFilePath(e.target.value);
              setIsSaved(false);
            }}
            className="bg-transparent border-b border-white/10 px-1 py-0.5 font-mono text-xs text-slate-200 outline-none w-72 focus:border-cyan-500"
            title="File path"
          />
          {!isSaved && <span className="text-[10px] text-amber-400 font-mono">• Modified</span>}
        </div>

        <button
          onClick={handleSave}
          disabled={isSaving}
          className="flex items-center space-x-1.5 px-2.5 py-1 rounded bg-white/5 hover:bg-white/10 text-slate-200 font-medium transition-colors"
        >
          {isSaved ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Save className="w-3.5 h-3.5 text-cyan-400" />}
          <span>{isSaving ? 'Saving...' : (isSaved ? 'Saved' : 'Save')}</span>
        </button>
      </div>

      {/* Editor Body */}
      <textarea
        value={content}
        onChange={(e) => {
          setContent(e.target.value);
          setIsSaved(false);
        }}
        className="flex-1 p-4 bg-transparent border-none outline-none font-mono text-xs text-slate-200 leading-relaxed resize-none cursor-text select-text"
        placeholder="Type here..."
      />

      {/* Status Bar */}
      <div className="h-6 px-3 bg-slate-900/80 border-t border-white/5 flex items-center justify-between text-[10px] text-slate-400 font-mono">
        <span>UTF-8 Plaintext</span>
        <span>
          {lines} lines, {chars} characters
        </span>
      </div>
    </div>
  );
};
