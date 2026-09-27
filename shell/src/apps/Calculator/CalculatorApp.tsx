import React, { useState, useEffect } from 'react';

export const CalculatorApp: React.FC = () => {
  const [display, setDisplay] = useState('0');
  const [prevValue, setPrevValue] = useState<number | null>(null);
  const [operation, setOperation] = useState<string | null>(null);
  const [waitingForNext, setWaitingForNext] = useState(false);

  const inputDigit = (digit: string) => {
    if (waitingForNext) {
      setDisplay(digit);
      setWaitingForNext(false);
    } else {
      setDisplay(display === '0' ? digit : display + digit);
    }
  };

  const inputDecimal = () => {
    if (waitingForNext) {
      setDisplay('0.');
      setWaitingForNext(false);
      return;
    }
    if (!display.includes('.')) {
      setDisplay(display + '.');
    }
  };

  const clear = () => {
    setDisplay('0');
    setPrevValue(null);
    setOperation(null);
    setWaitingForNext(false);
  };

  const toggleSign = () => {
    const val = parseFloat(display);
    setDisplay(String(-val));
  };

  const inputPercent = () => {
    const val = parseFloat(display);
    setDisplay(String(val / 100));
  };

  const performOperation = (nextOp: string) => {
    const inputValue = parseFloat(display);

    if (prevValue === null) {
      setPrevValue(inputValue);
    } else if (operation) {
      const current = prevValue || 0;
      let result = current;

      switch (operation) {
        case '+':
          result = current + inputValue;
          break;
        case '-':
          result = current - inputValue;
          break;
        case '×':
          result = current * inputValue;
          break;
        case '÷':
          result = inputValue !== 0 ? current / inputValue : 0;
          break;
        default:
          break;
      }

      setPrevValue(result);
      setDisplay(String(result));
    }

    setWaitingForNext(true);
    setOperation(nextOp === '=' ? null : nextOp);
  };

  // Keyboard navigation
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if (e.key >= '0' && e.key <= '9') inputDigit(e.key);
      if (e.key === '.') inputDecimal();
      if (e.key === '=' || e.key === 'Enter') performOperation('=');
      if (e.key === '+') performOperation('+');
      if (e.key === '-') performOperation('-');
      if (e.key === '*') performOperation('×');
      if (e.key === '/') performOperation('÷');
      if (e.key === 'Escape' || e.key === 'c' || e.key === 'C') clear();
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  });

  return (
    <div className="flex flex-col h-full w-full bg-slate-950 p-3 select-none justify-between">
      {/* Number Display */}
      <div className="h-20 flex items-end justify-end px-3 py-2 text-right">
        <span className="text-4xl font-light font-mono text-white tracking-tight truncate max-w-full">
          {display}
        </span>
      </div>

      {/* Button Grid */}
      <div className="grid grid-cols-4 gap-2 text-sm font-medium">
        {/* Row 1 */}
        <button
          onClick={clear}
          className="h-12 rounded-xl bg-slate-700/80 hover:bg-slate-600 text-slate-200 transition-colors flex items-center justify-center font-semibold"
        >
          {display !== '0' ? 'C' : 'AC'}
        </button>
        <button
          onClick={toggleSign}
          className="h-12 rounded-xl bg-slate-700/80 hover:bg-slate-600 text-slate-200 transition-colors flex items-center justify-center"
        >
          ±
        </button>
        <button
          onClick={inputPercent}
          className="h-12 rounded-xl bg-slate-700/80 hover:bg-slate-600 text-slate-200 transition-colors flex items-center justify-center"
        >
          %
        </button>
        <button
          onClick={() => performOperation('÷')}
          className={`h-12 rounded-xl text-white text-lg transition-colors flex items-center justify-center font-bold ${
            operation === '÷' ? 'bg-white text-amber-500' : 'bg-amber-500 hover:bg-amber-400'
          }`}
        >
          ÷
        </button>

        {/* Row 2 */}
        <button
          onClick={() => inputDigit('7')}
          className="h-12 rounded-xl bg-slate-800/80 hover:bg-slate-700 text-white transition-colors flex items-center justify-center"
        >
          7
        </button>
        <button
          onClick={() => inputDigit('8')}
          className="h-12 rounded-xl bg-slate-800/80 hover:bg-slate-700 text-white transition-colors flex items-center justify-center"
        >
          8
        </button>
        <button
          onClick={() => inputDigit('9')}
          className="h-12 rounded-xl bg-slate-800/80 hover:bg-slate-700 text-white transition-colors flex items-center justify-center"
        >
          9
        </button>
        <button
          onClick={() => performOperation('×')}
          className={`h-12 rounded-xl text-white text-lg transition-colors flex items-center justify-center font-bold ${
            operation === '×' ? 'bg-white text-amber-500' : 'bg-amber-500 hover:bg-amber-400'
          }`}
        >
          ×
        </button>

        {/* Row 3 */}
        <button
          onClick={() => inputDigit('4')}
          className="h-12 rounded-xl bg-slate-800/80 hover:bg-slate-700 text-white transition-colors flex items-center justify-center"
        >
          4
        </button>
        <button
          onClick={() => inputDigit('5')}
          className="h-12 rounded-xl bg-slate-800/80 hover:bg-slate-700 text-white transition-colors flex items-center justify-center"
        >
          5
        </button>
        <button
          onClick={() => inputDigit('6')}
          className="h-12 rounded-xl bg-slate-800/80 hover:bg-slate-700 text-white transition-colors flex items-center justify-center"
        >
          6
        </button>
        <button
          onClick={() => performOperation('-')}
          className={`h-12 rounded-xl text-white text-lg transition-colors flex items-center justify-center font-bold ${
            operation === '-' ? 'bg-white text-amber-500' : 'bg-amber-500 hover:bg-amber-400'
          }`}
        >
          –
        </button>

        {/* Row 4 */}
        <button
          onClick={() => inputDigit('1')}
          className="h-12 rounded-xl bg-slate-800/80 hover:bg-slate-700 text-white transition-colors flex items-center justify-center"
        >
          1
        </button>
        <button
          onClick={() => inputDigit('2')}
          className="h-12 rounded-xl bg-slate-800/80 hover:bg-slate-700 text-white transition-colors flex items-center justify-center"
        >
          2
        </button>
        <button
          onClick={() => inputDigit('3')}
          className="h-12 rounded-xl bg-slate-800/80 hover:bg-slate-700 text-white transition-colors flex items-center justify-center"
        >
          3
        </button>
        <button
          onClick={() => performOperation('+')}
          className={`h-12 rounded-xl text-white text-lg transition-colors flex items-center justify-center font-bold ${
            operation === '+' ? 'bg-white text-amber-500' : 'bg-amber-500 hover:bg-amber-400'
          }`}
        >
          +
        </button>

        {/* Row 5 */}
        <button
          onClick={() => inputDigit('0')}
          className="col-span-2 h-12 rounded-xl bg-slate-800/80 hover:bg-slate-700 text-white transition-colors flex items-center justify-start pl-6"
        >
          0
        </button>
        <button
          onClick={inputDecimal}
          className="h-12 rounded-xl bg-slate-800/80 hover:bg-slate-700 text-white transition-colors flex items-center justify-center font-bold"
        >
          .
        </button>
        <button
          onClick={() => performOperation('=')}
          className="h-12 rounded-xl bg-amber-500 hover:bg-amber-400 text-white text-lg transition-colors flex items-center justify-center font-bold"
        >
          =
        </button>
      </div>
    </div>
  );
};
