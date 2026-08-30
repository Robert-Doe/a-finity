// Module 17 — lexical error recovery.
//
// Module 16's lexer throws at the first bad character. A real scanner reports
// the error, RESYNCHRONIZES, and keeps going — one scan finds every error.
//
//   PANIC_ONE      skip one character, ERROR token for it, resume (one ERROR per bad char)
//   PANIC_TO_SYNC  skip to a plausible token start (in the alphabet AND the DFA
//                  can begin a token there); one ERROR per garbage run

export const Strategy = { PANIC_ONE: "PANIC_ONE", PANIC_TO_SYNC: "PANIC_TO_SYNC" };

// maximal-munch match at `pos`; [endPos, ruleIndex] or [-1, -1]
export function longestToken(lex, input, pos) {
  let state = lex.dfa.start;
  let lastPos = -1, lastRule = -1;
  const at = lex.stateToken.get(state);
  if (at !== undefined) { lastPos = pos; lastRule = at; }
  for (let i = pos; i < input.length; i++) {
    const sym = input[i];
    if (!lex.dfa.alphabet.includes(sym)) break;
    state = lex.dfa.step(state, sym);
    const ri = lex.stateToken.get(state);
    if (ri !== undefined) { lastPos = i + 1; lastRule = ri; }
  }
  return [lastPos, lastRule];
}

export function tokenize(lex, input, strategy) {
  const tokens = [];
  const errors = [];
  let pos = 0;

  while (pos < input.length) {
    const [end, rule] = longestToken(lex, input, pos);
    if (end > pos) {
      const kind = lex.rules[rule].name;
      if (!lex.skip.has(kind)) tokens.push({ kind, lexeme: input.slice(pos, end), pos });
      pos = end;
      continue;
    }
    // lexical error at pos: recover
    const errStart = pos;
    let errEnd = pos + 1;
    if (strategy === Strategy.PANIC_TO_SYNC) {
      while (errEnd < input.length) {
        const c = input[errEnd];
        if (lex.dfa.alphabet.includes(c) && longestToken(lex, input, errEnd)[0] > errEnd) break;
        errEnd++;
      }
    }
    const garbage = input.slice(errStart, errEnd);
    errors.push({ from: errStart, to: errEnd, text: garbage });
    tokens.push({ kind: "ERROR", lexeme: garbage, pos: errStart });
    pos = errEnd;
  }
  return { tokens, errors };
}

export function errStr(e) {
  return e.from + 1 === e.to
    ? `position ${e.from} ('${e.text}')`
    : `positions ${e.from}..${e.to - 1} ("${e.text}")`;
}
