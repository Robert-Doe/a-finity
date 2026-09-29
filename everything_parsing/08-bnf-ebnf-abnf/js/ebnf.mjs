// Module 08 — EBNF, and the proof that its extra operators add nothing.
//
//   x?  ->  Opt   with   Opt  -> x | epsilon
//   x*  ->  Rep   with   Rep  -> x Rep | epsilon
//   x+  ->  Plus  with   Plus -> x Plus | x
//   (...)  absorbed by the parser
//
// EBNF meta-grammar:
//   grammar = rule+ ;  rule = IDENT '=' alt ';' ;  alt = seq ('|' seq)*
//   seq = rep+ ;  rep = atom ('?'|'*'|'+')? ;  atom = '(' alt ')' | STRING | IDENT

// ── AST node constructors ──
const T   = (text)  => ({ k: "T", text });
const NT  = (name)  => ({ k: "NT", name });
const SEQ = (parts) => ({ k: "SEQ", parts });
const ALT = (parts) => ({ k: "ALT", parts });
const OPT = (inner) => ({ k: "OPT", inner });
const STAR = (inner) => ({ k: "STAR", inner });
const PLUS = (inner) => ({ k: "PLUS", inner });

export class EbnfGrammar {
  constructor() { this.rules = new Map(); this.start = null; }
  isNonterminal(s) { return this.rules.has(s); }
}

// ─────────────────────────────────────────── parsing

export function parse(src) {
  const toks = lex(src);
  let i = 0;
  const peek = () => (i < toks.length ? toks[i] : "");
  const next = () => toks[i++];
  const eof = () => i >= toks.length;
  const expect = (s) => { if (peek() !== s) throw new Error(`EBNF: expected '${s}', got '${peek()}'`); i++; };
  const ident = () => {
    const s = next();
    if (!s || s.startsWith("'") || "=;|()?*+".includes(s))
      throw new Error(`EBNF: expected an identifier, got '${s}'`);
    return s;
  };

  const alt = () => {
    const parts = [seq()];
    while (peek() === "|") { next(); parts.push(seq()); }
    return parts.length === 1 ? parts[0] : ALT(parts);
  };
  const seq = () => {
    const parts = [];
    while (peek() !== "|" && peek() !== ")" && peek() !== ";" && !eof()) parts.push(rep());
    if (parts.length === 0) throw new Error("EBNF: empty sequence");
    return parts.length === 1 ? parts[0] : SEQ(parts);
  };
  const rep = () => {
    const a = atom();
    const q = peek();
    if (q === "?") { next(); return OPT(a); }
    if (q === "*") { next(); return STAR(a); }
    if (q === "+") { next(); return PLUS(a); }
    return a;
  };
  const atom = () => {
    const s = peek();
    if (s === "(") { next(); const e = alt(); expect(")"); return e; }
    if (s.startsWith("'")) { next(); return T(s.slice(1, -1)); }
    return NT(ident());
  };

  const g = new EbnfGrammar();
  while (!eof()) {
    const name = ident();
    expect("=");
    const body = alt();
    expect(";");
    g.rules.set(name, body);
    if (g.start === null) g.start = name;
  }
  return g;
}

function lex(src) {
  const out = [];
  let i = 0;
  const n = src.length;
  while (i < n) {
    const c = src[i];
    if (/\s/.test(c)) { i++; continue; }
    if (c === "/" && src[i + 1] === "/") { while (i < n && src[i] !== "\n") i++; continue; }
    if (c === "'" || c === '"') {
      let j = i + 1;
      while (j < n && src[j] !== c) j++;
      out.push("'" + src.slice(i + 1, j) + "'");
      i = j + 1;
      continue;
    }
    if ("=;|()?*+".includes(c)) { out.push(c); i++; continue; }
    if (/[A-Za-z0-9_]/.test(c)) {
      let j = i;
      while (j < n && /[A-Za-z0-9_]/.test(src[j])) j++;
      out.push(src.slice(i, j));
      i = j;
      continue;
    }
    throw new Error(`EBNF: unexpected character '${c}' at ${i}`);
  }
  return out;
}

// ─────────────────────────────────────────── desugar to pure BNF text

