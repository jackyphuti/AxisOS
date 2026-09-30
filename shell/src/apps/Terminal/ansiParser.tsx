// AxisOS ANSI Escape Sequence Parser & TTY Styler
// Supports SGR text formatting, 16-color, 256-color, 24-bit TrueColor, and cursor-erase sanitation

import React from 'react';

// Standard 16 ANSI color palette
const ANSI_16_FG: Record<number, string> = {
  30: '#1e293b', // Black / Dark slate
  31: '#ef4444', // Red
  32: '#22c55e', // Green
  33: '#eab308', // Yellow
  34: '#3b82f6', // Blue
  35: '#a855f7', // Magenta
  36: '#06b6d4', // Cyan
  37: '#e2e8f0', // White
  90: '#64748b', // Bright Black (Gray)
  91: '#f87171', // Bright Red
  92: '#4ade80', // Bright Green
  93: '#fde047', // Bright Yellow
  94: '#60a5fa', // Bright Blue
  95: '#c084fc', // Bright Magenta
  96: '#38bdf8', // Bright Cyan
  97: '#ffffff', // Bright White
};

const ANSI_16_BG: Record<number, string> = {
  40: '#0f172a',
  41: '#7f1d1d',
  42: '#14532d',
  43: '#713f12',
  44: '#1e3a8a',
  45: '#581c87',
  46: '#164e63',
  47: '#cbd5e1',
  100: '#334155',
  101: '#991b1b',
  102: '#166534',
  103: '#854d0e',
  104: '#1d4ed8',
  105: '#6b21a8',
  106: '#0e7490',
  107: '#f1f5f9',
};

// 256-color lookup table generator
function get256Color(n: number): string {
  if (n < 16) {
    if (n < 8) return ANSI_16_FG[30 + n] || '#cbd5e1';
    return ANSI_16_FG[90 + (n - 8)] || '#ffffff';
  }
  if (n >= 16 && n <= 231) {
    // 6x6x6 color cube
    const index = n - 16;
    const r = Math.floor(index / 36);
    const g = Math.floor((index % 36) / 6);
    const b = index % 6;
    const toHex = (v: number) => (v === 0 ? '00' : (v * 40 + 55).toString(16).padStart(2, '0'));
    return `#${toHex(r)}${toHex(g)}${toHex(b)}`;
  }
  if (n >= 232 && n <= 255) {
    // Grayscale ramp
    const gray = (n - 232) * 10 + 8;
    const hex = gray.toString(16).padStart(2, '0');
    return `#${hex}${hex}${hex}`;
  }
  return '#cbd5e1';
}

/**
 * Strip all ANSI escape sequences to get plain text string
 */
