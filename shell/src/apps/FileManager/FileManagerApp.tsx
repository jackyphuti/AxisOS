import React, { useState, useEffect } from 'react';
import {
  Folder,
  FileText,
  Image as ImageIcon,
  HardDrive,
  Home,
  Download,
  LayoutGrid,
  List,
  Search,
  ArrowLeft,
  ArrowRight,
  FolderPlus,
  Trash2,
  RefreshCw,
} from 'lucide-react';
import { systemService, FileEntry } from '../../services/systemService';
import { useSystemState } from '../../context/SystemStateContext';
import { useWindowManager } from '../../context/WindowManagerContext';

export const FileManagerApp: React.FC = () => {
  const { systemInfo } = useSystemState();
  const { openApp } = useWindowManager();

  const userHome = systemInfo.homeDir || `/home/${systemInfo.username || 'axis'}`;
  const [currentPath, setCurrentPath] = useState<string>(userHome);
  const [items, setItems] = useState<FileEntry[]>([]);
  const [history, setHistory] = useState<string[]>([userHome]);
  const [histIdx, setHistIdx] = useState(0);
  const [viewMode, setViewMode] = useState<'grid' | 'list'>('grid');
  const [filterQuery, setFilterQuery] = useState('');
  const [selectedFile, setSelectedFile] = useState<FileEntry | null>(null);
  const [isLoading, setIsLoading] = useState(false);

  const loadDirectory = async (targetPath: string) => {
    setIsLoading(true);
    try {
      const res = await systemService.readDirectory(targetPath);
      setItems(res.items || []);
      setSelectedFile(null);
    } catch {}
    setIsLoading(false);
  };

  useEffect(() => {
    loadDirectory(currentPath);
  }, [currentPath]);

  const navigateTo = (newPath: string) => {
    setCurrentPath(newPath);
    const newHist = history.slice(0, histIdx + 1);
    newHist.push(newPath);
    setHistory(newHist);
    setHistIdx(newHist.length - 1);
  };

  const handleBack = () => {
    if (histIdx > 0) {
      const prev = history[histIdx - 1];
      setHistIdx(histIdx - 1);
      setCurrentPath(prev);
    }
  };

  const handleForward = () => {
    if (histIdx < history.length - 1) {
      const next = history[histIdx + 1];
      setHistIdx(histIdx + 1);
      setCurrentPath(next);
    }
  };

  const handleCreateFolder = async () => {
    const folderName = prompt('Enter new folder name:', 'New Folder');
    if (!folderName) return;
    const target = `${currentPath}/${folderName}`.replace(/\/+/g, '/');
    await systemService.createDirectory(target);
    await loadDirectory(currentPath);
  };

  const handleDeleteItem = async () => {
    if (!selectedFile) return;
    if (confirm(`Are you sure you want to delete "${selectedFile.name}"?`)) {
      await systemService.deleteItem(selectedFile.fullPath);
      setSelectedFile(null);
      await loadDirectory(currentPath);
    }
  };

  const filteredItems = items.filter((item) =>
    item.name.toLowerCase().includes(filterQuery.toLowerCase())
  );

  return (
    <div className="flex h-full w-full bg-slate-950 text-slate-100 select-none">
      {/* Finder Left Sidebar */}
      <div className="w-52 bg-slate-900/80 border-r border-white/10 p-3 flex flex-col gap-1 text-xs">
        <div className="px-2 py-1 text-[11px] font-semibold text-slate-400">Favorites</div>

        <button
          onClick={() => navigateTo(userHome)}
          className={`flex items-center space-x-2.5 px-2.5 py-1.5 rounded-lg transition-colors text-left ${
            currentPath === userHome ? 'bg-blue-600 text-white font-medium shadow-sm' : 'text-slate-300 hover:bg-white/5'
          }`}
        >
          <Home className="w-4 h-4 text-blue-400" />
          <span>Home</span>
        </button>

        <button
          onClick={() => navigateTo(`${userHome}/Documents`)}
          className={`flex items-center space-x-2.5 px-2.5 py-1.5 rounded-lg transition-colors text-left ${
            currentPath.includes('Documents') ? 'bg-blue-600 text-white font-medium shadow-sm' : 'text-slate-300 hover:bg-white/5'
          }`}
        >
          <Folder className="w-4 h-4 text-sky-400" />
          <span>Documents</span>
        </button>

        <button
          onClick={() => navigateTo(`${userHome}/Downloads`)}
          className={`flex items-center space-x-2.5 px-2.5 py-1.5 rounded-lg transition-colors text-left ${
            currentPath.includes('Downloads') ? 'bg-blue-600 text-white font-medium shadow-sm' : 'text-slate-300 hover:bg-white/5'
          }`}
        >
          <Download className="w-4 h-4 text-indigo-400" />
          <span>Downloads</span>
        </button>

        <button
          onClick={() => navigateTo(`${userHome}/Pictures`)}
          className={`flex items-center space-x-2.5 px-2.5 py-1.5 rounded-lg transition-colors text-left ${
            currentPath.includes('Pictures') ? 'bg-blue-600 text-white font-medium shadow-sm' : 'text-slate-300 hover:bg-white/5'
          }`}
        >
          <ImageIcon className="w-4 h-4 text-purple-400" />
          <span>Pictures</span>
        </button>

        <div className="my-2 border-t border-white/5"></div>
        <div className="px-2 py-1 text-[11px] font-semibold text-slate-400">Locations</div>

        <button
          onClick={() => navigateTo('/')}
          className={`flex items-center space-x-2.5 px-2.5 py-1.5 rounded-lg transition-colors text-left ${
            currentPath === '/' ? 'bg-blue-600 text-white font-medium shadow-sm' : 'text-slate-300 hover:bg-white/5'
          }`}
        >
          <HardDrive className="w-4 h-4 text-slate-400" />
          <span>AxisOS Root (/)</span>
        </button>
      </div>

      {/* Main File View */}
      <div className="flex-1 flex flex-col bg-slate-950/70">
        {/* Finder Toolbar */}
        <div className="h-10 px-3.5 flex items-center justify-between border-b border-white/10 text-xs bg-slate-900/50">
          <div className="flex items-center space-x-2">
            <button
              onClick={handleBack}
              disabled={histIdx === 0}
              className={`p-1 rounded ${histIdx === 0 ? 'text-slate-600' : 'text-slate-300 hover:bg-white/10'}`}
              title="Back"
            >
              <ArrowLeft className="w-4 h-4" />
            </button>
            <button
              onClick={handleForward}
              disabled={histIdx >= history.length - 1}
              className={`p-1 rounded ${histIdx >= history.length - 1 ? 'text-slate-600' : 'text-slate-300 hover:bg-white/10'}`}
              title="Forward"
            >
              <ArrowRight className="w-4 h-4" />
            </button>
            <span className="font-semibold text-white ml-2 truncate max-w-xs">{currentPath}</span>
          </div>

          <div className="flex items-center space-x-2">
            {/* New Folder & Delete */}
            <button
              onClick={handleCreateFolder}
              className="p-1.5 rounded-lg bg-white/5 hover:bg-white/10 text-slate-300 flex items-center gap-1 text-[11px]"
              title="New Folder"
            >
              <FolderPlus className="w-3.5 h-3.5 text-cyan-400" />
              <span>New Folder</span>
            </button>

            {selectedFile && (
              <button
                onClick={handleDeleteItem}
                className="p-1.5 rounded-lg bg-rose-500/20 hover:bg-rose-500/30 text-rose-300 flex items-center gap-1 text-[11px]"
                title="Delete item"
              >
                <Trash2 className="w-3.5 h-3.5" />
                <span>Delete</span>
              </button>
            )}

            <button
              onClick={() => loadDirectory(currentPath)}
              className="p-1.5 rounded-lg bg-white/5 hover:bg-white/10 text-slate-300"
              title="Refresh"
            >
              <RefreshCw className={`w-3.5 h-3.5 ${isLoading ? 'animate-spin text-cyan-400' : ''}`} />
            </button>

            {/* View Mode Buttons */}
            <div className="flex items-center p-0.5 rounded-lg bg-white/5 border border-white/10">
              <button
                onClick={() => setViewMode('grid')}
                className={`p-1 rounded-md ${viewMode === 'grid' ? 'bg-white/20 text-white' : 'text-slate-400'}`}
              >
                <LayoutGrid className="w-3.5 h-3.5" />
              </button>
              <button
                onClick={() => setViewMode('list')}
                className={`p-1 rounded-md ${viewMode === 'list' ? 'bg-white/20 text-white' : 'text-slate-400'}`}
              >
                <List className="w-3.5 h-3.5" />
              </button>
            </div>

            {/* Filter Search */}
            <div className="relative">
              <Search className="w-3 h-3 absolute left-2 top-2 text-slate-400" />
              <input
                type="text"
                placeholder="Search"
                value={filterQuery}
                onChange={(e) => setFilterQuery(e.target.value)}
                className="pl-6 pr-2 py-1 bg-white/5 border border-white/10 rounded-lg text-[11px] text-slate-200 outline-none w-28 focus:w-36 transition-all"
              />
            </div>
          </div>
        </div>

        {/* File Container */}
        {viewMode === 'grid' ? (
          <div className="flex-1 p-5 grid grid-cols-3 sm:grid-cols-4 md:grid-cols-5 gap-4 overflow-y-auto content-start">
            {filteredItems.map((item) => (
              <div
                key={item.name}
                onClick={() => setSelectedFile(item)}
                onDoubleClick={() => {
                  if (item.type === 'folder') {
                    navigateTo(item.fullPath);
                  } else {
                    openApp('text-editor');
                  }
                }}
                className={`flex flex-col items-center p-3 rounded-xl cursor-pointer text-center group transition-all border ${
                  selectedFile?.name === item.name
                    ? 'bg-blue-600/30 border-blue-500 text-white'
                    : 'border-transparent hover:bg-white/5 text-slate-300'
                }`}
              >
                <div className="w-14 h-14 rounded-2xl flex items-center justify-center mb-1 group-hover:scale-105 transition-transform">
                  {item.type === 'folder' ? (
                    <Folder className="w-10 h-10 text-sky-400 fill-sky-400/30 drop-shadow" />
                  ) : item.name.endsWith('.png') || item.name.endsWith('.jpg') ? (
                    <ImageIcon className="w-10 h-10 text-purple-400 drop-shadow" />
                  ) : (
                    <FileText className="w-10 h-10 text-slate-200 drop-shadow" />
                  )}
                </div>
                <span className="text-xs font-medium truncate max-w-full group-hover:text-white">
                  {item.name}
                </span>
                <span className="text-[10px] text-slate-500 font-mono">{item.size}</span>
              </div>
            ))}
          </div>
        ) : (
          <div className="flex-1 overflow-y-auto">
            <table className="w-full text-left text-xs border-collapse">
              <thead>
                <tr className="border-b border-white/10 text-slate-400 font-medium bg-slate-900/30">
                  <th className="py-2 px-4">Name</th>
                  <th className="py-2 px-4">Date Modified</th>
                  <th className="py-2 px-4">Size</th>
                  <th className="py-2 px-4">Kind</th>
                </tr>
              </thead>
              <tbody>
                {filteredItems.map((item) => (
                  <tr
                    key={item.name}
                    onClick={() => setSelectedFile(item)}
                    onDoubleClick={() => {
                      if (item.type === 'folder') navigateTo(item.fullPath);
                      else openApp('text-editor');
                    }}
                    className={`border-b border-white/5 cursor-pointer hover:bg-white/5 ${
                      selectedFile?.name === item.name ? 'bg-blue-600/30 text-white' : 'text-slate-300'
                    }`}
                  >
                    <td className="py-1.5 px-4 flex items-center space-x-2">
                      {item.type === 'folder' ? (
                        <Folder className="w-4 h-4 text-sky-400" />
                      ) : (
                        <FileText className="w-4 h-4 text-slate-400" />
                      )}
                      <span className="font-medium">{item.name}</span>
                    </td>
                    <td className="py-1.5 px-4 text-slate-500 font-mono text-[11px]">{item.modified}</td>
                    <td className="py-1.5 px-4 text-slate-500 font-mono text-[11px]">{item.size}</td>
                    <td className="py-1.5 px-4 text-slate-500 capitalize">{item.type}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        )}

        {/* Footer info bar */}
        <div className="h-6 px-4 bg-slate-900/60 border-t border-white/10 flex items-center justify-between text-[11px] text-slate-500 font-mono">
          <span>{filteredItems.length} items</span>
          <span>AxisOS Linux Filesystem</span>
        </div>
      </div>
    </div>
  );
};
