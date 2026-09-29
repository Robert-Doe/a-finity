// Module 16 — the pattern language a real token spec needs, as SUGAR over
// Module 3's six regex node kinds.
//
// Module 3's syntax has no escaping and no character classes, so it cannot
// spell `(`, `|`, a space or "any digit" without pain. This layer adds exactly
// what a working token spec needs, and compiles every piece of it down to
// Empty / Epsilon / Char / Union / Concat / Star. Nothing downstream changes:
// Thompson (Module 12) and the subset construction (Module 13) run unmodified.
//
//   pattern = seq ('|' seq)*
//   seq     = item*                          (zero items = epsilon)
//   item    = atom ('*' | '+' | '?')*
//   atom    = '(' pattern ')'
//           | '[' '^'? classItem+ ']'        character class, ranges a-z
//           | '"' literalChar* '"'           quoted literal, no escaping needed
//           | '\' escape                     \n \t \s(space) or any metachar
//           | plainChar
//
// SYMBOLS. The automaton file format (Module 10) separates fields with
// whitespace and treats '#' as a comment, so four characters get a symbol
// NAME instead of standing for themselves: space=SP, tab=TAB, newline=NL,
// '#'=HASH. Every other printable ASCII character is its own symbol.

import { epsilon, chr, alt, cat, star } from "./regex.mjs";

// The source alphabet: tab, newline, and printable ASCII 0x20..0x7E.
export const SOURCE_CHARS = (() => {
  const out = ["\t", "\n"];
  for (let c = 0x20; c <= 0x7e; c++) out.push(String.fromCharCode(c));
  return out;
})();

const NAMED = new Map([[" ", "SP"], ["\t", "TAB"], ["\n", "NL"], ["#", "HASH"]]);

export function symbolOf(ch) {
  return NAMED.get(ch) ?? ch;
}

export function isSourceChar(ch) {
  return ch === "\t" || ch === "\n" || (ch >= " " && ch <= "~");
}

const META = new Set(["(", ")", "|", "*", "+", "?", "[", "]", '"', "\\"]);

export class PatternError extends Error {}

export function parsePattern(src) {
  const p = new PatternParser(src);
  const node = p.pattern();
  if (!p.eof()) throw new PatternError(`unexpected '${p.peek()}' at column ${p.pos + 1}`);
  return node;
}

function unionOf(chars) {
  if (chars.length === 0) throw new PatternError("empty character class");
  let node = chr(symbolOf(chars[0]));
  for (let i = 1; i < chars.length; i++) node = alt(node, chr(symbolOf(chars[i])));
  return node;
}

class PatternParser {
  constructor(s) { this.s = s; this.pos = 0; }
  eof()  { return this.pos >= this.s.length; }
  peek() { return this.s[this.pos]; }
  take(c) { if (this.peek() === c) { this.pos++; return true; } return false; }

  pattern() {
    let left = this.seq();
    while (this.take("|")) left = alt(left, this.seq());
    return left;
  }

  seq() {
    let left = null;
    while (!this.eof() && this.peek() !== "|" && this.peek() !== ")") {
      const next = this.item();
      left = left === null ? next : cat(left, next);
    }
    return left ?? epsilon();
  }

  item() {
    let base = this.atom();
    for (;;) {
      if (this.take("*"))      base = star(base);
      else if (this.take("+")) base = cat(base, star(base));
      else if (this.take("?")) base = alt(base, epsilon());
      else return base;
    }
  }

  atom() {
    const col = this.pos + 1;
    const c = this.peek();
    if (this.take("(")) {
      const inner = this.pattern();
      if (!this.take(")")) throw new PatternError(`'(' at column ${col} is never closed`);
      return inner;
    }
    if (this.take("[")) return this.charClass(col);
    if (this.take('"')) return this.literal(col);
    if (this.take("\\")) return chr(symbolOf(this.escape(col)));
    if (c === " " || c === "\t")
      throw new PatternError(`bare whitespace at column ${col}; write \\s, \\t or a quoted " "`);
    if (META.has(c)) throw new PatternError(`'${c}' at column ${col} has nothing to apply to`);
    this.pos++;
    if (!isSourceChar(c)) throw new PatternError(`character at column ${col} is outside the source alphabet`);
    return chr(symbolOf(c));
  }

  escape(col) {
    if (this.eof()) throw new PatternError(`pattern ends inside an escape at column ${col}`);
    const e = this.s[this.pos++];
    if (e === "n") return "\n";
    if (e === "t") return "\t";
    if (e === "s") return " ";
    if (META.has(e) || e === "^" || e === "-") return e;
    throw new PatternError(`unknown escape '\\${e}' at column ${col}`);
  }

  classChar(col) {
    if (this.eof()) throw new PatternError(`'[' at column ${col} is never closed`);
    if (this.take("\\")) return this.escape(this.pos);
    return this.s[this.pos++];
  }

  charClass(col) {
    const negate = this.take("^");
    const picked = new Set();
    while (!this.take("]")) {
      const lo = this.classChar(col);
      if (this.peek() === "-" && this.s[this.pos + 1] !== "]" && this.pos + 1 < this.s.length) {
        this.pos++;
        const hi = this.classChar(col);
        if (hi < lo) throw new PatternError(`backwards range ${lo}-${hi} in class at column ${col}`);
        for (let k = lo.charCodeAt(0); k <= hi.charCodeAt(0); k++) picked.add(String.fromCharCode(k));
      } else {
        picked.add(lo);
      }
    }
    const chars = SOURCE_CHARS.filter(ch => negate ? !picked.has(ch) : picked.has(ch));
    return unionOf(chars);
  }

  literal(col) {
    let node = null;
    while (!this.take('"')) {
      if (this.eof()) throw new PatternError(`quoted literal at column ${col} is never closed`);
      const ch = this.s[this.pos++];
      const next = chr(symbolOf(ch));
      node = node === null ? next : cat(node, next);
    }
    if (node === null) throw new PatternError(`empty quoted literal at column ${col}`);
    return node;
  }
}
