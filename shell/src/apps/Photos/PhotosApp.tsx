import React, { useState } from 'react';
import {
  Image as ImageIcon,
  ZoomIn,
  ZoomOut,
  RotateCw,
  FolderOpen,
  Play,
  Pause,
  Monitor,
  Heart,
  ChevronLeft,
  ChevronRight,
  Download,
  Check,
} from 'lucide-react';
import { useSystemState, WALLPAPERS } from '../../context/SystemStateContext';

interface PhotoItem {
  id: string;
  title: string;
  category: 'Wallpapers' | 'Nature' | 'Abstract' | 'User';
  gradient: string;
  url?: string;
  date: string;
}

const DEFAULT_PHOTOS: PhotoItem[] = [
  {
    id: 'photo-1',
    title: 'Horizon Cyan & Violet',
    category: 'Wallpapers',
    gradient: 'linear-gradient(135deg, #091220 0%, #082f49 40%, #0369a1 70%, #0284c7 100%)',
    date: 'Sep 27, 2026',
  },
  {
    id: 'photo-2',
    title: 'Sonoma Twilight Rose',
    category: 'Wallpapers',
    gradient: 'linear-gradient(135deg, #180922 0%, #4a044e 40%, #701a75 70%, #86198f 100%)',
    date: 'Sep 25, 2026',
  },
  {
    id: 'photo-3',
    title: 'Emerald Forest Deep',
    category: 'Nature',
    gradient: 'linear-gradient(135deg, #022c22 0%, #064e3b 40%, #047857 70%, #059669 100%)',
    date: 'Sep 22, 2026',
  },
  {
    id: 'photo-4',
    title: 'Solar Flare Amber',
    category: 'Abstract',
    gradient: 'linear-gradient(135deg, #1c1917 0%, #451a03 40%, #78350f 70%, #9a3412 100%)',
    date: 'Sep 20, 2026',
  },
  {
    id: 'photo-5',
    title: 'Nordic Deep Midnight',
    category: 'Wallpapers',
    gradient: 'linear-gradient(135deg, #020617 0%, #0f172a 40%, #1e293b 70%, #334155 100%)',
    date: 'Sep 18, 2026',
  },
  {
    id: 'photo-6',
    title: 'Electric Neon Synthwave',
    category: 'Abstract',
    gradient: 'linear-gradient(135deg, #31103f 0%, #701a75 35%, #ec4899 70%, #06b6d4 100%)',
    date: 'Sep 15, 2026',
  },
];

