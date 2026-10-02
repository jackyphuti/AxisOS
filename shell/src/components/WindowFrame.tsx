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

  if (win.isMinimized && !win.animating) {
    return null;
  }

  const windowStyle: React.CSSProperties = win.isMaximized
    ? {
        position: 'fixed',
        top: 28,
        left: 0,
        width: '100vw',
        height: 'calc(100vh - 28px)',
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

  const hasIntegratedTitlebar = ['settings', 'software', 'browser', 'music'].includes(win.appId);

  const handleIntegratedDragMouseDown = (e: React.MouseEvent) => {
    if (win.isMaximized) return;
    if ((e.target as HTMLElement).closest('.window-traffic-light, input, button, a, textarea')) return;
    if (!(e.target as HTMLElement).closest('[data-window-drag]')) return;

    setIsDragging(true);
    dragStartRef.current = {
      mouseX: e.clientX,
      mouseY: e.clientY,
      winX: win.x,
      winY: win.y,
    };
  };

  const handleIntegratedDoubleClick = (e: React.MouseEvent) => {
    if ((e.target as HTMLElement).closest('.window-traffic-light, input, button, a, textarea')) return;
    if ((e.target as HTMLElement).closest('[data-window-drag]')) {
      toggleMaximizeWindow(win.id);
    }
  };

  return (
    <div
      style={windowStyle}
      onMouseDown={(e) => {
        handleMouseDown();
        if (hasIntegratedTitlebar) handleIntegratedDragMouseDown(e);
      }}
      onDoubleClick={(e) => {
        if (hasIntegratedTitlebar) handleIntegratedDoubleClick(e);
      }}
      className={`flex flex-col ${win.isMaximized ? 'rounded-none' : 'rounded-2xl'} overflow-hidden backdrop-blur-3xl transition-all duration-200 ease-out ${
        win.animating === 'closing'
          ? 'scale-90 opacity-0 blur-[2px] pointer-events-none duration-180 ease-in'
          : win.animating === 'minimizing'
          ? 'translate-y-32 scale-50 opacity-0 blur-[1px] pointer-events-none duration-200 ease-in'
          : win.animating === 'restoring'
          ? 'animate-in fade-in zoom-in-95 duration-200 ease-out'
          : isFocused
          ? 'ring-1 ring-black/10 dark:ring-white/20 shadow-[0_20px_50px_rgba(0,0,0,0.18)] dark:shadow-[0_25px_60px_-15px_rgba(0,0,0,0.8)] opacity-100'
          : 'ring-1 ring-black/5 dark:ring-white/10 shadow-[0_10px_30px_rgba(0,0,0,0.1)] dark:shadow-[0_15px_35px_-10px_rgba(0,0,0,0.6)] opacity-95'
      } bg-white dark:bg-slate-950/85`}
    >
      {/* macOS Unified Titlebar for standard apps */}
      {!hasIntegratedTitlebar && (
        <div
          onMouseDown={handleTitleBarMouseDown}
          onDoubleClick={() => toggleMaximizeWindow(win.id)}
          className="h-9 px-3.5 flex items-center justify-between bg-[#EBEBEB] dark:bg-slate-900/60 border-b border-black/10 dark:border-white/10 cursor-default select-none relative shrink-0"
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
            <span className="text-xs font-semibold text-slate-700 dark:text-slate-300 tracking-tight truncate max-w-[55%]">
              {win.title}
            </span>
          </div>

          {/* Right placeholder to keep balance */}
          <div className="w-14" />
        </div>
      )}

      {/* Window Body */}
      <div className="flex-1 overflow-auto bg-transparent text-slate-900 dark:text-slate-100 relative flex flex-col">
        {children}
      </div>
    </div>
  );
};
