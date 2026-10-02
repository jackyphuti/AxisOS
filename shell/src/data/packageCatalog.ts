// AxisOS Universal Application & Package Catalog
// Shared between Desktop App Store (SoftwareApp) and Terminal Engine (axis/apt)

export interface CatalogPackage {
  id: string;
  name: string;
  packageName: string;
  version: string;
  category: 'today' | 'apps' | 'games' | 'develop' | 'create' | 'updates' | 'account';
  description: string;
  longDescription: string;
  size: string;
  iconType:
    | 'browser'
    | 'terminal'
    | 'photos'
    | 'music'
    | 'code'
    | 'game'
    | 'draw'
    | 'chat'
    | 'video'
    | 'office'
    | 'box'
    | 'cpu'
    | 'activity'
    | 'generic';
  squircleBg: string;
  iconColor: string;
  installed: boolean;
  developer: string;
  aliases: string[];
  binaryPath: string;
}

export const CATALOG_PACKAGES: CatalogPackage[] = [
  // ----------------------------------------------------
  // Core Desktop & Essential Apps
  // ----------------------------------------------------
  {
    id: 'browser',
    name: 'Browser',
    packageName: 'chromium',
    version: '128.0',
    category: 'apps',
    description: 'Fast, private, Wayland-native',
    longDescription:
      'Official high-performance web browser powered by Google Chromium, Blink & V8 engine with hardware-accelerated Wayland rasterization.',
    size: '98 MB',
    iconType: 'browser',
    squircleBg: 'bg-[#E1F0FF]',
    iconColor: 'text-[#007AFF]',
    installed: true,
    developer: 'AxisOS Project',
    aliases: ['chromium', 'chrome', 'web'],
    binaryPath: '/usr/bin/chromium',
  },
  {
    id: 'terminal',
    name: 'Terminal',
    packageName: 'kitty',
    version: '0.36.0',
    category: 'develop',
    description: 'GPU-accelerated shell',
    longDescription:
      'Fast, GPU-accelerated terminal emulator for AxisOS. Features true-color rendering, ligatures, tabs, and direct POSIX compliance.',
    size: '24 MB',
    iconType: 'terminal',
    squircleBg: 'bg-[#E2F7E7]',
    iconColor: 'text-[#16A34A]',
    installed: true,
    developer: 'Kovid Goyal & AxisOS',
    aliases: ['kitty', 'console', 'shell'],
    binaryPath: '/usr/bin/kitty',
  },
  {
    id: 'photos',
    name: 'Photos',
    packageName: 'axis-photos',
    version: '2.0.1',
    category: 'create',
    description: 'Organize and edit',
    longDescription:
      'Native image catalog and dynamic wallpaper gallery for AxisOS. Offers hardware GPU canvas filters, rotation, and high dynamic range display.',
    size: '18 MB',
    iconType: 'photos',
    squircleBg: 'bg-[#FDE6F0]',
    iconColor: 'text-[#E11D48]',
    installed: true,
    developer: 'AxisOS Studio',
    aliases: ['gallery', 'images', 'image-viewer'],
    binaryPath: '/usr/bin/axis-photos',
  },
  {
    id: 'music',
    name: 'Music',
    packageName: 'axis-music',
    version: '2.0.0',
    category: 'apps',
    description: 'Your library, lossless',
    longDescription:
      'Lossless audio player and Web Audio synthesizer engine. Built-in generative synthesizers, live spectrum visualizer, and local audio importer.',
    size: '16 MB',
    iconType: 'music',
    squircleBg: 'bg-[#FDEBD9]',
    iconColor: 'text-[#EA580C]',
    installed: true,
    developer: 'Axis Sound Lab',
    aliases: ['player', 'audio', 'sound'],
    binaryPath: '/usr/bin/axis-music',
  },
  {
    id: 'tiler',
    name: 'Tiler',
    packageName: 'cage-tiler',
    version: '1.4.2',
    category: 'apps',
    description: 'Tiling, gestures, and native Wayland apps',
    longDescription:
      'Automatic window tiling manager and multi-touch trackpad gesture daemon for the Cage Wayland compositor.',
    size: '12 MB',
    iconType: 'generic',
    squircleBg: 'bg-[#EEF0FF]',
    iconColor: 'text-[#6366F1]',
    installed: false,
    developer: 'Wayland Community',
    aliases: ['cage-tiler', 'wm', 'tiling'],
    binaryPath: '/usr/bin/cage-tiler',
  },

  // ----------------------------------------------------
  // Developer Applications
  // ----------------------------------------------------
  {
    id: 'vscode',
    name: 'Visual Studio Code',
    packageName: 'code',
    version: '1.92.0',
    category: 'develop',
    description: 'Code editing. Redefined.',
    longDescription:
      'Free, powerful code editor by Microsoft. Built-in Git commands, syntax highlighting, IntelliSense code completion, snippets, and code refactoring.',
    size: '89 MB',
    iconType: 'code',
    squircleBg: 'bg-[#E1F0FF]',
    iconColor: 'text-[#007AFF]',
    installed: false,
    developer: 'Microsoft Corporation',
    aliases: ['code', 'vs-code'],
    binaryPath: '/usr/bin/code',
  },
  {
    id: 'codium',
    name: 'VSCodium',
    packageName: 'codium',
    version: '1.92.0',
    category: 'develop',
    description: 'Freely-licensed binary distribution of VS Code',
    longDescription:
      'Community-driven, freely-licensed binary distribution of Microsoft’s editor VS Code with all proprietary telemetry and tracking completely removed.',
    size: '86 MB',
    iconType: 'code',
    squircleBg: 'bg-[#E1EEFD]',
    iconColor: 'text-[#007AFF]',
    installed: false,
    developer: 'VSCodium Community',
    aliases: ['vscodium'],
    binaryPath: '/usr/bin/codium',
  },
  {
    id: 'git',
    name: 'Git Version Control',
    packageName: 'git',
    version: '2.45.2',
    category: 'develop',
    description: 'Fast distributed version control system',
    longDescription:
      'The world standard version control system for software development and project tracking with branching, merging, and remote syncing.',
    size: '32 MB',
    iconType: 'code',
    squircleBg: 'bg-[#FDEBD9]',
    iconColor: 'text-[#EA580C]',
    installed: true,
    developer: 'Git Community',
    aliases: ['git-core', 'vcs'],
    binaryPath: '/usr/bin/git',
  },
  {
    id: 'python3',
    name: 'Python 3.12',
    packageName: 'python3',
    version: '3.12.5',
    category: 'develop',
    description: 'Interactive high-level programming language & pip',
    longDescription:
      'Modern Python programming language runtime with standard libraries, pip package manager, virtual environments, and C extensions support.',
    size: '42 MB',
    iconType: 'code',
    squircleBg: 'bg-[#FFF2D6]',
    iconColor: 'text-[#D97706]',
    installed: true,
    developer: 'Python Software Foundation',
    aliases: ['python', 'pip', 'py'],
    binaryPath: '/usr/bin/python3',
  },
  {
    id: 'nodejs',
    name: 'Node.js & npm',
    packageName: 'nodejs',
    version: '20.17.0',
    category: 'develop',
    description: 'JavaScript runtime built on Chrome V8 engine',
    longDescription:
      'Asynchronous event-driven JavaScript runtime designed to build scalable network applications with built-in npm package manager.',
    size: '38 MB',
    iconType: 'cpu',
    squircleBg: 'bg-[#E2F7E7]',
    iconColor: 'text-[#16A34A]',
    installed: true,
    developer: 'OpenJS Foundation',
    aliases: ['node', 'npm', 'js'],
    binaryPath: '/usr/bin/node',
  },
  {
    id: 'rust',
    name: 'Rust Toolchain',
    packageName: 'rustc',
    version: '1.80.1',
    category: 'develop',
    description: 'Blazing fast memory-safe systems programming',
    longDescription:
      'Official Rust programming language compiler (rustc) and Cargo package manager for zero-cost abstractions, thread safety, and high performance.',
    size: '110 MB',
    iconType: 'cpu',
    squircleBg: 'bg-[#FDEBD9]',
    iconColor: 'text-[#EA580C]',
    installed: false,
    developer: 'Rust Core Team',
    aliases: ['rustc', 'cargo'],
    binaryPath: '/usr/bin/rustc',
  },
  {
    id: 'docker',
    name: 'Docker CE',
    packageName: 'docker.io',
    version: '27.1.1',
    category: 'develop',
    description: 'Automated container virtualization engine',
    longDescription:
      'Open platform for developing, shipping, and running applications inside secure and portable Linux containers.',
    size: '95 MB',
    iconType: 'box',
    squircleBg: 'bg-[#E1F0FF]',
    iconColor: 'text-[#007AFF]',
    installed: false,
    developer: 'Docker Inc.',
    aliases: ['docker', 'containerd'],
    binaryPath: '/usr/bin/docker',
  },
  {
    id: 'fastfetch',
    name: 'Fastfetch',
    packageName: 'fastfetch',
    version: '2.22.0',
    category: 'develop',
    description: 'Lightning-fast neofetch replacement in C',
    longDescription:
      'High-performance system hardware and OS information tool written in pure C for instantaneous terminal display.',
    size: '2.4 MB',
    iconType: 'terminal',
    squircleBg: 'bg-[#EBE9FD]',
    iconColor: 'text-[#6366F1]',
    installed: true,
    developer: 'Linus Dierheimer',
    aliases: ['fetch'],
    binaryPath: '/usr/bin/fastfetch',
  },
  {
    id: 'htop',
    name: 'htop Monitor',
    packageName: 'htop',
    version: '3.3.0',
    category: 'develop',
    description: 'Interactive real-time process viewer',
    longDescription:
      'Interactive text-mode process viewer and system monitor. Shows CPU meters, memory gauges, process trees, and kill controls.',
    size: '1.8 MB',
    iconType: 'activity',
    squircleBg: 'bg-[#E2F7E7]',
    iconColor: 'text-[#16A34A]',
    installed: true,
    developer: 'Hisham Muhammad',
    aliases: ['top'],
    binaryPath: '/usr/bin/htop',
  },
  {
    id: 'neovim',
    name: 'Neovim',
    packageName: 'neovim',
    version: '0.10.1',
    category: 'develop',
    description: 'Hyperextensible Vim-based text editor',
    longDescription:
      'Harness modern Lua scripting, native Language Server Protocol (LSP) client support, and asynchronous IO directly inside the GPU-accelerated AxisOS terminal.',
    size: '14 MB',
    iconType: 'terminal',
    squircleBg: 'bg-[#E2F7E7]',
    iconColor: 'text-[#16A34A]',
    installed: false,
    developer: 'Neovim Core Team',
    aliases: ['nvim', 'vim'],
    binaryPath: '/usr/bin/nvim',
  },
  {
    id: 'cpp-tools',
    name: 'C/C++ Build Toolchain',
    packageName: 'build-essential',
    version: '12.9',
    category: 'develop',
    description: 'Compilers, CMake, and build orchestration',
    longDescription:
      'Complete C/C++ compilation toolchain bundled with GNU GCC, Clang/LLVM, CMake, GNU Make, and the GDB debugger for native systems development.',
    size: '135 MB',
    iconType: 'cpu',
    squircleBg: 'bg-[#E1F0FF]',
    iconColor: 'text-[#007AFF]',
    installed: false,
    developer: 'GNU & LLVM Community',
    aliases: ['gcc', 'g++', 'clang', 'cmake', 'make', 'c++'],
    binaryPath: '/usr/bin/gcc',
  },
  {
    id: 'dotnet',
    name: 'Microsoft .NET SDK',
    packageName: 'dotnet-sdk-8.0',
    version: '8.0.401',
    category: 'develop',
    description: 'Cross-platform C# development framework',
    longDescription:
      'Architect high-throughput enterprise backends, cloud-native microservices, and ASP.NET Core web APIs with native ahead-of-time (AOT) compilation.',
    size: '210 MB',
    iconType: 'cpu',
    squircleBg: 'bg-[#EBE9FD]',
    iconColor: 'text-[#6366F1]',
    installed: false,
    developer: 'Microsoft Corporation',
    aliases: ['csharp', 'dotnet-sdk', 'aspnet'],
    binaryPath: '/usr/bin/dotnet',
  },
  {
    id: 'flutter',
    name: 'Flutter & Dart SDK',
    packageName: 'flutter-sdk',
    version: '3.24.2',
    category: 'develop',
    description: 'Google multi-platform UI framework',
    longDescription:
      'Craft fluid 60fps native Wayland desktop applications and mobile experiences with stateful hot reload, rich widgets, and ahead-of-time compilation.',
    size: '480 MB',
    iconType: 'code',
    squircleBg: 'bg-[#E1F0FF]',
    iconColor: 'text-[#02569B]',
    installed: false,
    developer: 'Google LLC',
    aliases: ['dart', 'flutter-sdk'],
    binaryPath: '/usr/bin/flutter',
  },
  {
    id: 'php',
    name: 'PHP 8 Runtime',
    packageName: 'php-cli',
    version: '8.3.10',
    category: 'develop',
    description: 'Server-side scripting language & PHP-FPM',
    longDescription:
      'Power scalable web services, enterprise WordPress environments, and modern Laravel microservices with PHP-FPM and standard database extensions.',
    size: '28 MB',
    iconType: 'code',
    squircleBg: 'bg-[#EBE9FD]',
    iconColor: 'text-[#777BB4]',
    installed: false,
    developer: 'The PHP Group',
    aliases: ['php8', 'php-fpm'],
    binaryPath: '/usr/bin/php',
  },
  {
    id: 'postgresql',
    name: 'PostgreSQL Database',
    packageName: 'postgresql',
    version: '16.4',
    category: 'develop',
    description: 'Advanced open-source relational database',
    longDescription:
      'Deploy rock-solid relational data layers with native support for complex SQL queries, JSONB document storage, and geospatial analysis.',
    size: '64 MB',
    iconType: 'box',
    squircleBg: 'bg-[#E1EEFD]',
    iconColor: 'text-[#336791]',
    installed: false,
    developer: 'PostgreSQL Global Development Group',
    aliases: ['postgres', 'psql'],
    binaryPath: '/usr/bin/psql',
  },
  {
    id: 'mongodb',
    name: 'MongoDB Community',
    packageName: 'mongodb-org',
    version: '7.0.12',
    category: 'develop',
    description: 'Flexible document-oriented NoSQL database',
    longDescription:
      'Store schema-flexible JSON/BSON records with high-throughput read/write performance and modern MongoDB Shell (mongosh) interactivity.',
    size: '82 MB',
    iconType: 'box',
    squircleBg: 'bg-[#E2F7E7]',
    iconColor: 'text-[#47A248]',
    installed: false,
    developer: 'MongoDB Inc.',
    aliases: ['mongo', 'mongosh'],
    binaryPath: '/usr/bin/mongosh',
  },
  {
    id: 'mssql-tools',
    name: 'Microsoft SQL Server CLI',
    packageName: 'mssql-tools18',
    version: '18.2.0',
    category: 'develop',
    description: 'Official SQL Server & Azure SQL utilities',
    longDescription:
      'Connect to remote Microsoft SQL Server and Azure SQL Database instances directly from the terminal using sqlcmd and bcp for database automation.',
    size: '16 MB',
    iconType: 'terminal',
    squircleBg: 'bg-[#FDEBD9]',
    iconColor: 'text-[#CC292B]',
    installed: false,
    developer: 'Microsoft Corporation',
    aliases: ['sqlcmd', 'bcp', 'mssql'],
    binaryPath: '/opt/mssql-tools18/bin/sqlcmd',
  },
  {
    id: 'vercel',
    name: 'Vercel CLI',
    packageName: 'vercel',
    version: '34.2.7',
    category: 'develop',
    description: 'Instant preview & edge deployment CLI',
    longDescription:
      'Deploy web applications and serverless microservices to Vercel global edge network in seconds with local simulation and instant rollouts.',
    size: '18 MB',
    iconType: 'terminal',
    squircleBg: 'bg-[#F5F5F7]',
    iconColor: 'text-[#000000]',
    installed: false,
    developer: 'Vercel Inc.',
    aliases: ['vercel-cli'],
    binaryPath: '/usr/bin/vercel',
  },
  {
    id: 'firebase',
    name: 'Firebase CLI',
    packageName: 'firebase-tools',
    version: '13.15.1',
    category: 'develop',
    description: 'Google Firebase cloud management suite',
    longDescription:
      'Test, emulate, and deploy cloud-native applications with Google Firebase Hosting, Firestore, and Cloud Functions directly from AxisOS.',
    size: '24 MB',
    iconType: 'terminal',
    squircleBg: 'bg-[#FFF2D6]',
    iconColor: 'text-[#FFCA28]',
    installed: false,
    developer: 'Google LLC',
    aliases: ['firebase-tools'],
    binaryPath: '/usr/bin/firebase',
  },
  {
    id: 'supabase',
    name: 'Supabase CLI',
    packageName: 'supabase',
    version: '1.191.3',
    category: 'develop',
    description: 'Local Docker stack & Postgres migrations',
    longDescription:
      'Spin up a complete local Supabase backend—including PostgreSQL, Auth, Realtime, and Edge Functions—powered by Docker on your AxisOS workstation.',
    size: '36 MB',
    iconType: 'terminal',
    squircleBg: 'bg-[#E2F7E7]',
    iconColor: 'text-[#3ECF8E]',
    installed: false,
    developer: 'Supabase Inc.',
    aliases: ['supabase-cli'],
    binaryPath: '/usr/bin/supabase',
  },

  // ----------------------------------------------------
  // Productivity & Media Applications
  // ----------------------------------------------------
  {
    id: 'discord',
    name: 'Discord',
    packageName: 'discord',
    version: '0.0.60',
    category: 'apps',
    description: 'Voice, video, and text chat',
    longDescription:
      'All-in-one communication platform with HD voice channels, video calls, direct messaging, and community server hubs.',
    size: '84 MB',
    iconType: 'chat',
    squircleBg: 'bg-[#EBE9FD]',
    iconColor: 'text-[#5865F2]',
    installed: false,
    developer: 'Discord Inc.',
    aliases: ['chat'],
    binaryPath: '/usr/bin/discord',
  },
  {
    id: 'spotify',
    name: 'Spotify',
    packageName: 'spotify-client',
    version: '1.2.42',
    category: 'apps',
    description: 'Music and podcasts for everyone',
    longDescription:
      'Stream millions of songs and podcasts from artists all around the world with personalized playlists and high-fidelity streaming.',
    size: '79 MB',
    iconType: 'music',
    squircleBg: 'bg-[#E2F7E7]',
    iconColor: 'text-[#1DB954]',
    installed: false,
    developer: 'Spotify AB',
    aliases: ['spotify-client'],
    binaryPath: '/usr/bin/spotify',
  },
  {
    id: 'telegram',
    name: 'Telegram Desktop',
    packageName: 'telegram-desktop',
    version: '5.4.1',
    category: 'apps',
    description: 'Fast and secure cloud messaging',
    longDescription:
      'Pure instant messaging — simple, fast, secure, and synced across all your devices with end-to-end encryption.',
    size: '42 MB',
    iconType: 'chat',
    squircleBg: 'bg-[#E1F0FF]',
    iconColor: 'text-[#229ED9]',
    installed: false,
    developer: 'Telegram FZ-LLC',
    aliases: ['tg', 'telegram-desktop'],
    binaryPath: '/usr/bin/telegram-desktop',
  },
  {
    id: 'vlc',
    name: 'VLC Media Player',
    packageName: 'vlc',
    version: '3.0.21',
    category: 'apps',
    description: 'The ultimate multimedia player',
    longDescription:
      'Free and open source cross-platform multimedia player and framework that plays most multimedia files, discs, streams, and devices.',
    size: '37 MB',
    iconType: 'video',
    squircleBg: 'bg-[#FDEBD9]',
    iconColor: 'text-[#EA580C]',
    installed: false,
    developer: 'VideoLAN Community',
    aliases: ['media-player'],
    binaryPath: '/usr/bin/vlc',
  },
  {
    id: 'libreoffice',
    name: 'LibreOffice',
    packageName: 'libreoffice',
    version: '24.2.5',
    category: 'apps',
    description: 'Powerful office productivity suite',
    longDescription:
      'Comprehensive open-source office suite: Writer (word processor), Calc (spreadsheets), Impress (presentations), and Draw (vector diagrams).',
    size: '178 MB',
    iconType: 'office',
    squircleBg: 'bg-[#E2F7E7]',
    iconColor: 'text-[#16A34A]',
    installed: false,
    developer: 'The Document Foundation',
    aliases: ['office', 'writer', 'calc'],
    binaryPath: '/usr/bin/libreoffice',
  },

  // ----------------------------------------------------
  // Creative & Design Applications
  // ----------------------------------------------------
  {
    id: 'blender',
    name: 'Blender',
    packageName: 'blender',
    version: '4.2.0',
    category: 'create',
    description: '3D modeling, VFX, and animation suite',
    longDescription:
      'Open-source 3D creation suite supporting modeling, rigging, animation, simulation, rendering, compositing, motion tracking, and video editing.',
    size: '240 MB',
    iconType: 'draw',
    squircleBg: 'bg-[#FDEBD9]',
    iconColor: 'text-[#F5792A]',
    installed: false,
    developer: 'Blender Foundation',
    aliases: ['3d', 'modeling'],
    binaryPath: '/usr/bin/blender',
  },
  {
    id: 'gimp',
    name: 'GIMP Photo Studio',
    packageName: 'gimp',
    version: '2.10.38',
    category: 'create',
    description: 'Advanced image manipulation & retouching',
    longDescription:
      'Professional image editing suite. Provides sophisticated tools for graphic design, retouching, drawing, layer blending, and free-form transformation.',
    size: '85 MB',
    iconType: 'draw',
    squircleBg: 'bg-[#FDE6F0]',
    iconColor: 'text-[#E11D48]',
    installed: false,
    developer: 'The GIMP Team',
    aliases: ['photoshop-alt', 'photo-editor'],
    binaryPath: '/usr/bin/gimp',
  },
  {
    id: 'inkscape',
    name: 'Inkscape',
    packageName: 'inkscape',
    version: '1.3.2',
    category: 'create',
    description: 'Professional vector graphics editor (SVG)',
    longDescription:
      'Professional vector graphics software for Linux. Create illustrations, logos, icons, diagrams, line arts, maps, and web graphics.',
    size: '48 MB',
    iconType: 'draw',
    squircleBg: 'bg-[#EBE9FD]',
    iconColor: 'text-[#6366F1]',
    installed: false,
    developer: 'Inkscape Project',
    aliases: ['illustrator-alt', 'vector'],
    binaryPath: '/usr/bin/inkscape',
  },
  {
    id: 'obs-studio',
    name: 'OBS Studio',
    packageName: 'obs-studio',
    version: '30.2.2',
    category: 'create',
    description: 'Video recording and live streaming',
    longDescription:
      'Free and open source software for video recording and live streaming on Wayland. Capture real-time video/audio mixing and high-fps recording.',
    size: '65 MB',
    iconType: 'video',
    squircleBg: 'bg-[#E1F0FF]',
    iconColor: 'text-[#007AFF]',
    installed: false,
    developer: 'OBS Project',
    aliases: ['obs', 'streamer'],
    binaryPath: '/usr/bin/obs',
  },
  {
    id: 'audacity',
    name: 'Audacity',
    packageName: 'audacity',
    version: '3.6.1',
    category: 'create',
    description: 'Multi-track audio editor and recorder',
    longDescription:
      'Easy-to-use, multi-track audio editor and recorder for podcasts, voiceovers, song mastering, sound effects, and audio restoration.',
    size: '32 MB',
    iconType: 'music',
    squircleBg: 'bg-[#E1EEFD]',
    iconColor: 'text-[#2563EB]',
    installed: false,
    developer: 'Audacity Team',
    aliases: ['audio-editor', 'sound-editor'],
    binaryPath: '/usr/bin/audacity',
  },

  // ----------------------------------------------------
  // Games & Entertainment
  // ----------------------------------------------------
  {
    id: 'supertuxkart',
    name: 'SuperTuxKart',
    packageName: 'supertuxkart',
    version: '1.4',
    category: 'games',
    description: '3D open-source arcade racing game',
    longDescription:
      'Fast-paced 3D arcade kart racer with a variety of characters, tracks, and game modes running at native 60fps on Wayland.',
    size: '620 MB',
    iconType: 'game',
    squircleBg: 'bg-[#E2F7E7]',
    iconColor: 'text-[#16A34A]',
    installed: false,
    developer: 'SuperTuxKart Team',
    aliases: ['kart', 'racing'],
    binaryPath: '/usr/bin/supertuxkart',
  },
  {
    id: 'steam',
    name: 'Steam',
    packageName: 'steam-installer',
    version: '1.0.0.79',
    category: 'games',
    description: 'The premier gaming platform',
    longDescription:
      'Digital distribution platform offering thousands of games, community features, automatic game updates, and Proton compatibility layer for Windows games.',
    size: '3.2 MB',
    iconType: 'game',
    squircleBg: 'bg-[#E1F0FF]',
    iconColor: 'text-[#1B2838]',
    installed: false,
    developer: 'Valve Corporation',
    aliases: ['steam-launcher'],
    binaryPath: '/usr/bin/steam',
  },
  {
    id: 'minetest',
    name: 'Minetest',
    packageName: 'minetest',
    version: '5.8.0',
    category: 'games',
    description: 'Open source voxel sandbox game',
    longDescription:
      'An open source infinite-world block sandbox game engine with survival, building, modding API, and active multiplayer servers.',
    size: '28 MB',
    iconType: 'game',
    squircleBg: 'bg-[#E2F7E7]',
    iconColor: 'text-[#16A34A]',
    installed: false,
    developer: 'Minetest Community',
    aliases: ['minecraft-alt', 'voxel'],
    binaryPath: '/usr/bin/minetest',
  },
  {
    id: 'retroarch',
    name: 'RetroArch',
    packageName: 'retroarch',
    version: '1.19.1',
    category: 'games',
    description: 'Classic gaming emulator frontend',
    longDescription:
      'Frontend for emulators, game engines and media players. Enables you to run classic games on a wide range of computers and consoles through slick GUI.',
    size: '45 MB',
    iconType: 'game',
    squircleBg: 'bg-[#FDE6F0]',
    iconColor: 'text-[#E11D48]',
    installed: false,
    developer: 'Libretro Community',
    aliases: ['emulator', 'retrogaming'],
    binaryPath: '/usr/bin/retroarch',
  },
];

