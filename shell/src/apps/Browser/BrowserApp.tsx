import React, { useState } from 'react';
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
} from 'lucide-react';

interface Bookmark {
  title: string;
  url: string;
  icon: string;
}

const BOOKMARKS: Bookmark[] = [
  { title: 'DuckDuckGo', url: 'https://duckduckgo.com', icon: '🦆' },
  { title: 'Linux Kernel', url: 'https://www.kernel.org', icon: '🐧' },
  { title: 'Wikipedia', url: 'https://www.wikipedia.org', icon: '📖' },
  { title: 'GitHub', url: 'https://github.com', icon: '🐙' },
  { title: 'Hacker News', url: 'https://news.ycombinator.com', icon: '⚡' },
];

export const BrowserApp: React.FC = () => {
  const [currentUrl, setCurrentUrl] = useState('https://duckduckgo.com');
  const [inputUrl, setInputUrl] = useState('https://duckduckgo.com');
  const [history, setHistory] = useState<string[]>(['https://duckduckgo.com']);
  const [historyIdx, setHistoryIdx] = useState(0);
  const [isLoading, setIsLoading] = useState(false);

  const navigateTo = (rawUrl: string) => {
    let target = rawUrl.trim();
    if (!target) return;

    if (!target.startsWith('http://') && !target.startsWith('https://')) {
      if (target.includes('.') && !target.includes(' ')) {
        target = `https://${target}`;
      } else {
        target = `https://duckduckgo.com/?q=${encodeURIComponent(target)}`;
      }
    }

    setInputUrl(target);
    setCurrentUrl(target);
    setIsLoading(true);
    setTimeout(() => setIsLoading(false), 800);

    const newHist = history.slice(0, historyIdx + 1);
    newHist.push(target);
    setHistory(newHist);
    setHistoryIdx(newHist.length - 1);
  };

  const handleBack = () => {
    if (historyIdx > 0) {
      const prev = history[historyIdx - 1];
      setHistoryIdx(historyIdx - 1);
      setInputUrl(prev);
      setCurrentUrl(prev);
    }
  };

  const handleForward = () => {
    if (historyIdx < history.length - 1) {
      const next = history[historyIdx + 1];
      setHistoryIdx(historyIdx + 1);
      setInputUrl(next);
      setCurrentUrl(next);
    }
  };

  const handleReload = () => {
    setIsLoading(true);
    setTimeout(() => setIsLoading(false), 500);
  };

  return (
    <div className="flex flex-col h-full w-full bg-slate-900 text-slate-100 select-none">
      {/* Top Safari Navigation Toolbar */}
      <div className="h-11 px-3 flex items-center gap-2 bg-slate-950/70 backdrop-blur-xl border-b border-white/10">
        {/* Navigation buttons */}
        <div className="flex items-center space-x-0.5">
          <button
            onClick={handleBack}
            disabled={historyIdx === 0}
            className={`p-1.5 rounded-md transition-colors ${
              historyIdx === 0 ? 'text-slate-600' : 'text-slate-300 hover:bg-white/10'
            }`}
          >
            <ChevronLeft className="w-4 h-4" />
          </button>
          <button
            onClick={handleForward}
            disabled={historyIdx >= history.length - 1}
            className={`p-1.5 rounded-md transition-colors ${
              historyIdx >= history.length - 1 ? 'text-slate-600' : 'text-slate-300 hover:bg-white/10'
            }`}
          >
            <ChevronRight className="w-4 h-4" />
          </button>
          <button
            onClick={handleReload}
            className="p-1.5 rounded-md text-slate-300 hover:bg-white/10 transition-colors"
          >
            <RotateCw className={`w-3.5 h-3.5 ${isLoading ? 'animate-spin text-sky-400' : ''}`} />
          </button>
        </div>

        {/* Omnibar / Address Bar */}
        <form
          onSubmit={(e) => {
            e.preventDefault();
            navigateTo(inputUrl);
          }}
          className="flex-1 max-w-xl mx-auto relative flex items-center"
        >
          <div className="w-full flex items-center px-3 py-1.5 bg-slate-800/80 hover:bg-slate-800 border border-white/10 focus-within:border-sky-500 rounded-lg transition-all text-xs">
            <Lock className="w-3 h-3 text-emerald-400 mr-2 shrink-0" />
            <input
              type="text"
              value={inputUrl}
              onChange={(e) => setInputUrl(e.target.value)}
              className="w-full bg-transparent outline-none text-slate-200 font-sans text-xs"
              placeholder="Search or enter website name"
            />
          </div>
        </form>

        {/* Share and Action */}
        <div className="flex items-center space-x-1">
          <button
            onClick={() => navigator.clipboard.writeText(currentUrl)}
            className="p-1.5 rounded-md text-slate-400 hover:text-white hover:bg-white/10 transition-colors"
            title="Copy URL"
          >
            <Share2 className="w-4 h-4" />
          </button>
        </div>
      </div>

      {/* Bookmarks Bar */}
      <div className="h-7 px-3 flex items-center space-x-4 bg-slate-900/60 border-b border-white/5 text-[11px] text-slate-400">
        {BOOKMARKS.map((bm) => (
          <button
            key={bm.title}
            onClick={() => navigateTo(bm.url)}
            className="flex items-center space-x-1 hover:text-white transition-colors"
          >
            <span>{bm.icon}</span>
            <span>{bm.title}</span>
          </button>
        ))}
      </div>

      {/* Webview Content */}
      <div className="flex-1 bg-white relative overflow-hidden">
        {isLoading && (
          <div className="absolute top-0 left-0 right-0 h-0.5 bg-sky-500 animate-pulse z-20"></div>
        )}
        <iframe
          src={currentUrl}
          title="AxisOS Safari"
          className="w-full h-full border-none"
          sandbox="allow-scripts allow-same-origin allow-forms allow-popups"
        />
      </div>
    </div>
  );
};
