import React, { useState, useEffect } from 'react';
import {
  FileText,
  Plus,
  Trash2,
  Search,
  Folder,
  Pin,
  Calendar,
  Clock,
  CheckSquare,
  Sparkles,
} from 'lucide-react';

interface Note {
  id: string;
  title: string;
  body: string;
  category: 'Personal' | 'Work' | 'Ideas';
  isPinned: boolean;
  updatedAt: string;
}

const DEFAULT_NOTES: Note[] = [
  {
    id: 'note-1',
    title: 'Welcome to AxisOS Notes',
    body: 'AxisOS pairs the Linux kernel with a hardware-accelerated Wayland desktop.\n\nKey features in this release:\n- Cage Wayland Compositor with KMS/DRM\n- Real system installation engine with Btrfs & EFI\n- Unattended background kernel upgrades\n- Native apps: Music Player, Safari, Terminal, Activity Monitor, and Notes',
    category: 'Personal',
    isPinned: true,
    updatedAt: 'Today at 4:20 PM',
  },
  {
    id: 'note-2',
    title: 'Linux Commands Quick Sheet',
    body: 'Useful bash commands:\n- uname -a (Check kernel architecture)\n- btrfs subvolume list / (Inspect Btrfs snapshots)\n- systemctl status axisos (Check Wayland session)\n- journalctl -u axisos-daemon -f (Stream live daemon logs)',
    category: 'Work',
    isPinned: true,
    updatedAt: 'Yesterday',
  },
  {
    id: 'note-3',
    title: 'Future App Ideas',
    body: '- Weather widget with live forecast\n- Podcast manager with RSS feed syndication\n- Code studio with syntax tree highlighting',
    category: 'Ideas',
    isPinned: false,
    updatedAt: 'Sep 25, 2026',
  },
];