export function stripAnsi(text: string): string {
  return text
    // eslint-disable-next-line no-control-regex
    .replace(/\x1b\[[0-9;]*[a-zA-Z]/g, '')
    // eslint-disable-next-line no-control-regex
    .replace(/\x1b\([a-zA-Z]/g, '')
    // eslint-disable-next-line no-control-regex
    .replace(/\x1b\][^\x07\x1b]*(\x07|\x1b\\)/g, '')
    .replace(/\r/g, '');
}

interface StyleState {
  color?: string;
  backgroundColor?: string;
  bold?: boolean;
  dim?: boolean;
  italic?: boolean;
  underline?: boolean;
  strikethrough?: boolean;
  inverse?: boolean;
}

interface TextSpan {
  text: string;
  style: StyleState;
}

/**
 * Parses raw text with ANSI escape codes into an array of styled TextSpans
 */
export function parseAnsiToSpans(rawText: string): TextSpan[] {
  // Normalize carriage returns: handle \r\n vs \r line resets
  const normalized = rawText.replace(/\r\n/g, '\n');
  const spans: TextSpan[] = [];

  let currentStyle: StyleState = {};
  // Regex to match ANSI escape sequences: \x1b[ ... m or other CSI
  // eslint-disable-next-line no-control-regex
  const ansiRegex = /\x1b\[([0-9;]*)?([a-zA-Z])/g;

  let lastIndex = 0;
  let match: RegExpExecArray | null;

  while ((match = ansiRegex.exec(normalized)) !== null) {
    const textChunk = normalized.substring(lastIndex, match.index);
    if (textChunk.length > 0) {
      spans.push({
        text: textChunk,
        style: { ...currentStyle },
      });
    }

    const params = match[1] ? match[1].split(';').map((p) => parseInt(p, 10)) : [0];
    const command = match[2];

    if (command === 'm') {
      // SGR (Select Graphic Rendition)
      let i = 0;
      while (i < params.length) {
        const code = isNaN(params[i]) ? 0 : params[i];

        if (code === 0) {
          // Reset
          currentStyle = {};
        } else if (code === 1) {
          currentStyle.bold = true;
        } else if (code === 2) {
          currentStyle.dim = true;
        } else if (code === 3) {
          currentStyle.italic = true;
        } else if (code === 4) {
          currentStyle.underline = true;
        } else if (code === 7) {
          currentStyle.inverse = true;
        } else if (code === 9) {
          currentStyle.strikethrough = true;
        } else if (code === 22) {
          currentStyle.bold = false;
          currentStyle.dim = false;
        } else if (code === 23) {
          currentStyle.italic = false;
        } else if (code === 24) {
          currentStyle.underline = false;
        } else if (code === 27) {
          currentStyle.inverse = false;
        } else if (code === 29) {
          currentStyle.strikethrough = false;
        } else if (code >= 30 && code <= 37) {
          // Standard FG
          currentStyle.color = ANSI_16_FG[code];
        } else if (code === 39) {
          // Default FG
          delete currentStyle.color;
        } else if (code >= 40 && code <= 47) {
          // Standard BG
          currentStyle.backgroundColor = ANSI_16_BG[code];
        } else if (code === 49) {
          // Default BG
          delete currentStyle.backgroundColor;
        } else if (code >= 90 && code <= 97) {
          // Bright FG
          currentStyle.color = ANSI_16_FG[code];
        } else if (code >= 100 && code <= 107) {
          // Bright BG
          currentStyle.backgroundColor = ANSI_16_BG[code];
        } else if (code === 38) {
          // Extended FG (256 or TrueColor)
          const mode = params[i + 1];
          if (mode === 5 && params[i + 2] !== undefined) {
            currentStyle.color = get256Color(params[i + 2]);
            i += 2;
          } else if (mode === 2 && params[i + 4] !== undefined) {
            const r = params[i + 2];
            const g = params[i + 3];
            const b = params[i + 4];
            currentStyle.color = `rgb(${r}, ${g}, ${b})`;
            i += 4;
          }
        } else if (code === 48) {
          // Extended BG (256 or TrueColor)
          const mode = params[i + 1];
          if (mode === 5 && params[i + 2] !== undefined) {
            currentStyle.backgroundColor = get256Color(params[i + 2]);
            i += 2;
          } else if (mode === 2 && params[i + 4] !== undefined) {
            const r = params[i + 2];
            const g = params[i + 3];
            const b = params[i + 4];
            currentStyle.backgroundColor = `rgb(${r}, ${g}, ${b})`;
            i += 4;
          }
        }
        i++;
      }
    }

    lastIndex = ansiRegex.lastIndex;
  }

  // Trailing text chunk
  const remaining = normalized.substring(lastIndex);
  if (remaining.length > 0) {
    spans.push({
      text: remaining,
      style: { ...currentStyle },
    });
  }

  return spans;
}

/**
 * Render ANSI string into formatted React Nodes
 */
export const AnsiRenderer: React.FC<{ text: string; className?: string }> = ({ text, className = '' }) => {
  const spans = React.useMemo(() => parseAnsiToSpans(text), [text]);

  if (spans.length === 0) {
    return null;
  }

  return (
    <span className={`inline-block font-mono leading-relaxed whitespace-pre-wrap break-all ${className}`}>
      {spans.map((span, idx) => {
        let fg = span.style.color;
        let bg = span.style.backgroundColor;

        if (span.style.inverse) {
          const temp = fg || '#e2e8f0';
          fg = bg || '#0d1117';
          bg = temp;
        }

        const inlineStyle: React.CSSProperties = {
          color: fg,
          backgroundColor: bg,
          fontWeight: span.style.bold ? 700 : undefined,
          opacity: span.style.dim ? 0.6 : undefined,
          fontStyle: span.style.italic ? 'italic' : undefined,
          textDecoration: [
            span.style.underline ? 'underline' : '',
            span.style.strikethrough ? 'line-through' : '',
          ]
            .filter(Boolean)
            .join(' ') || undefined,
        };

        return (
          <span key={idx} style={inlineStyle}>
            {span.text}
          </span>
        );
      })}
    </span>
  );
};
