import React, { useState } from 'react';
import { Save, FileText, Check, FolderOpen } from 'lucide-react';
import { systemService } from '../../services/systemService';
import { useSystemState } from '../../context/SystemStateContext';

export const TextEditorApp: React.FC<{ params?: Record<string, any> }> = ({ params }) => {
  const { systemInfo } = useSystemState();
  const defaultDir = systemInfo.homeDir || `/home/${systemInfo.username || 'axis'}`;
  const [filePath, setFilePath] = useState<string>(params?.filePath || `${defaultDir}/welcome.txt`);
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

  React.useEffect(() => {
    if (params?.filePath) {
      setFilePath(params.filePath);
      systemService.readFile(params.filePath).then((text) => {
        if (text) setContent(text);
      });
    }
  }, [params?.filePath]);
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

  const handleOpen = async () => {
    const target = prompt('Enter full path of file to open:', filePath);
    if (!target) return;
    try {
      const data = await systemService.readFileContent(target);
      setFilePath(target);
      setContent(data);
      setIsSaved(true);
    } catch (err: any) {
      alert(`Could not open file: ${err.message}`);
    }
  };

  return (
    <div className="flex flex-col h-full w-full bg-[#0a0e0b] text-slate-100 select-none">
      {/* Top toolbar */}
      <div className="h-9 px-3 flex items-center justify-between bg-[#121814] border-b border-white/10 text-xs">
        <div className="flex items-center space-x-2">
          <FileText className="w-4 h-4 text-[#87cf3e]" />
          <input
            type="text"
            value={filePath}
            onChange={(e) => {
              setFilePath(e.target.value);
              setIsSaved(false);
            }}
            className="bg-transparent border-b border-white/15 px-1 py-0.5 font-mono text-xs text-white outline-none w-72 focus:border-[#87cf3e]"
            title="File path"
          />
          {!isSaved && <span className="text-[10px] text-amber-400 font-mono">• Modified</span>}
        </div>

        <div className="flex items-center space-x-2">
          <button
            onClick={handleOpen}
            className="flex items-center space-x-1.5 px-2.5 py-1 rounded-lg bg-[#18221b] hover:bg-[#202e24] text-slate-200 border border-white/10 hover:border-[#87cf3e]/40 font-medium transition-colors cursor-pointer shadow-xs"
            title="Open existing file from disk"
          >
            <FolderOpen className="w-3.5 h-3.5 text-[#87cf3e]" />
            <span>Open</span>
          </button>

          <button
            onClick={handleSave}
            disabled={isSaving}
            className="flex items-center space-x-1.5 px-2.5 py-1 rounded-lg bg-[#18221b] hover:bg-[#202e24] text-slate-200 border border-white/10 hover:border-[#87cf3e]/40 font-medium transition-colors cursor-pointer shadow-xs"
          >
            {isSaved ? <Check className="w-3.5 h-3.5 text-[#87cf3e]" /> : <Save className="w-3.5 h-3.5 text-[#87cf3e]" />}
            <span>{isSaving ? 'Saving...' : (isSaved ? 'Saved' : 'Save')}</span>
          </button>
        </div>
      </div>

      {/* Editor Body */}
      <textarea
        value={content}
        onChange={(e) => {
          setContent(e.target.value);
          setIsSaved(false);
        }}
        className="flex-1 p-4 bg-transparent border-none outline-none font-mono text-xs text-slate-100 leading-relaxed resize-none cursor-text select-text"
        placeholder="Type here..."
      />

      {/* Status Bar */}
      <div className="h-6 px-3 bg-[#121814] border-t border-white/10 flex items-center justify-between text-[10px] text-slate-400 font-mono">
        <span>UTF-8 Plaintext</span>
        <span>
          {lines} lines, {chars} characters
        </span>
      </div>
    </div>
  );
};
