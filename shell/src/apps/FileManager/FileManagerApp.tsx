// AxisOS High-Performance Graphical File Manager
// Complete Asynchronous I/O, Real-Time State Syncing, XDG Base Directory & Trash Specs,
// Hardware Hotplugging, MIME Detection, Breadcrumb Navigation, Grid/List Views, Operations Queue, and Properties Dialog

import React, { useState, useEffect, useRef, useMemo, useCallback } from 'react';
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
  ArrowUp,
  FolderPlus,
  Trash2,
  RefreshCw,
  ChevronRight,
  Music,
  Film,
  FileCode,
  Archive,
  Terminal,
  Copy,
  Scissors,
  Edit3,
  Shield,
  Info,
  Usb,
  AlertTriangle,
  Check,
  X,
  Play,
  FilePlus,
  Lock,
} from 'lucide-react';
import { systemService, FileEntry, DiskDrive } from '../../services/systemService';
import { useSystemState } from '../../context/SystemStateContext';
import { useWindowManager } from '../../context/WindowManagerContext';

interface ActiveFileOp {
  id: string;
  type: 'copy' | 'move' | 'delete';
  source: string;
  destination?: string;
  progress: number;
  speedMb: string;
  etaSec: number;
  status: 'running' | 'completed' | 'error';
}

interface TrashedItem {
  name: string;
  originalPath: string;
  deletionDate: string;
  size: string;
  fullPath: string;
  type: 'folder' | 'file';
}

interface ContextMenuState {
  x: number;
  y: number;
  targetItem?: FileEntry | null;
}