// Helper: Get set of installed package names across local storage and defaults
export function getInstalledPackageSet(): Set<string> {
  const defaults = new Set([
    'browser',
    'chromium',
    'terminal',
    'kitty',
    'photos',
    'axis-photos',
    'music',
    'axis-music',
    'git',
    'python3',
    'nodejs',
    'fastfetch',
    'htop',
  ]);

  try {
    const raw = localStorage.getItem('axisos_installed_packages');
    if (raw) {
      const arr = JSON.parse(raw);
      if (Array.isArray(arr)) {
        arr.forEach((p: string) => defaults.add(p.toLowerCase()));
      }
    }
  } catch {}

  return defaults;
}

// Helper: Save package as installed and notify shell components
export function markPackageInstalled(pkgIdOrName: string) {
  const current = getInstalledPackageSet();
  current.add(pkgIdOrName.toLowerCase());
  try {
    localStorage.setItem('axisos_installed_packages', JSON.stringify(Array.from(current)));
    window.dispatchEvent(new Event('axisos-packages-changed'));
  } catch {}
}

// Helper: Remove package from installed set
export function markPackageUninstalled(pkgIdOrName: string) {
  const current = getInstalledPackageSet();
  current.delete(pkgIdOrName.toLowerCase());
  try {
    localStorage.setItem('axisos_installed_packages', JSON.stringify(Array.from(current)));
    window.dispatchEvent(new Event('axisos-packages-changed'));
  } catch {}
}
