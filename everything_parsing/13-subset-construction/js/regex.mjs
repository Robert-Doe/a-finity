// Module 03 — a regular expression is a tree of exactly six node kinds:
//
//   BASE CASES        OPERATORS
//   Empty   (∅)       Union   (r | s)
//   Epsilon (ε)       Concat  (r s)
//   Char    (c)       Star    (r *)
//
// `+` and `?` are sugar, desugared at parse time:
//   r+  ->  Concat(r, Star(r))       r?  ->  Union(r, Epsilon)
//
// Precedence, tightest first:  *  >  concatenation  >  |
// Grammar the parser implements (hand-rolled; the technique is Module 19):
//   regex  = concat ('|' concat)*
//   concat = repeat*                 (juxtaposition; zero items = ε)
//   repeat = atom ('*' | '+' | '?')*
//   atom   = '(' regex ')' | CHAR

import { Language } from "./language.mjs";

export const K = { EMPTY: "Empty", EPS: "Epsilon", CHAR: "Char", ALT: "Union", CAT: "Concat", STAR: "Star" };

export const empty   = ()        => ({ k: K.EMPTY });
export const epsilon = ()        => ({ k: K.EPS });
export const chr     = (c)       => ({ k: K.CHAR, c });
export const alt     = (l, r)    => ({ k: K.ALT, l, r });
export const cat     = (l, r)    => ({ k: K.CAT, l, r });
export const star    = (x)       => ({ k: K.STAR, x });

// ─────────────────────────────────────────── the meaning: a Language

export function toLanguage(node, maxLen) {
  switch (node.k) {
    case K.EMPTY: return Language.EMPTY;
    case K.EPS:   return Language.EPSILON;
    case K.CHAR:  return Language.of(node.c);
    case K.ALT:   return toLanguage(node.l, maxLen).union(toLanguage(node.r, maxLen)).truncate(maxLen);
    case K.CAT:   return toLanguage(node.l, maxLen).concat(toLanguage(node.r, maxLen)).truncate(maxLen);
    case K.STAR:  return toLanguage(node.x, maxLen).star(maxLen);
    default: throw new Error("bad node " + node.k);
  }
}

// ─────────────────────────────────────────── the meaning: exact matching

// The set of suffixes of w that remain after `node` consumes a prefix.
function leftovers(node, w) {
  const out = new Set();
  switch (node.k) {
    case K.EMPTY: break;
    case K.EPS:   out.add(w); break;
    case K.CHAR:  if (w.length > 0 && w[0] === node.c) out.add(w.slice(1)); break;
    case K.ALT:
      for (const s of leftovers(node.l, w)) out.add(s);
      for (const s of leftovers(node.r, w)) out.add(s);
      break;
    case K.CAT:
      for (const mid of leftovers(node.l, w))
        for (const s of leftovers(node.r, mid)) out.add(s);
      break;
    case K.STAR:
      out.add(w);                                   // zero repetitions
      for (const rest of leftovers(node.x, w))
        if (rest !== w)                              // guard: subexpr must consume something
          for (const s of leftovers(node, rest)) out.add(s);
      break;
  }
  return out;
}

export function matches(node, w) {
  return leftovers(node, w).has("");
}

// ─────────────────────────────────────────── rendering

export function tree(node) {
  switch (node.k) {
    case K.EMPTY: return "EMPTY";
    case K.EPS:   return "eps";
    case K.CHAR:  return node.c;
    case K.ALT:   return "(alt " + tree(node.l) + " " + tree(node.r) + ")";
    case K.CAT:   return "(cat " + tree(node.l) + " " + tree(node.r) + ")";
    case K.STAR:  return "(star " + tree(node.x) + ")";
    default: throw new Error("bad node " + node.k);
  }
}

// ─────────────────────────────────────────── the parser

export function parse(src) {
  const p = new Parser(src);
  const r = p.regex();
  if (!p.eof())
    throw new Error(`unexpected '${p.peek()}' at position ${p.pos} in /${src}/`);
  return r;
}

class Parser {
  constructor(s) { this.s = s; this.pos = 0; }
  eof()    { return this.pos >= this.s.length; }
  peek()   { return this.eof() ? "\0" : this.s[this.pos]; }
  advance() { return this.s[this.pos++]; }
  take(c)  { if (this.peek() === c) { this.pos++; return true; } return false; }

  // regex = concat ('|' concat)*
  regex() {
    let left = this.concat();
    while (this.take("|")) left = alt(left, this.concat());
    return left;
  }

  // concat = repeat*   (juxtaposition; zero items = ε)
  concat() {
    if (this.endsConcat()) return epsilon();
    let left = this.repeat();
    while (!this.endsConcat()) left = cat(left, this.repeat());
    return left;
  }
  endsConcat() {
    const c = this.peek();
    return this.eof() || c === "|" || c === ")";
  }

  // repeat = atom ('*' | '+' | '?')*
  repeat() {
    let base = this.atom();
    for (;;) {
      if (this.take("*"))      base = star(base);
      else if (this.take("+")) base = cat(base, star(base)); // r+ = r r*
      else if (this.take("?")) base = alt(base, epsilon());  // r? = r | ε
      else return base;
    }
  }

  // atom = '(' regex ')' | CHAR
  atom() {
    if (this.take("(")) {
      const inner = this.regex();
      if (!this.take(")")) throw new Error(`missing ')' in /${this.s}/ at position ${this.pos}`);
      return inner;
    }
    if (this.eof()) throw new Error(`unexpected end of regex /${this.s}/`);
    const c = this.peek();
    if (c === "*" || c === "+" || c === "?" || c === ")" || c === "|")
      throw new Error(`'${c}' has nothing to apply to at position ${this.pos} in /${this.s}/`);
    this.advance();
    return chr(c);
  }
}
