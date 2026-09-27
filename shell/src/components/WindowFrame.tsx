import React, { useState, useRef, useEffect } from 'react';
import { useWindowManager } from '../context/WindowManagerContext';
import { WindowState } from '../types/os';

interface WindowFrameProps {
  window: WindowState;
  children: React.ReactNode;
}

export const WindowFrame: React.FC<WindowFrameProps> = ({ window: win, children }) => {
  const {
    activeWindowId,
    focusWindow,
    closeWindow,
    minimizeWindow,
    toggleMaximizeWindow,
    updateWindowPosition,
  } = useWindowManager();

  const isFocused = activeWindowId === win.id;
  const [isDragging, setIsDragging] = useState(false);
  const [isHoveringControls, setIsHoveringControls] = useState(false);
  const dragStartRef = useRef<{ mouseX: number; mouseY: number; winX: number; winY: number }>({
    mouseX: 0,
    mouseY: 0,
    winX: win.x,
    winY: win.y,
  });

  const handleMouseDown = () => {
    focusWindow(win.id);
  };

  const handleTitleBarMouseDown = (e: React.MouseEvent) => {
    if (win.isMaximized) return;
    if ((e.target as HTMLElement).closest('.window-traffic-light')) return;

    setIsDragging(true);
    dragStartRef.current = {
      mouseX: e.clientX,
      mouseY: e.clientY,
      winX: win.x,
      winY: win.y,
    };
  };

  useEffect(() => {
    if (!isDragging) return;

    const handleMouseMove = (e: MouseEvent) => {
      const deltaX = e.clientX - dragStartRef.current.mouseX;
      const deltaY = e.clientY - dragStartRef.current.mouseY;

      const newX = Math.max(0, Math.min(window.innerWidth - 80, dragStartRef.current.winX + deltaX));
      const newY = Math.max(28, Math.min(window.innerHeight - 80, dragStartRef.current.winY + deltaY));

      updateWindowPosition(win.id, newX, newY);
    };

    const handleMouseUp = () => {
      setIsDragging(false);
    };

    window.addEventListener('mousemove', handleMouseMove);
    window.addEventListener('mouseup', handleMouseUp);

    return () => {
      window.removeEventListener('mousemove', handleMouseMove);
      window.removeEventListener('mouseup', handleMouseUp);
    };
  }, [isDragging, win.id, updateWindowPosition]);

  if (win.isMinimized) {
    return null;
  }

  const windowStyle: React.CSSProperties = win.isMaximized
    ? {
        position: 'fixed',
        top: 28,
        left: 0,
        width: '100vw',
        height: 'calc(100vh - 28px - 82px)',
        zIndex: win.zIndex,
      }
    : {
        position: 'absolute',
        left: win.x,
        top: win.y,
        width: win.width,
        height: win.height,
        zIndex: win.zIndex,
      };

  return (
    <div
      style={windowStyle}
      onMouseDown={handleMouseDown}
      className={`flex flex-col rounded-2xl overflow-hidden backdrop-blur-3xl transition-all duration-150 ${
        isFocused
          ? 'ring-1 ring-white/20 shadow-[0_25px_60px_-15px_rgba(0,0,0,0.8)] opacity-100'
          : 'ring-1 ring-white/10 shadow-[0_15px_35px_-10px_rgba(0,0,0,0.6)] opacity-95'
      } bg-slate-950/85`}
    >
      {/* macOS Unified Titlebar */}
      <div
        onMouseDown={handleTitleBarMouseDown}
        onDoubleClick={() => toggleMaximizeWindow(win.id)}
        className="h-9 px-3.5 flex items-center justify-between bg-slate-900/60 border-b border-white/10 cursor-default select-none relative"
      >
        {/* Left: macOS Traffic Lights */}
        <div
          className="flex items-center space-x-2 z-10"
          onMouseEnter={() => setIsHoveringControls(true)}
          onMouseLeave={() => setIsHoveringControls(false)}
        >
          {/* Close (Red) */}
          <button
            onClick={() => closeWindow(win.id)}
            title="Close"
            className="window-traffic-light w-3 h-3 rounded-full bg-[#ff5f56] border border-[#e0443e] flex items-center justify-center cursor-pointer transition-transform active:scale-90"
          >
            <span
              className={`text-[8px] font-black text-rose-950 leading-none ${
                isHoveringControls ? 'opacity-100' : 'opacity-0'
              }`}
            >
              ×
            </span>
          </button>

          {/* Minimize (Yellow) */}
          <button
            onClick={() => minimizeWindow(win.id)}
            title="Minimize"
            className="window-traffic-light w-3 h-3 rounded-full bg-[#ffbd2e] border border-[#dea123] flex items-center justify-center cursor-pointer transition-transform active:scale-90"
          >
            <span
              className={`text-[9px] font-black text-amber-950 leading-none -translate-y-0.5 ${
                isHoveringControls ? 'opacity-100' : 'opacity-0'
              }`}
            >
              –
            </span>
          </button>

          {/* Maximize / Zoom (Green) */}
          <button
            onClick={() => toggleMaximizeWindow(win.id)}
            title={win.isMaximized ? 'Restore' : 'Zoom'}
            className="window-traffic-light w-3 h-3 rounded-full bg-[#27c93f] border border-[#1aab29] flex items-center justify-center cursor-pointer transition-transform active:scale-90"
          >
            <span
              className={`text-[7px] font-black text-emerald-950 leading-none ${
                isHoveringControls ? 'opacity-100' : 'opacity-0'
              }`}
            >
              {win.isMaximized ? '⤢' : '+'}
            </span>
          </button>
        </div>

        {/* Center: Window Title */}
        <div className="absolute inset-0 flex items-center justify-center pointer-events-none">
          <span className="text-xs font-semibold text-slate-300/90 tracking-tight truncate max-w-[55%]">
            {win.title}
          </span>
        </div>

        {/* Right placeholder to keep balance */}
        <div className="w-14" />
      </div>

      {/* Window Body */}
      <div className="flex-1 overflow-auto bg-slate-900/60 text-slate-100 relative">
        {children}
      </div>
    </div>
  );
};
