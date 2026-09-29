// Module 28 — read a grammar the way language specs write it (EBNF), and
// desugar it into the plain BNF that Modules 20-27 analyse.
//
//   rule    = NAME '::=' alt                  a new rule starts at "NAME ::="
//   alt     = seq ('|' seq)*
//   seq     = item*                           (zero items = epsilon)
//   item    = NAME | "quoted"
//           | '{' alt '}'                     zero or more
//           | '[' alt ']'                     optional
//           | '(' alt ')'                     grouping
//
// NAMES. A name defined by some rule is a nonterminal. An UPPER_CASE name that
// no rule defines is a token class from the scanner (IDENT, INT_LIT). Any other
// undefined name is an error, because it is almost always a typo.
//
// DESUGARING. Each bracket becomes a fresh helper nonterminal named after the
// rule it sits in, so every BNF production can be traced back to the line of
// EBNF that produced it:
//
//   { X }     ->  R_repN  ->  X R_repN | epsilon        (right-recursive: LL-friendly)
//   [ X ]     ->  R_optN  ->  X | epsilon
//   ( X | Y ) ->  R_grpN  ->  X | Y                     (a one-branch group is inlined)

import { Grammar } from "./grammar.mjs";

export class EbnfError extends Error {
  constructor(line, msg) { super(`ebnf line ${line}: ${msg}`); this.line = line; }
}

// ─────────────────────────────────────────── lexing

function lex(text) {
  const toks = [];
  const lines = text.split(/\r?\n/);
  lines.forEach((raw, i) => {
    const line = i + 1;
    let k = 0;
    while (k < raw.length) {
      const c = raw[k];
      if (c === " " || c === "\t") { k++; continue; }
      if (c === "#") break;
      if (raw.startsWith("::=", k)) { toks.push({ t: "::=", line }); k += 3; continue; }
      if ("|{}[]()".includes(c)) { toks.push({ t: c, line }); k++; continue; }
      if (c === '"') {
        const close = raw.indexOf('"', k + 1);
        if (close < 0) throw new EbnfError(line, "unterminated quoted terminal");
        if (close === k + 1) throw new EbnfError(line, "empty quoted terminal");
        toks.push({ t: "LIT", v: raw.slice(k + 1, close), line });
        k = close + 1;
        continue;
      }
      const m = /^[A-Za-z_][A-Za-z0-9_]*/.exec(raw.slice(k));
      if (!m) throw new EbnfError(line, `unexpected '${c}'`);
      toks.push({ t: "NAME", v: m[0], line });
      k += m[0].length;
    }
  });
  toks.push({ t: "EOF", line: lines.length });
  return toks;
}

// ─────────────────────────────────────────── parsing to an EBNF tree
//   node = { k: "alt", seqs: [[item...]...] } | { k: "name", v } | { k: "lit", v }
//        | { k: "rep" | "opt" | "grp", body: alt }

class Parser {
  constructor(toks) { this.toks = toks; this.i = 0; }
  peek(o = 0) { return this.toks[this.i + o]; }
  next() { return this.toks[this.i++]; }
  expect(t, what) {
    const tok = this.next();
    if (tok.t !== t) throw new EbnfError(tok.line, `expected ${what}, found ${show(tok)}`);
    return tok;
  }
  ruleStartsHere() { return this.peek().t === "NAME" && this.peek(1).t === "::="; }

  rules() {
    const out = [];
    while (this.peek().t !== "EOF") {
      if (!this.ruleStartsHere()) throw new EbnfError(this.peek().line, `expected 'Name ::=', found ${show(this.peek())}`);
      const name = this.next();
      this.next();
      out.push({ name: name.v, line: name.line, body: this.alt() });
    }
    return out;
  }

  alt() {
    const seqs = [this.seq()];
    while (this.peek().t === "|") { this.next(); seqs.push(this.seq()); }
    return { k: "alt", seqs };
  }

  seq() {
    const items = [];
    for (;;) {
      const t = this.peek().t;
      if (t === "EOF" || t === "|" || t === ")" || t === "]" || t === "}" || this.ruleStartsHere()) return items;
      items.push(this.item());
    }
  }

