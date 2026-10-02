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
  Compass,
  Radio,
  Disc,
  User,
  ListMusic,
  FolderOpen,
  Sliders,
  Sparkles,
} from 'lucide-react';
import { useWindowManager } from '../../context/WindowManagerContext';

interface Track {
  id: string;
  title: string;
  artist: string;
  album: string;
  duration: number; // in seconds
  squircleBg: string;
  iconColor: string;
  isCustom?: boolean;
  audioBlobUrl?: string;
  bpm?: number;
}

const DEFAULT_PLAYLIST: Track[] = [
  {
    id: 'track-1',
    title: 'Golden hour',
    artist: 'Nova Lane',
    album: 'Morning mix',
    duration: 204,
    squircleBg: 'bg-[#FFF1D6]',
    iconColor: 'text-[#BA7517]',
    bpm: 90,
  },
  {
    id: 'track-2',
    title: 'Midnight drive',
    artist: 'Nova Lane',
    album: 'Deep focus',
    duration: 222,
    squircleBg: 'bg-[#EBE9FD]',
    iconColor: 'text-[#6366F1]',
    bpm: 110,
  },
  {
    id: 'track-3',
    title: 'Horizon Sunrise',
    artist: 'Axis Sound Lab',
    album: 'Wayland Vibes Vol. 1',
    duration: 180,
    squircleBg: 'bg-[#FDE8D7]',
    iconColor: 'text-[#EA580C]',
    bpm: 85,
  },
  {
    id: 'track-4',
    title: 'Favorites Beat',
    artist: 'Neon Driver',
    album: 'Favorites',
    duration: 210,
    squircleBg: 'bg-[#FCE7F0]',
    iconColor: 'text-[#E11D48]',
    bpm: 105,
  },
  {
    id: 'track-5',
    title: 'Deep Space Orbit',
    artist: 'Stellar Drifter',
    album: 'Chill',
    duration: 240,
    squircleBg: 'bg-[#E2F6E7]',
    iconColor: 'text-[#16A34A]',
    bpm: 70,
  },
];

