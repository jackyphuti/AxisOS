import React, { useState, useEffect } from 'react';
import {
  ChevronLeft,
  ChevronRight,
  RotateCw,
  Search,
  Lock,
  Share2,
  Plus,
  X,
  Globe,
  Star,
  ExternalLink,
  FileText,
  Mail,
  Code2,
  Play,
  MapPin,
  Check,
  Cpu,
  Compass,
} from 'lucide-react';
import { systemService } from '../../services/systemService';
import { useWindowManager } from '../../context/WindowManagerContext';

interface Tab {
  id: string;
  title: string;
  url: string;
  inputUrl: string;
  history: string[];
  historyIdx: number;
  isLoading: boolean;
}

interface FavoriteItem {
  id: string;
  label: string;
  icon: React.ComponentType<{ className?: string; strokeWidth?: number }>;
  bg: string;
  color: string;
  url: string;
}

const FAVORITES: FavoriteItem[] = [
  {
    id: 'docs',
    label: 'Docs',
    icon: FileText,
    bg: 'bg-[#FFF2D6]',
    color: 'text-[#D97706]',
    url: 'https://wayland.freedesktop.org/docs/html/',
  },
  {
    id: 'mail',
    label: 'Mail',
    icon: Mail,
    bg: 'bg-[#E2F7E7]',
    color: 'text-[#16A34A]',
    url: 'https://mail.google.com',
  },
  {
    id: 'code',
    label: 'Code',
    icon: Code2,
    bg: 'bg-[#EBE9FD]',
    color: 'text-[#6366F1]',
    url: 'https://github.com',
  },
  {
    id: 'video',
    label: 'Video',
    icon: Play,
    bg: 'bg-[#FCE7F0]',
    color: 'text-[#E11D48]',
    url: 'https://youtube.com',
  },
  {
    id: 'maps',
    label: 'Maps',
    icon: MapPin,
    bg: 'bg-[#E1F0FF]',
    color: 'text-[#007AFF]',
    url: 'https://www.openstreetmap.org',
  },
];

