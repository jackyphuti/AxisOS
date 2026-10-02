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
  FolderOpen,
  ListMusic,
  Heart,
  Sliders,
} from 'lucide-react';
import { useSystemState, ACCENT_COLOR_MAP } from '../../context/SystemStateContext';

interface Track {
  id: string;
  title: string;
  artist: string;
  album: string;
  duration: number; // in seconds
  coverGradient: string;
  isCustom?: boolean;
  audioBlobUrl?: string;
  bpm?: number;
}

const DEFAULT_PLAYLIST: Track[] = [
  {
    id: 'track-1',
    title: 'Horizon Sunrise',
    artist: 'Axis Sound Lab',
    album: 'Wayland Vibes Vol. 1',
    duration: 180,
    coverGradient: 'from-cyan-500 via-blue-600 to-indigo-700',
    bpm: 85,
  },
  {
    id: 'track-2',
    title: 'Cyberpunk Boulevard',
    artist: 'Neon Driver',
    album: 'Retrowave 2084',
    duration: 210,
    coverGradient: 'from-pink-500 via-rose-600 to-purple-800',
    bpm: 110,
  },
  {
    id: 'track-3',
    title: 'Deep Space Orbit',
    artist: 'Stellar Drifter',
    album: 'Cosmic Ambient',
    duration: 240,
    coverGradient: 'from-emerald-400 via-teal-600 to-slate-900',
    bpm: 70,
  },
  {
    id: 'track-4',
    title: 'Btrfs Pulse',
    artist: 'Kernel Groove',
    album: 'Debian Midnight',
    duration: 195,
    coverGradient: 'from-amber-400 via-orange-600 to-rose-700',
    bpm: 120,
  },
];

