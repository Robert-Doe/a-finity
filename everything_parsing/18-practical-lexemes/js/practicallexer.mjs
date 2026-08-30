// Module 18 — messy real-world lexemes, as a HAND-CODED scanner.
//
//   RESERVED WORDS  scan an identifier, then look it up in a keyword table
//   COMMENTS        // to end of line ; /* ... */ ; NESTED /* /* */ */ needs a
//                   DEPTH COUNTER -- not regular
//   STRINGS         "..." with \n \t \" \\ \uXXXX escapes; unterminated = error
//   NUMBERS         123  12.5  1e10  1.5e-3  0xFF  -- a small state machine

export const KEYWORDS = new Set(["let", "in", "if", "then", "else", "true", "false"]);

const isLetter = c => (c >= "a" && c <= "z") || (c >= "A" && c <= "Z") || c === "_";
const isDigit = c => c >= "0" && c <= "9";
const isHex = c => isDigit(c) || (c >= "a" && c <= "f") || (c >= "A" && c <= "F");

export function lex(src) {
  let pos = 0;
  const tokens = [];
  const errors = [];

  const peek = (k = 0) => (pos + k < src.length ? src[pos + k] : "\0");
  const advance = () => src[pos++];

  const run = () => {
    while (pos < src.length) {
      const c = peek();
      if (c === " " || c === "\t" || c === "\n") { advance(); continue; }
      if (c === "/" && peek(1) === "/") { skipLineComment(); continue; }
      if (c === "/" && peek(1) === "*") { skipBlockComment(); continue; }
      if (isLetter(c)) { identifierOrKeyword(); continue; }
      if (isDigit(c)) { number(); continue; }
      if (c === '"') { string(); continue; }
      if ("+-*=".includes(c)) { tokens.push({ kind: "OP", text: advance() }); continue; }
      errors.push(`lexical error at position ${pos} (char '${c}')`);
      advance();
    }
  };

  const identifierOrKeyword = () => {
    const start = pos;
    while (isLetter(peek()) || isDigit(peek())) advance();
    const text = src.slice(start, pos);
    tokens.push({ kind: KEYWORDS.has(text) ? "KW_" + text.toUpperCase() : "ID", text });
  };

  const skipLineComment = () => { while (pos < src.length && peek() !== "\n") advance(); };

  const skipBlockComment = () => {
    const start = pos;
    advance(); advance();
    let depth = 1;                                  // <-- the counter: NOT regular
    while (pos < src.length && depth > 0) {
      if (peek() === "/" && peek(1) === "*") { advance(); advance(); depth++; }
      else if (peek() === "*" && peek(1) === "/") { advance(); advance(); depth--; }
      else advance();
    }
    if (depth > 0) errors.push(`unterminated block comment opened at position ${start}`);
  };

  const string = () => {
    const start = pos;
    advance();
    let value = "";
    while (pos < src.length && peek() !== '"' && peek() !== "\n") {
      const c = advance();
      if (c === "\\") {
        const e = pos < src.length ? advance() : "\0";
        if (e === "n") value += "\n";
        else if (e === "t") value += "\t";
        else if (e === '"') value += '"';
        else if (e === "\\") value += "\\";
        else if (e === "u") value += unicodeEscape();
        else errors.push(`bad string escape '\\${e}' near position ${pos - 1}`);
      } else {
        value += c;
      }
    }
    if (peek() !== '"') {
      errors.push(`unterminated string literal opened at position ${start}`);
      tokens.push({ kind: "STRING?", text: value });
      return;
    }
    advance();
    tokens.push({ kind: "STRING", text: value });

    function unicodeEscape() {
      let hex = "";
      for (let i = 0; i < 4 && isHex(peek()); i++) hex += advance();
      if (hex.length !== 4) { errors.push(`bad \\u escape near position ${pos}`); return "?"; }
      return String.fromCharCode(parseInt(hex, 16));
    }
  };

  const number = () => {
    const start = pos;
    if (peek() === "0" && (peek(1) === "x" || peek(1) === "X")) {
      advance(); advance();
      const h = pos;
      while (isHex(peek())) advance();
      if (pos === h) errors.push(`hex literal with no digits at position ${start}`);
      tokens.push({ kind: "HEX", text: src.slice(start, pos) });
      return;
    }
    while (isDigit(peek())) advance();
    let kind = "INT";
    if (peek() === "." && isDigit(peek(1))) {
      kind = "FLOAT";
      advance();
      while (isDigit(peek())) advance();
    }
    if (peek() === "e" || peek() === "E") {
      const mark = pos;
      advance();
      if (peek() === "+" || peek() === "-") advance();
      if (isDigit(peek())) { kind = "FLOAT"; while (isDigit(peek())) advance(); }
      else pos = mark;
    }
    tokens.push({ kind, text: src.slice(start, pos) });
  };

  run();
  return { tokens, errors };
}

export function tokStr(t) {
  return t.kind + ' "' + t.text.replace(/\n/g, "\\n").replace(/\t/g, "\\t") + '"';
}