export const NotesApp: React.FC = () => {
  const [notes, setNotes] = useState<Note[]>(() => {
    try {
      const saved = localStorage.getItem('axisos_notes');
      return saved ? JSON.parse(saved) : DEFAULT_NOTES;
    } catch {
      return DEFAULT_NOTES;
    }
  });

  const [activeNoteId, setActiveNoteId] = useState<string>(notes[0]?.id || 'note-1');
  const [activeCategory, setActiveCategory] = useState<string>('All');
  const [searchQuery, setSearchQuery] = useState('');

  // Persist notes
  useEffect(() => {
    try {
      localStorage.setItem('axisos_notes', JSON.stringify(notes));
    } catch {}
  }, [notes]);

  const activeNote = notes.find((n) => n.id === activeNoteId) || notes[0];

  const handleCreateNote = () => {
    const newNote: Note = {
      id: `note-${Date.now()}`,
      title: 'New Note',
      body: '',
      category: activeCategory === 'All' ? 'Personal' : (activeCategory as any),
      isPinned: false,
      updatedAt: 'Just now',
    };
    setNotes((prev) => [newNote, ...prev]);
    setActiveNoteId(newNote.id);
  };

  const handleDeleteNote = (id: string, e?: React.MouseEvent) => {
    if (e) e.stopPropagation();
    const remaining = notes.filter((n) => n.id !== id);
    setNotes(remaining);
    if (activeNoteId === id && remaining.length > 0) {
      setActiveNoteId(remaining[0].id);
    }
  };

  const handleUpdateNote = (field: 'title' | 'body', value: string) => {
    if (!activeNote) return;
    setNotes((prev) =>
      prev.map((n) =>
        n.id === activeNoteId
          ? {
              ...n,
              [field]: value,
              updatedAt: 'Just now',
            }
          : n
      )
    );
  };

  const togglePin = (id: string, e: React.MouseEvent) => {
    e.stopPropagation();
    setNotes((prev) =>
      prev.map((n) => (n.id === id ? { ...n, isPinned: !n.isPinned } : n))
    );
  };

  const filteredNotes = notes
    .filter((n) => (activeCategory === 'All' ? true : n.category === activeCategory))
    .filter(
      (n) =>
        n.title.toLowerCase().includes(searchQuery.toLowerCase()) ||
        n.body.toLowerCase().includes(searchQuery.toLowerCase())
    )
    .sort((a, b) => (b.isPinned ? 1 : 0) - (a.isPinned ? 1 : 0));

  const categories = ['All', 'Personal', 'Work', 'Ideas'];

  return (
    <div className="flex h-full w-full bg-[#0a0e0b] text-slate-100 select-none overflow-hidden font-sans">
      {/* 1. Folders Column */}
      <div className="w-48 bg-[#121814] border-r border-white/10 p-3 flex flex-col justify-between">
        <div>
          <div className="flex items-center space-x-2 px-2 py-1 mb-3">
            <div className="w-6 h-6 rounded-lg bg-[#87cf3e]/20 text-[#87cf3e] flex items-center justify-center font-bold text-xs">
              <FileText className="w-3.5 h-3.5" />
            </div>
            <span className="font-bold text-xs text-white">Axis Notes</span>
          </div>

          <div className="flex flex-col gap-0.5">
            {categories.map((cat) => (
              <button
                key={cat}
                onClick={() => setActiveCategory(cat)}
                className={`w-full px-2.5 py-1.5 rounded-lg text-left text-xs transition-colors flex items-center justify-between ${
                  activeCategory === cat
                    ? 'bg-[#87cf3e] text-black font-bold shadow-xs'
                    : 'text-slate-400 hover:text-white hover:bg-white/5'
                }`}
              >
                <div className="flex items-center space-x-2">
                  <Folder className={`w-3.5 h-3.5 ${activeCategory === cat ? 'text-black' : 'text-[#87cf3e]'}`} />
                  <span>{cat}</span>
                </div>
                <span className="text-[10px] opacity-70">
                  {cat === 'All' ? notes.length : notes.filter((n) => n.category === cat).length}
                </span>
              </button>
            ))}
          </div>
        </div>

        <div className="text-[10px] text-slate-500 font-mono px-2">
          {notes.length} total notes
        </div>
      </div>

      {/* 2. Notes List Column */}
      <div className="w-64 bg-[#141b16] border-r border-white/10 flex flex-col">
        {/* Top search & new note button */}
        <div className="p-2.5 border-b border-white/10 flex items-center gap-2">
          <div className="relative flex-1">
            <Search className="w-3 h-3 absolute left-2.5 top-2 text-slate-400" />
            <input
              type="text"
              placeholder="Search notes"
              value={searchQuery}
              onChange={(e) => setSearchQuery(e.target.value)}
              className="w-full pl-7 pr-2 py-1 bg-black/30 border border-white/10 rounded-lg text-[11px] text-white outline-none focus:border-[#87cf3e] placeholder-slate-500"
            />
          </div>
          <button
            onClick={handleCreateNote}
            className="p-1.5 rounded-lg bg-[#87cf3e] hover:bg-[#76bb33] text-black font-bold transition-colors cursor-pointer shadow-xs"
            title="Create note"
          >
            <Plus className="w-4 h-4" />
          </button>
        </div>

        {/* Notes Items */}
        <div className="flex-1 overflow-y-auto p-1.5 flex flex-col gap-1">
          {filteredNotes.map((note) => {
            const isSelected = note.id === activeNoteId;
            return (
              <div
                key={note.id}
                onClick={() => setActiveNoteId(note.id)}
                className={`p-2.5 rounded-xl cursor-pointer transition-all border text-left flex flex-col gap-1 group relative ${
                  isSelected
                    ? 'bg-[#87cf3e]/20 border-[#87cf3e]/40 text-white shadow-xs'
                    : 'bg-[#18221b] border-white/5 text-slate-300 hover:bg-white/5'
                }`}
              >
                <div className="flex items-center justify-between">
                  <span className="font-semibold text-xs truncate max-w-[150px]">
                    {note.title || 'Untitled'}
                  </span>
                  <div className="flex items-center space-x-1">
                    <button
                      onClick={(e) => togglePin(note.id, e)}
                      className={`p-0.5 rounded text-slate-400 hover:text-[#87cf3e] ${
                        note.isPinned ? 'text-[#87cf3e]' : 'opacity-0 group-hover:opacity-100'
                      }`}
                      title={note.isPinned ? 'Unpin' : 'Pin'}
                    >
                      <Pin className={`w-3 h-3 ${note.isPinned ? 'fill-[#87cf3e]' : ''}`} />
                    </button>
                    <button
                      onClick={(e) => handleDeleteNote(note.id, e)}
                      className="opacity-0 group-hover:opacity-100 p-0.5 rounded text-slate-400 hover:text-rose-500"
                      title="Delete note"
                    >
                      <Trash2 className="w-3 h-3" />
                    </button>
                  </div>
                </div>

                <div className="flex items-center space-x-2 text-[10px] text-slate-400 font-mono">
                  <span>{note.updatedAt}</span>
                  <span className="truncate max-w-[100px] text-[#87cf3e]">
                    {note.body.slice(0, 30) || 'No content'}
                  </span>
                </div>
              </div>
            );
          })}
        </div>
      </div>

      {/* 3. Note Content Editor */}
      <div className="flex-1 flex flex-col bg-[#0a0e0b]">
        {activeNote ? (
          <>
            {/* Header / Date */}
            <div className="h-10 px-6 flex items-center justify-between border-b border-white/10 text-xs text-slate-400 bg-[#121814]">
              <span className="font-mono text-[11px]">{activeNote.updatedAt}</span>
              <div className="flex items-center space-x-2">
                <span className="px-2 py-0.5 rounded-full bg-[#87cf3e]/10 text-[10px] text-[#87cf3e] border border-[#87cf3e]/20">
                  {activeNote.category}
                </span>
                <button
                  onClick={() => handleDeleteNote(activeNote.id)}
                  className="p-1 rounded text-slate-400 hover:text-rose-500 cursor-pointer"
                  title="Delete note"
                >
                  <Trash2 className="w-3.5 h-3.5" />
                </button>
              </div>
            </div>

            {/* Note Title Input */}
            <input
              type="text"
              value={activeNote.title}
              onChange={(e) => handleUpdateNote('title', e.target.value)}
              placeholder="Title"
              className="px-6 pt-4 pb-2 bg-transparent text-xl font-bold text-slate-900 dark:text-slate-100 outline-none placeholder-slate-400"
            />

            {/* Note Body Textarea */}
            <textarea
              value={activeNote.body}
              onChange={(e) => handleUpdateNote('body', e.target.value)}
              placeholder="Type your note here..."
              className="flex-1 px-6 pb-6 bg-transparent text-xs text-slate-800 dark:text-slate-200 leading-relaxed outline-none resize-none font-sans cursor-text select-text placeholder-slate-400"
            />
          </>
        ) : (
          <div className="flex-1 flex items-center justify-center text-slate-400 dark:text-slate-500 text-xs">
            No note selected. Click "+" to create one.
          </div>
        )}
      </div>
    </div>
  );
};
