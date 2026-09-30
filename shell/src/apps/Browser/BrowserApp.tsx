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
  Shield,
  Compass,
  Bookmark,
  Home,
  Sliders,
  Cpu,
  Terminal,
  Layers,
  Sparkles,
  Check,
} from 'lucide-react';
import { systemService } from '../../services/systemService';

interface Tab {
  id: string;
  title: string;
  url: string;
  inputUrl: string;
  history: string[];
  historyIdx: number;
  isLoading: boolean;
}

interface QuickSite {
  title: string;
  url: string;
  icon: string;
  bgGradient: string;
}

const QUICK_SITES: QuickSite[] = [
  { title: 'DuckDuckGo', url: 'https://duckduckgo.com', icon: '🦆', bgGradient: 'from-orange-500 to-amber-600' },
  { title: 'Google', url: 'https://www.google.com', icon: '🔍', bgGradient: 'from-blue-500 via-red-500 to-yellow-500' },
  { title: 'GitHub', url: 'https://github.com', icon: '🐙', bgGradient: 'from-gray-800 to-black' },
  { title: 'Wikipedia', url: 'https://www.wikipedia.org', icon: '📖', bgGradient: 'from-slate-600 to-slate-800' },
  { title: 'Linux Kernel', url: 'https://www.kernel.org', icon: '🐧', bgGradient: 'from-amber-500 to-yellow-600' },
  { title: 'Hacker News', url: 'https://news.ycombinator.com', icon: '⚡', bgGradient: 'from-orange-600 to-amber-700' },
  { title: 'Reddit', url: 'https://www.reddit.com', icon: '🤖', bgGradient: 'from-orange-500 to-rose-600' },
  { title: 'YouTube', url: 'https://www.youtube.com', icon: '▶️', bgGradient: 'from-red-600 to-red-800' },
];

