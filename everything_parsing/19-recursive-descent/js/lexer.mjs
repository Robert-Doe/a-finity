// Module 19 — a deliberately tiny tokenizer (mirror of java/Lexer.java).
// Part II built the real lexer; this module is about parsing, so it uses the
// smallest scanner that yields a clean token stream. Every token carries its
// start position; the stream ends with an explicit EOF token.

export class LexError extends Error {
  constructor(msg, pos) { super(msg); this.pos = pos; }
}

export function tokStr(t) {
  if (t.kind === "EOF") return "EOF";
  return t.kind === t.text ? t.kind : `${t.kind}(${t.text})`;
}

const isDigit = c => c >= "0" && c <= "9";
const isLetter = c => (c >= "A" && c <= "Z") || (c >= "a" && c <= "z");
const isAlnum = c => isDigit(c) || isLetter(c);

export function lex(src) {
  const out = [];
  let i = 0;
  while (i < src.length) {
    const c = src[i];
    if (c === " " || c === "\t") { i++; continue; }
    if (isDigit(c)) {
      const s = i;
      while (i < src.length && isDigit(src[i])) i++;
      out.push({ kind: "num", text: src.slice(s, i), pos: s });
    } else if (isLetter(c)) {
      const s = i;
      while (i < src.length && isAlnum(src[i])) i++;
      out.push({ kind: "id", text: src.slice(s, i), pos: s });
    } else if (c === "+" || c === "*" || c === "(" || c === ")") {
      out.push({ kind: c, text: c, pos: i });
      i++;
    } else {
      throw new LexError(`stray character '${c}' at position ${i}`, i);
    }
  }
  out.push({ kind: "EOF", text: "", pos: src.length });
  return out;
}