export const BrowserApp: React.FC = () => {
  const { closeWindow, minimizeWindow, toggleMaximizeWindow, windows } = useWindowManager();
  const currentWindow = windows.find((w) => w.appId === 'browser');

  const [tabs, setTabs] = useState<Tab[]>([
    {
      id: 'tab-1',
      title: 'New tab',
      url: 'about:home',
      inputUrl: '',
      history: ['about:home'],
      historyIdx: 0,
      isLoading: false,
    },
    {
      id: 'tab-2',
      title: 'Wayland docs',
      url: 'https://wayland.freedesktop.org/docs/html/',
      inputUrl: 'https://wayland.freedesktop.org/docs/html/',
      history: ['https://wayland.freedesktop.org/docs/html/'],
      historyIdx: 0,
      isLoading: false,
    },
  ]);
  const [activeTabId, setActiveTabId] = useState<string>('tab-1');
  const [bookmarks, setBookmarks] = useState<string[]>(() => {
    try {
      const saved = localStorage.getItem('axis_browser_bookmarks');
      return saved
        ? JSON.parse(saved)
        : ['https://wayland.freedesktop.org/docs/html/', 'https://github.com'];
    } catch {
      return ['https://wayland.freedesktop.org/docs/html/', 'https://github.com'];
    }
  });

  const [searchQuery, setSearchQuery] = useState('');
  const [copiedNotification, setCopiedNotification] = useState(false);
  const [isNativeLaunching, setIsNativeLaunching] = useState(false);
  const [isHoveringControls, setIsHoveringControls] = useState(false);

  useEffect(() => {
    try {
      localStorage.setItem('axis_browser_bookmarks', JSON.stringify(bookmarks));
    } catch {}
  }, [bookmarks]);

  const activeTab = tabs.find((t) => t.id === activeTabId) || tabs[0];

  const updateActiveTab = (patch: Partial<Tab>) => {
    setTabs((prev) =>
      prev.map((t) => (t.id === activeTabId ? { ...t, ...patch } : t))
    );
  };

  const navigateTo = (rawUrl: string) => {
    let target = rawUrl.trim();
    if (!target) return;

    if (target === 'about:home' || target === 'home') {
      updateActiveTab({
        url: 'about:home',
        inputUrl: '',
        title: 'New tab',
        history: [...activeTab.history.slice(0, activeTab.historyIdx + 1), 'about:home'],
        historyIdx: activeTab.historyIdx + 1,
        isLoading: false,
      });
      return;
    }

    if (!target.startsWith('http://') && !target.startsWith('https://')) {
      if (target.includes('.') && !target.includes(' ')) {
        target = `https://${target}`;
      } else {
        target = `https://duckduckgo.com/?q=${encodeURIComponent(target)}`;
      }
    }

    let pageTitle = target.replace(/^https?:\/\//, '').split('/')[0];

    updateActiveTab({
      url: target,
      inputUrl: target,
      title: pageTitle,
      history: [...activeTab.history.slice(0, activeTab.historyIdx + 1), target],
      historyIdx: activeTab.historyIdx + 1,
      isLoading: true,
    });

    setTimeout(() => {
      updateActiveTab({ isLoading: false });
    }, 600);
  };

  const handleCreateTab = () => {
    const newId = `tab-${Date.now()}`;
    const newTab: Tab = {
      id: newId,
      title: 'New tab',
      url: 'about:home',
      inputUrl: '',
      history: ['about:home'],
      historyIdx: 0,
      isLoading: false,
    };
    setTabs((prev) => [...prev, newTab]);
    setActiveTabId(newId);
  };

  const handleCloseTab = (id: string, e: React.MouseEvent) => {
    e.stopPropagation();
    if (tabs.length === 1) {
      updateActiveTab({
        url: 'about:home',
        inputUrl: '',
        title: 'New tab',
        history: ['about:home'],
        historyIdx: 0,
        isLoading: false,
      });
      return;
    }

    const filtered = tabs.filter((t) => t.id !== id);
    setTabs(filtered);
    if (activeTabId === id) {
      setActiveTabId(filtered[filtered.length - 1].id);
    }
  };

  const handleBack = () => {
    if (activeTab.historyIdx > 0) {
      const nextIdx = activeTab.historyIdx - 1;
      const prevUrl = activeTab.history[nextIdx];
      updateActiveTab({
        historyIdx: nextIdx,
        url: prevUrl,
        inputUrl: prevUrl === 'about:home' ? '' : prevUrl,
        title: prevUrl === 'about:home' ? 'New tab' : prevUrl.replace(/^https?:\/\//, '').split('/')[0],
      });
    }
  };

  const handleForward = () => {
    if (activeTab.historyIdx < activeTab.history.length - 1) {
      const nextIdx = activeTab.historyIdx + 1;
      const nextUrl = activeTab.history[nextIdx];
      updateActiveTab({
        historyIdx: nextIdx,
        url: nextUrl,
        inputUrl: nextUrl === 'about:home' ? '' : nextUrl,
        title: nextUrl === 'about:home' ? 'New tab' : nextUrl.replace(/^https?:\/\//, '').split('/')[0],
      });
    }
  };

  const handleReload = () => {
    updateActiveTab({ isLoading: true });
    setTimeout(() => updateActiveTab({ isLoading: false }), 500);
  };

  const toggleBookmark = () => {
    if (activeTab.url === 'about:home') return;
    if (bookmarks.includes(activeTab.url)) {
      setBookmarks((prev) => prev.filter((b) => b !== activeTab.url));
    } else {
      setBookmarks((prev) => [...prev, activeTab.url]);
    }
  };

  const handleOpenNativeChromium = async () => {
    setIsNativeLaunching(true);
    const targetUrl = activeTab.url === 'about:home' ? 'https://duckduckgo.com' : activeTab.url;
    await systemService.openInNativeChromium(targetUrl);
    setTimeout(() => setIsNativeLaunching(false), 1500);
  };

  const isBookmarked = bookmarks.includes(activeTab.url);

  const getEmbedUrl = (raw: string) => {
    if (raw === 'about:home') return 'about:blank';
    return raw;
  };

  return (
    <div className="flex flex-col h-full w-full bg-[#FFFFFF] text-[#1C1C1E] select-none overflow-hidden font-sans">
      {/* ======================================================== */}
      {/* TOP TAB STRIP: Traffic Lights + Native Chrome Tabs        */}
      {/* ======================================================== */}
      <div
        data-window-drag
        className="h-10 px-3 pt-1.5 flex items-center bg-[#F7F7F9] border-b border-[#EAEAEB] shrink-0 overflow-x-auto"
      >
        {/* Left: Window Traffic Lights */}
        <div
          className="flex items-center space-x-2 mr-3 shrink-0"
          onMouseEnter={() => setIsHoveringControls(true)}
          onMouseLeave={() => setIsHoveringControls(false)}
        >
          <button
            onClick={() => currentWindow && closeWindow(currentWindow.id)}
            className="w-3 h-3 rounded-full bg-[#FF5F56] border border-[#E0443E] flex items-center justify-center cursor-pointer transition-transform active:scale-90"
            title="Close"
          >
            <span
              className={`text-[8px] font-black text-rose-950 leading-none ${
                isHoveringControls ? 'opacity-100' : 'opacity-0'
              }`}
            >
              ×
            </span>
          </button>
          <button
            onClick={() => currentWindow && minimizeWindow(currentWindow.id)}
            className="w-3 h-3 rounded-full bg-[#FFBD2E] border border-[#DEA123] flex items-center justify-center cursor-pointer transition-transform active:scale-90"
            title="Minimize"
          >
            <span
              className={`text-[9px] font-black text-amber-950 leading-none -translate-y-0.5 ${
                isHoveringControls ? 'opacity-100' : 'opacity-0'
              }`}
            >
              –
            </span>
          </button>
          <button
            onClick={() => currentWindow && toggleMaximizeWindow(currentWindow.id)}
            className="w-3 h-3 rounded-full bg-[#27C93F] border border-[#1AAB29] flex items-center justify-center cursor-pointer transition-transform active:scale-90"
            title="Zoom"
          >
            <span
              className={`text-[7px] font-black text-emerald-950 leading-none ${
                isHoveringControls ? 'opacity-100' : 'opacity-0'
              }`}
            >
              +
            </span>
          </button>
        </div>

        {/* Tab List */}
        <div className="flex items-center gap-1.5 flex-1 min-w-0">
          {tabs.map((tab) => {
            const isActive = tab.id === activeTabId;
            return (
              <div
                key={tab.id}
                onClick={() => setActiveTabId(tab.id)}
                className={`group flex items-center space-x-2 px-3.5 py-1.5 rounded-t-xl text-xs max-w-[200px] min-w-[120px] cursor-pointer transition-all border-t border-x ${
                  isActive
                    ? 'bg-white border-[#EAEAEB] text-[#1C1C1E] font-medium shadow-2xs'
                    : 'bg-transparent border-transparent text-[#5C5C60] hover:bg-black/5 hover:text-[#1C1C1E]'
                }`}
              >
                <div className="w-4 h-4 rounded-full bg-[#E1F0FF] text-[#007AFF] flex items-center justify-center text-[9px] shrink-0 font-bold">
                  <Globe className="w-2.5 h-2.5" strokeWidth={2} />
                </div>
                <span className="truncate flex-1 text-xs">{tab.title}</span>
                <button
                  onClick={(e) => handleCloseTab(tab.id, e)}
                  className="opacity-0 group-hover:opacity-100 p-0.5 rounded-full hover:bg-black/10 text-[#8E8E93] hover:text-[#1C1C1E] transition-opacity"
                >
                  <X className="w-3 h-3" strokeWidth={2} />
                </button>
              </div>
            );
          })}

          <button
            onClick={handleCreateTab}
            className="p-1 rounded-lg text-[#8E8E93] hover:text-[#1C1C1E] hover:bg-black/5 transition-colors shrink-0"
            title="New Tab"
          >
            <Plus className="w-3.5 h-3.5" strokeWidth={2} />
          </button>
        </div>
      </div>

      {/* ======================================================== */}
      {/* NAVIGATION & ADDRESS BAR                                 */}
      {/* ======================================================== */}
      <div className="h-11 px-3 flex items-center gap-2 bg-white border-b border-[#EAEAEB] shrink-0">
        <div className="flex items-center space-x-0.5">
          <button
            onClick={handleBack}
            disabled={activeTab.historyIdx === 0}
            className={`p-1.5 rounded-lg transition-colors ${
              activeTab.historyIdx === 0 ? 'text-[#D1D1D6]' : 'text-[#5C5C60] hover:bg-black/5'
            }`}
            title="Back"
          >
            <ChevronLeft className="w-4 h-4" strokeWidth={1.8} />
          </button>
          <button
            onClick={handleForward}
            disabled={activeTab.historyIdx >= activeTab.history.length - 1}
            className={`p-1.5 rounded-lg transition-colors ${
              activeTab.historyIdx >= activeTab.history.length - 1 ? 'text-[#D1D1D6]' : 'text-[#5C5C60] hover:bg-black/5'
            }`}
            title="Forward"
          >
            <ChevronRight className="w-4 h-4" strokeWidth={1.8} />
          </button>
          <button
            onClick={handleReload}
            className="p-1.5 rounded-lg text-[#5C5C60] hover:bg-black/5 transition-colors"
            title="Reload"
          >
            <RotateCw className={`w-3.5 h-3.5 ${activeTab.isLoading ? 'animate-spin text-[#007AFF]' : ''}`} strokeWidth={1.8} />
          </button>
        </div>

        {/* Omnibar / Address bar */}
        <form
          onSubmit={(e) => {
            e.preventDefault();
            navigateTo(activeTab.inputUrl);
          }}
          className="flex-1 max-w-xl mx-auto relative flex items-center"
        >
          <div className="w-full flex items-center px-3.5 py-1.5 bg-[#F0F1F4] rounded-full border border-transparent focus-within:border-black/10 focus-within:bg-white transition-all text-xs shadow-2xs">
            {activeTab.url.startsWith('https://') ? (
              <Lock className="w-3.5 h-3.5 text-[#34C759] mr-2 shrink-0" strokeWidth={2} />
            ) : (
              <Search className="w-3.5 h-3.5 text-[#8E8E93] mr-2 shrink-0" strokeWidth={2} />
            )}
            <input
              type="text"
              value={activeTab.inputUrl}
              onChange={(e) => updateActiveTab({ inputUrl: e.target.value })}
              className="w-full bg-transparent outline-none text-[#1C1C1E] text-xs font-sans placeholder-[#8E8E93]"
              placeholder="Search or enter website address..."
            />
            {activeTab.url !== 'about:home' && (
              <button
                type="button"
                onClick={toggleBookmark}
                className="p-0.5 rounded text-[#8E8E93] hover:text-[#BA7517] transition-colors"
                title="Bookmark page"
              >
                <Star className={`w-3.5 h-3.5 ${isBookmarked ? 'fill-[#BA7517] text-[#BA7517]' : ''}`} strokeWidth={1.8} />
              </button>
            )}
          </div>
        </form>

        {/* Right Actions */}
        <div className="flex items-center space-x-1.5">
          <button
            onClick={handleOpenNativeChromium}
            disabled={isNativeLaunching}
            className="px-3 py-1 rounded-full bg-[#F0F2F5] hover:bg-[#E5E9F0] text-[#007AFF] font-semibold text-xs flex items-center gap-1.5 transition-colors active:scale-95"
            title="Launch full native Chromium browser window on desktop"
          >
            <Cpu className="w-3.5 h-3.5" strokeWidth={2} />
            <span>{isNativeLaunching ? 'Opening...' : 'Chromium'}</span>
          </button>

          {activeTab.url !== 'about:home' && (
            <a
              href={activeTab.url}
              target="_blank"
              rel="noreferrer"
              className="p-1.5 rounded-lg text-[#5C5C60] hover:text-[#1C1C1E] hover:bg-black/5 transition-colors"
              title="Open externally in host browser"
            >
              <ExternalLink className="w-4 h-4" strokeWidth={1.8} />
            </a>
          )}

          <button
            onClick={() => {
              if (activeTab.url !== 'about:home') {
                navigator.clipboard.writeText(activeTab.url);
                setCopiedNotification(true);
                setTimeout(() => setCopiedNotification(false), 1500);
              }
            }}
            className="p-1.5 rounded-lg text-[#5C5C60] hover:text-[#1C1C1E] hover:bg-black/5 transition-colors relative"
            title="Share / Copy Link"
          >
            {copiedNotification ? (
              <Check className="w-4 h-4 text-[#34C759]" strokeWidth={2} />
            ) : (
              <Share2 className="w-4 h-4" strokeWidth={1.8} />
            )}
          </button>
        </div>
      </div>

      {/* ======================================================== */}
      {/* MAIN VIEW AREA                                           */}
      {/* ======================================================== */}
      <div className="flex-1 relative overflow-hidden bg-white flex flex-col">
        {activeTab.isLoading && (
          <div className="absolute top-0 left-0 right-0 h-0.5 bg-[#007AFF] animate-pulse z-20" />
        )}

        {/* If Home Screen (about:home), show Pixel-Perfect Mockup 2 */}
        {activeTab.url === 'about:home' ? (
          <div className="flex-1 flex flex-col items-center justify-center p-8 overflow-y-auto">
            {/* Centered pastel blue squircle icon */}
            <div className="w-16 h-16 rounded-2xl bg-[#E1F0FF] text-[#007AFF] flex items-center justify-center shadow-2xs mb-4">
              <Globe className="w-8 h-8" strokeWidth={1.75} />
            </div>

            {/* Title */}
            <h1 className="text-2xl font-bold text-[#1C1C1E] tracking-tight mb-4">
              Axis Browser
            </h1>

            {/* Centered Search Pill */}
            <form
              onSubmit={(e) => {
                e.preventDefault();
                navigateTo(searchQuery);
              }}
              className="w-full max-w-md relative"
            >
              <div className="w-full h-10 px-4 rounded-full bg-[#F0F1F4] border border-transparent focus-within:border-black/10 focus-within:bg-white flex items-center gap-2.5 text-xs text-[#1C1C1E] shadow-2xs transition-all">
                <Search className="w-4 h-4 text-[#8E8E93] shrink-0" strokeWidth={2} />
                <input
                  type="text"
                  value={searchQuery}
                  onChange={(e) => setSearchQuery(e.target.value)}
                  placeholder="Search the web or enter address"
                  className="w-full bg-transparent outline-none text-xs placeholder-[#8E8E93]"
                />
              </div>
            </form>

            {/* Favorites Section */}
            <div className="mt-10 flex flex-col items-center">
              <div className="text-xs font-semibold text-[#8E8E93] uppercase tracking-wider mb-5">
                Favorites
              </div>

              {/* 5 Pastel Squircle Tiles */}
              <div className="flex items-center gap-6">
                {FAVORITES.map((fav) => {
                  const Icon = fav.icon;
                  return (
                    <button
                      key={fav.id}
                      onClick={() => navigateTo(fav.url)}
                      className="flex flex-col items-center gap-2 group transition-transform active:scale-95 cursor-pointer"
                    >
                      <div
                        className={`w-14 h-14 rounded-2xl ${fav.bg} ${fav.color} flex items-center justify-center shadow-2xs group-hover:shadow-xs group-hover:scale-105 transition-all`}
                      >
                        <Icon className="w-6 h-6" strokeWidth={1.8} />
                      </div>
                      <span className="text-xs font-medium text-[#5C5C60] group-hover:text-[#1C1C1E] transition-colors">
                        {fav.label}
                      </span>
                    </button>
                  );
                })}
              </div>
            </div>
          </div>
        ) : (
          /* Live Web Frame */
          <div className="flex-1 w-full h-full relative bg-white">
            <iframe
              src={getEmbedUrl(activeTab.url)}
              title={activeTab.title}
              className="w-full h-full border-none"
              sandbox="allow-scripts allow-same-origin allow-forms allow-popups allow-downloads allow-modals"
            />
          </div>
        )}
      </div>
    </div>
  );
};
