const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('axisAPI', {
  getSystemInfo: () => ipcRenderer.invoke('system:getInfo'),
  executeCommand: (cmd, cwd) => ipcRenderer.invoke('system:exec', { cmd, cwd }),
  readDirectory: (dirPath) => ipcRenderer.invoke('fs:readDir', dirPath),
  readFile: (filePath) => ipcRenderer.invoke('fs:readFile', filePath),
  writeFile: (filePath, content) => ipcRenderer.invoke('fs:writeFile', { filePath, content }),
});
