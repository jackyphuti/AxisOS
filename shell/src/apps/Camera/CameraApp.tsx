import React, { useState, useEffect, useRef } from 'react';
import {
  Camera,
  Download,
  Trash2,
  RefreshCw,
  Sparkles,
  Timer,
  Sliders,
  Image as ImageIcon,
  Check,
} from 'lucide-react';
import { useSystemState } from '../../context/SystemStateContext';

type FilterType = 'normal' | 'sepia' | 'grayscale' | 'cyberpunk' | 'invert';

interface CapturedPhoto {
  id: string;
  dataUrl: string;
  timestamp: string;
  filter: FilterType;
}

export const CameraApp: React.FC = () => {
  const { setWallpaper } = useSystemState();
  const videoRef = useRef<HTMLVideoElement>(null);
  const canvasRef = useRef<HTMLCanvasElement>(null);

  const [hasPermission, setHasPermission] = useState<boolean | null>(null);
  const [activeFilter, setActiveFilter] = useState<FilterType>('normal');
  const [isMirrored, setIsMirrored] = useState(true);
  const [countdown, setCountdown] = useState<number | null>(null);
  const [useTimer, setUseTimer] = useState(false);
  const [flash, setFlash] = useState(false);
  const [photos, setPhotos] = useState<CapturedPhoto[]>([]);
  const [selectedPhoto, setSelectedPhoto] = useState<CapturedPhoto | null>(null);

  // Audio shutter sound generator via Web Audio API
  const playShutterSound = () => {
    try {
      const AudioCtx = window.AudioContext || (window as any).webkitAudioContext;
      if (!AudioCtx) return;
      const ctx = new AudioCtx();
      const osc = ctx.createOscillator();
      const gain = ctx.createGain();

      osc.type = 'triangle';
      osc.frequency.setValueAtTime(1200, ctx.currentTime);
      osc.frequency.exponentialRampToValueAtTime(80, ctx.currentTime + 0.15);

      gain.gain.setValueAtTime(0.4, ctx.currentTime);
      gain.gain.exponentialRampToValueAtTime(0.01, ctx.currentTime + 0.15);

      osc.connect(gain);
      gain.connect(ctx.destination);
      osc.start();
      osc.stop(ctx.currentTime + 0.15);
    } catch {}
  };

  // Start Camera Stream
  useEffect(() => {
    let stream: MediaStream | null = null;

    const startCamera = async () => {
      try {
        stream = await navigator.mediaDevices.getUserMedia({
          video: { width: { ideal: 1280 }, height: { ideal: 720 } },
          audio: false,
        });
        if (videoRef.current) {
          videoRef.current.srcObject = stream;
        }
        setHasPermission(true);
      } catch (err) {
        console.warn('Camera access unavailable (no webcam hardware or permission denied):', err);
        setHasPermission(false);
      }
    };

    startCamera();

    return () => {
      if (stream) {
        stream.getTracks().forEach((track) => track.stop());
      }
    };
  }, []);

  const getFilterCSS = (filter: FilterType) => {
    switch (filter) {
      case 'sepia':
        return 'sepia(0.85) contrast(1.1) brightness(0.95)';
      case 'grayscale':
        return 'grayscale(1) contrast(1.2) brightness(0.9)';
      case 'cyberpunk':
        return 'hue-rotate(190deg) saturate(2.2) contrast(1.3)';
      case 'invert':
        return 'invert(0.9) hue-rotate(180deg)';
      case 'normal':
      default:
        return 'none';
    }
  };

  const takeSnapshot = () => {
    const video = videoRef.current;
    const canvas = canvasRef.current;
    if (!video || !canvas) return;

    // Flash animation & sound
    setFlash(true);
    playShutterSound();
    setTimeout(() => setFlash(false), 200);

    const width = video.videoWidth || 640;
    const height = video.videoHeight || 480;
    canvas.width = width;
    canvas.height = height;

    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    ctx.save();
    if (isMirrored) {
      ctx.translate(width, 0);
      ctx.scale(-1, 1);
    }
    ctx.filter = getFilterCSS(activeFilter);
    ctx.drawImage(video, 0, 0, width, height);
    ctx.restore();

    const dataUrl = canvas.toDataURL('image/png');
    const newPhoto: CapturedPhoto = {
      id: Date.now().toString(),
      dataUrl,
      timestamp: new Date().toLocaleTimeString([], { hour: '2-digit', minute: '2-digit' }),
      filter: activeFilter,
    };

    setPhotos((prev) => [newPhoto, ...prev]);
    setSelectedPhoto(newPhoto);
  };

  const handleCaptureClick = () => {
    if (useTimer) {
      setCountdown(3);
      const interval = setInterval(() => {
        setCountdown((c) => {
          if (c === null || c <= 1) {
            clearInterval(interval);
            takeSnapshot();
            return null;
          }
          return c - 1;
        });
      }, 1000);
    } else {
      takeSnapshot();
    }
  };

  const downloadPhoto = (photo: CapturedPhoto) => {
    const a = document.createElement('a');
    a.href = photo.dataUrl;
    a.download = `axisos-capture-${photo.id}.png`;
    a.click();
  };

  return (
    <div className="flex flex-col h-full w-full bg-slate-950 text-white select-none overflow-hidden font-sans">
      {/* Hidden processing canvas */}
      <canvas ref={canvasRef} className="hidden" />

      {/* Camera Viewport & Flash Overlay */}
      <div className="relative flex-1 bg-black flex items-center justify-center overflow-hidden">
        {/* Flash animation */}
        {flash && <div className="absolute inset-0 z-40 bg-white animate-out fade-out duration-200" />}

        {/* Self-timer overlay */}
        {countdown !== null && (
          <div className="absolute inset-0 z-30 flex items-center justify-center bg-black/40 backdrop-blur-xs">
            <span className="text-8xl font-black text-white drop-shadow-2xl animate-ping">
              {countdown}
            </span>
          </div>
        )}

        {/* Live Video Feed or Fallback Pattern */}
        {hasPermission === false ? (
          <div className="flex flex-col items-center justify-center text-center p-6 space-y-4">
            <div className="w-20 h-20 rounded-3xl bg-slate-800 flex items-center justify-center text-slate-500">
              <Camera className="w-10 h-10" />
            </div>
            <div>
              <h2 className="text-sm font-semibold text-slate-200">No Webcam Connected</h2>
              <p className="text-xs text-slate-400 mt-1 max-w-sm">
                Webcam device not detected or permission denied. You can connect a USB camera or test inside real hardware.
              </p>
            </div>
          </div>
        ) : (
          <video
            ref={videoRef}
            autoPlay
            playsInline
            muted
            style={{
              filter: getFilterCSS(activeFilter),
              transform: isMirrored ? 'scaleX(-1)' : 'none',
            }}
            className="w-full h-full object-contain transition-all duration-300"
          />
        )}

        {/* Quick Filter Overlays */}
        <div className="absolute top-4 left-4 z-20 flex items-center space-x-1 bg-black/50 backdrop-blur-md p-1 rounded-2xl border border-white/10">
          {(['normal', 'sepia', 'grayscale', 'cyberpunk', 'invert'] as FilterType[]).map((f) => (
            <button
              key={f}
              onClick={() => setActiveFilter(f)}
              className={`px-2.5 py-1 rounded-xl text-[11px] font-medium capitalize transition-all ${
                activeFilter === f
                  ? 'bg-blue-600 text-white shadow-md'
                  : 'text-slate-300 hover:text-white hover:bg-white/10'
              }`}
            >
              {f}
            </button>
          ))}
        </div>

        {/* Controls Overlay (Timer, Mirror) */}
        <div className="absolute top-4 right-4 z-20 flex items-center space-x-2 bg-black/50 backdrop-blur-md p-1 rounded-2xl border border-white/10 text-xs">
          <button
            onClick={() => setUseTimer(!useTimer)}
            className={`p-1.5 rounded-xl transition-colors ${
              useTimer ? 'bg-orange-500 text-white' : 'text-slate-300 hover:bg-white/10'
            }`}
            title="3-Second Timer"
          >
            <Timer className="w-4 h-4" />
          </button>
          <button
            onClick={() => setIsMirrored(!isMirrored)}
            className={`p-1.5 rounded-xl transition-colors ${
              isMirrored ? 'bg-blue-600 text-white' : 'text-slate-300 hover:bg-white/10'
            }`}
            title="Mirror Camera"
          >
            <RefreshCw className="w-4 h-4" />
          </button>
        </div>

        {/* Shutter Capture Button */}
        <div className="absolute bottom-6 inset-x-0 flex items-center justify-center z-20">
          <button
            onClick={handleCaptureClick}
            className="w-18 h-18 rounded-full border-4 border-white p-1 shadow-2xl active:scale-95 transition-transform"
          >
            <div className="w-full h-full rounded-full bg-red-600 hover:bg-red-500 transition-colors shadow-inner flex items-center justify-center">
              <Camera className="w-6 h-6 text-white" />
            </div>
          </button>
        </div>
      </div>

      {/* Captured Photos Bottom Strip */}
      <div className="h-28 border-t border-white/10 bg-slate-900/90 px-4 py-2 flex items-center space-x-3 overflow-x-auto">
        {photos.length === 0 ? (
          <div className="text-xs text-slate-500 flex items-center space-x-2">
            <ImageIcon className="w-4 h-4" />
            <span>Captured snapshots will appear here</span>
          </div>
        ) : (
          photos.map((p) => {
            const isSelected = selectedPhoto?.id === p.id;
            return (
              <div
                key={p.id}
                onClick={() => setSelectedPhoto(p)}
                className={`relative w-24 h-20 rounded-xl overflow-hidden shrink-0 border-2 transition-all cursor-pointer group ${
                  isSelected ? 'border-blue-500 scale-105 shadow-lg' : 'border-white/20 opacity-80 hover:opacity-100'
                }`}
              >
                <img src={p.dataUrl} alt="Snapshot" className="w-full h-full object-cover" />
                <div className="absolute inset-0 bg-black/40 opacity-0 group-hover:opacity-100 flex items-center justify-center space-x-1.5 transition-opacity">
                  <button
                    onClick={(e) => {
                      e.stopPropagation();
                      downloadPhoto(p);
                    }}
                    className="p-1 rounded-md bg-white/20 hover:bg-white/40 text-white"
                    title="Download"
                  >
                    <Download className="w-3.5 h-3.5" />
                  </button>
                  <button
                    onClick={(e) => {
                      e.stopPropagation();
                      setWallpaper({
                        id: `camera-snap-${p.id}`,
                        name: `Camera Snapshot ${p.timestamp}`,
                        gradient: `url("${p.dataUrl}") center/cover no-repeat`,
                        previewColor: '#0284c7',
                      });
                    }}
                    className="p-1 rounded-md bg-white/20 hover:bg-white/40 text-white"
                    title="Set as Wallpaper"
                  >
                    <Check className="w-3.5 h-3.5" />
                  </button>
                  <button
                    onClick={(e) => {
                      e.stopPropagation();
                      setPhotos(photos.filter((item) => item.id !== p.id));
                      if (selectedPhoto?.id === p.id) setSelectedPhoto(null);
                    }}
                    className="p-1 rounded-md bg-red-500/40 hover:bg-red-500/80 text-white"
                    title="Delete"
                  >
                    <Trash2 className="w-3.5 h-3.5" />
                  </button>
                </div>
              </div>
            );
          })
        )}
      </div>
    </div>
  );
};
