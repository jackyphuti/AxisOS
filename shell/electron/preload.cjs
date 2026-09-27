const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('axisAPI', {
  getSystemInfo: () => ipcRenderer.invoke('system:getInfo'),
  executeCommand: (cmd, cwd) => ipcRenderer.invoke('system:exec', { cmd, cwd }),
  readDirectory: (dirPath) => ipcRenderer.invoke('fs:readDir', dirPath),
  readFile: (filePath) => ipcRenderer.invoke('fs:readFile', filePath),
  writeFile: (filePath, content) => ipcRenderer.invoke('fs:writeFile', { filePath, content }),
  createDirectory: (dirPath) => ipcRenderer.invoke('fs:createDir', dirPath),
  deleteItem: (targetPath) => ipcRenderer.invoke('fs:deleteFile', targetPath),
  getDisks: () => ipcRenderer.invoke('installer:getDisks'),
  startInstall: (config) => ipcRenderer.invoke('installer:start', config),
  getInstallStatus: () => ipcRenderer.invoke('installer:status'),
  powerAction: (action) => ipcRenderer.invoke('system:power', action),
});