export const BrowserApp: React.FC = () => {
  const [tabs, setTabs] = useState<Tab[]>([
    {
      id: 'tab-1',
      title: 'Axis Browser',
      url: 'about:home',
      inputUrl: '',
      history: ['about:home'],
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
        : ['https://duckduckgo.com', 'https://github.com', 'https://www.kernel.org'];
    } catch {
      return ['https://duckduckgo.com', 'https://github.com', 'https://www.kernel.org'];
    }
  });
  const [searchEngine, setSearchEngine] = useState<'duckduckgo' | 'google' | 'bing'>('duckduckgo');
  const [copiedNotification, setCopiedNotification] = useState(false);
  const [isNativeLaunching, setIsNativeLaunching] = useState(false);

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
        title: 'Axis Browser',
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
        if (searchEngine === 'google') {
          target = `https://www.google.com/search?q=${encodeURIComponent(target)}`;
        } else if (searchEngine === 'bing') {
          target = `https://www.bing.com/search?q=${encodeURIComponent(target)}`;
        } else {
          target = `https://duckduckgo.com/?q=${encodeURIComponent(target)}`;
        }
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
    }, 700);
  };

  const handleCreateTab = () => {
    const newId = `tab-${Date.now()}`;
    const newTab: Tab = {
      id: newId,
      title: 'Axis Browser',
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
        title: 'Axis Browser',
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
        title: prevUrl === 'about:home' ? 'Axis Browser' : prevUrl.replace(/^https?:\/\//, '').split('/')[0],
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
        title: nextUrl === 'about:home' ? 'Axis Browser' : nextUrl.replace(/^https?:\/\//, '').split('/')[0],
      });
    }
  };

  const handleReload = () => {
    updateActiveTab({ isLoading: true });
    setTimeout(() => updateActiveTab({ isLoading: false }), 600);
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

  // Compute embed URL (renders directly in sandbox iframe or blank for home)
  const getEmbedUrl = (raw: string) => {
    if (raw === 'about:home') return 'about:blank';
    return raw;
  };

  return (
    <div className="flex flex-col h-full w-full bg-slate-950 text-slate-100 select-none overflow-hidden font-sans">
      {/* Top Tab Strip */}
      <div className="h-9 px-2 pt-1 flex items-center gap-1 bg-slate-900 border-b border-white/10 overflow-x-auto">
        {tabs.map((tab) => {
          const isActive = tab.id === activeTabId;
          return (
            <div
              key={tab.id}
              onClick={() => setActiveTabId(tab.id)}
              className={`group flex items-center space-x-2 px-3 py-1.5 rounded-t-xl text-xs max-w-[190px] min-w-[120px] cursor-pointer transition-all border-t border-x ${
                isActive
                  ? 'bg-slate-950 border-white/15 text-white font-medium shadow-sm'
                  : 'bg-white/5 border-transparent text-slate-400 hover:bg-white/10 hover:text-slate-200'
              }`}
            >
              <div className="w-3.5 h-3.5 rounded-full bg-gradient-to-tr from-cyan-400 to-blue-600 flex items-center justify-center text-[8px] text-white shrink-0 font-black">
                A
              </div>
              <span className="truncate flex-1 text-[11px]">{tab.title}</span>
              <button
                onClick={(e) => handleCloseTab(tab.id, e)}
                className="opacity-0 group-hover:opacity-100 p-0.5 rounded-full hover:bg-white/20 text-slate-400 hover:text-white transition-opacity"
              >
                <X className="w-3 h-3" />
              </button>
            </div>
          );
        })}

        <button
          onClick={handleCreateTab}
          className="p-1 rounded-lg text-slate-400 hover:text-white hover:bg-white/10 transition-colors"
          title="New Tab"
        >
          <Plus className="w-4 h-4" />
        </button>
      </div>

      {/* Navigation & Address Bar */}
      <div className="h-11 px-3 flex items-center gap-2 bg-slate-950/90 border-b border-white/10">
        <div className="flex items-center space-x-0.5">
          <button
            onClick={handleBack}
            disabled={activeTab.historyIdx === 0}
            className={`p-1.5 rounded-lg transition-colors ${
              activeTab.historyIdx === 0 ? 'text-slate-600' : 'text-slate-300 hover:bg-white/10'
            }`}
          >
            <ChevronLeft className="w-4 h-4" />
          </button>
          <button
            onClick={handleForward}
            disabled={activeTab.historyIdx >= activeTab.history.length - 1}
            className={`p-1.5 rounded-lg transition-colors ${
              activeTab.historyIdx >= activeTab.history.length - 1 ? 'text-slate-600' : 'text-slate-300 hover:bg-white/10'
            }`}
          >
            <ChevronRight className="w-4 h-4" />
          </button>
          <button
            onClick={handleReload}
            className="p-1.5 rounded-lg text-slate-300 hover:bg-white/10 transition-colors"
          >
            <RotateCw className={`w-3.5 h-3.5 ${activeTab.isLoading ? 'animate-spin text-cyan-400' : ''}`} />
          </button>
          <button
            onClick={() => navigateTo('about:home')}
            className="p-1.5 rounded-lg text-slate-300 hover:bg-white/10 transition-colors"
            title="Home"
          >
            <Home className="w-3.5 h-3.5" />
          </button>
        </div>

        {/* Omnibar */}
        <form
          onSubmit={(e) => {
            e.preventDefault();
            navigateTo(activeTab.inputUrl);
          }}
          className="flex-1 max-w-2xl mx-auto relative flex items-center"
        >
          <div className="w-full flex items-center px-3 py-1.5 bg-slate-900 border border-white/10 focus-within:border-cyan-500 rounded-xl transition-all text-xs">
            {activeTab.url.startsWith('https://') ? (
              <Lock className="w-3.5 h-3.5 text-emerald-400 mr-2 shrink-0" />
            ) : (
              <Search className="w-3.5 h-3.5 text-slate-400 mr-2 shrink-0" />
            )}
            <input
              type="text"
              value={activeTab.inputUrl}
              onChange={(e) => updateActiveTab({ inputUrl: e.target.value })}
              className="w-full bg-transparent outline-none text-slate-200 text-xs font-sans placeholder-slate-500"
              placeholder="Search or enter web address (Chromium Engine)..."
            />
            {activeTab.url !== 'about:home' && (
              <button
                type="button"
                onClick={toggleBookmark}
                className="p-1 rounded text-slate-400 hover:text-amber-400 transition-colors"
                title="Bookmark page"
              >
                <Star className={`w-3.5 h-3.5 ${isBookmarked ? 'fill-amber-400 text-amber-400' : ''}`} />
              </button>
            )}
          </div>
        </form>

        {/* Right Action Buttons */}
        <div className="flex items-center space-x-1.5">
          {/* Launch Native Chromium button */}
          <button
            onClick={handleOpenNativeChromium}
            disabled={isNativeLaunching}
            className="px-2.5 py-1 rounded-lg bg-cyan-500/10 hover:bg-cyan-500/20 text-cyan-300 border border-cyan-500/30 text-[11px] font-medium flex items-center gap-1.5 transition-colors"
            title="Launch full native Chromium browser window on desktop"
          >
            <div className="w-2.5 h-2.5 rounded-full bg-cyan-400 animate-pulse" />
            <span>{isNativeLaunching ? 'Opening Chromium...' : 'Native Chromium'}</span>
          </button>

          {activeTab.url !== 'about:home' && (
            <a
              href={activeTab.url}
              target="_blank"
              rel="noreferrer"
              className="p-1.5 rounded-lg text-slate-400 hover:text-white hover:bg-white/10 transition-colors"
              title="Open externally in host browser"
            >
              <ExternalLink className="w-4 h-4" />
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
            className="p-1.5 rounded-lg text-slate-400 hover:text-white hover:bg-white/10 transition-colors relative"
            title="Share / Copy Link"
          >
            {copiedNotification ? <Check className="w-4 h-4 text-emerald-400" /> : <Share2 className="w-4 h-4" />}
          </button>
        </div>
      </div>

      {/* Bookmarks Bar */}
      {bookmarks.length > 0 && (
        <div className="h-7 px-3 flex items-center space-x-3 bg-slate-900/60 border-b border-white/5 text-[11px] text-slate-400 overflow-x-auto">
          <Bookmark className="w-3 h-3 text-cyan-400 shrink-0" />
          {bookmarks.map((bm) => {
            const label = bm.replace(/^https?:\/\//, '').replace(/^www\./, '').split('/')[0];
            return (
              <button
                key={bm}
                onClick={() => navigateTo(bm)}
                className="hover:text-white transition-colors truncate max-w-[130px]"
                title={bm}
              >
                {label}
              </button>
            );
          })}
        </div>
      )}

      {/* Main View Area */}
      <div className="flex-1 relative overflow-hidden bg-slate-950 flex flex-col">
        {activeTab.isLoading && (
          <div className="absolute top-0 left-0 right-0 h-0.5 bg-gradient-to-r from-cyan-400 to-blue-600 animate-pulse z-20" />
        )}

        {/* If New Tab (about:home), show Speed Dial Dashboard */}
        {activeTab.url === 'about:home' ? (
          <div className="flex-1 flex flex-col items-center justify-center p-8 overflow-y-auto">
            {/* Axis Browser Chromium Badge Logo */}
            <div className="relative mb-4">
              <div className="w-16 h-16 rounded-2xl bg-gradient-to-tr from-cyan-500 via-blue-600 to-indigo-700 flex items-center justify-center text-white shadow-xl shadow-cyan-950/60">
                <Globe className="w-8 h-8 text-white" />
              </div>
              <div className="absolute -bottom-1 -right-1 px-1.5 py-0.5 rounded-full bg-cyan-400 text-slate-950 font-black text-[9px] shadow-sm">
                CR
              </div>
            </div>

            <h1 className="text-2xl font-black text-slate-100 tracking-tight">Axis Browser</h1>
            <p className="text-xs text-slate-400 mt-1 max-w-sm text-center">
              Powered by Chromium (Blink & V8) with hardware accelerated Wayland rasterization.
            </p>

            {/* Search Engine Selector Pills */}
            <div className="flex items-center gap-1.5 mt-4 p-1 rounded-xl bg-white/5 border border-white/10 text-[11px]">
              {(['duckduckgo', 'google', 'bing'] as const).map((eng) => (
                <button
                  key={eng}
                  onClick={() => setSearchEngine(eng)}
                  className={`px-2.5 py-1 rounded-lg capitalize transition-colors ${
                    searchEngine === eng
                      ? 'bg-cyan-500 text-slate-950 font-bold'
                      : 'text-slate-400 hover:text-white'
                  }`}
                >
                  {eng}
                </button>
              ))}
            </div>

            {/* Quick Search Box */}
            <form
              onSubmit={(e) => {
                e.preventDefault();
                navigateTo(activeTab.inputUrl);
              }}
              className="w-full max-w-md mt-4 relative"
            >
              <input
                type="text"
                value={activeTab.inputUrl}
                onChange={(e) => updateActiveTab({ inputUrl: e.target.value })}
                placeholder={`Search with ${searchEngine} or enter a URL...`}
                className="w-full px-4 py-3 pl-11 rounded-2xl bg-slate-900 border border-white/10 text-xs text-slate-100 focus:outline-none focus:ring-2 focus:ring-cyan-500 shadow-xl"
              />
              <Search className="w-4 h-4 text-slate-400 absolute left-4 top-3.5" />
            </form>

            {/* Favorites Grid */}
            <div className="mt-8 w-full max-w-xl">
              <div className="text-[11px] font-semibold text-slate-400 uppercase tracking-wider mb-3 px-1 text-left">
                Top Sites & Bookmarks
              </div>
              <div className="grid grid-cols-4 gap-3">
                {QUICK_SITES.map((site) => (
                  <button
                    key={site.title}
                    onClick={() => navigateTo(site.url)}
                    className="p-3.5 rounded-2xl bg-white/5 hover:bg-white/10 border border-white/5 hover:border-white/15 flex flex-col items-center gap-2 group transition-all text-center"
                  >
                    <div
                      className={`w-11 h-11 rounded-xl bg-gradient-to-tr ${site.bgGradient} flex items-center justify-center text-lg shadow-md group-hover:scale-105 transition-transform`}
                    >
                      {site.icon}
                    </div>
                    <span className="text-xs font-medium text-slate-300 group-hover:text-white truncate max-w-full">
                      {site.title}
                    </span>
                  </button>
                ))}
              </div>
            </div>

            {/* App Store Install Notice & Native Launch */}
            <div className="mt-8 flex flex-col items-center gap-2.5">
              <button
                onClick={handleOpenNativeChromium}
                className="px-4 py-2 rounded-xl bg-gradient-to-r from-cyan-500 to-blue-600 text-white font-semibold text-xs shadow-lg shadow-cyan-500/20 hover:opacity-95 flex items-center gap-2 transition-all active:scale-95"
              >
                <Cpu className="w-4 h-4" />
                <span>Launch Native Chromium Window (Host Subsystem)</span>
              </button>
              <div className="flex items-center gap-2 text-[11px] text-slate-500 font-mono">
                <Shield className="w-3.5 h-3.5 text-emerald-400" />
                <span>Chromium Engine 128.0 • Wayland Native GPU Sandbox Active</span>
              </div>
            </div>
          </div>
        ) : (
          /* Live Web Frame with Proxy */
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

      {/* Chromium Engine Status Bar */}
      <div className="h-6 px-3 bg-slate-900 border-t border-white/10 flex items-center justify-between text-[10px] text-slate-400 font-mono">
        <div className="flex items-center space-x-2">
          <span className="inline-block w-2 h-2 rounded-full bg-emerald-400" />
          <span>Chromium Core (Blink/V8)</span>
          <span className="text-slate-600">•</span>
          <span>GPU Raster: Hardware Accelerated</span>
        </div>
        <div>
          <span>{activeTab.url === 'about:home' ? 'Ready' : activeTab.url}</span>
        </div>
      </div>
    </div>
  );
};
