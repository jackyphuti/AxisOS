import React from 'react';
import { fireEvent, render, screen, waitFor } from '@testing-library/react';
import { describe, it, expect, vi, beforeEach } from 'vitest';
import { SystemStateProvider, useSystemState } from '../context/SystemStateContext';
import { WindowManagerProvider, useWindowManager } from '../context/WindowManagerContext';
import { systemService } from '../services/systemService';

function SystemStateProbe() {
  const { isLoadingSystemInfo, systemLoadError, hardwareLoadError, isLiveEnvironment, systemInfo, toggleWifi } = useSystemState();

  return (
    <div>
      <span>{`loading:${String(isLoadingSystemInfo)}`}</span>
      <span>{`error:${systemLoadError ?? 'none'}`}</span>
      <span>{`hardwareError:${hardwareLoadError ?? 'none'}`}</span>
      <span>{`live:${String(isLiveEnvironment)}`}</span>
      <span>{`os:${systemInfo.osName}`}</span>
      <button onClick={() => toggleWifi()}>toggle-wifi</button>
    </div>
  );
}

function WindowProbe() {
  const { windows, openApp, isAppRunning } = useWindowManager();

  return (
    <div>
      <button onClick={() => openApp('terminal')}>open-terminal</button>
      <span>{`count:${windows.length}`}</span>
      <span>{`running:${String(isAppRunning('terminal'))}`}</span>
    </div>
  );
}

describe('SystemStateProvider', () => {
  beforeEach(() => {
    vi.restoreAllMocks();
  });

  it('tracks loading state and resolves system metadata', async () => {
    vi.stubGlobal(
      'fetch',
      vi.fn()
        .mockResolvedValueOnce({
          ok: true,
          json: async () => ({
            osName: 'AxisOS Linux 1.0',
            osVersion: 'Horizon',
            kernelVersion: '6.12.0-axisos-amd64',
            architecture: 'x86_64',
            cpuModel: 'Intel Core',
            cpuCores: 8,
            gpuModel: 'Radeon',
            totalMemory: '16.0 GB Unified Memory',
            freeMemory: '12.4 GB Available',
            storageDevices: [],
            hostname: 'axis-pc',
            username: 'axis',
            uptime: 'up 1 hour',
            homeDir: '/home/axis',
            isLiveEnvironment: true,
          }),
        })
        .mockResolvedValueOnce({
          ok: true,
          json: async () => ({ enabled: true, connected: true, currentSsid: 'Axis-Fiber-5G' }),
        })
        .mockResolvedValueOnce({
          ok: true,
          json: async () => ({ enabled: true, controller: 'Intel Bluetooth' }),
        })
        .mockResolvedValueOnce({
          ok: true,
          json: async () => ({ volume: 70, isMuted: false, sinkName: 'Audio Out' }),
        })
    );

    render(
      <SystemStateProvider>
        <SystemStateProbe />
      </SystemStateProvider>
    );

    expect(screen.getByText('loading:true')).toBeInTheDocument();

    await waitFor(() => {
      expect(screen.getByText('live:true')).toBeInTheDocument();
    });

    expect(screen.getByText('os:AxisOS Linux 1.0')).toBeInTheDocument();
    expect(screen.getByText('error:none')).toBeInTheDocument();
  });

  it('surfaces a hardware action error when async toggling fails', async () => {
    vi.spyOn(systemService, 'getSystemInfo').mockResolvedValue({
      osName: 'AxisOS Linux 1.0',
      osVersion: 'Horizon',
      kernelVersion: '6.12.0-axisos-amd64',
      architecture: 'x86_64',
      cpuModel: 'Intel Core',
      cpuCores: 8,
      gpuModel: 'Radeon',
      totalMemory: '16.0 GB Unified Memory',
      freeMemory: '12.4 GB Available',
      storageDevices: [],
      hostname: 'axis-pc',
      username: 'axis',
      uptime: 'up 1 hour',
      homeDir: '/home/axis',
      isLiveEnvironment: false,
    });
    vi.spyOn(systemService, 'getWifiStatus').mockResolvedValue({ enabled: true, connected: true, currentSsid: 'Axis-Fiber-5G', signal: 85 });
    vi.spyOn(systemService, 'getBluetoothStatus').mockResolvedValue({ enabled: true, controller: 'Intel Bluetooth' });
    vi.spyOn(systemService, 'getAudioStatus').mockResolvedValue({ volume: 70, isMuted: false, sinkName: 'Audio Out' });
    vi.spyOn(systemService, 'toggleWifi').mockRejectedValue(new Error('network down'));

    render(
      <SystemStateProvider>
        <SystemStateProbe />
      </SystemStateProvider>
    );

    fireEvent.click(screen.getByRole('button', { name: 'toggle-wifi' }));

    await waitFor(() => {
      expect(screen.getByText('hardwareError:network down')).toBeInTheDocument();
    });

    expect(screen.getByText('loading:false')).toBeInTheDocument();
  });
});

describe('WindowManagerProvider', () => {
  it('prevents duplicate app windows and tracks open count', async () => {
    render(
      <WindowManagerProvider>
        <WindowProbe />
      </WindowManagerProvider>
    );

    const button = screen.getByRole('button', { name: 'open-terminal' });
    fireEvent.click(button);
    fireEvent.click(button);

    await waitFor(() => {
      expect(screen.getByText('count:1')).toBeInTheDocument();
    });

    expect(screen.getByText('running:true')).toBeInTheDocument();
  });
});
