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
    id: 'google',
    label: 'Google',
    icon: Search,
    bg: 'bg-[#121814]',
    color: 'text-[#87cf3e]',
    url: 'https://www.google.com/webhp?igu=1',
  },
  {
    id: 'docs',
    label: 'Debian / Docs',
    icon: FileText,
    bg: 'bg-[#121814]',
    color: 'text-[#87cf3e]',
    url: 'https://www.debian.org/doc/',
  },
  {
    id: 'packages',
    label: 'Packages',
    icon: Code2,
    bg: 'bg-[#121814]',
    color: 'text-[#87cf3e]',
    url: 'https://packages.debian.org/',
  },
  {
    id: 'code',
    label: 'GitHub',
    icon: Code2,
    bg: 'bg-[#121814]',
    color: 'text-[#87cf3e]',
    url: 'https://github.com',
  },
  {
    id: 'video',
    label: 'YouTube',
    icon: Play,
    bg: 'bg-[#121814]',
    color: 'text-[#87cf3e]',
    url: 'https://youtube.com',
  },
  {
    id: 'maps',
    label: 'Maps',
    icon: MapPin,
    bg: 'bg-[#121814]',
    color: 'text-[#87cf3e]',
    url: 'https://maps.google.com',
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
        : ['https://www.google.com/webhp?igu=1', 'https://wayland.freedesktop.org/docs/html/', 'https://github.com'];
    } catch {
      return ['https://www.google.com/webhp?igu=1', 'https://wayland.freedesktop.org/docs/html/', 'https://github.com'];
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

  const navigateTo = (
    rawUrl: string,
    searchCategory?: 'all' | 'images' | 'news' | 'videos' | 'maps',
    lucky?: boolean
  ) => {
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
        const q = encodeURIComponent(target);
        if (lucky) {
          target = `https://www.google.com/search?igu=1&btnI=1&q=${q}`;
        } else if (searchCategory === 'images') {
          target = `https://www.google.com/search?igu=1&tbm=isch&q=${q}`;
        } else if (searchCategory === 'news') {
          target = `https://www.google.com/search?igu=1&tbm=nws&q=${q}`;
        } else if (searchCategory === 'videos') {
          target = `https://www.google.com/search?igu=1&tbm=vid&q=${q}`;
        } else if (searchCategory === 'maps') {
          target = `https://maps.google.com/maps?q=${q}`;
        } else {
          target = `https://www.google.com/search?igu=1&q=${q}`;
        }
      }
    }

    let pageTitle = target.replace(/^https?:\/\//, '').split('/')[0];
    if (target.includes('google.com/search')) {
      const qMatch = target.match(/[?&]q=([^&]+)/);
      if (qMatch) {
        pageTitle = `${decodeURIComponent(qMatch[1])} - Google Search`;
      } else {
        pageTitle = 'Google Search';
      }
    } else if (target.includes('google.com')) {
      pageTitle = 'Google';
    }

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
    const targetUrl = activeTab.url === 'about:home' ? 'https://www.google.com' : activeTab.url;
    await systemService.openInNativeChromium(targetUrl);
    setTimeout(() => setIsNativeLaunching(false), 1500);
  };

  const isBookmarked = bookmarks.includes(activeTab.url);

  const getEmbedUrl = (raw: string) => {
    if (raw === 'about:home') return 'about:blank';
    if (raw.includes('google.com') && !raw.includes('igu=1')) {
      const separator = raw.includes('?') ? '&' : '?';
      return `${raw}${separator}igu=1`;
    }
    return raw;
  };

  return (
    <div className="flex flex-col h-full w-full bg-[#0a0e0b] text-slate-100 select-none overflow-hidden font-sans">
      {/* ======================================================== */}
      {/* TOP TAB STRIP: Window Controls + Tabs                     */}
      {/* ======================================================== */}
      <div
        data-window-drag
        className="h-10 px-3 pt-1.5 flex items-center bg-[#0f1411] border-b border-white/10 shrink-0 overflow-x-auto"
      >
        {/* Left: Window Controls */}
        <div
          className="flex items-center space-x-2 mr-3 shrink-0"
          onMouseEnter={() => setIsHoveringControls(true)}
          onMouseLeave={() => setIsHoveringControls(false)}
        >
          <button
            onClick={() => currentWindow && closeWindow(currentWindow.id)}
            className="w-3 h-3 rounded-full bg-rose-500/80 border border-rose-600 flex items-center justify-center cursor-pointer transition-transform active:scale-90"
            title="Close"
          >
            <span
              className={`text-[8px] font-black text-black leading-none ${
                isHoveringControls ? 'opacity-100' : 'opacity-0'
              }`}
            >
              ×
            </span>
          </button>
          <button
            onClick={() => currentWindow && minimizeWindow(currentWindow.id)}
            className="w-3 h-3 rounded-full bg-amber-500/80 border border-amber-600 flex items-center justify-center cursor-pointer transition-transform active:scale-90"
            title="Minimize"
          >
            <span
              className={`text-[9px] font-black text-black leading-none -translate-y-0.5 ${
                isHoveringControls ? 'opacity-100' : 'opacity-0'
              }`}
            >
              –
            </span>
          </button>
          <button
            onClick={() => currentWindow && toggleMaximizeWindow(currentWindow.id)}
            className="w-3 h-3 rounded-full bg-[#87cf3e] border border-emerald-600 flex items-center justify-center cursor-pointer transition-transform active:scale-90"
            title="Zoom"
          >
            <span
              className={`text-[7px] font-black text-black leading-none ${
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
                    ? 'bg-[#18221b] border-white/10 text-white font-semibold border-t-2 !border-t-[#87cf3e] shadow-sm'
                    : 'bg-transparent border-transparent text-slate-400 hover:bg-white/5 hover:text-white'
                }`}
              >
                <div className="w-4 h-4 rounded-full bg-[#87cf3e]/20 text-[#87cf3e] flex items-center justify-center text-[9px] shrink-0 font-bold">
                  <Globe className="w-2.5 h-2.5" strokeWidth={2} />
                </div>
                <span className="truncate flex-1 text-xs">{tab.title}</span>
                <button
                  onClick={(e) => handleCloseTab(tab.id, e)}
                  className="opacity-0 group-hover:opacity-100 p-0.5 rounded-full hover:bg-white/10 text-slate-400 hover:text-white transition-opacity"
                >
                  <X className="w-3 h-3" strokeWidth={2} />
                </button>
              </div>
            );
          })}

          <button
            onClick={handleCreateTab}
            className="p-1 rounded-lg text-slate-400 hover:text-white hover:bg-white/5 transition-colors shrink-0"
            title="New Tab"
          >
            <Plus className="w-3.5 h-3.5" strokeWidth={2} />
          </button>
        </div>
      </div>

      {/* ======================================================== */}
      {/* NAVIGATION & ADDRESS BAR                                 */}
      {/* ======================================================== */}
      <div className="h-11 px-3 flex items-center gap-2 bg-[#121814] border-b border-white/10 shrink-0">
        <div className="flex items-center space-x-0.5">
          <button
            onClick={handleBack}
            disabled={activeTab.historyIdx === 0}
            className={`p-1.5 rounded-lg transition-colors ${
              activeTab.historyIdx === 0 ? 'text-slate-600' : 'text-slate-300 hover:bg-white/5'
            }`}
            title="Back"
          >
            <ChevronLeft className="w-4 h-4" strokeWidth={1.8} />
          </button>
          <button
            onClick={handleForward}
            disabled={activeTab.historyIdx >= activeTab.history.length - 1}
            className={`p-1.5 rounded-lg transition-colors ${
              activeTab.historyIdx >= activeTab.history.length - 1 ? 'text-slate-600' : 'text-slate-300 hover:bg-white/5'
            }`}
            title="Forward"
          >
            <ChevronRight className="w-4 h-4" strokeWidth={1.8} />
          </button>
          <button
            onClick={handleReload}
            className="p-1.5 rounded-lg text-slate-300 hover:bg-white/5 transition-colors"
            title="Reload"
          >
            <RotateCw className={`w-3.5 h-3.5 ${activeTab.isLoading ? 'animate-spin text-[#87cf3e]' : ''}`} strokeWidth={1.8} />
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
          <div className="w-full flex items-center px-3.5 py-1.5 bg-[#18221b] rounded-full border border-white/10 focus-within:border-[#87cf3e] transition-all text-xs shadow-inner">
            {activeTab.url.startsWith('https://') ? (
              <Lock className="w-3.5 h-3.5 text-[#87cf3e] mr-2 shrink-0" strokeWidth={2} />
            ) : (
              <Search className="w-3.5 h-3.5 text-slate-400 mr-2 shrink-0" strokeWidth={2} />
            )}
            <input
              type="text"
              value={activeTab.inputUrl}
              onChange={(e) => updateActiveTab({ inputUrl: e.target.value })}
              className="w-full bg-transparent outline-none text-white text-xs font-sans placeholder-slate-500"
              placeholder="Search or enter website address..."
            />
            {activeTab.url !== 'about:home' && (
              <button
                type="button"
                onClick={toggleBookmark}
                className="p-0.5 rounded text-slate-400 hover:text-[#87cf3e] transition-colors"
                title="Bookmark page"
              >
                <Star className={`w-3.5 h-3.5 ${isBookmarked ? 'fill-[#87cf3e] text-[#87cf3e]' : ''}`} strokeWidth={1.8} />
              </button>
            )}
          </div>
        </form>

        {/* Right Actions */}
        <div className="flex items-center space-x-1.5">
          <button
            onClick={handleOpenNativeChromium}
            disabled={isNativeLaunching}
            className="px-3 py-1 rounded-full bg-[#87cf3e] hover:bg-[#76bb33] text-black font-bold text-xs flex items-center gap-1.5 transition-colors active:scale-95 shadow-md shadow-[#87cf3e]/20"
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
              className="p-1.5 rounded-lg text-slate-300 hover:text-white hover:bg-white/5 transition-colors"
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
            className="p-1.5 rounded-lg text-slate-300 hover:text-white hover:bg-white/5 transition-colors relative"
            title="Share / Copy Link"
          >
            {copiedNotification ? (
              <Check className="w-4 h-4 text-[#87cf3e]" strokeWidth={2} />
            ) : (
              <Share2 className="w-4 h-4" strokeWidth={1.8} />
            )}
          </button>
        </div>
      </div>

      {/* ======================================================== */}
      {/* MAIN VIEW AREA                                           */}
      {/* ======================================================== */}
      <div className="flex-1 relative overflow-hidden bg-[#0a0e0b] flex flex-col">
        {activeTab.isLoading && (
          <div className="absolute top-0 left-0 right-0 h-0.5 bg-[#87cf3e] animate-pulse z-20" />
        )}

        {/* If Home Screen (about:home), show Search Home */}
        {activeTab.url === 'about:home' ? (
          <div className="flex-1 flex flex-col items-center justify-center p-8 overflow-y-auto">
            {/* Centered Green Squircle Icon */}
            <div className="w-16 h-16 rounded-2xl bg-[#18221b] text-[#87cf3e] border border-[#87cf3e]/30 flex items-center justify-center shadow-lg shadow-[#87cf3e]/10 mb-3">
              <Globe className="w-8 h-8" strokeWidth={1.75} />
            </div>

            {/* Title */}
            <h1 className="text-2xl font-bold text-white tracking-tight mb-1">
              Axis Browser
            </h1>
            <div className="flex items-center gap-1.5 mb-5 text-xs text-slate-400">
              <span>Fast, private, and lightweight web explorer</span>
            </div>

            {/* Centered Search Pill */}
            <form
              onSubmit={(e) => {
                e.preventDefault();
                navigateTo(searchQuery);
              }}
              className="w-full max-w-lg relative"
            >
              <div className="w-full h-11 px-4 rounded-full bg-[#18221b] border border-white/15 focus-within:border-[#87cf3e] flex items-center gap-2.5 text-xs text-white shadow-inner transition-all">
                <Search className="w-4 h-4 text-[#87cf3e] shrink-0" strokeWidth={2} />
                <input
                  type="text"
                  value={searchQuery}
                  onChange={(e) => setSearchQuery(e.target.value)}
                  placeholder="Search the web or enter website address..."
                  className="w-full bg-transparent outline-none text-xs text-white placeholder-slate-500"
                  autoFocus
                />
                {searchQuery && (
                  <button
                    type="button"
                    onClick={() => setSearchQuery('')}
                    className="p-1 rounded-full hover:bg-white/10 text-slate-400 hover:text-white"
                  >
                    <X className="w-3.5 h-3.5" strokeWidth={2} />
                  </button>
                )}
              </div>

              {/* Action buttons */}
              <div className="flex items-center justify-center gap-3 mt-4">
                <button
                  type="submit"
                  className="px-5 py-2 rounded-xl bg-[#87cf3e] hover:bg-[#76bb33] text-black font-bold text-xs transition-colors cursor-pointer shadow-md shadow-[#87cf3e]/20 active:scale-95"
                >
                  Search
                </button>
                <button
                  type="button"
                  onClick={() => navigateTo(searchQuery, 'all', true)}
                  className="px-5 py-2 rounded-xl bg-[#18221b] hover:bg-[#233026] text-xs font-medium text-slate-200 border border-white/10 transition-colors cursor-pointer active:scale-95"
                >
                  Quick Open
                </button>
              </div>

              {/* Category Search Chips */}
              <div className="flex items-center justify-center gap-2 mt-3">
                {[
                  { label: 'All', cat: 'all' as const },
                  { label: 'Images', cat: 'images' as const },
                  { label: 'News', cat: 'news' as const },
                  { label: 'Videos', cat: 'videos' as const },
                  { label: 'Maps', cat: 'maps' as const },
                ].map((item) => (
                  <button
                    key={item.label}
                    type="button"
                    onClick={() => navigateTo(searchQuery || 'Google', item.cat)}
                    className="px-2.5 py-1 rounded-full text-[11px] font-medium text-slate-400 hover:text-[#87cf3e] hover:bg-white/5 transition-colors cursor-pointer"
                  >
                    {item.label}
                  </button>
                ))}
              </div>
            </form>

            {/* Favorites Section */}
            <div className="mt-8 flex flex-col items-center">
              <div className="text-xs font-semibold text-slate-400 uppercase tracking-wider mb-4">
                Favorites & Shortcuts
              </div>

              {/* Squircle Tiles */}
              <div className="flex items-center gap-5 flex-wrap justify-center max-w-xl">
                {FAVORITES.map((fav) => {
                  const Icon = fav.icon;
                  return (
                    <button
                      key={fav.id}
                      onClick={() => navigateTo(fav.url)}
                      className="flex flex-col items-center gap-2 group transition-transform active:scale-95 cursor-pointer"
                    >
                      <div
                        className="w-14 h-14 rounded-2xl bg-[#18221b] text-[#87cf3e] border border-[#87cf3e]/20 group-hover:border-[#87cf3e] group-hover:bg-[#202e24] flex items-center justify-center shadow-md group-hover:scale-105 transition-all"
                      >
                        <Icon className="w-6 h-6" strokeWidth={1.8} />
                      </div>
                      <span className="text-xs font-medium text-slate-300 group-hover:text-white transition-colors">
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
