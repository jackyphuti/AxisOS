import React, { useState } from 'react';
import { Save, FileText, Check } from 'lucide-react';

export const TextEditorApp: React.FC = () => {
  const [content, setContent] = useState<string>(
`# Welcome to AxisOS
# ===================
# This is a sample text file created on your new operating system.
# AxisOS is built directly on the Linux kernel with a modern Wayland shell.

def init_axis_kernel():
    print("Mounting virtual filesystems (/proc, /sys, /dev)...")
    print("Starting systemd init and user session...")
    print("Welcome to Horizon!")

init_axis_kernel()
`
  );
  const [isSaved, setIsSaved] = useState(true);

  const lines = content.split('\n').length;
  const chars = content.length;

  const handleSave = () => {
    setIsSaved(true);
    setTimeout(() => {}, 2000);
  };

  return (
    <div className="flex flex-col h-full w-full bg-slate-950 text-slate-100 select-none">
      {/* Top toolbar */}
      <div className="h-9 px-3 flex items-center justify-between bg-slate-900 border-b border-white/5 text-xs">
        <div className="flex items-center space-x-2">
          <FileText className="w-4 h-4 text-cyan-400" />
          <span className="font-semibold text-slate-200">welcome.py</span>
          {!isSaved && <span className="text-[10px] text-amber-400 font-mono">• Modified</span>}
        </div>

        <button
          onClick={handleSave}
          className="flex items-center space-x-1.5 px-2.5 py-1 rounded bg-white/5 hover:bg-white/10 text-slate-200 font-medium transition-colors"
        >
          {isSaved ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Save className="w-3.5 h-3.5 text-cyan-400" />}
          <span>{isSaved ? 'Saved' : 'Save'}</span>
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
        <span>Python • UTF-8</span>
        <span>
          {lines} lines, {chars} characters
        </span>
      </div>
    </div>
  );
};
