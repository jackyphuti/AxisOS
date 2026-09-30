// AxisOS Shell Lexical Analyzer & Tokenizer
// Compliant with POSIX shell grammar: quotes, escapes, variable expansion, operators, and I/O redirection

export type Operator = '&&' | '||' | ';';

export interface Redirection {
  type: '>' | '>>' | '<' | '2>' | '2>>' | '&>' | '>&' | '2>&1';
  target: string;
}

export interface CommandAST {
  raw: string;
  argv: string[];
  redirections: Redirection[];
}

export interface PipelineAST {
  commands: CommandAST[];
}

export interface StatementAST {
  pipeline: PipelineAST;
  operator?: Operator;
}

/**
 * Expand environment variables ($VAR, ${VAR}, $?, $$, $0, $USER, etc.)
 */
export function expandVariables(text: string, env: Record<string, string>): string {
  let result = '';
  let i = 0;
  const len = text.length;

  while (i < len) {
    const ch = text[i];

    if (ch === '\\' && i + 1 < len) {
      // Escaped character
      const next = text[i + 1];
      if (next === '$') {
        result += '$';
        i += 2;
        continue;
      }
      result += ch + next;
      i += 2;
      continue;
    }

    if (ch === '$') {
      i++;
      if (i >= len) {
        result += '$';
        break;
      }

      // Check special variables
      if (text[i] === '?') {
        result += env['?'] !== undefined ? env['?'] : '0';
        i++;
        continue;
      }
      if (text[i] === '$') {
        result += '1042'; // Shell PID
        i++;
        continue;
      }
      if (text[i] === '#') {
        result += '0';
        i++;
        continue;
      }
      if (text[i] === '0') {
        result += env['0'] || 'axis-sh';
        i++;
        continue;
      }

      // Check ${VAR}
      if (text[i] === '{') {
        const closeIdx = text.indexOf('}', i + 1);
        if (closeIdx !== -1) {
          const varName = text.substring(i + 1, closeIdx);
          result += env[varName] !== undefined ? env[varName] : '';
          i = closeIdx + 1;
          continue;
        }
      }

      // Standard variable name [A-Za-z_][A-Za-z0-9_]*
      let varName = '';
      while (i < len && /[A-Za-z0-9_]/.test(text[i])) {
        varName += text[i];
        i++;
      }

      if (varName.length > 0) {
        result += env[varName] !== undefined ? env[varName] : '';
      } else {
        result += '$';
      }
      continue;
    }

    result += ch;
    i++;
  }

  return result;
}

/**
 * Tokenize a raw command line into a list of word tokens and operator tokens
 */
export interface ShellToken {
  type: 'WORD' | 'OP' | 'PIPE' | 'REDIRECT';
  value: string;
  op?: string;
}