export const PhotosApp: React.FC = () => {
  const { setWallpaper } = useSystemState();
  const [photos, setPhotos] = useState<PhotoItem[]>(DEFAULT_PHOTOS);
  const [activePhotoId, setActivePhotoId] = useState<string>(DEFAULT_PHOTOS[0].id);
  const [activeCategory, setActiveCategory] = useState<string>('All');
  const [zoomLevel, setZoomLevel] = useState(1);
  const [rotation, setRotation] = useState(0);
  const [wallpaperSetSuccess, setWallpaperSetSuccess] = useState(false);
  const [likedPhotos, setLikedPhotos] = useState<Record<string, boolean>>({});

  const activePhoto = photos.find((p) => p.id === activePhotoId) || photos[0];

  const handleImportPhoto = (e: React.ChangeEvent<HTMLInputElement>) => {
    const files = e.target.files;
    if (!files || files.length === 0) return;
    const file = files[0];
    const objectUrl = URL.createObjectURL(file);

    const newPhoto: PhotoItem = {
      id: `user-${Date.now()}`,
      title: file.name.replace(/\.[^/.]+$/, ''),
      category: 'User',
      gradient: `url("${objectUrl}") center/cover no-repeat`,
      url: objectUrl,
      date: 'Today',
    };

    setPhotos((prev) => [newPhoto, ...prev]);
    setActivePhotoId(newPhoto.id);
  };

  const handleSetWallpaper = () => {
    setWallpaper({
      id: `custom-wall-${Date.now()}`,
      name: activePhoto.title,
      gradient: activePhoto.gradient,
      previewColor: '#0284c7',
    });
    setWallpaperSetSuccess(true);
    setTimeout(() => setWallpaperSetSuccess(false), 2000);
  };

  const filteredPhotos = activeCategory === 'All'
    ? photos
    : photos.filter((p) => p.category === activeCategory);

  const categories = ['All', 'Wallpapers', 'Nature', 'Abstract', 'User'];

  return (
    <div className="flex h-full w-full bg-[#F5F5F7] dark:bg-[#1E1E1E] text-slate-900 dark:text-slate-100 select-none overflow-hidden font-sans">
      <input
        type="file"
        id="photos-import-input"
        onChange={handleImportPhoto}
        accept="image/*"
        className="hidden"
      />

      {/* Left Sidebar */}
      <div className="w-56 bg-white/70 dark:bg-slate-900/80 backdrop-blur-md border-r border-black/5 dark:border-white/10 p-3.5 flex flex-col justify-between">
        <div>
          <div className="flex items-center justify-between mb-4">
            <div className="flex items-center space-x-2">
              <div className="w-7 h-7 rounded-xl bg-gradient-to-tr from-amber-400 via-rose-500 to-indigo-600 flex items-center justify-center text-white shadow-md">
                <ImageIcon className="w-4 h-4" />
              </div>
              <span className="font-bold text-xs text-slate-800 dark:text-white">Photos</span>
            </div>
            <label
              htmlFor="photos-import-input"
              className="p-1.5 rounded-lg bg-black/5 dark:bg-white/5 hover:bg-black/10 dark:hover:bg-white/10 text-slate-600 dark:text-slate-300 transition-colors text-xs cursor-pointer flex items-center gap-1"
              title="Import image"
            >
              <FolderOpen className="w-3.5 h-3.5" />
            </label>
          </div>

          <div className="text-[11px] font-semibold text-slate-400 dark:text-slate-500 uppercase tracking-wider mb-2 px-1">
            Albums
          </div>

          <div className="flex flex-col gap-1">
            {categories.map((cat) => (
              <button
                key={cat}
                onClick={() => setActiveCategory(cat)}
                className={`px-3 py-1.5 rounded-xl text-left text-xs transition-colors flex items-center justify-between ${
                  activeCategory === cat
                    ? 'bg-[#007AFF] text-white font-medium shadow-xs'
                    : 'text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white hover:bg-black/5 dark:hover:bg-white/5'
                }`}
              >
                <span>{cat}</span>
                <span className="text-[10px] opacity-70">
                  {cat === 'All' ? photos.length : photos.filter((p) => p.category === cat).length}
                </span>
              </button>
            ))}
          </div>
        </div>

        {/* Thumbnail Filmstrip */}
        <div className="border-t border-black/5 dark:border-white/10 pt-3">
          <div className="text-[10px] text-slate-400 dark:text-slate-500 font-mono mb-2">Gallery Stream</div>
          <div className="grid grid-cols-3 gap-1.5 max-h-36 overflow-y-auto">
            {filteredPhotos.map((p) => (
              <button
                key={p.id}
                onClick={() => {
                  setActivePhotoId(p.id);
                  setZoomLevel(1);
                  setRotation(0);
                }}
                className={`h-12 rounded-lg overflow-hidden border transition-all ${
                  p.id === activePhotoId ? 'border-sky-500 ring-2 ring-sky-500/40' : 'border-black/10 dark:border-white/10 hover:border-black/30 dark:hover:border-white/30'
                }`}
                style={{ background: p.gradient }}
              />
            ))}
          </div>
        </div>
      </div>

      {/* Main Image Inspector */}
      <div className="flex-1 flex flex-col bg-[#F5F5F7] dark:bg-slate-950/80 justify-between">
        {/* Top Control Toolbar */}
        <div className="h-10 px-4 flex items-center justify-between border-b border-black/5 dark:border-white/10 bg-white/80 dark:bg-slate-900/40 text-xs text-slate-800 dark:text-slate-200">
          <div>
            <span className="font-semibold text-slate-800 dark:text-white">{activePhoto.title}</span>
            <span className="text-[11px] text-slate-500 dark:text-slate-400 ml-2 font-mono">{activePhoto.date}</span>
          </div>

          <div className="flex items-center space-x-1.5">
            <button
              onClick={() => setZoomLevel((z) => Math.max(0.5, z - 0.25))}
              className="p-1.5 rounded-lg text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white hover:bg-black/5 dark:hover:bg-white/10 transition-colors"
              title="Zoom Out"
            >
              <ZoomOut className="w-3.5 h-3.5" />
            </button>
            <span className="text-[11px] font-mono text-slate-500 dark:text-slate-400 w-10 text-center">
              {Math.round(zoomLevel * 100)}%
            </span>
            <button
              onClick={() => setZoomLevel((z) => Math.min(3, z + 0.25))}
              className="p-1.5 rounded-lg text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white hover:bg-black/5 dark:hover:bg-white/10 transition-colors"
              title="Zoom In"
            >
              <ZoomIn className="w-3.5 h-3.5" />
            </button>
            <button
              onClick={() => setRotation((r) => (r + 90) % 360)}
              className="p-1.5 rounded-lg text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-white hover:bg-black/5 dark:hover:bg-white/10 transition-colors"
              title="Rotate 90°"
            >
              <RotateCw className="w-3.5 h-3.5" />
            </button>

            <div className="h-4 w-px bg-black/10 dark:bg-white/10 mx-1"></div>

            <button
              onClick={handleSetWallpaper}
              className={`px-3 py-1 rounded-lg text-xs font-semibold flex items-center gap-1.5 transition-all ${
                wallpaperSetSuccess
                  ? 'bg-emerald-600 text-white'
                  : 'bg-black/5 dark:bg-white/10 hover:bg-black/10 dark:hover:bg-white/15 text-slate-700 dark:text-slate-200 border border-black/5 dark:border-white/10'
              }`}
            >
              {wallpaperSetSuccess ? <Check className="w-3.5 h-3.5" /> : <Monitor className="w-3.5 h-3.5" />}
              <span>{wallpaperSetSuccess ? 'Wallpaper Applied!' : 'Set as Desktop Wallpaper'}</span>
            </button>
          </div>
        </div>

        {/* Big Preview Area */}
        <div className="flex-1 flex items-center justify-center p-8 overflow-hidden relative">
          <div
            className="w-full max-w-2xl h-96 rounded-2xl shadow-2xl shadow-black/40 transition-transform duration-300 border border-black/10 dark:border-white/15 flex items-center justify-center relative overflow-hidden"
            style={{
              background: activePhoto.gradient,
              transform: `scale(${zoomLevel}) rotate(${rotation}deg)`,
            }}
          >
            <div className="absolute inset-0 bg-gradient-to-b from-white/10 via-transparent to-black/30 pointer-events-none"></div>
            <div className="text-center p-6 select-none">
              <div className="text-2xl font-black text-white drop-shadow-lg tracking-tight">
                {activePhoto.title}
              </div>
              <div className="text-xs text-white/80 font-mono mt-1 drop-shadow">
                AxisOS High Dynamic Wallpaper ({activePhoto.category})
              </div>
            </div>
          </div>
        </div>

        {/* Bottom Footer Info */}
        <div className="h-8 px-4 bg-white/80 dark:bg-slate-900/60 border-t border-black/5 dark:border-white/10 flex items-center justify-between text-[11px] font-mono text-slate-500">
          <span>{activePhoto.category} Gallery</span>
          <span>AxisOS GPU Accelerated Canvas</span>
        </div>
      </div>
    </div>
  );
};
