// Module 17 — lexical error recovery.
//
// Module 16's scanner throws at the first bad character. A working compiler
// reports the error, RESYNCHRONIZES, and keeps going, so one scan finds every
// lexical error instead of hiding all but the first.
//
//   SKIP_ONE         drop one character, emit an ERROR token for it, resume.
//                    One error per bad character.
//   SKIP_TO_RESTART  drop characters until some token can start again, emit
//                    ONE ERROR token for the whole run. One error per run.

export const Strategy = { SKIP_ONE: "SKIP_ONE", SKIP_TO_RESTART: "SKIP_TO_RESTART" };

export function scanRecovering(sc, src, strategy) {
  const tokens = [];
  const errors = [];
  let pos = 0, line = 1, col = 1;
  const advance = (text) => {
    for (const ch of text) { if (ch === "\n") { line++; col = 1; } else col++; }
  };

  while (pos < src.length) {
    const m = sc.longestMatch(src, pos);
    if (m !== null) {
      const rule = sc.rules[m.rule];
      const lexeme = src.slice(pos, m.end);
      if (rule.action === "token") tokens.push({ kind: sc.kindOf(rule.name, lexeme), lexeme, line, col });
      advance(lexeme);
      pos = m.end;
      continue;
    }
    // no token starts at pos: report, then resynchronize
    let end = pos + 1;
    if (strategy === Strategy.SKIP_TO_RESTART)
      while (end < src.length && sc.longestMatch(src, end) === null) end++;
    const text = src.slice(pos, end);
    errors.push({ line, col, text });
    tokens.push({ kind: "ERROR", lexeme: text, line, col });
    advance(text);
    pos = end;
  }
  tokens.push({ kind: "EOF", lexeme: "", line, col });
  return { tokens, errors };
}

export function errorStr(e) {
  return e.line + ":" + e.col + ' "' + e.text.replaceAll("\\", "\\\\").replaceAll('"', '\\"') + '"';
}