export function tokenizeCommandLine(input: string, env: Record<string, string>): ShellToken[] {
  const tokens: ShellToken[] = [];
  let currentWord = '';
  let inSingleQuote = false;
  let inDoubleQuote = false;
  let i = 0;
  const len = input.length;

  const flushWord = () => {
    if (currentWord.length > 0) {
      tokens.push({ type: 'WORD', value: currentWord });
      currentWord = '';
    }
  };

  while (i < len) {
    const ch = input[i];

    // Inside single quotes: literal verbatim until closing single quote
    if (inSingleQuote) {
      if (ch === "'") {
        inSingleQuote = false;
      } else {
        currentWord += ch;
      }
      i++;
      continue;
    }

    // Inside double quotes: handles escapes and variable expansion
    if (inDoubleQuote) {
      if (ch === '"') {
        inDoubleQuote = false;
        i++;
        continue;
      }
      if (ch === '\\' && i + 1 < len) {
        const next = input[i + 1];
        if (next === '"' || next === '\\' || next === '$' || next === '`') {
          currentWord += next;
          i += 2;
          continue;
        }
      }
      if (ch === '$') {
        // Expand variable within double quotes
        let j = i + 1;
        if (j < len && (input[j] === '?' || input[j] === '$' || input[j] === '0')) {
          const varText = input.substring(i, j + 1);
          currentWord += expandVariables(varText, env);
          i = j + 1;
          continue;
        }
        if (j < len && input[j] === '{') {
          const end = input.indexOf('}', j + 1);
          if (end !== -1) {
            const varText = input.substring(i, end + 1);
            currentWord += expandVariables(varText, env);
            i = end + 1;
            continue;
          }
        }
        let varName = '';
        while (j < len && /[A-Za-z0-9_]/.test(input[j])) {
          varName += input[j];
          j++;
        }
        if (varName.length > 0) {
          currentWord += env[varName] !== undefined ? env[varName] : '';
          i = j;
        } else {
          currentWord += '$';
          i++;
        }
        continue;
      }

      currentWord += ch;
      i++;
      continue;
    }

    // Outside quotes: check escape
    if (ch === '\\' && i + 1 < len) {
      currentWord += input[i + 1];
      i += 2;
      continue;
    }

    // Opening single quote
    if (ch === "'") {
      inSingleQuote = true;
      i++;
      continue;
    }

    // Opening double quote
    if (ch === '"') {
      inDoubleQuote = true;
      i++;
      continue;
    }

    // Whitespace terminates word
    if (ch === ' ' || ch === '\t' || ch === '\n' || ch === '\r') {
      flushWord();
      i++;
      continue;
    }

    // Multi-character operator checks
    if (input.startsWith('2>&1', i)) {
      flushWord();
      tokens.push({ type: 'REDIRECT', value: '2>&1' });
      i += 4;
      continue;
    }
    if (input.startsWith('2>>', i)) {
      flushWord();
      tokens.push({ type: 'REDIRECT', value: '2>>' });
      i += 3;
      continue;
    }
    if (input.startsWith('2>', i)) {
      flushWord();
      tokens.push({ type: 'REDIRECT', value: '2>' });
      i += 2;
      continue;
    }
    if (input.startsWith('>>', i)) {
      flushWord();
      tokens.push({ type: 'REDIRECT', value: '>>' });
      i += 2;
      continue;
    }
    if (input.startsWith('&&', i)) {
      flushWord();
      tokens.push({ type: 'OP', value: '&&', op: '&&' });
      i += 2;
      continue;
    }
    if (input.startsWith('||', i)) {
      flushWord();
      tokens.push({ type: 'OP', value: '||', op: '||' });
      i += 2;
      continue;
    }
    if (input.startsWith('&>', i)) {
      flushWord();
      tokens.push({ type: 'REDIRECT', value: '&>' });
      i += 2;
      continue;
    }
    if (input.startsWith('>&', i)) {
      flushWord();
      tokens.push({ type: 'REDIRECT', value: '>&' });
      i += 2;
      continue;
    }

    // Single-character operator checks
    if (ch === '|') {
      flushWord();
      tokens.push({ type: 'PIPE', value: '|' });
      i++;
      continue;
    }
    if (ch === ';') {
      flushWord();
      tokens.push({ type: 'OP', value: ';', op: ';' });
      i++;
      continue;
    }
    if (ch === '>') {
      flushWord();
      tokens.push({ type: 'REDIRECT', value: '>' });
      i++;
      continue;
    }
    if (ch === '<') {
      flushWord();
      tokens.push({ type: 'REDIRECT', value: '<' });
      i++;
      continue;
    }

    // Variable expansion outside quotes
    if (ch === '$') {
      let j = i + 1;
      if (j < len && (input[j] === '?' || input[j] === '$' || input[j] === '0')) {
        const varText = input.substring(i, j + 1);
        currentWord += expandVariables(varText, env);
        i = j + 1;
        continue;
      }
      if (j < len && input[j] === '{') {
        const end = input.indexOf('}', j + 1);
        if (end !== -1) {
          const varText = input.substring(i, end + 1);
          currentWord += expandVariables(varText, env);
          i = end + 1;
          continue;
        }
      }
      let varName = '';
      while (j < len && /[A-Za-z0-9_]/.test(input[j])) {
        varName += input[j];
        j++;
      }
      if (varName.length > 0) {
        currentWord += env[varName] !== undefined ? env[varName] : '';
        i = j;
      } else {
        currentWord += '$';
        i++;
      }
      continue;
    }

    currentWord += ch;
    i++;
  }

  flushWord();
  return tokens;
}

/**
 * Parse tokens into structured Statement ASTs
 */
export function parseCommandLine(input: string, env: Record<string, string>): StatementAST[] {
  const tokens = tokenizeCommandLine(input, env);
  const statements: StatementAST[] = [];

  let currentPipeline: CommandAST[] = [];
  let currentArgv: string[] = [];
  let currentRedirections: Redirection[] = [];
  let currentRawTokens: string[] = [];

  const flushCommand = () => {
    if (currentArgv.length > 0 || currentRedirections.length > 0) {
      currentPipeline.push({
        raw: currentRawTokens.join(' '),
        argv: currentArgv,
        redirections: currentRedirections,
      });
      currentArgv = [];
      currentRedirections = [];
      currentRawTokens = [];
    }
  };

  const flushStatement = (op?: Operator) => {
    flushCommand();
    if (currentPipeline.length > 0) {
      statements.push({
        pipeline: { commands: currentPipeline },
        operator: op,
      });
      currentPipeline = [];
    }
  };

  for (let idx = 0; idx < tokens.length; idx++) {
    const token = tokens[idx];

    if (token.type === 'OP') {
      flushStatement(token.op as Operator);
      continue;
    }

    if (token.type === 'PIPE') {
      flushCommand();
      continue;
    }

    if (token.type === 'REDIRECT') {
      currentRawTokens.push(token.value);
      if (token.value === '2>&1') {
        currentRedirections.push({ type: '2>&1', target: '&1' });
        continue;
      }

      // Next token should be the target file
      const nextToken = tokens[idx + 1];
      if (nextToken && nextToken.type === 'WORD') {
        currentRedirections.push({
          type: token.value as any,
          target: nextToken.value,
        });
        currentRawTokens.push(nextToken.value);
        idx++; // skip target
      } else {
        // Syntax error: missing redirect target
        currentRedirections.push({
          type: token.value as any,
          target: '',
        });
      }
      continue;
    }

    if (token.type === 'WORD') {
      currentArgv.push(token.value);
      currentRawTokens.push(token.value);
    }
  }

  flushStatement();
  return statements;
}