export const FileManagerApp: React.FC<{ params?: Record<string, any> }> = ({ params }) => {
  const { systemInfo } = useSystemState();
  const { openApp } = useWindowManager();

  const userHome = systemInfo.homeDir || `/home/${systemInfo.username || 'axis'}`;
  const initialPath = params?.path || params?.cwd || userHome;

  // Navigation State
  const [currentPath, setCurrentPath] = useState<string>(initialPath);
  const [items, setItems] = useState<FileEntry[]>([]);
  const [history, setHistory] = useState<string[]>([initialPath]);
  const [histIdx, setHistIdx] = useState(0);

  // Address Bar State
  const [isEditingPath, setIsEditingPath] = useState(false);
  const [pathInput, setPathInput] = useState(initialPath);

  // View & Filter State
  const [viewMode, setViewMode] = useState<'grid' | 'list'>('grid');
  const [filterQuery, setFilterQuery] = useState('');
  const [sortBy, setSortBy] = useState<'name' | 'size' | 'modified' | 'type'>('name');
  const [sortOrder, setSortOrder] = useState<'asc' | 'desc'>('asc');
  const [selectedFile, setSelectedFile] = useState<FileEntry | null>(null);
  const [isLoading, setIsLoading] = useState(false);
  const [permissionDenied, setPermissionDenied] = useState(false);
  const [permErrorMsg, setPermErrorMsg] = useState('');

  // Hardware Hotplugging & Removable Drives
  const [removableDisks, setRemovableDisks] = useState<DiskDrive[]>([]);

  // XDG Trash Mode
  const [isTrashView, setIsTrashView] = useState(false);
  const [trashItems, setTrashItems] = useState<TrashedItem[]>([]);

  // Operations Queue & Clipboard
  const [clipboard, setClipboard] = useState<{ mode: 'copy' | 'cut'; files: string[] } | null>(null);
  const [activeOps, setActiveOps] = useState<ActiveFileOp[]>([]);
  const [showOpsQueue, setShowOpsQueue] = useState(false);

  // Properties Dialog State
  const [propertiesItem, setPropertiesItem] = useState<FileEntry | null>(null);
  const [folderSizeData, setFolderSizeData] = useState<{ bytes: number; humanSize: string; itemCount: number } | null>(null);
  const [propertiesTab, setPropertiesTab] = useState<'general' | 'permissions'>('general');
  const [editPermissions, setEditPermissions] = useState<string>('0755');

  // Context Menu State
  const [contextMenu, setContextMenu] = useState<ContextMenuState | null>(null);

  // Drag and Drop State
  const [draggedItem, setDraggedItem] = useState<FileEntry | null>(null);
  const [dragOverFolder, setDragOverFolder] = useState<string | null>(null);

  // Load Directory Items
  const loadDirectory = useCallback(async (targetPath: string) => {
    setIsLoading(true);
    setPermissionDenied(false);
    setPermErrorMsg('');
    try {
      const res = await systemService.readDirectory(targetPath);
      if (res.permissionDenied) {
        setPermissionDenied(true);
        setPermErrorMsg(res.error || 'Permission Denied: Superuser (sudo) privileges required.');
        setItems([]);
      } else {
        setItems(res.items || []);
      }
      setSelectedFile(null);
    } catch (err: any) {
      setPermissionDenied(true);
      setPermErrorMsg(err.message || 'Error reading directory');
    } finally {
      setIsLoading(false);
    }
  }, []);

  // Load Trash Items
  const loadTrash = useCallback(async () => {
    setIsLoading(true);
    try {
      const items = await systemService.getTrashItems();
      setTrashItems(items);
    } catch {}
    setIsLoading(false);
  }, []);

  // Query Hotplugged Disks
  const refreshDisks = useCallback(async () => {
    try {
      const disks = await systemService.getDisks();
      const removable = disks.filter(
        (d) => d.type?.includes('USB') || d.isLiveMedium || d.id.includes('sdb') || d.id.includes('sdc')
      );
      setRemovableDisks(removable);
    } catch {}
  }, []);

  // Real-Time Filesystem State Syncing
  useEffect(() => {
    if (isTrashView) {
      loadTrash();
    } else {
      loadDirectory(currentPath);
    }
    setPathInput(currentPath);
  }, [currentPath, isTrashView, loadDirectory, loadTrash]);

  // Periodic polling for hardware hotplugging and directory changes
  useEffect(() => {
    refreshDisks();
    const diskInterval = setInterval(refreshDisks, 5000);

    // Silent background poll to catch changes made in terminal
    const dirSyncInterval = setInterval(() => {
      if (!isTrashView && !isLoading) {
        systemService.readDirectory(currentPath).then((res) => {
          if (!res.permissionDenied && res.items && res.items.length !== items.length) {
            setItems(res.items);
          }
        }).catch(() => {});
      }
    }, 3000);

    return () => {
      clearInterval(diskInterval);
      clearInterval(dirSyncInterval);
    };
  }, [currentPath, isTrashView, isLoading, items.length, refreshDisks]);

  // Active operations progress simulation/polling
  useEffect(() => {
    if (activeOps.length === 0) return;
    const interval = setInterval(() => {
      setActiveOps((prev) =>
        prev
          .map((op): ActiveFileOp => {
            if (op.status === 'running') {
              const nextProgress = Math.min(100, op.progress + 25);
              return {
                ...op,
                progress: nextProgress,
                status: nextProgress >= 100 ? 'completed' : 'running',
              };
            }
            return op;
          })
          .filter((op) => op.status !== 'completed' || Date.now() - 5000 < 0)
      );
    }, 700);
    return () => clearInterval(interval);
  }, [activeOps.length]);

  // Navigation Handlers
  const navigateTo = (newPath: string) => {
    setIsTrashView(false);
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
      setIsTrashView(false);
    }
  };

  const handleForward = () => {
    if (histIdx < history.length - 1) {
      const next = history[histIdx + 1];
      setHistIdx(histIdx + 1);
      setCurrentPath(next);
      setIsTrashView(false);
    }
  };

  const handleJumpUp = () => {
    if (currentPath === '/' || isTrashView) return;
    const parent = currentPath.substring(0, currentPath.lastIndexOf('/')) || '/';
    navigateTo(parent);
  };

  const handleAddressSubmit = (e: React.FormEvent) => {
    e.preventDefault();
    setIsEditingPath(false);
    const clean = pathInput.trim();
    if (clean) {
      navigateTo(clean.startsWith('~') ? clean.replace('~', userHome) : clean);
    }
  };

  // MIME type icon dispatcher
  const renderItemIcon = (item: FileEntry, sizeClass = 'w-10 h-10') => {
    if (item.type === 'folder') {
      return <Folder className={`${sizeClass} text-sky-400 fill-sky-400/30 drop-shadow`} />;
    }

    const mime = item.mimeType || '';
    const name = item.name.toLowerCase();

    if (mime.startsWith('image/') || name.endsWith('.png') || name.endsWith('.jpg') || name.endsWith('.jpeg') || name.endsWith('.svg') || name.endsWith('.webp')) {
      return <ImageIcon className={`${sizeClass} text-purple-400 drop-shadow`} />;
    }
    if (mime.startsWith('audio/') || name.endsWith('.mp3') || name.endsWith('.wav') || name.endsWith('.flac')) {
      return <Music className={`${sizeClass} text-emerald-400 drop-shadow`} />;
    }
    if (mime.startsWith('video/') || name.endsWith('.mp4') || name.endsWith('.mkv') || name.endsWith('.webm')) {
      return <Film className={`${sizeClass} text-rose-400 drop-shadow`} />;
    }
    if (
      mime.includes('text/x-') ||
      name.endsWith('.ts') ||
      name.endsWith('.tsx') ||
      name.endsWith('.js') ||
      name.endsWith('.py') ||
      name.endsWith('.c') ||
      name.endsWith('.cpp') ||
      name.endsWith('.rs') ||
      name.endsWith('.json')
    ) {
      return <FileCode className={`${sizeClass} text-amber-400 drop-shadow`} />;
    }
    if (name.endsWith('.tar') || name.endsWith('.gz') || name.endsWith('.zip') || name.endsWith('.deb') || name.endsWith('.iso')) {
      return <Archive className={`${sizeClass} text-orange-400 drop-shadow`} />;
    }
    if (item.isExecutable || name.endsWith('.sh') || name.endsWith('.bin')) {
      return <Play className={`${sizeClass} text-emerald-400 fill-emerald-400/30 drop-shadow`} />;
    }

    return <FileText className={`${sizeClass} text-slate-300 drop-shadow`} />;
  };

  // Double click execution handling
  const handleItemOpen = (item: FileEntry) => {
    if (item.type === 'folder') {
      navigateTo(item.fullPath);
      return;
    }

    const name = item.name.toLowerCase();
    const mime = item.mimeType || '';

    if (name.endsWith('.png') || name.endsWith('.jpg') || name.endsWith('.jpeg') || name.endsWith('.svg') || mime.startsWith('image/')) {
      openApp('photos', { filePath: item.fullPath });
    } else if (name.endsWith('.mp3') || name.endsWith('.wav') || name.endsWith('.flac') || mime.startsWith('audio/')) {
      openApp('music', { filePath: item.fullPath });
    } else if (name.endsWith('.html') || name.endsWith('.pdf')) {
      openApp('browser', { filePath: item.fullPath });
    } else if (item.isExecutable || name.endsWith('.sh')) {
      openApp('terminal', { cwd: currentPath, cmd: `./${item.name}` });
    } else {
      openApp('text-editor', { filePath: item.fullPath });
    }
  };

  // File Operations: Create, Move to Trash, Restore, Permissions
  const handleCreateFolder = async () => {
    const folderName = prompt('Enter new folder name:', 'New Folder');
    if (!folderName) return;
    const target = `${currentPath}/${folderName}`.replace(/\/+/g, '/');
    await systemService.createDirectory(target);
    await loadDirectory(currentPath);
  };

  const handleCreateFile = async () => {
    const fileName = prompt('Enter new file name:', 'untitled.txt');
    if (!fileName) return;
    const target = `${currentPath}/${fileName}`.replace(/\/+/g, '/');
    await systemService.writeFile(target, '');
    await loadDirectory(currentPath);
  };

  const handleMoveToTrash = async (item: FileEntry) => {
    await systemService.moveToTrash(item.fullPath);
    setSelectedFile(null);
    await loadDirectory(currentPath);
  };

  const handlePermanentDelete = async (item: FileEntry) => {
    if (confirm(`Permanently delete "${item.name}"? This cannot be undone.`)) {
      await systemService.deleteItem(item.fullPath);
      setSelectedFile(null);
      await loadDirectory(currentPath);
    }
  };

  const handleEmptyTrash = async () => {
    if (confirm('Are you sure you want to empty the Trash? All items will be permanently erased.')) {
      await systemService.emptyTrash();
      await loadTrash();
    }
  };

  const handleRestoreTrashItem = async (item: TrashedItem) => {
    await systemService.restoreTrashItem(item.name);
    await loadTrash();
  };

  // Clipboard operations (Copy, Cut, Paste)
  const handleCopy = (item: FileEntry, mode: 'copy' | 'cut') => {
    setClipboard({ mode, files: [item.fullPath] });
  };

  const handlePaste = async () => {
    if (!clipboard || clipboard.files.length === 0) return;
    const src = clipboard.files[0];
    const fileName = src.substring(src.lastIndexOf('/') + 1);
    const dest = `${currentPath}/${fileName}`.replace(/\/+/g, '/');

    // Launch background operation in Operations Queue
    const opId = `op-${Date.now()}`;
    const newOp: ActiveFileOp = {
      id: opId,
      type: clipboard.mode === 'cut' ? 'move' : 'copy',
      source: src,
      destination: dest,
      progress: 20,
      speedMb: '48.5',
      etaSec: 2,
      status: 'running',
    };
    setActiveOps((prev) => [...prev, newOp]);
    setShowOpsQueue(true);

    await systemService.startFileOp({
      type: clipboard.mode === 'cut' ? 'move' : 'copy',
      source: src,
      destination: dest,
    });

    if (clipboard.mode === 'cut') setClipboard(null);
    setTimeout(() => loadDirectory(currentPath), 800);
  };

  // Open Properties Modal
  const handleOpenProperties = async (item: FileEntry) => {
    setPropertiesItem(item);
    setPropertiesTab('general');
    setEditPermissions(item.permissions || '0755');
    setFolderSizeData(null);

    if (item.type === 'folder') {
      const data = await systemService.calcFolderSize(item.fullPath);
      setFolderSizeData(data);
    }
  };

  const handleSavePermissions = async () => {
    if (!propertiesItem) return;
    await systemService.chmod(propertiesItem.fullPath, editPermissions);
    setPropertiesItem(null);
    await loadDirectory(currentPath);
  };

  // Mount/Unmount USB Disk
  const handleMountDisk = async (d: DiskDrive) => {
    const res = await systemService.mountDisk(d.id);
    if (res.success && res.mountpoint) {
      navigateTo(res.mountpoint);
    }
    refreshDisks();
  };

  const handleUnmountDisk = async (d: DiskDrive) => {
    await systemService.unmountDisk(d.id);
    refreshDisks();
    if (currentPath.includes(d.id)) navigateTo(userHome);
  };

  // Breadcrumbs Generator
  const breadcrumbSegments = useMemo(() => {
    if (currentPath === '/') return [{ name: 'Root', path: '/' }];
    const parts = currentPath.split('/').filter(Boolean);
    const segs: { name: string; path: string }[] = [{ name: 'Root', path: '/' }];
    let accum = '';
    for (const p of parts) {
      accum += `/${p}`;
      segs.push({ name: p === systemInfo.username ? 'Home' : p, path: accum });
    }
    return segs;
  }, [currentPath, systemInfo.username]);

  // Filtered & Sorted Items
  const processedItems = useMemo(() => {
    let list = items.filter((item) => item.name.toLowerCase().includes(filterQuery.toLowerCase()));

    list.sort((a, b) => {
      // Folders always sorted first
      if (a.type === 'folder' && b.type !== 'folder') return -1;
      if (a.type !== 'folder' && b.type === 'folder') return 1;

      let comp = 0;
      if (sortBy === 'name') comp = a.name.localeCompare(b.name);
      else if (sortBy === 'size') comp = (a.rawSize || 0) - (b.rawSize || 0);
      else if (sortBy === 'modified') comp = (a.rawMtime || 0) - (b.rawMtime || 0);
      else if (sortBy === 'type') comp = (a.mimeType || '').localeCompare(b.mimeType || '');

      return sortOrder === 'asc' ? comp : -comp;
    });

    return list;
  }, [items, filterQuery, sortBy, sortOrder]);

  const toggleSort = (field: 'name' | 'size' | 'modified' | 'type') => {
    if (sortBy === field) {
      setSortOrder(sortOrder === 'asc' ? 'desc' : 'asc');
    } else {
      setSortBy(field);
      setSortOrder('asc');
    }
  };

  return (
    <div
      onClick={() => setContextMenu(null)}
      className="flex h-full w-full bg-[#0a0e14] text-slate-100 select-none overflow-hidden font-sans text-xs"
    >
      {/* ==================== LEFT MASTER SIDEBAR (PLACES) ==================== */}
      <div className="w-56 bg-[#0f141c]/95 border-r border-slate-800/80 p-3 flex flex-col gap-1 text-xs shrink-0 select-none backdrop-blur-xl">
        <div className="px-2 py-1 text-[11px] font-bold text-slate-400 tracking-wider uppercase">Favorites</div>

        <button
          onClick={() => navigateTo(userHome)}
          className={`flex items-center space-x-2.5 px-2.5 py-1.5 rounded-lg transition-colors text-left ${
            !isTrashView && currentPath === userHome ? 'bg-blue-600 text-white font-medium shadow-md shadow-blue-600/30' : 'text-slate-300 hover:bg-white/5'
          }`}
        >
          <Home className="w-4 h-4 text-blue-400" />
          <span>Home</span>
        </button>

        <button
          onClick={() => navigateTo(`${userHome}/Desktop`)}
          className={`flex items-center space-x-2.5 px-2.5 py-1.5 rounded-lg transition-colors text-left ${
            !isTrashView && currentPath.endsWith('Desktop') ? 'bg-blue-600 text-white font-medium shadow-md shadow-blue-600/30' : 'text-slate-300 hover:bg-white/5'
          }`}
        >
          <Folder className="w-4 h-4 text-cyan-400" />
          <span>Desktop</span>
        </button>

        <button
          onClick={() => navigateTo(`${userHome}/Documents`)}
          className={`flex items-center space-x-2.5 px-2.5 py-1.5 rounded-lg transition-colors text-left ${
            !isTrashView && currentPath.includes('Documents') ? 'bg-blue-600 text-white font-medium shadow-md shadow-blue-600/30' : 'text-slate-300 hover:bg-white/5'
          }`}
        >
          <Folder className="w-4 h-4 text-sky-400" />
          <span>Documents</span>
        </button>

        <button
          onClick={() => navigateTo(`${userHome}/Downloads`)}
          className={`flex items-center space-x-2.5 px-2.5 py-1.5 rounded-lg transition-colors text-left ${
            !isTrashView && currentPath.includes('Downloads') ? 'bg-blue-600 text-white font-medium shadow-md shadow-blue-600/30' : 'text-slate-300 hover:bg-white/5'
          }`}
        >
          <Download className="w-4 h-4 text-indigo-400" />
          <span>Downloads</span>
        </button>

        <button
          onClick={() => navigateTo(`${userHome}/Pictures`)}
          className={`flex items-center space-x-2.5 px-2.5 py-1.5 rounded-lg transition-colors text-left ${
            !isTrashView && currentPath.includes('Pictures') ? 'bg-blue-600 text-white font-medium shadow-md shadow-blue-600/30' : 'text-slate-300 hover:bg-white/5'
          }`}
        >
          <ImageIcon className="w-4 h-4 text-purple-400" />
          <span>Pictures</span>
        </button>

        <button
          onClick={() => navigateTo(`${userHome}/Music`)}
          className={`flex items-center space-x-2.5 px-2.5 py-1.5 rounded-lg transition-colors text-left ${
            !isTrashView && currentPath.includes('Music') ? 'bg-blue-600 text-white font-medium shadow-md shadow-blue-600/30' : 'text-slate-300 hover:bg-white/5'
          }`}
        >
          <Music className="w-4 h-4 text-emerald-400" />
          <span>Music</span>
        </button>

        <button
          onClick={() => navigateTo(`${userHome}/Videos`)}
          className={`flex items-center space-x-2.5 px-2.5 py-1.5 rounded-lg transition-colors text-left ${
            !isTrashView && currentPath.includes('Videos') ? 'bg-blue-600 text-white font-medium shadow-md shadow-blue-600/30' : 'text-slate-300 hover:bg-white/5'
          }`}
        >
          <Film className="w-4 h-4 text-rose-400" />
          <span>Videos</span>
        </button>

        <div className="my-2 border-t border-slate-800/80"></div>
        <div className="px-2 py-1 text-[11px] font-bold text-slate-400 tracking-wider uppercase">Drives & Locations</div>

        <button
          onClick={() => navigateTo('/')}
          className={`flex items-center space-x-2.5 px-2.5 py-1.5 rounded-lg transition-colors text-left ${
            !isTrashView && currentPath === '/' ? 'bg-blue-600 text-white font-medium shadow-md shadow-blue-600/30' : 'text-slate-300 hover:bg-white/5'
          }`}
        >
          <HardDrive className="w-4 h-4 text-slate-400" />
          <span>AxisOS Root (/)</span>
        </button>

        {/* Dynamic Removable USB Storage Hotplugging */}
        {removableDisks.map((d) => (
          <div
            key={d.id}
            className="flex items-center justify-between px-2.5 py-1.5 rounded-lg hover:bg-white/5 text-slate-300 group"
          >
            <button
              onClick={() => handleMountDisk(d)}
              className="flex items-center space-x-2 truncate flex-1 text-left"
            >
              <Usb className="w-4 h-4 text-amber-400 shrink-0" />
              <span className="truncate">{d.model || d.name}</span>
            </button>
            <button
              onClick={() => handleUnmountDisk(d)}
              title="Eject drive"
              className="p-1 hover:bg-white/10 rounded text-slate-500 hover:text-rose-400 opacity-0 group-hover:opacity-100 transition-opacity"
            >
              <X className="w-3 h-3" />
            </button>
          </div>
        ))}

        <div className="my-2 border-t border-slate-800/80"></div>

        {/* XDG Trash Entry */}
        <button
          onClick={() => {
            setIsTrashView(true);
            loadTrash();
          }}
          className={`flex items-center justify-between px-2.5 py-1.5 rounded-lg transition-colors text-left ${
            isTrashView ? 'bg-rose-600 text-white font-medium shadow-md shadow-rose-600/30' : 'text-slate-300 hover:bg-white/5'
          }`}
        >
          <div className="flex items-center space-x-2.5">
            <Trash2 className="w-4 h-4 text-rose-400" />
            <span>Trash</span>
          </div>
          {trashItems.length > 0 && (
            <span className="px-1.5 py-0.2 rounded-full bg-rose-500/30 text-rose-200 text-[10px] font-bold">
              {trashItems.length}
            </span>
          )}
        </button>
      </div>

      {/* ==================== MAIN CONTENT & VIEWPORT ==================== */}
      <div
        onContextMenu={(e) => {
          e.preventDefault();
          setContextMenu({ x: e.clientX, y: e.clientY, targetItem: null });
        }}
        className="flex-1 flex flex-col bg-[#0d121a]/90 overflow-hidden"
      >
        {/* Top Navigation & Toolbar Bar */}
        <div className="h-11 px-3.5 flex items-center justify-between border-b border-slate-800/80 bg-[#111722]/80 shrink-0 gap-2">
          {/* Back, Forward, Up Navigation Controls */}
          <div className="flex items-center space-x-1.5">
            <button
              onClick={handleBack}
              disabled={histIdx === 0}
              className={`p-1.5 rounded-md ${histIdx === 0 ? 'text-slate-600' : 'text-slate-300 hover:bg-white/10'}`}
              title="Back"
            >
              <ArrowLeft className="w-4 h-4" />
            </button>
            <button
              onClick={handleForward}
              disabled={histIdx >= history.length - 1}
              className={`p-1.5 rounded-md ${histIdx >= history.length - 1 ? 'text-slate-600' : 'text-slate-300 hover:bg-white/10'}`}
              title="Forward"
            >
              <ArrowRight className="w-4 h-4" />
            </button>
            <button
              onClick={handleJumpUp}
              disabled={currentPath === '/' || isTrashView}
              className={`p-1.5 rounded-md ${currentPath === '/' || isTrashView ? 'text-slate-600' : 'text-slate-300 hover:bg-white/10'}`}
              title="Parent Directory"
            >
              <ArrowUp className="w-4 h-4" />
            </button>
          </div>

          {/* Interactive Breadcrumb Path Navigation or Direct Text Field */}
          <div className="flex-1 max-w-xl mx-2">
            {isEditingPath ? (
              <form onSubmit={handleAddressSubmit}>
                <input
                  type="text"
                  value={pathInput}
                  onChange={(e) => setPathInput(e.target.value)}
                  onBlur={() => setIsEditingPath(false)}
                  autoFocus
                  className="w-full bg-[#182232] border border-blue-500 rounded-lg px-2.5 py-1 text-xs text-white outline-none font-mono shadow-inner"
                />
              </form>
            ) : (
              <div
                onClick={() => setIsEditingPath(true)}
                className="flex items-center bg-[#151c28] border border-slate-800 hover:border-slate-700 rounded-lg px-2 py-1 text-xs text-slate-300 cursor-text overflow-x-auto scrollbar-none"
              >
                {isTrashView ? (
                  <span className="font-semibold text-rose-400">Trash Bin</span>
                ) : (
                  breadcrumbSegments.map((seg, idx) => (
                    <React.Fragment key={seg.path}>
                      <button
                        onClick={(e) => {
                          e.stopPropagation();
                          navigateTo(seg.path);
                        }}
                        className="hover:text-blue-400 font-medium px-1 py-0.5 rounded transition-colors whitespace-nowrap"
                      >
                        {seg.name}
                      </button>
                      {idx < breadcrumbSegments.length - 1 && (
                        <ChevronRight className="w-3.5 h-3.5 text-slate-600 shrink-0" />
                      )}
                    </React.Fragment>
                  ))
                )}
              </div>
            )}
          </div>

          {/* Right Action Icons & View Toggles */}
          <div className="flex items-center space-x-2">
            {/* Active Operations Queue Popover Button */}
            {activeOps.length > 0 && (
              <button
                onClick={() => setShowOpsQueue(!showOpsQueue)}
                className="flex items-center gap-1.5 px-2 py-1 rounded-lg bg-sky-500/20 text-sky-300 border border-sky-500/40 text-[11px] animate-pulse"
              >
                <RefreshCw className="w-3 h-3 animate-spin" />
                <span>{activeOps[0].speedMb} MB/s</span>
              </button>
            )}

            {isTrashView ? (
              <button
                onClick={handleEmptyTrash}
                disabled={trashItems.length === 0}
                className="px-2.5 py-1 rounded-lg bg-rose-500/20 hover:bg-rose-500/30 text-rose-300 border border-rose-500/40 flex items-center gap-1 text-[11px] disabled:opacity-40"
              >
                <Trash2 className="w-3.5 h-3.5" />
                <span>Empty Trash</span>
              </button>
            ) : (
              <>
                <button
                  onClick={handleCreateFolder}
                  className="p-1.5 rounded-lg bg-white/5 hover:bg-white/10 text-slate-300 flex items-center gap-1 text-[11px]"
                  title="New Folder"
                >
                  <FolderPlus className="w-3.5 h-3.5 text-cyan-400" />
                  <span className="hidden sm:inline">New Folder</span>
                </button>
                <button
                  onClick={handleCreateFile}
                  className="p-1.5 rounded-lg bg-white/5 hover:bg-white/10 text-slate-300 flex items-center gap-1 text-[11px]"
                  title="New Document"
                >
                  <FilePlus className="w-3.5 h-3.5 text-emerald-400" />
                  <span className="hidden sm:inline">New File</span>
                </button>
              </>
            )}

            <button
              onClick={() => (isTrashView ? loadTrash() : loadDirectory(currentPath))}
              className="p-1.5 rounded-lg bg-white/5 hover:bg-white/10 text-slate-300"
              title="Refresh"
            >
              <RefreshCw className={`w-3.5 h-3.5 ${isLoading ? 'animate-spin text-cyan-400' : ''}`} />
            </button>

            {/* View Mode Toggle */}
            <div className="flex items-center p-0.5 rounded-lg bg-slate-900 border border-slate-800">
              <button
                onClick={() => setViewMode('grid')}
                className={`p-1 rounded ${viewMode === 'grid' ? 'bg-blue-600 text-white' : 'text-slate-400 hover:text-white'}`}
                title="Icon Grid View"
              >
                <LayoutGrid className="w-3.5 h-3.5" />
              </button>
              <button
                onClick={() => setViewMode('list')}
                className={`p-1 rounded ${viewMode === 'list' ? 'bg-blue-600 text-white' : 'text-slate-400 hover:text-white'}`}
                title="Details List View"
              >
                <List className="w-3.5 h-3.5" />
              </button>
            </div>

            {/* Search Filter */}
            <div className="relative">
              <Search className="w-3 h-3 absolute left-2 top-2 text-slate-400" />
              <input
                type="text"
                placeholder="Filter..."
                value={filterQuery}
                onChange={(e) => setFilterQuery(e.target.value)}
                className="pl-6 pr-2 py-1 bg-slate-900 border border-slate-800 rounded-lg text-[11px] text-slate-200 outline-none w-24 focus:w-36 transition-all"
              />
            </div>
          </div>
        </div>

        {/* Operations Queue Floating Popover */}
        {showOpsQueue && activeOps.length > 0 && (
          <div className="absolute top-12 right-6 w-80 bg-slate-900/95 border border-slate-700/80 rounded-xl p-3 shadow-2xl z-40 text-xs">
            <div className="flex items-center justify-between pb-2 border-b border-slate-800">
              <span className="font-bold text-slate-200">Active File Operations</span>
              <button onClick={() => setShowOpsQueue(false)} className="text-slate-400 hover:text-white">
                <X className="w-3.5 h-3.5" />
              </button>
            </div>
            <div className="space-y-3 pt-2">
              {activeOps.map((op) => (
                <div key={op.id} className="space-y-1">
                  <div className="flex justify-between text-[11px]">
                    <span className="truncate max-w-[180px] font-medium text-slate-300">
                      {op.type.toUpperCase()}: {op.source.split('/').pop()}
                    </span>
                    <span className="text-cyan-400 font-mono">{op.speedMb} MB/s</span>
                  </div>
                  <div className="w-full bg-slate-800 rounded-full h-1.5 overflow-hidden">
                    <div className="bg-blue-500 h-full transition-all duration-300" style={{ width: `${op.progress}%` }} />
                  </div>
                  <div className="flex justify-between text-[10px] text-slate-500">
                    <span>ETA ~{op.etaSec}s</span>
                    <span>{op.progress}%</span>
                  </div>
                </div>
              ))}
            </div>
          </div>
        )}

        {/* Permission Denied UI State */}
        {permissionDenied && (
          <div className="flex-1 flex flex-col items-center justify-center p-8 text-center">
            <div className="w-16 h-16 rounded-2xl bg-rose-500/10 border border-rose-500/20 flex items-center justify-center mb-4">
              <Lock className="w-8 h-8 text-rose-400" />
            </div>
            <h3 className="text-base font-bold text-white mb-1">Permission Denied</h3>
            <p className="text-xs text-slate-400 max-w-md mb-4">{permErrorMsg}</p>
            <div className="flex gap-2">
              <button
                onClick={() => navigateTo(userHome)}
                className="px-4 py-2 rounded-xl bg-blue-600 hover:bg-blue-500 text-white font-medium text-xs shadow-lg shadow-blue-600/30"
              >
                Return to Home
              </button>
              <button
                onClick={() => openApp('terminal', { cwd: currentPath, cmd: 'sudo su' })}
                className="px-4 py-2 rounded-xl bg-white/10 hover:bg-white/15 text-slate-200 font-medium text-xs flex items-center gap-1.5"
              >
                <Terminal className="w-3.5 h-3.5 text-cyan-400" />
                <span>Open in Terminal (sudo)</span>
              </button>
            </div>
          </div>
        )}

        {/* File Content Area */}
        {!permissionDenied && (
          <div className="flex-1 overflow-y-auto">
            {isTrashView ? (
              /* Trash Content */
              trashItems.length === 0 ? (
                <div className="h-full flex flex-col items-center justify-center text-slate-500 space-y-2">
                  <Trash2 className="w-12 h-12 stroke-[1.2]" />
                  <span>Trash is empty</span>
                </div>
              ) : (
                <div className="p-4 grid grid-cols-2 sm:grid-cols-4 md:grid-cols-5 gap-3">
                  {trashItems.map((item) => (
                    <div
                      key={item.name}
                      className="flex flex-col items-center p-3 rounded-xl bg-slate-900/40 border border-slate-800 hover:border-slate-700 text-center"
                    >
                      <Trash2 className="w-10 h-10 text-rose-400/80 mb-2" />
                      <span className="font-medium text-slate-200 truncate max-w-full text-xs">{item.name}</span>
                      <span className="text-[10px] text-slate-500 truncate max-w-full">{item.originalPath}</span>
                      <button
                        onClick={() => handleRestoreTrashItem(item)}
                        className="mt-2 px-2 py-1 rounded bg-white/10 hover:bg-blue-600 hover:text-white text-[10px] text-slate-300"
                      >
                        Restore
                      </button>
                    </div>
                  ))}
                </div>
              )
            ) : viewMode === 'grid' ? (
              /* Grid View */
              <div className="p-5 grid grid-cols-3 sm:grid-cols-4 md:grid-cols-5 lg:grid-cols-6 gap-4 content-start">
                {processedItems.map((item) => (
                  <div
                    key={item.name}
                    draggable
                    onDragStart={() => setDraggedItem(item)}
                    onDragOver={(e) => {
                      if (item.type === 'folder') {
                        e.preventDefault();
                        setDragOverFolder(item.fullPath);
                      }
                    }}
                    onDragLeave={() => setDragOverFolder(null)}
                    onDrop={() => {
                      if (draggedItem && item.type === 'folder' && draggedItem.fullPath !== item.fullPath) {
                        systemService.startFileOp({
                          type: 'move',
                          source: draggedItem.fullPath,
                          destination: `${item.fullPath}/${draggedItem.name}`,
                        }).then(() => loadDirectory(currentPath));
                      }
                      setDragOverFolder(null);
                      setDraggedItem(null);
                    }}
                    onClick={() => setSelectedFile(item)}
                    onDoubleClick={() => handleItemOpen(item)}
                    onContextMenu={(e) => {
                      e.preventDefault();
                      e.stopPropagation();
                      setSelectedFile(item);
                      setContextMenu({ x: e.clientX, y: e.clientY, targetItem: item });
                    }}
                    className={`flex flex-col items-center p-3 rounded-2xl cursor-pointer text-center group transition-all border ${
                      dragOverFolder === item.fullPath
                        ? 'bg-blue-600/40 border-blue-400 scale-105'
                        : selectedFile?.name === item.name
                        ? 'bg-blue-600/30 border-blue-500 text-white shadow-lg shadow-blue-500/10'
                        : 'border-transparent hover:bg-white/5 text-slate-300'
                    }`}
                  >
                    <div className="w-14 h-14 rounded-2xl flex items-center justify-center mb-1 group-hover:scale-105 transition-transform">
                      {renderItemIcon(item)}
                    </div>
                    <span className="text-xs font-medium truncate max-w-full group-hover:text-white px-1">
                      {item.name}
                    </span>
                    <span className="text-[10px] text-slate-500 font-mono mt-0.5">{item.size}</span>
                  </div>
                ))}
              </div>
            ) : (
              /* Details List View */
              <table className="w-full text-left text-xs border-collapse">
                <thead>
                  <tr className="border-b border-slate-800 text-slate-400 font-semibold bg-[#111722]/60 select-none">
                    <th onClick={() => toggleSort('name')} className="py-2 px-4 cursor-pointer hover:text-white">
                      Name {sortBy === 'name' ? (sortOrder === 'asc' ? '↑' : '↓') : ''}
                    </th>
                    <th onClick={() => toggleSort('size')} className="py-2 px-4 cursor-pointer hover:text-white">
                      Size {sortBy === 'size' ? (sortOrder === 'asc' ? '↑' : '↓') : ''}
                    </th>
                    <th onClick={() => toggleSort('type')} className="py-2 px-4 cursor-pointer hover:text-white">
                      Kind {sortBy === 'type' ? (sortOrder === 'asc' ? '↑' : '↓') : ''}
                    </th>
                    <th onClick={() => toggleSort('modified')} className="py-2 px-4 cursor-pointer hover:text-white">
                      Date Modified {sortBy === 'modified' ? (sortOrder === 'asc' ? '↑' : '↓') : ''}
                    </th>
                    <th className="py-2 px-4">Permissions</th>
                    <th className="py-2 px-4">Owner</th>
                  </tr>
                </thead>
                <tbody>
                  {processedItems.map((item) => (
                    <tr
                      key={item.name}
                      onClick={() => setSelectedFile(item)}
                      onDoubleClick={() => handleItemOpen(item)}
                      onContextMenu={(e) => {
                        e.preventDefault();
                        e.stopPropagation();
                        setSelectedFile(item);
                        setContextMenu({ x: e.clientX, y: e.clientY, targetItem: item });
                      }}
                      className={`border-b border-slate-800/40 cursor-pointer hover:bg-white/5 transition-colors ${
                        selectedFile?.name === item.name ? 'bg-blue-600/30 text-white' : 'text-slate-300'
                      }`}
                    >
                      <td className="py-1.5 px-4 flex items-center space-x-2">
                        {renderItemIcon(item, 'w-4 h-4')}
                        <span className="font-medium truncate max-w-xs">{item.name}</span>
                      </td>
                      <td className="py-1.5 px-4 text-slate-400 font-mono text-[11px]">{item.size}</td>
                      <td className="py-1.5 px-4 text-slate-400 capitalize truncate max-w-[120px]">
                        {item.type === 'folder' ? 'Folder' : item.mimeType?.split('/')[1] || item.type}
                      </td>
                      <td className="py-1.5 px-4 text-slate-400 font-mono text-[11px]">{item.modified}</td>
                      <td className="py-1.5 px-4 text-slate-400 font-mono text-[11px]">{item.modeStr || item.permissions || '0755'}</td>
                      <td className="py-1.5 px-4 text-slate-400 text-[11px]">{item.owner || 'axis'}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            )}
          </div>
        )}

        {/* ==================== FOOTER STATUS BAR ==================== */}
        <div className="h-6 px-4 bg-[#0f141c] border-t border-slate-800/80 flex items-center justify-between text-[11px] text-slate-400 font-mono shrink-0">
          <span>
            {isTrashView ? `${trashItems.length} items in Trash` : `${processedItems.length} items`}
          </span>
          <div className="flex items-center gap-3">
            {selectedFile && (
              <span>
                Selected: {selectedFile.name} ({selectedFile.size})
              </span>
            )}
            <span className="text-slate-600">|</span>
            <span>AxisOS VFS (POSIX ext4)</span>
          </div>
        </div>
      </div>

      {/* ==================== CONTEXT MENU ==================== */}
      {contextMenu && (
        <div
          onClick={(e) => e.stopPropagation()}
          className="fixed z-50 w-52 bg-slate-900/95 backdrop-blur-2xl border border-slate-700/80 rounded-xl p-1.5 shadow-2xl text-xs text-slate-200 flex flex-col gap-0.5 animate-in fade-in duration-100"
          style={{ top: Math.min(contextMenu.y, window.innerHeight - 280), left: Math.min(contextMenu.x, window.innerWidth - 220) }}
        >
          {contextMenu.targetItem ? (
            /* Item Right-Click */
            <>
              <button
                onClick={() => {
                  handleItemOpen(contextMenu.targetItem!);
                  setContextMenu(null);
                }}
                className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-blue-600 hover:text-white text-left"
              >
                <Play className="w-3.5 h-3.5 text-emerald-400" />
                <span>Open</span>
              </button>
              <button
                onClick={() => {
                  openApp('text-editor', { filePath: contextMenu.targetItem!.fullPath });
                  setContextMenu(null);
                }}
                className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-blue-600 hover:text-white text-left"
              >
                <Edit3 className="w-3.5 h-3.5 text-sky-400" />
                <span>Edit in Text Editor</span>
              </button>
              {contextMenu.targetItem.type === 'folder' && (
                <button
                  onClick={() => {
                    openApp('terminal', { cwd: contextMenu.targetItem!.fullPath });
                    setContextMenu(null);
                  }}
                  className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-blue-600 hover:text-white text-left"
                >
                  <Terminal className="w-3.5 h-3.5 text-amber-400" />
                  <span>Open in Terminal</span>
                </button>
              )}
              <div className="my-1 border-t border-slate-800" />
              <button
                onClick={() => {
                  handleCopy(contextMenu.targetItem!, 'copy');
                  setContextMenu(null);
                }}
                className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-blue-600 hover:text-white text-left"
              >
                <Copy className="w-3.5 h-3.5 text-slate-400" />
                <span>Copy</span>
              </button>
              <button
                onClick={() => {
                  handleCopy(contextMenu.targetItem!, 'cut');
                  setContextMenu(null);
                }}
                className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-blue-600 hover:text-white text-left"
              >
                <Scissors className="w-3.5 h-3.5 text-slate-400" />
                <span>Cut</span>
              </button>
              <div className="my-1 border-t border-slate-800" />
              <button
                onClick={() => {
                  handleMoveToTrash(contextMenu.targetItem!);
                  setContextMenu(null);
                }}
                className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-rose-600 hover:text-white text-left text-rose-300"
              >
                <Trash2 className="w-3.5 h-3.5 text-rose-400" />
                <span>Move to Trash</span>
              </button>
              <button
                onClick={() => {
                  handlePermanentDelete(contextMenu.targetItem!);
                  setContextMenu(null);
                }}
                className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-rose-600 hover:text-white text-left text-rose-400"
              >
                <X className="w-3.5 h-3.5" />
                <span>Delete Permanently</span>
              </button>
              <div className="my-1 border-t border-slate-800" />
              <button
                onClick={() => {
                  handleOpenProperties(contextMenu.targetItem!);
                  setContextMenu(null);
                }}
                className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-blue-600 hover:text-white text-left"
              >
                <Info className="w-3.5 h-3.5 text-cyan-400" />
                <span>Properties...</span>
              </button>
            </>
          ) : (
            /* Blank Canvas Right-Click */
            <>
              <button
                onClick={() => {
                  handleCreateFolder();
                  setContextMenu(null);
                }}
                className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-blue-600 hover:text-white text-left"
              >
                <FolderPlus className="w-3.5 h-3.5 text-cyan-400" />
                <span>New Folder</span>
              </button>
              <button
                onClick={() => {
                  handleCreateFile();
                  setContextMenu(null);
                }}
                className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-blue-600 hover:text-white text-left"
              >
                <FilePlus className="w-3.5 h-3.5 text-emerald-400" />
                <span>New File</span>
              </button>
              {clipboard && (
                <button
                  onClick={() => {
                    handlePaste();
                    setContextMenu(null);
                  }}
                  className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-blue-600 hover:text-white text-left"
                >
                  <Copy className="w-3.5 h-3.5 text-amber-400" />
                  <span>Paste Item</span>
                </button>
              )}
              <div className="my-1 border-t border-slate-800" />
              <button
                onClick={() => {
                  openApp('terminal', { cwd: currentPath });
                  setContextMenu(null);
                }}
                className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-blue-600 hover:text-white text-left"
              >
                <Terminal className="w-3.5 h-3.5 text-amber-400" />
                <span>Open in Terminal</span>
              </button>
              <button
                onClick={() => {
                  loadDirectory(currentPath);
                  setContextMenu(null);
                }}
                className="flex items-center space-x-2 px-2.5 py-1.5 rounded-lg hover:bg-blue-600 hover:text-white text-left"
              >
                <RefreshCw className="w-3.5 h-3.5 text-slate-400" />
                <span>Reload Directory</span>
              </button>
            </>
          )}
        </div>
      )}

      {/* ==================== PROPERTIES DIALOG (TABBED MODAL) ==================== */}
      {propertiesItem && (
        <div className="fixed inset-0 bg-black/60 backdrop-blur-sm z-50 flex items-center justify-center p-4">
          <div className="w-[420px] bg-slate-900 border border-slate-700/80 rounded-2xl shadow-2xl overflow-hidden flex flex-col text-xs text-slate-200 animate-in zoom-in-95 duration-150">
            {/* Modal Header */}
            <div className="px-4 py-3 bg-slate-950/80 border-b border-slate-800 flex items-center justify-between">
              <span className="font-bold text-white text-sm">
                {propertiesItem.name} Properties
              </span>
              <button
                onClick={() => setPropertiesItem(null)}
                className="p-1 rounded-lg text-slate-400 hover:text-white hover:bg-white/10"
              >
                <X className="w-4 h-4" />
              </button>
            </div>

            {/* Tabs */}
            <div className="flex border-b border-slate-800 bg-slate-950/40 text-[11px] font-medium">
              <button
                onClick={() => setPropertiesTab('general')}
                className={`flex-1 py-2 text-center border-b-2 transition-colors ${
                  propertiesTab === 'general' ? 'border-blue-500 text-blue-400 font-semibold' : 'border-transparent text-slate-400 hover:text-slate-200'
                }`}
              >
                General
              </button>
              <button
                onClick={() => setPropertiesTab('permissions')}
                className={`flex-1 py-2 text-center border-b-2 transition-colors ${
                  propertiesTab === 'permissions' ? 'border-blue-500 text-blue-400 font-semibold' : 'border-transparent text-slate-400 hover:text-slate-200'
                }`}
              >
                Permissions (chmod)
              </button>
            </div>

            {/* Tab Body */}
            <div className="p-4 space-y-3">
              {propertiesTab === 'general' ? (
                <>
                  <div className="flex items-center gap-3 pb-3 border-b border-slate-800">
                    <div className="w-14 h-14 rounded-2xl bg-slate-800/80 flex items-center justify-center">
                      {renderItemIcon(propertiesItem, 'w-9 h-9')}
                    </div>
                    <div className="flex-1 truncate">
                      <div className="font-bold text-white text-sm truncate">{propertiesItem.name}</div>
                      <div className="text-slate-400 text-[11px] capitalize">{propertiesItem.mimeType || propertiesItem.type}</div>
                    </div>
                  </div>

                  <div className="space-y-2 text-[11px]">
                    <div className="flex justify-between py-1 border-b border-slate-800/50">
                      <span className="text-slate-400">Location:</span>
                      <span className="text-slate-200 font-mono truncate max-w-[240px]">{propertiesItem.fullPath}</span>
                    </div>
                    <div className="flex justify-between py-1 border-b border-slate-800/50">
                      <span className="text-slate-400">Total Size:</span>
                      <span className="text-cyan-400 font-mono font-medium">
                        {folderSizeData ? `${folderSizeData.humanSize} (${folderSizeData.itemCount} items)` : propertiesItem.size}
                      </span>
                    </div>
                    <div className="flex justify-between py-1 border-b border-slate-800/50">
                      <span className="text-slate-400">Date Modified:</span>
                      <span className="text-slate-200">{propertiesItem.modified}</span>
                    </div>
                    <div className="flex justify-between py-1 border-b border-slate-800/50">
                      <span className="text-slate-400">POSIX Type:</span>
                      <span className="text-slate-200 capitalize">{propertiesItem.type}</span>
                    </div>
                  </div>
                </>
              ) : (
                /* Permissions Tab */
                <div className="space-y-3">
                  <div className="space-y-1">
                    <span className="text-slate-400 text-[11px]">Owner / Group</span>
                    <div className="p-2 rounded-lg bg-slate-950/60 border border-slate-800 font-mono text-[11px] flex justify-between">
                      <span>Owner: <strong className="text-emerald-400">{propertiesItem.owner || 'axis'}</strong></span>
                      <span>Group: <strong className="text-sky-400">{propertiesItem.group || 'axis'}</strong></span>
                    </div>
                  </div>

                  <div className="space-y-1">
                    <span className="text-slate-400 text-[11px]">Octal Permission (e.g. 0755, 0644)</span>
                    <input
                      type="text"
                      value={editPermissions}
                      onChange={(e) => setEditPermissions(e.target.value)}
                      className="w-full bg-slate-950 border border-slate-700 rounded-lg px-3 py-1.5 font-mono text-xs text-white outline-none focus:border-blue-500"
                    />
                  </div>

                  <div className="p-2.5 rounded-lg bg-slate-950/40 border border-slate-800 text-[11px] space-y-1.5">
                    <div className="text-slate-300 font-semibold">Standard Access Rights:</div>
                    <div className="grid grid-cols-3 gap-2 text-slate-400">
                      <div>Owner: Read, Write, Exec</div>
                      <div>Group: Read, Exec</div>
                      <div>Others: Read, Exec</div>
                    </div>
                  </div>
                </div>
              )}
            </div>

            {/* Modal Footer */}
            <div className="px-4 py-3 bg-slate-950/80 border-t border-slate-800 flex justify-end gap-2">
              <button
                onClick={() => setPropertiesItem(null)}
                className="px-3 py-1.5 rounded-xl bg-white/10 hover:bg-white/15 text-slate-300 text-xs"
              >
                Close
              </button>
              {propertiesTab === 'permissions' && (
                <button
                  onClick={handleSavePermissions}
                  className="px-4 py-1.5 rounded-xl bg-blue-600 hover:bg-blue-500 text-white font-medium text-xs shadow-md shadow-blue-600/30"
                >
                  Apply chmod
                </button>
              )}
            </div>
          </div>
        </div>
      )}
    </div>
  );
};