export function toBnfText(g) {
  let out = "%start " + g.start + "\n";
  const counter = { n: 0 };
  const extra = new Map();       // reserve-then-fill: creation order preserved

  for (const [name, body] of g.rules) {
    out += ruleText(name, alternatives(body, counter, extra)) + "\n";
  }
  for (const text of extra.values()) out += text + "\n";
  return out;
}

function ruleText(lhs, alts) {
  return lhs + " -> " + alts.map(a => (a.length === 0 ? "epsilon" : a.join(" "))).join(" | ");
}

function alternatives(e, counter, extra) {
  if (e.k === "ALT") return e.parts.map(p => seqSymbols(p, counter, extra));
  return [seqSymbols(e, counter, extra)];
}

function seqSymbols(e, counter, extra) {
  if (e.k === "SEQ") return e.parts.map(p => symbol(p, counter, extra));
  return [symbol(e, counter, extra)];
}

function symbol(e, counter, extra) {
  if (e.k === "T") return e.text;
  if (e.k === "NT") return e.name;

  const prefix = e.k === "OPT" ? "opt_" : e.k === "STAR" ? "rep_" : e.k === "PLUS" ? "plus_" : "grp_";
  const name = prefix + (++counter.n);
  extra.set(name, "");

  let body;
  if (e.k === "OPT") {
    body = name + " -> " + symbol(e.inner, counter, extra) + " | epsilon";
  } else if (e.k === "STAR") {
    const x = symbol(e.inner, counter, extra);
    body = name + " -> " + x + " " + name + " | epsilon";
  } else if (e.k === "PLUS") {
    const x = symbol(e.inner, counter, extra);
    body = name + " -> " + x + " " + name + " | " + x;
  } else {
    body = ruleText(name, alternatives(e, counter, extra));
  }
  extra.set(name, body);
  return name;
}

// ─────────────────────────────────────────── exact matching over the EBNF AST

export function accepts(g, input) {
  return leftovers(g, g.rules.get(g.start), input, 0).has(input.length);
}

function leftovers(g, e, input, pos) {
  const out = new Set();
  if (pos > input.length) return out;
  switch (e.k) {
    case "T":
      if (pos < input.length && input[pos] === e.text) out.add(pos + 1);
      break;
    case "NT": {
      const body = g.rules.get(e.name);
      if (body === undefined) {
        if (pos < input.length && input[pos] === e.name) out.add(pos + 1);
      } else {
        for (const p of leftovers(g, body, input, pos)) out.add(p);
      }
      break;
    }
    case "SEQ": {
      let cur = new Set([pos]);
      for (const part of e.parts) {
        const nxt = new Set();
        for (const p of cur) for (const q of leftovers(g, part, input, p)) nxt.add(q);
        cur = nxt;
      }
      for (const p of cur) out.add(p);
      break;
    }
    case "ALT":
      for (const part of e.parts) for (const p of leftovers(g, part, input, pos)) out.add(p);
      break;
    case "OPT":
      out.add(pos);
      for (const p of leftovers(g, e.inner, input, pos)) out.add(p);
      break;
    case "STAR":
      out.add(pos);
      for (const p of leftovers(g, e.inner, input, pos))
        if (p !== pos) for (const q of leftovers(g, e, input, p)) out.add(q);
      break;
    case "PLUS":
      for (const p of leftovers(g, SEQ([e.inner, STAR(e.inner)]), input, pos)) out.add(p);
      break;
  }
  return out;
}

// ─────────────────────────────────────────── ISO-style rendering

export function iso(e) {
  switch (e.k) {
    case "T":  return "'" + e.text + "'";
    case "NT": return e.name;
    case "SEQ": return e.parts.map(iso).join(" ");
    case "ALT": return "( " + e.parts.map(iso).join(" | ") + " )";
    case "OPT": return "[ " + iso(e.inner) + " ]";
    case "STAR": return "{ " + iso(e.inner) + " }";
    case "PLUS": return iso(e.inner) + " { " + iso(e.inner) + " }";
  }
}

export function isoRule(e) {
  if (e.k === "ALT") return e.parts.map(iso).join(" | ");
  return iso(e);
}