export const MusicApp: React.FC = () => {
  const { accentColor } = useSystemState();
  const accent = ACCENT_COLOR_MAP[accentColor];

  const [playlist, setPlaylist] = useState<Track[]>(DEFAULT_PLAYLIST);
  const [currentTrackIndex, setCurrentTrackIndex] = useState(0);
  const [isPlaying, setIsPlaying] = useState(false);
  const [currentTime, setCurrentTime] = useState(0);
  const [volume, setVolume] = useState(0.8);
  const [isMuted, setIsMuted] = useState(false);
  const [isShuffle, setIsShuffle] = useState(false);
  const [isRepeat, setIsRepeat] = useState(false);
  const [likedTracks, setLikedTracks] = useState<Record<string, boolean>>({});

  const currentTrack = playlist[currentTrackIndex] || playlist[0];

  // Web Audio Context and Synthesis Engine
  const audioCtxRef = useRef<AudioContext | null>(null);
  const analyserRef = useRef<AnalyserNode | null>(null);
  const gainNodeRef = useRef<GainNode | null>(null);
  const synthTimerRef = useRef<any>(null);
  const canvasRef = useRef<HTMLCanvasElement | null>(null);
  const audioElementRef = useRef<HTMLAudioElement | null>(null);
  const fileInputRef = useRef<HTMLInputElement | null>(null);

  // Initialize Audio Context on demand
  const ensureAudioContext = () => {
    if (!audioCtxRef.current) {
      const AudioCtx = window.AudioContext || (window as any).webkitAudioContext;
      const ctx = new AudioCtx();
      const analyser = ctx.createAnalyser();
      analyser.fftSize = 64;
      const gainNode = ctx.createGain();
      gainNode.gain.setValueAtTime(isMuted ? 0 : volume, ctx.currentTime);

      gainNode.connect(analyser);
      analyser.connect(ctx.destination);

      audioCtxRef.current = ctx;
      analyserRef.current = analyser;
      gainNodeRef.current = gainNode;
    }

    if (audioCtxRef.current.state === 'suspended') {
      audioCtxRef.current.resume();
    }
  };

  // Play a musical synthesizer note for built-in generative tracks
  const playSynthNote = (freq: number, duration: number, type: OscillatorType = 'sine') => {
    if (!audioCtxRef.current || !gainNodeRef.current) return;
    const ctx = audioCtxRef.current;
    const osc = ctx.createOscillator();
    const noteGain = ctx.createGain();

    osc.type = type;
    osc.frequency.setValueAtTime(freq, ctx.currentTime);

    noteGain.gain.setValueAtTime(0.001, ctx.currentTime);
    noteGain.gain.exponentialRampToValueAtTime(0.3, ctx.currentTime + 0.05);
    noteGain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + duration);

    osc.connect(noteGain);
    noteGain.connect(gainNodeRef.current);

    osc.start();
    osc.stop(ctx.currentTime + duration);
  };

  // Generative music synthesizer player for built-in tracks
  useEffect(() => {
    if (!isPlaying) {
      if (synthTimerRef.current) clearInterval(synthTimerRef.current);
      if (audioElementRef.current) audioElementRef.current.pause();
      return;
    }

    ensureAudioContext();

    if (currentTrack.isCustom && currentTrack.audioBlobUrl) {
      if (!audioElementRef.current) {
        audioElementRef.current = new Audio(currentTrack.audioBlobUrl);
      } else {
        if (audioElementRef.current.src !== currentTrack.audioBlobUrl) {
          audioElementRef.current.src = currentTrack.audioBlobUrl;
        }
      }
      audioElementRef.current.volume = isMuted ? 0 : volume;
      audioElementRef.current.currentTime = currentTime;
      audioElementRef.current.play().catch(() => {});
      return;
    }

    // Generative chord progression based on track
    const chordSets: Record<string, number[][]> = {
      'track-1': [[261.63, 329.63, 392.0], [293.66, 349.23, 440.0], [329.63, 392.0, 493.88], [220.0, 261.63, 329.63]], // C - Dm - Em - Am
      'track-2': [[220.0, 261.63, 329.63], [174.61, 220.0, 261.63], [196.0, 246.94, 293.66], [164.81, 196.0, 246.94]], // Am - F - G - Em
      'track-3': [[130.81, 196.0, 261.63], [146.83, 220.0, 293.66], [164.81, 246.94, 329.63], [110.0, 164.81, 220.0]], // Space drones
      'track-4': [[293.66, 369.99, 440.0], [329.63, 415.3, 493.88], [261.63, 329.63, 392.0], [220.0, 277.18, 329.63]],
    };

    const chords = chordSets[currentTrack.id] || chordSets['track-1'];
    let step = 0;

    synthTimerRef.current = setInterval(() => {
      const chordIndex = Math.floor(step / 4) % chords.length;
      const noteIndex = step % chords[chordIndex].length;
      const freq = chords[chordIndex][noteIndex];

      // Bass note on downbeats
      if (step % 4 === 0) {
        playSynthNote(freq / 2, 0.8, 'triangle');
      }

      // Melody arpeggio
      playSynthNote(freq, 0.4, 'sine');

      // Subtle percussive tick
      if (step % 2 === 0) {
        playSynthNote(120, 0.05, 'sawtooth');
      }

      step += 1;
    }, 450);

    return () => {
      if (synthTimerRef.current) clearInterval(synthTimerRef.current);
    };
  }, [isPlaying, currentTrackIndex, currentTrack.id]);

  // Track progress timer
  useEffect(() => {
    let timer: any = null;
    if (isPlaying) {
      timer = setInterval(() => {
        setCurrentTime((prev) => {
          if (prev >= currentTrack.duration) {
            handleNext();
            return 0;
          }
          return prev + 1;
        });
      }, 1000);
    }
    return () => {
      if (timer) clearInterval(timer);
    };
  }, [isPlaying, currentTrack.duration]);

  // Live Canvas Visualizer
  useEffect(() => {
    let animId: number;
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const render = () => {
      animId = requestAnimationFrame(render);
      ctx.clearRect(0, 0, canvas.width, canvas.height);

      const barCount = 28;
      const barWidth = canvas.width / barCount - 2;

      let freqData = new Uint8Array(barCount);
      if (analyserRef.current && isPlaying) {
        analyserRef.current.getByteFrequencyData(freqData);
      }

      for (let i = 0; i < barCount; i++) {
        let val = freqData[i] || 0;
        if (!isPlaying) val = 6;
        const barHeight = Math.max(4, (val / 255) * canvas.height * 0.9);
        const x = i * (barWidth + 2);
        const y = canvas.height - barHeight;

        const grad = ctx.createLinearGradient(0, canvas.height, 0, 0);
        grad.addColorStop(0, '#06b6d4');
        grad.addColorStop(1, '#8b5cf6');

        ctx.fillStyle = grad;
        ctx.beginPath();
        ctx.roundRect(x, y, barWidth, barHeight, [2, 2, 0, 0]);
        ctx.fill();
      }
    };

    render();
    return () => cancelAnimationFrame(animId);
  }, [isPlaying]);

  // Volume handler
  useEffect(() => {
    if (gainNodeRef.current && audioCtxRef.current) {
      gainNodeRef.current.gain.setValueAtTime(isMuted ? 0 : volume, audioCtxRef.current.currentTime);
    }
    if (audioElementRef.current) {
      audioElementRef.current.volume = isMuted ? 0 : volume;
    }
  }, [volume, isMuted]);

  const togglePlay = () => {
    ensureAudioContext();
    setIsPlaying(!isPlaying);
  };

  const handleNext = () => {
    setCurrentTime(0);
    if (isShuffle) {
      const nextIdx = Math.floor(Math.random() * playlist.length);
      setCurrentTrackIndex(nextIdx);
    } else {
      setCurrentTrackIndex((prev) => (prev + 1) % playlist.length);
    }
  };

  const handlePrev = () => {
    if (currentTime > 3) {
      setCurrentTime(0);
      return;
    }
    setCurrentTime(0);
    setCurrentTrackIndex((prev) => (prev - 1 + playlist.length) % playlist.length);
  };

  const toggleLike = (id: string) => {
    setLikedTracks((prev) => ({ ...prev, [id]: !prev[id] }));
  };

  const handleImportAudio = (e: React.ChangeEvent<HTMLInputElement>) => {
    const files = e.target.files;
    if (!files || files.length === 0) return;
    const file = files[0];
    const blobUrl = URL.createObjectURL(file);

    const newTrack: Track = {
      id: `custom-${Date.now()}`,
      title: file.name.replace(/\.[^/.]+$/, ''),
      artist: 'Local User',
      album: 'Imported Audio',
      duration: 210,
      coverGradient: 'from-emerald-500 via-teal-600 to-indigo-700',
      isCustom: true,
      audioBlobUrl: blobUrl,
    };

    setPlaylist((prev) => [newTrack, ...prev]);
    setCurrentTrackIndex(0);
    setCurrentTime(0);
    setIsPlaying(true);
  };

  const formatTime = (secs: number) => {
    const m = Math.floor(secs / 60);
    const s = Math.floor(secs % 60);
    return `${m}:${s < 10 ? '0' : ''}${s}`;
  };

  return (
    <div className="flex h-full w-full bg-[#F5F5F7] dark:bg-[#1E1E1E] text-slate-900 dark:text-slate-100 select-none overflow-hidden font-sans">
      <input
        type="file"
        ref={fileInputRef}
        onChange={handleImportAudio}
        accept="audio/*"
        className="hidden"
      />

      {/* Left Sidebar / Playlist */}
      <div className="w-64 bg-white/70 dark:bg-slate-900/80 backdrop-blur-md border-r border-black/5 dark:border-white/10 p-4 flex flex-col justify-between">
        <div>
          {/* Header */}
          <div className="flex items-center justify-between mb-4">
            <div className="flex items-center space-x-2">
              <div className="w-8 h-8 rounded-xl bg-gradient-to-tr from-rose-500 to-pink-600 flex items-center justify-center text-white shadow-md shadow-rose-950/20">
                <Music className="w-4 h-4" />
              </div>
              <span className="font-bold text-sm text-slate-800 dark:text-slate-100">Axis Music</span>
            </div>
            <button
              onClick={() => fileInputRef.current?.click()}
              className="p-1.5 rounded-lg bg-black/5 dark:bg-white/5 hover:bg-black/10 dark:hover:bg-white/10 text-slate-600 dark:text-slate-300 transition-colors text-xs flex items-center gap-1"
              title="Import audio file"
            >
              <FolderOpen className="w-3.5 h-3.5" />
            </button>
          </div>

          <div className="text-[11px] font-semibold text-slate-400 dark:text-slate-500 uppercase tracking-wider mb-2 px-1">
            Now Playing Queue
          </div>

          {/* Track List */}
          <div className="flex flex-col gap-1 overflow-y-auto max-h-[380px] pr-1">
            {playlist.map((track, idx) => {
              const isSelected = idx === currentTrackIndex;
              return (
                <button
                  key={track.id}
                  onClick={() => {
                    setCurrentTrackIndex(idx);
                    setCurrentTime(0);
                    setIsPlaying(true);
                  }}
                  className={`flex items-center justify-between p-2 rounded-xl text-left transition-all ${
                    isSelected
                      ? 'bg-rose-500/15 text-slate-900 dark:text-white border border-rose-500/30 shadow-xs'
                      : 'hover:bg-black/5 dark:hover:bg-white/5 text-slate-600 dark:text-slate-400 hover:text-slate-900 dark:hover:text-slate-200'
                  }`}
                >
                  <div className="flex items-center space-x-2.5 truncate">
                    <div
                      className={`w-7 h-7 rounded-lg bg-gradient-to-tr ${track.coverGradient} flex items-center justify-center text-white shrink-0 text-[10px] font-bold shadow-xs`}
                    >
                      {idx + 1}
                    </div>
                    <div className="truncate">
                      <div className="text-xs font-semibold truncate leading-tight text-slate-800 dark:text-slate-100">{track.title}</div>
                      <div className="text-[10px] text-slate-500 dark:text-slate-400 truncate">{track.artist}</div>
                    </div>
                  </div>
                  <span className="text-[10px] font-mono text-slate-400 dark:text-slate-500 shrink-0 ml-2">
                    {formatTime(track.duration)}
                  </span>
                </button>
              );
            })}
          </div>
        </div>

        {/* Ambient Mode Badge */}
        <div className="p-2.5 rounded-xl bg-black/5 dark:bg-white/5 border border-black/5 dark:border-white/5 text-[11px] text-slate-600 dark:text-slate-400 flex items-center gap-2">
          <Sliders className="w-3.5 h-3.5 text-cyan-500" />
          <span>Real-time Web Audio Synthesizer</span>
        </div>
      </div>

      {/* Main Player Display */}
      <div className="flex-1 flex flex-col justify-between p-6 bg-white dark:bg-slate-950/70 relative">
        {/* Top bar */}
        <div className="flex items-center justify-between">
          <span className="text-xs font-mono text-slate-500 dark:text-slate-400">
            {currentTrack.album} • {currentTrackIndex + 1} of {playlist.length}
          </span>
          <button
            onClick={() => toggleLike(currentTrack.id)}
            className={`p-2 rounded-full hover:bg-black/5 dark:hover:bg-white/10 transition-colors ${
              likedTracks[currentTrack.id] ? 'text-rose-500' : 'text-slate-400'
            }`}
          >
            <Heart className={`w-4 h-4 ${likedTracks[currentTrack.id] ? 'fill-rose-500' : ''}`} />
          </button>
        </div>

        {/* Album Artwork & Animated Visualizer */}
        <div className="flex flex-col items-center justify-center my-auto">
          {/* Vinyl / Cover Art Box */}
          <div
            className={`w-44 h-44 rounded-3xl bg-gradient-to-tr ${currentTrack.coverGradient} shadow-2xl shadow-rose-950/20 p-4 flex flex-col justify-between relative overflow-hidden transition-all duration-700 ${
              isPlaying ? 'scale-105' : 'scale-95 opacity-80'
            }`}
          >
            <div className="absolute inset-0 bg-gradient-to-b from-white/20 to-transparent pointer-events-none"></div>
            <div className="w-7 h-7 rounded-full bg-white/20 flex items-center justify-center text-white text-xs font-bold">
              ▲
            </div>
            <div className="z-10">
              <div className="text-base font-black text-white tracking-tight leading-tight drop-shadow">
                {currentTrack.title}
              </div>
              <div className="text-xs text-white/80 font-medium drop-shadow">{currentTrack.artist}</div>
            </div>
          </div>

          {/* Title & Artist info */}
          <div className="mt-4 text-center">
            <h2 className="text-lg font-bold text-slate-900 dark:text-slate-100">{currentTrack.title}</h2>
            <p className="text-xs text-slate-500 dark:text-slate-400 mt-0.5">{currentTrack.artist}</p>
          </div>

          {/* Live Frequency Spectrum Canvas */}
          <div className="w-64 h-12 mt-4">
            <canvas ref={canvasRef} width={256} height={48} className="w-full h-full" />
          </div>
        </div>

        {/* Player Controls Bar */}
        <div className="flex flex-col gap-3 max-w-lg mx-auto w-full">
          {/* Scrub bar */}
          <div className="flex items-center gap-2 text-[10px] font-mono text-slate-500 dark:text-slate-400">
            <span>{formatTime(currentTime)}</span>
            <input
              type="range"
              min="0"
              max={currentTrack.duration}
              value={currentTime}
              onChange={(e) => setCurrentTime(parseInt(e.target.value, 10))}
              className="flex-1 accent-rose-500 h-1 bg-black/10 dark:bg-white/10 rounded-full cursor-pointer"
            />
            <span>{formatTime(currentTrack.duration)}</span>
          </div>

          {/* Playback action buttons */}
          <div className="flex items-center justify-between">
            <div className="flex items-center space-x-2">
              <button
                onClick={() => setIsShuffle(!isShuffle)}
                className={`p-2 rounded-lg transition-colors ${
                  isShuffle ? 'text-rose-500 bg-rose-500/10' : 'text-slate-500 dark:text-slate-400 hover:text-slate-800 dark:hover:text-white'
                }`}
                title="Shuffle"
              >
                <Shuffle className="w-4 h-4" />
              </button>
              <button
                onClick={() => setIsRepeat(!isRepeat)}
                className={`p-2 rounded-lg transition-colors ${
                  isRepeat ? 'text-rose-500 bg-rose-500/10' : 'text-slate-500 dark:text-slate-400 hover:text-slate-800 dark:hover:text-white'
                }`}
                title="Repeat"
              >
                <Repeat className="w-4 h-4" />
              </button>
            </div>

            {/* Core Play / Skip Buttons */}
            <div className="flex items-center space-x-4">
              <button
                onClick={handlePrev}
                className="p-2 text-slate-600 dark:text-slate-300 hover:text-slate-900 dark:hover:text-white transition-colors"
                title="Previous track"
              >
                <SkipBack className="w-5 h-5" />
              </button>

              <button
                onClick={togglePlay}
                className="w-12 h-12 rounded-full bg-gradient-to-r from-rose-500 to-pink-600 hover:from-rose-400 hover:to-pink-500 text-white flex items-center justify-center shadow-lg shadow-rose-950/30 transition-transform active:scale-95"
                title={isPlaying ? 'Pause' : 'Play'}
              >
                {isPlaying ? <Pause className="w-6 h-6 fill-white" /> : <Play className="w-6 h-6 fill-white ml-0.5" />}
              </button>

              <button
                onClick={handleNext}
                className="p-2 text-slate-600 dark:text-slate-300 hover:text-slate-900 dark:hover:text-white transition-colors"
                title="Next track"
              >
                <SkipForward className="w-5 h-5" />
              </button>
            </div>

            {/* Volume Control */}
            <div className="flex items-center space-x-2">
              <button
                onClick={() => setIsMuted(!isMuted)}
                className="text-slate-500 dark:text-slate-400 hover:text-slate-800 dark:hover:text-white transition-colors"
                title={isMuted ? 'Unmute' : 'Mute'}
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
                  setVolume(parseFloat(e.target.value));
                  setIsMuted(false);
                }}
                className="w-20 accent-rose-500 h-1 bg-black/10 dark:bg-white/10 rounded-full cursor-pointer"
              />
            </div>
          </div>
        </div>
      </div>
    </div>
  );
};
