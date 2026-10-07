import React, { useState, useEffect, useRef } from 'react';
import {
  Play,
  Pause,
  SkipBack,
  SkipForward,
  Volume2,
  VolumeX,
  Shuffle,
  Repeat,
  Music,
  Heart,
  Disc,
  User,
  ListMusic,
  FolderOpen,
  Plus,
  Trash2,
} from 'lucide-react';
import { useWindowManager } from '../../context/WindowManagerContext';

export interface AudioTrack {
  id: string;
  title: string;
  artist: string;
  album: string;
  duration: number; // seconds
  audioBlobUrl: string;
}

export const MusicApp: React.FC = () => {
  const { closeWindow, minimizeWindow, toggleMaximizeWindow, windows } = useWindowManager();
  const currentWindow = windows.find((w) => w.appId === 'music');

  const [activeNav, setActiveNav] = useState<'library' | 'albums' | 'artists' | 'playlists'>('library');
  const [playlist, setPlaylist] = useState<AudioTrack[]>([]);
  const [currentTrackIndex, setCurrentTrackIndex] = useState(0);
  const [isPlaying, setIsPlaying] = useState(false);
  const [currentTime, setCurrentTime] = useState(0);
  const [volume, setVolume] = useState(0.85);
  const [isMuted, setIsMuted] = useState(false);
  const [isShuffle, setIsShuffle] = useState(false);
  const [isRepeat, setIsRepeat] = useState(false);
  const [likedTracks, setLikedTracks] = useState<Record<string, boolean>>({});

  const fileInputRef = useRef<HTMLInputElement | null>(null);
  const audioElementRef = useRef<HTMLAudioElement | null>(null);
  const audioCtxRef = useRef<AudioContext | null>(null);
  const analyserRef = useRef<AnalyserNode | null>(null);
  const canvasRef = useRef<HTMLCanvasElement | null>(null);

  const currentTrack = playlist[currentTrackIndex] || null;

  // Audio Context & Analyser Initialization
  const ensureAudioContext = () => {
    if (!audioCtxRef.current) {
      const AudioCtx = window.AudioContext || (window as any).webkitAudioContext;
      if (AudioCtx) {
        audioCtxRef.current = new AudioCtx();
        analyserRef.current = audioCtxRef.current.createAnalyser();
        analyserRef.current.fftSize = 64;
      }
    }
    if (audioCtxRef.current && audioCtxRef.current.state === 'suspended') {
      audioCtxRef.current.resume();
    }
  };

  // Synchronize HTML5 Audio element
  useEffect(() => {
    if (!audioElementRef.current) {
      const audio = new Audio();
      audio.preload = 'auto';
      audio.onended = () => {
        handleNext();
      };
      audio.ontimeupdate = () => {
        setCurrentTime(Math.floor(audio.currentTime));
      };
      audio.onloadedmetadata = () => {
        if (playlist[currentTrackIndex] && !playlist[currentTrackIndex].duration) {
          const dur = Math.floor(audio.duration) || 0;
          setPlaylist((prev) =>
            prev.map((t, idx) => (idx === currentTrackIndex ? { ...t, duration: dur } : t))
          );
        }
      };
      audioElementRef.current = audio;
    }
  }, [playlist, currentTrackIndex]);

  // Handle Playback Change
  useEffect(() => {
    const audio = audioElementRef.current;
    if (!audio) return;

    if (currentTrack && currentTrack.audioBlobUrl) {
      if (audio.src !== currentTrack.audioBlobUrl) {
        audio.src = currentTrack.audioBlobUrl;
        audio.currentTime = 0;
      }
      if (isPlaying) {
        ensureAudioContext();
        audio.play().catch(() => {});
      } else {
        audio.pause();
      }
    } else {
      audio.pause();
      setIsPlaying(false);
    }
  }, [currentTrack, isPlaying]);

  // Volume & Mute handling
  useEffect(() => {
    if (audioElementRef.current) {
      audioElementRef.current.volume = isMuted ? 0 : volume;
    }
  }, [volume, isMuted]);

  // Spectrum Canvas Visualizer
  useEffect(() => {
    let animId: number;
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const render = () => {
      animId = requestAnimationFrame(render);
      ctx.clearRect(0, 0, canvas.width, canvas.height);

      const barCount = 16;
      const barWidth = canvas.width / barCount - 2;

      for (let i = 0; i < barCount; i++) {
        let barHeight = 4;
        if (isPlaying) {
          barHeight = Math.max(4, Math.sin(Date.now() / 200 + i) * 12 + 14);
        }
        const x = i * (barWidth + 2);
        const y = canvas.height - barHeight;

        ctx.fillStyle = '#87cf3e';
        ctx.beginPath();
        ctx.roundRect(x, y, barWidth, barHeight, [2, 2, 0, 0]);
        ctx.fill();
      }
    };

    render();
    return () => cancelAnimationFrame(animId);
  }, [isPlaying]);

  const togglePlay = () => {
    if (playlist.length === 0) return;
    ensureAudioContext();
    setIsPlaying(!isPlaying);
  };

  const handleNext = () => {
    if (playlist.length === 0) return;
    if (isShuffle) {
      const nextIdx = Math.floor(Math.random() * playlist.length);
      setCurrentTrackIndex(nextIdx);
    } else {
      setCurrentTrackIndex((prev) => (prev + 1) % playlist.length);
    }
    setCurrentTime(0);
    setIsPlaying(true);
  };

  const handlePrev = () => {
    if (playlist.length === 0) return;
    setCurrentTrackIndex((prev) => (prev - 1 + playlist.length) % playlist.length);
    setCurrentTime(0);
    setIsPlaying(true);
  };

  const handleSeek = (newTime: number) => {
    setCurrentTime(newTime);
    if (audioElementRef.current) {
      audioElementRef.current.currentTime = newTime;
    }
  };

  const handleImportFiles = (e: React.ChangeEvent<HTMLInputElement>) => {
    const files = e.target.files;
    if (!files || files.length === 0) return;

    const newTracks: AudioTrack[] = [];
    for (let i = 0; i < files.length; i++) {
      const file = files[i];
      const url = URL.createObjectURL(file);
      const cleanName = file.name.replace(/\.[^/.]+$/, '');
      newTracks.push({
        id: `track-${Date.now()}-${i}`,
        title: cleanName,
        artist: 'Local Audio',
        album: 'My Music',
        duration: 180,
        audioBlobUrl: url,
      });
    }

    setPlaylist((prev) => [...prev, ...newTracks]);
    if (!isPlaying && playlist.length === 0) {
      setCurrentTrackIndex(0);
      setIsPlaying(true);
    }
    if (fileInputRef.current) fileInputRef.current.value = '';
  };

  const removeTrack = (id: string, e: React.MouseEvent) => {
    e.stopPropagation();
    setPlaylist((prev) => prev.filter((t) => t.id !== id));
    if (currentTrack?.id === id) {
      setIsPlaying(false);
      setCurrentTime(0);
    }
  };

  const formatTime = (secs: number) => {
    const m = Math.floor(secs / 60);
    const s = Math.floor(secs % 60);
    return `${m}:${s < 10 ? '0' : ''}${s}`;
  };

  return (
    <div className="flex flex-col h-full w-full bg-[#18201b] text-slate-100 select-none overflow-hidden font-sans">
      {/* Hidden File Picker */}
      <input
        ref={fileInputRef}
        type="file"
        accept="audio/*,.mp3,.wav,.ogg,.flac,.m4a"
        multiple
        className="hidden"
        onChange={handleImportFiles}
      />

      {/* Main Body with Sidebar & Content */}
      <div className="flex-1 flex overflow-hidden">
        {/* Linux Mint Dark Charcoal Sidebar */}
        <div className="w-56 bg-[#141b16] border-r border-[#87cf3e]/15 flex flex-col justify-between p-3.5 shrink-0">
          <div>
            {/* macOS Window Traffic Lights */}
            <div
              data-window-drag
              className="flex items-center space-x-2 pb-5 pt-1 pl-1 cursor-default select-none"
            >
              <button
                onClick={() => currentWindow && closeWindow(currentWindow.id)}
                className="w-3 h-3 rounded-full bg-[#ff5f56] border border-[#e0443e] cursor-pointer"
                title="Close"
              />
              <button
                onClick={() => currentWindow && minimizeWindow(currentWindow.id)}
                className="w-3 h-3 rounded-full bg-[#ffbd2e] border border-[#dea123] cursor-pointer"
                title="Minimize"
              />
              <button
                onClick={() => currentWindow && toggleMaximizeWindow(currentWindow.id)}
                className="w-3 h-3 rounded-full bg-[#27c93f] border border-[#1aab29] cursor-pointer"
                title="Maximize"
              />
            </div>

            {/* Sidebar Navigation */}
            <div className="text-[11px] font-bold text-[#87cf3e] uppercase tracking-wider px-2 mb-2">
              Music Library
            </div>
            <div className="flex flex-col gap-1">
              <button
                onClick={() => setActiveNav('library')}
                className={`w-full flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-colors ${
                  activeNav === 'library'
                    ? 'bg-[#87cf3e] text-black font-bold shadow-sm'
                    : 'text-slate-300 hover:bg-[#87cf3e]/10'
                }`}
              >
                <Music className="w-4 h-4" />
                <span>All Tracks</span>
                {playlist.length > 0 && (
                  <span className="ml-auto text-[10px] font-mono opacity-80">{playlist.length}</span>
                )}
              </button>

              <button
                onClick={() => setActiveNav('albums')}
                className={`w-full flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-colors ${
                  activeNav === 'albums'
                    ? 'bg-[#87cf3e] text-black font-bold shadow-sm'
                    : 'text-slate-300 hover:bg-[#87cf3e]/10'
                }`}
              >
                <Disc className="w-4 h-4" />
                <span>Albums</span>
              </button>

              <button
                onClick={() => setActiveNav('artists')}
                className={`w-full flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-colors ${
                  activeNav === 'artists'
                    ? 'bg-[#87cf3e] text-black font-bold shadow-sm'
                    : 'text-slate-300 hover:bg-[#87cf3e]/10'
                }`}
              >
                <User className="w-4 h-4" />
                <span>Artists</span>
              </button>

              <button
                onClick={() => setActiveNav('playlists')}
                className={`w-full flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-colors ${
                  activeNav === 'playlists'
                    ? 'bg-[#87cf3e] text-black font-bold shadow-sm'
                    : 'text-slate-300 hover:bg-[#87cf3e]/10'
                }`}
              >
                <ListMusic className="w-4 h-4" />
                <span>Playlists</span>
              </button>
            </div>
          </div>

          {/* Import Music Files Button */}
          <div className="pt-3 border-t border-[#87cf3e]/15">
            <button
              onClick={() => fileInputRef.current?.click()}
              className="w-full flex items-center justify-center gap-2 px-3 py-2 rounded-xl bg-[#87cf3e]/15 hover:bg-[#87cf3e]/25 text-[#87cf3e] border border-[#87cf3e]/30 text-xs font-semibold transition-colors"
            >
              <Plus className="w-4 h-4" />
              <span>Import Audio Files</span>
            </button>
          </div>
        </div>

        {/* Main Content Pane */}
        <div className="flex-1 flex flex-col bg-[#18201b] overflow-hidden">
          {/* Header strip */}
          <div data-window-drag className="h-8 shrink-0 flex items-center px-6 justify-between border-b border-[#87cf3e]/10">
            <span className="text-xs font-medium text-slate-400">Axis Music Player</span>
            <div className="flex items-center gap-2 text-xs text-slate-400">
              <FolderOpen className="w-3.5 h-3.5" />
              <span>Local Storage: /home/axis/Music</span>
            </div>
          </div>

          {/* Track List or Clean Empty State */}
          <div className="flex-1 overflow-y-auto p-6">
            {playlist.length === 0 ? (
              <div className="h-full flex flex-col items-center justify-center text-center max-w-md mx-auto gap-4">
                <div className="w-16 h-16 rounded-2xl bg-[#87cf3e]/15 border border-[#87cf3e]/30 flex items-center justify-center text-[#87cf3e] shadow-lg">
                  <Music className="w-8 h-8" />
                </div>
                <div>
                  <h2 className="text-lg font-bold text-slate-100">Your Music Library is Ready</h2>
                  <p className="text-xs text-slate-400 mt-1 leading-relaxed">
                    No dummy or sample tracks are loaded. Import your own real music files (.mp3, .wav, .flac, .ogg) from storage or flash drives to begin playback.
                  </p>
                </div>
                <button
                  onClick={() => fileInputRef.current?.click()}
                  className="px-5 py-2.5 rounded-xl bg-[#87cf3e] text-black text-xs font-bold hover:bg-[#76bb33] transition-colors shadow-md flex items-center gap-2"
                >
                  <FolderOpen className="w-4 h-4" />
                  <span>Choose Music Files</span>
                </button>
              </div>
            ) : (
              <div className="flex flex-col gap-4">
                <div className="flex items-center justify-between">
                  <div>
                    <h1 className="text-xl font-bold text-slate-100">All Tracks</h1>
                    <p className="text-xs text-slate-400">{playlist.length} items loaded</p>
                  </div>
                  <button
                    onClick={() => fileInputRef.current?.click()}
                    className="px-3 py-1.5 rounded-lg bg-[#87cf3e]/15 hover:bg-[#87cf3e]/25 text-[#87cf3e] border border-[#87cf3e]/30 text-xs font-medium transition-colors flex items-center gap-1.5"
                  >
                    <Plus className="w-3.5 h-3.5" />
                    <span>Add More</span>
                  </button>
                </div>

                <div className="flex flex-col divide-y divide-[#87cf3e]/10 bg-[#141b16] rounded-2xl border border-[#87cf3e]/15 overflow-hidden">
                  {playlist.map((track, idx) => {
                    const isCurrent = idx === currentTrackIndex;
                    return (
                      <div
                        key={track.id}
                        onClick={() => {
                          setCurrentTrackIndex(idx);
                          setIsPlaying(true);
                        }}
                        className={`px-4 py-3 flex items-center justify-between cursor-pointer transition-colors ${
                          isCurrent ? 'bg-[#87cf3e]/15' : 'hover:bg-[#87cf3e]/5'
                        }`}
                      >
                        <div className="flex items-center space-x-3 truncate">
                          <button
                            onClick={(e) => {
                              e.stopPropagation();
                              if (isCurrent) togglePlay();
                              else {
                                setCurrentTrackIndex(idx);
                                setIsPlaying(true);
                              }
                            }}
                            className={`w-8 h-8 rounded-lg flex items-center justify-center shrink-0 transition-colors ${
                              isCurrent ? 'bg-[#87cf3e] text-black font-bold' : 'bg-white/5 text-slate-300 hover:text-white'
                            }`}
                          >
                            {isCurrent && isPlaying ? (
                              <Pause className="w-4 h-4" />
                            ) : (
                              <Play className="w-4 h-4 ml-0.5" />
                            )}
                          </button>
                          <div className="overflow-hidden">
                            <div className={`text-xs font-semibold truncate ${isCurrent ? 'text-[#87cf3e]' : 'text-slate-100'}`}>
                              {track.title}
                            </div>
                            <div className="text-[11px] text-slate-400 truncate">{track.artist}</div>
                          </div>
                        </div>

                        <div className="flex items-center space-x-4 shrink-0">
                          <span className="text-xs font-mono text-slate-400">
                            {formatTime(track.duration)}
                          </span>
                          <button
                            onClick={(e) => removeTrack(track.id, e)}
                            className="p-1 rounded text-slate-500 hover:text-rose-400 transition-colors"
                            title="Remove track"
                          >
                            <Trash2 className="w-3.5 h-3.5" />
                          </button>
                        </div>
                      </div>
                    );
                  })}
                </div>
              </div>
            )}
          </div>
        </div>
      </div>

      {/* Linux Mint Bottom Now-Playing & Playback Bar */}
      <div className="h-20 bg-[#141b16] border-t border-[#87cf3e]/20 px-6 flex items-center justify-between shrink-0">
        {/* Left: Track Information */}
        <div className="w-1/4 flex items-center space-x-3 overflow-hidden">
          {currentTrack ? (
            <>
              <div className="w-11 h-11 rounded-xl bg-[#87cf3e]/20 border border-[#87cf3e]/30 flex items-center justify-center text-[#87cf3e] shrink-0">
                <Music className="w-5 h-5" />
              </div>
              <div className="overflow-hidden">
                <div className="text-xs font-semibold text-slate-100 truncate">{currentTrack.title}</div>
                <div className="text-[11px] text-slate-400 truncate">{currentTrack.artist}</div>
              </div>
            </>
          ) : (
            <div className="text-xs text-slate-500 italic">No track selected</div>
          )}
        </div>

        {/* Center: Controls & Timeline */}
        <div className="flex-1 max-w-lg flex flex-col items-center gap-1.5">
          <div className="flex items-center space-x-5">
            <button
              onClick={() => setIsShuffle(!isShuffle)}
              className={`p-1 transition-colors ${isShuffle ? 'text-[#87cf3e]' : 'text-slate-500 hover:text-slate-300'}`}
              title="Shuffle"
            >
              <Shuffle className="w-3.5 h-3.5" />
            </button>
            <button
              onClick={handlePrev}
              disabled={playlist.length === 0}
              className="text-slate-300 hover:text-white transition-colors"
              title="Previous"
            >
              <SkipBack className="w-4 h-4" />
            </button>
            <button
              onClick={togglePlay}
              disabled={playlist.length === 0}
              className="w-9 h-9 rounded-full bg-[#87cf3e] text-black flex items-center justify-center hover:scale-105 active:scale-95 transition-transform shadow-md"
              title={isPlaying ? 'Pause' : 'Play'}
            >
              {isPlaying ? <Pause className="w-4 h-4" /> : <Play className="w-4 h-4 ml-0.5" />}
            </button>
            <button
              onClick={handleNext}
              disabled={playlist.length === 0}
              className="text-slate-300 hover:text-white transition-colors"
              title="Next"
            >
              <SkipForward className="w-4 h-4" />
            </button>
            <button
              onClick={() => setIsRepeat(!isRepeat)}
              className={`p-1 transition-colors ${isRepeat ? 'text-[#87cf3e]' : 'text-slate-500 hover:text-slate-300'}`}
              title="Repeat"
            >
              <Repeat className="w-3.5 h-3.5" />
            </button>
          </div>

          {/* Timeline Bar */}
          <div className="w-full flex items-center space-x-3 text-[10px] font-mono text-slate-400">
            <span>{formatTime(currentTime)}</span>
            <input
              type="range"
              min="0"
              max={currentTrack?.duration || 100}
              value={currentTime}
              onChange={(e) => handleSeek(Number(e.target.value))}
              className="flex-1 h-1 bg-white/10 rounded-full appearance-none cursor-pointer accent-[#87cf3e]"
            />
            <span>{formatTime(currentTrack?.duration || 0)}</span>
          </div>
        </div>

        {/* Right: Visualizer & Volume */}
        <div className="w-1/4 flex items-center justify-end space-x-4">
          {/* Animated Mini Spectrum */}
          <canvas ref={canvasRef} width={80} height={20} className="rounded" />

          {/* Volume Control */}
          <div className="flex items-center space-x-2">
            <button
              onClick={() => setIsMuted(!isMuted)}
              className="text-slate-400 hover:text-slate-200 transition-colors"
            >
              {isMuted || volume === 0 ? <VolumeX className="w-4 h-4" /> : <Volume2 className="w-4 h-4" />}
            </button>
            <input
              type="range"
              min="0"
              max="1"
              step="0.05"
              value={isMuted ? 0 : volume}
              onChange={(e) => {
                setVolume(Number(e.target.value));
                if (isMuted) setIsMuted(false);
              }}
              className="w-20 h-1 bg-white/10 rounded-full appearance-none cursor-pointer accent-[#87cf3e]"
            />
          </div>
        </div>
      </div>
    </div>
  );
};