  item() {
    const tok = this.next();
    if (tok.t === "NAME") return { k: "name", v: tok.v, line: tok.line };
    if (tok.t === "LIT") return { k: "lit", v: tok.v };
    const close = { "{": "}", "[": "]", "(": ")" }[tok.t];
    if (!close) throw new EbnfError(tok.line, `unexpected ${show(tok)}`);
    const body = this.alt();
    this.expect(close, `'${close}' to match '${tok.t}' on line ${tok.line}`);
    return { k: tok.t === "{" ? "rep" : tok.t === "[" ? "opt" : "grp", body };
  }
}

function show(tok) {
  if (tok.t === "EOF") return "end of file";
  if (tok.t === "NAME") return `'${tok.v}'`;
  if (tok.t === "LIT") return `"${tok.v}"`;
  return `'${tok.t}'`;
}

// ─────────────────────────────────────────── rendering EBNF back to text

export function ebnfText(node) {
  switch (node.k) {
    case "alt":  return node.seqs.map(s => s.length === 0 ? "epsilon" : s.map(ebnfText).join(" ")).join(" | ");
    case "name": return node.v;
    case "lit":  return '"' + node.v + '"';
    case "rep":  return "{ " + ebnfText(node.body) + " }";
    case "opt":  return "[ " + ebnfText(node.body) + " ]";
    case "grp":  return "( " + ebnfText(node.body) + " )";
  }
  throw new Error("bad node " + node.k);
}

// ─────────────────────────────────────────── desugaring to BNF

export const isTokenClass = (name) => /^[A-Z][A-Z0-9_]*$/.test(name);
export const literal = (v) => '"' + v + '"';

// Returns { grammar, rules, helpers, origin }:
//   grammar  a Module 2 Grammar (productions carry no extra fields)
//   rules    the parsed EBNF rules, in file order
//   helpers  [{ name, owner, kind, text }] for each generated nonterminal
//   origin   Map: nonterminal -> the EBNF rule it came from
export function desugar(text) {
  const rules = new Parser(lex(text)).rules();
  if (rules.length === 0) throw new EbnfError(1, "no rules");

  const defined = new Map();
  for (const r of rules) {
    if (defined.has(r.name)) throw new EbnfError(r.line, `${r.name} is defined twice (first on line ${defined.get(r.name)})`);
    defined.set(r.name, r.line);
  }

  const bodies = new Map();          // nonterminal -> [[symbol...]...]
  const helpers = [];
  const origin = new Map();
  const counters = new Map();

  const addNt = (name, owner) => { bodies.set(name, []); origin.set(name, owner); };
  const fresh = (owner, kind) => {
    const n = (counters.get(owner) ?? 0) + 1;
    counters.set(owner, n);
    return owner + "_" + kind + n;
  };

  const lowerSeq = (items, owner) => {
    const out = [];
    for (const it of items) {
      if (it.k === "name") {
        if (!defined.has(it.v) && !isTokenClass(it.v))
          throw new EbnfError(it.line, `'${it.v}' is not defined (token classes are UPPER_CASE)`);
        out.push(it.v);
      } else if (it.k === "lit") {
        out.push(literal(it.v));
      } else if (it.k === "grp" && it.body.seqs.length === 1) {
        out.push(...lowerSeq(it.body.seqs[0], owner));
      } else {
        const name = fresh(owner, it.k);
        addNt(name, owner);
        helpers.push({ name, owner, kind: it.k, text: ebnfText(it) });
        const alts = it.body.seqs.map(s => lowerSeq(s, owner));
        if (it.k === "rep") {
          for (const a of alts) bodies.get(name).push([...a, name]);
          bodies.get(name).push([]);
        } else if (it.k === "opt") {
          for (const a of alts) bodies.get(name).push(a);
          bodies.get(name).push([]);
        } else {
          for (const a of alts) bodies.get(name).push(a);
        }
        out.push(name);
      }
    }
    return out;
  };

  for (const r of rules) {
    addNt(r.name, r.name);
    // helpers created while lowering r are appended after r, in creation order
    const alts = r.body.seqs.map(s => lowerSeq(s, r.name));
    bodies.set(r.name, alts);
  }

  // Emit rules first (file order), each followed by its helpers.
  const emitOrder = [];
  for (const r of rules) {
    emitOrder.push(r.name);
    for (const h of helpers) if (h.owner === r.name) emitOrder.push(h.name);
  }
  const prods = [];
  for (const nt of emitOrder)
    for (const rhs of bodies.get(nt)) prods.push({ index: prods.length, lhs: nt, rhs });

  const grammar = new Grammar(prods, rules[0].name, new Set(emitOrder));
  return { grammar, rules, helpers, origin };
}