export const MusicApp: React.FC = () => {
  const { closeWindow, minimizeWindow, toggleMaximizeWindow, windows } = useWindowManager();
  const currentWindow = windows.find((w) => w.appId === 'music');

  const [activeNav, setActiveNav] = useState<'listen-now' | 'browse' | 'radio' | 'albums' | 'artists' | 'playlists'>('listen-now');
  const [playlist, setPlaylist] = useState<Track[]>(DEFAULT_PLAYLIST);
  const [currentTrackIndex, setCurrentTrackIndex] = useState(0);
  const [isPlaying, setIsPlaying] = useState(false);
  const [currentTime, setCurrentTime] = useState(0);
  const [volume, setVolume] = useState(0.8);
  const [isMuted, setIsMuted] = useState(false);
  const [isShuffle, setIsShuffle] = useState(false);
  const [isRepeat, setIsRepeat] = useState(false);
  const [likedTracks, setLikedTracks] = useState<Record<string, boolean>>({ 'track-1': true });
  const [isHoveringControls, setIsHoveringControls] = useState(false);

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

  const playSynthNote = (freq: number, duration: number, type: OscillatorType = 'sine') => {
    if (!audioCtxRef.current || !gainNodeRef.current) return;
    const ctx = audioCtxRef.current;
    const osc = ctx.createOscillator();
    const noteGain = ctx.createGain();

    osc.type = type;
    osc.frequency.setValueAtTime(freq, ctx.currentTime);

    noteGain.gain.setValueAtTime(0.001, ctx.currentTime);
    noteGain.gain.exponentialRampToValueAtTime(0.25, ctx.currentTime + 0.05);
    noteGain.gain.exponentialRampToValueAtTime(0.001, ctx.currentTime + duration);

    osc.connect(noteGain);
    noteGain.connect(gainNodeRef.current);

    osc.start();
    osc.stop(ctx.currentTime + duration);
  };

  // Generative chord progression based on track
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

    const chordSets: Record<string, number[][]> = {
      'track-1': [[261.63, 329.63, 392.0], [293.66, 349.23, 440.0], [329.63, 392.0, 493.88], [220.0, 261.63, 329.63]],
      'track-2': [[220.0, 261.63, 329.63], [174.61, 220.0, 261.63], [196.0, 246.94, 293.66], [164.81, 196.0, 246.94]],
      'track-3': [[261.63, 329.63, 392.0], [349.23, 440.0, 523.25], [392.0, 493.88, 587.33], [220.0, 261.63, 329.63]],
      'track-4': [[293.66, 369.99, 440.0], [329.63, 415.3, 493.88], [261.63, 329.63, 392.0], [220.0, 277.18, 329.63]],
      'track-5': [[130.81, 196.0, 261.63], [146.83, 220.0, 293.66], [164.81, 246.94, 329.63], [110.0, 164.81, 220.0]],
    };

    const chords = chordSets[currentTrack.id] || chordSets['track-1'];
    let step = 0;

    synthTimerRef.current = setInterval(() => {
      const chordIndex = Math.floor(step / 4) % chords.length;
      const noteIndex = step % chords[chordIndex].length;
      const freq = chords[chordIndex][noteIndex];

      if (step % 4 === 0) {
        playSynthNote(freq / 2, 0.7, 'triangle');
      }
      playSynthNote(freq, 0.35, 'sine');
      if (step % 2 === 0) {
        playSynthNote(120, 0.04, 'sawtooth');
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

  // Frequency Spectrum Canvas Visualizer
  useEffect(() => {
    let animId: number;
    const canvas = canvasRef.current;
    if (!canvas) return;
    const ctx = canvas.getContext('2d');
    if (!ctx) return;

    const render = () => {
      animId = requestAnimationFrame(render);
      ctx.clearRect(0, 0, canvas.width, canvas.height);

      const barCount = 18;
      const barWidth = canvas.width / barCount - 2;

      let freqData = new Uint8Array(barCount);
      if (analyserRef.current && isPlaying) {
        analyserRef.current.getByteFrequencyData(freqData);
      }

      for (let i = 0; i < barCount; i++) {
        let val = freqData[i] || 0;
        if (!isPlaying) val = 4;
        const barHeight = Math.max(3, (val / 255) * canvas.height * 0.85);
        const x = i * (barWidth + 2);
        const y = canvas.height - barHeight;

        ctx.fillStyle = '#BA7517';
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

  const playTrack = (index: number) => {
    setCurrentTrackIndex(index);
    setCurrentTime(0);
    ensureAudioContext();
    setIsPlaying(true);
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
      artist: 'Local Audio',
      album: 'Imported',
      duration: 210,
      squircleBg: 'bg-[#FFF1D6]',
      iconColor: 'text-[#BA7517]',
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

  const madeForYouCards = [
    {
      id: 'morning-mix',
      title: 'Morning mix',
      subtitle: 'Updated today',
      bg: 'bg-[#FDE8D7]',
      textColor: 'text-[#EA580C]',
      trackIndex: 0,
    },
    {
      id: 'deep-focus',
      title: 'Deep focus',
      subtitle: 'Instrumental',
      bg: 'bg-[#E1EEFD]',
      textColor: 'text-[#007AFF]',
      trackIndex: 1,
    },
    {
      id: 'favorites',
      title: 'Favorites',
      subtitle: '128 songs',
      bg: 'bg-[#FCE7F0]',
      textColor: 'text-[#E11D48]',
      trackIndex: 3,
    },
    {
      id: 'chill',
      title: 'Chill',
      subtitle: 'Easy listening',
      bg: 'bg-[#E2F6E7]',
      textColor: 'text-[#16A34A]',
      trackIndex: 4,
    },
  ];

  return (
    <div className="flex flex-col h-full w-full bg-[#FFFFFF] text-[#1C1C1E] select-none overflow-hidden font-sans">
      <input
        type="file"
        ref={fileInputRef}
        onChange={handleImportAudio}
        accept="audio/*"
        className="hidden"
      />

      <div className="flex-1 flex overflow-hidden">
        {/* ======================================================== */}
        {/* SIDEBAR: Traffic Lights + Apple/Mac style Library Nav     */}
        {/* ======================================================== */}
        <div
          data-window-drag
          className="w-52 bg-[#F7F7F9] border-r border-[#EAEAEB] p-3.5 flex flex-col justify-between shrink-0"
        >
          <div>
            {/* Top: Window Traffic Lights */}
            <div
              className="flex items-center space-x-2 mb-5 px-1 pt-0.5"
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

            {/* Top Navigation */}
            <div className="flex flex-col gap-1">
              <button
                onClick={() => setActiveNav('listen-now')}
                className={`w-full flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-all ${
                  activeNav === 'listen-now'
                    ? 'bg-[#FFF1D6] text-[#8A580C] font-semibold'
                    : 'text-[#5C5C60] hover:bg-black/5 font-medium'
                }`}
              >
                <Play className="w-4 h-4 fill-current" strokeWidth={1.5} />
                <span>Listen now</span>
              </button>

              <button
                onClick={() => setActiveNav('browse')}
                className={`w-full flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-all ${
                  activeNav === 'browse'
                    ? 'bg-[#FFF1D6] text-[#8A580C] font-semibold'
                    : 'text-[#5C5C60] hover:bg-black/5 font-medium'
                }`}
              >
                <Compass className="w-4 h-4" strokeWidth={1.8} />
                <span>Browse</span>
              </button>

              <button
                onClick={() => setActiveNav('radio')}
                className={`w-full flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-all ${
                  activeNav === 'radio'
                    ? 'bg-[#FFF1D6] text-[#8A580C] font-semibold'
                    : 'text-[#5C5C60] hover:bg-black/5 font-medium'
                }`}
              >
                <Radio className="w-4 h-4" strokeWidth={1.8} />
                <span>Radio</span>
              </button>
            </div>

            {/* Library Section */}
            <div className="mt-6">
              <div className="text-xs font-semibold text-[#8E8E93] uppercase tracking-wider px-3 mb-2">
                Library
              </div>
              <div className="flex flex-col gap-1">
                <button
                  onClick={() => setActiveNav('albums')}
                  className={`w-full flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-all ${
                    activeNav === 'albums'
                      ? 'bg-[#FFF1D6] text-[#8A580C] font-semibold'
                      : 'text-[#5C5C60] hover:bg-black/5 font-medium'
                  }`}
                >
                  <Disc className="w-4 h-4" strokeWidth={1.8} />
                  <span>Albums</span>
                </button>

                <button
                  onClick={() => setActiveNav('artists')}
                  className={`w-full flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-all ${
                    activeNav === 'artists'
                      ? 'bg-[#FFF1D6] text-[#8A580C] font-semibold'
                      : 'text-[#5C5C60] hover:bg-black/5 font-medium'
                  }`}
                >
                  <User className="w-4 h-4" strokeWidth={1.8} />
                  <span>Artists</span>
                </button>

                <button
                  onClick={() => setActiveNav('playlists')}
                  className={`w-full flex items-center gap-2.5 px-3 py-2 rounded-xl text-xs transition-all ${
                    activeNav === 'playlists'
                      ? 'bg-[#FFF1D6] text-[#8A580C] font-semibold'
                      : 'text-[#5C5C60] hover:bg-black/5 font-medium'
                  }`}
                >
                  <ListMusic className="w-4 h-4" strokeWidth={1.8} />
                  <span>Playlists</span>
                </button>
              </div>
            </div>
          </div>

          {/* Bottom: Import Local Audio button */}
          <div className="border-t border-[#EAEAEB] pt-3">
            <button
              onClick={() => fileInputRef.current?.click()}
              className="w-full flex items-center gap-2 px-3 py-2 rounded-xl text-xs text-[#5C5C60] hover:text-[#1C1C1E] hover:bg-black/5 transition-colors"
              title="Import audio file (.mp3, .wav, .ogg)"
            >
              <FolderOpen className="w-4 h-4 text-[#8E8E93]" strokeWidth={1.8} />
              <span>Import Audio</span>
            </button>
          </div>
        </div>

        {/* ======================================================== */}
        {/* MAIN DISPLAY CANVAS (Pure White #FFFFFF)                  */}
        {/* ======================================================== */}
        <div className="flex-1 flex flex-col overflow-hidden bg-[#FFFFFF]">
          {/* Draggable Title Header strip */}
          <div data-window-drag className="h-8 shrink-0 bg-transparent" />

          {/* Scrollable Content */}
          <div className="flex-1 overflow-y-auto px-8 pb-8 flex flex-col gap-7">
            {/* Page Header */}
            <div>
              <h1 className="text-3xl font-bold text-[#1C1C1E] tracking-tight">
                {activeNav === 'listen-now' ? 'Listen now' : activeNav.replace('-', ' ')}
              </h1>
            </div>

            {/* Section: Made for you */}
            <div className="flex flex-col gap-3">
              <h2 className="text-base font-bold text-[#1C1C1E]">Made for you</h2>

              <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
                {madeForYouCards.map((card) => (
                  <button
                    key={card.id}
                    onClick={() => playTrack(card.trackIndex)}
                    className="flex flex-col text-left group transition-transform active:scale-98 cursor-pointer"
                  >
                    {/* Square Pastel Album Artwork */}
                    <div
                      className={`w-full aspect-square rounded-2xl ${card.bg} ${card.textColor} p-4 flex flex-col justify-between shadow-2xs group-hover:shadow-xs group-hover:scale-[1.02] transition-all relative overflow-hidden`}
                    >
                      <div className="w-7 h-7 rounded-full bg-white/40 flex items-center justify-center">
                        <Music className="w-3.5 h-3.5" strokeWidth={2} />
                      </div>
                      <div className="opacity-0 group-hover:opacity-100 transition-opacity absolute right-3 bottom-3 w-9 h-9 rounded-full bg-white text-[#1C1C1E] flex items-center justify-center shadow-md">
                        <Play className="w-4 h-4 fill-current ml-0.5" />
                      </div>
                    </div>

                    {/* Metadata */}
                    <div className="mt-2.5">
                      <h3 className="text-sm font-semibold text-[#1C1C1E] group-hover:text-[#BA7517] transition-colors leading-snug">
                        {card.title}
                      </h3>
                      <p className="text-xs text-[#8E8E93] mt-0.5">{card.subtitle}</p>
                    </div>
                  </button>
                ))}
              </div>
            </div>

            {/* Section: Recently played */}
            <div className="flex flex-col gap-3">
              <h2 className="text-base font-bold text-[#1C1C1E]">Recently played</h2>

              <div className="flex flex-col divide-y divide-[#EFEFF1]">
                {playlist.map((track, idx) => {
                  const isCurrent = idx === currentTrackIndex;
                  return (
                    <div
                      key={track.id}
                      onClick={() => playTrack(idx)}
                      className={`flex items-center justify-between py-3 px-2 rounded-xl transition-all cursor-pointer ${
                        isCurrent ? 'bg-[#FFF9EE]' : 'hover:bg-[#F7F7F9]'
                      }`}
                    >
                      <div className="flex items-center gap-3.5">
                        <div
                          className={`w-11 h-11 rounded-xl ${track.squircleBg} ${track.iconColor} flex items-center justify-center shrink-0 shadow-2xs`}
                        >
                          <Music className="w-5 h-5" strokeWidth={1.8} />
                        </div>
                        <div>
                          <h4 className={`text-sm font-semibold ${isCurrent ? 'text-[#BA7517]' : 'text-[#1C1C1E]'}`}>
                            {track.title}
                          </h4>
                          <p className="text-xs text-[#8E8E93]">{track.artist}</p>
                        </div>
                      </div>

                      <div className="flex items-center gap-4">
                        <span className="text-xs text-[#8E8E93] font-mono">
                          {formatTime(track.duration)}
                        </span>
                        <button
                          onClick={(e) => {
                            e.stopPropagation();
                            toggleLike(track.id);
                          }}
                          className={`p-1.5 rounded-full hover:bg-black/5 transition-colors ${
                            likedTracks[track.id] ? 'text-rose-500' : 'text-[#8E8E93]'
                          }`}
                        >
                          <Heart
                            className={`w-4 h-4 ${likedTracks[track.id] ? 'fill-rose-500' : ''}`}
                            strokeWidth={1.8}
                          />
                        </button>
                      </div>
                    </div>
                  );
                })}
              </div>
            </div>
          </div>
        </div>
      </div>

      {/* ======================================================== */}
      {/* BOTTOM PLAYER BAR (Pixel-Perfect Mockup 3)               */}
      {/* ======================================================== */}
      <div className="h-20 bg-white border-t border-[#EAEAEB] px-6 flex items-center justify-between shrink-0 shadow-2xs">
        {/* Left: Amber Squircle Art + Title + Artist */}
        <div className="flex items-center gap-3.5 w-60">
          <div className="w-12 h-12 rounded-xl bg-[#FFF1D6] text-[#BA7517] flex items-center justify-center shrink-0 shadow-2xs">
            <Music className="w-5 h-5" strokeWidth={1.8} />
          </div>
          <div className="truncate">
            <div className="text-xs font-semibold text-[#1C1C1E] truncate">
              {currentTrack.title}
            </div>
            <div className="text-[11px] text-[#8E8E93] truncate">{currentTrack.artist}</div>
          </div>
          <button
            onClick={() => toggleLike(currentTrack.id)}
            className={`p-1.5 rounded-full hover:bg-black/5 transition-colors ml-1 shrink-0 ${
              likedTracks[currentTrack.id] ? 'text-rose-500' : 'text-[#8E8E93]'
            }`}
          >
            <Heart
              className={`w-4 h-4 ${likedTracks[currentTrack.id] ? 'fill-rose-500' : ''}`}
              strokeWidth={1.8}
            />
          </button>
        </div>

        {/* Center: Controls + Amber Scrubber Bar */}
        <div className="flex flex-col items-center gap-1.5 max-w-md w-full px-4">
          {/* Action buttons */}
          <div className="flex items-center space-x-5">
            <button
              onClick={() => setIsShuffle(!isShuffle)}
              className={`p-1 transition-colors ${
                isShuffle ? 'text-[#BA7517]' : 'text-[#8E8E93] hover:text-[#1C1C1E]'
              }`}
              title="Shuffle"
            >
              <Shuffle className="w-3.5 h-3.5" strokeWidth={1.8} />
            </button>

            <button
              onClick={handlePrev}
              className="p-1 text-[#1C1C1E] hover:opacity-75 transition-opacity"
              title="Previous"
            >
              <SkipBack className="w-4 h-4 fill-current" />
            </button>

            {/* Solid Black Play/Pause Circle */}
            <button
              onClick={togglePlay}
              className="w-10 h-10 rounded-full bg-[#1C1C1E] text-white flex items-center justify-center hover:bg-black transition-transform active:scale-95 shadow-xs"
              title={isPlaying ? 'Pause' : 'Play'}
            >
              {isPlaying ? (
                <Pause className="w-4 h-4 fill-current" />
              ) : (
                <Play className="w-4 h-4 fill-current ml-0.5" />
              )}
            </button>

            <button
              onClick={handleNext}
              className="p-1 text-[#1C1C1E] hover:opacity-75 transition-opacity"
              title="Next"
            >
              <SkipForward className="w-4 h-4 fill-current" />
            </button>

            <button
              onClick={() => setIsRepeat(!isRepeat)}
              className={`p-1 transition-colors ${
                isRepeat ? 'text-[#BA7517]' : 'text-[#8E8E93] hover:text-[#1C1C1E]'
              }`}
              title="Repeat"
            >
              <Repeat className="w-3.5 h-3.5" strokeWidth={1.8} />
            </button>
          </div>

          {/* Scrub bar */}
          <div className="flex items-center gap-2.5 w-full text-[10px] font-mono text-[#8E8E93]">
            <span className="w-7 text-right">{formatTime(currentTime)}</span>
            <input
              type="range"
              min="0"
              max={currentTrack.duration}
              value={currentTime}
              onChange={(e) => setCurrentTime(parseInt(e.target.value, 10))}
              className="flex-1 accent-[#BA7517] h-1 bg-[#F0F1F4] rounded-full cursor-pointer"
            />
            <span className="w-7">{formatTime(currentTrack.duration)}</span>
          </div>
        </div>

        {/* Right: Volume & Real-time Visualizer Mini Canvas */}
        <div className="flex items-center justify-end space-x-3 w-60">
          <div className="w-16 h-4 opacity-70">
            <canvas ref={canvasRef} width={64} height={16} className="w-full h-full" />
          </div>

          <div className="flex items-center space-x-1.5">
            <button
              onClick={() => setIsMuted(!isMuted)}
              className="text-[#8E8E93] hover:text-[#1C1C1E] transition-colors"
              title={isMuted ? 'Unmute' : 'Mute'}
            >
              {isMuted || volume === 0 ? (
                <VolumeX className="w-4 h-4" strokeWidth={1.8} />
              ) : (
                <Volume2 className="w-4 h-4" strokeWidth={1.8} />
              )}
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
              className="w-20 accent-[#BA7517] h-1 bg-[#F0F1F4] rounded-full cursor-pointer"
            />
          </div>
        </div>
      </div>
    </div>
  );
};
