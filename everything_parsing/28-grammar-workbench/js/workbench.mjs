// Module 28 — the grammar workbench: EBNF in, diagnosis and a working
// table-driven parser out. Every analysis is an earlier module's component:
//
//   FIRST / FOLLOW / PREDICT   Modules 20-22   (predict.mjs)
//   left recursion, Paull      Module 23       (leftrec.mjs)
//   LL(1) table                Module 25       (ll1table.mjs)
//   table-driven parsing       Module 26       (parse() below)
//
// What is new here is the glue a language designer actually needs: reading
// the grammar as written, explaining each conflict in terms of the rules that
// cause it, and running the finished table on real tokens.

import { Grammar, productionToString } from "./grammar.mjs";
import { Predict, EPS, END } from "./predict.mjs";
import { LL1Table } from "./ll1table.mjs";
import { G, paull, hasLeftRecursion } from "./leftrec.mjs";
import { isTokenClass } from "./ebnf.mjs";

const pad = (s, w) => { s = String(s); return s.length >= w ? s : s + " ".repeat(w - s.length); };
const sorted = (xs) => [...xs].sort();
const plural = (n, one, many) => n + " " + (n === 1 ? one : many);

// ─────────────────────────────────────────── diagnosis

// One conflict = a nonterminal, a lookahead, and >= 2 productions predicting it.
export function conflicts(g, pr) {
  const out = [];
  for (const nt of g.nonterminals) {
    const ps = g.productionsFor(nt);
    const byTok = new Map();
    for (const p of ps)
      for (const t of sorted(pr.predict(p))) {
        if (!byTok.has(t)) byTok.set(t, []);
        byTok.get(t).push(p);
      }
    for (const [t, list] of byTok) if (list.length > 1) out.push({ nt, token: t, prods: list });
  }
  return out;
}

// How does production p come to predict terminal t? Either a chain of
// leftmost derivations ending in t, or "t follows" when p can vanish.
export function explain(g, pr, p, t) {
  const chain = firstChain(g, pr, p.rhs, t, new Set([p.lhs]));
  if (chain !== null) return p.lhs + " -> " + chain.join(" -> ");
  return p.lhs + " -> epsilon, and " + t + " is in FOLLOW(" + p.lhs + ")";
}

function firstChain(g, pr, seq, t, seen) {
  for (const sym of seq) {
    if (sym === t) return [t];
    if (g.isNonterminal(sym) && !seen.has(sym) && pr.firstOf(sym).has(t)) {
      seen.add(sym);
      for (const q of g.productionsFor(sym)) {
        if (!pr.firstOfSeq(q.rhs).has(t)) continue;
        const rest = firstChain(g, pr, q.rhs, t, seen);
        if (rest !== null) return [sym, ...rest];
      }
    }
    if (!pr.firstOf(sym).has(EPS)) return null;   // sym can't vanish: t must come from it
  }
  return null;
}

export function fromG(lg) {
  const prods = [];
  for (const nt of lg.order)
    for (const rhs of lg.prods.get(nt)) prods.push({ index: prods.length, lhs: nt, rhs: [...rhs] });
  return new Grammar(prods, lg.start, new Set(lg.order));
}

export function report(name, d) {
  const g = d.grammar;
  const pr = new Predict(g);
  let sb = "";
  const p = (s = "") => { sb += s + "\n"; };

  const terminals = g.terminals();
  p("grammar: " + name);
  p("  " + d.rules.length + " EBNF rules -> " + g.productions.length + " BNF productions | "
    + d.rules.length + " rules + " + d.helpers.length + " helpers = " + g.nonterminals.size
    + " nonterminals | " + terminals.size + " terminals ("
    + plural([...terminals].filter(isTokenClass).length, "token class", "token classes") + ")");
  if (d.helpers.length > 0) {
    p("  helpers:");
    for (const h of d.helpers) p("    " + pad(h.name, 18) + " " + h.text);
  }
  const nullable = [...g.nonterminals].filter(nt => pr.firstOf(nt).has(EPS));
  p("  nullable: " + (nullable.length === 0 ? "(none)" : nullable.join(" ")));

  let work = g;
  let workPr = pr;
  const lr = hasLeftRecursion(G.from(g));
  p("  left recursion: " + (lr ? "YES" : "none"));
  if (lr) {
    work = fromG(paull(g));
    workPr = new Predict(work);
    p("  removed with Paull's algorithm (Module 23):");
    for (const nt of work.nonterminals)
      p("    " + nt + " -> " + work.productionsFor(nt).map(q => q.rhs.length === 0 ? "epsilon" : q.rhs.join(" ")).join(" | "));
  }

  const cs = conflicts(work, workPr);
  if (cs.length === 0) {
    const table = new LL1Table(work);
    let filled = 0;
    for (const nt of work.nonterminals) for (const c of table.columns) if (table.cell(nt, c).length > 0) filled++;
    const cells = work.nonterminals.size * table.columns.length;
    p("  LL(1): YES  table " + work.nonterminals.size + " x " + table.columns.length + ", "
      + filled + " of " + cells + " cells used");
  } else {
    p("  LL(1): NO, " + plural(cs.length, "conflict", "conflicts"));
    for (const c of cs) {
      const owner = d.origin.get(c.nt) ?? c.nt;
      p("    " + c.nt + (owner === c.nt ? "" : " (inside rule " + owner + ")") + " on " + c.token + ":");
      for (const q of c.prods) p("      [" + productionToString(q) + "]  via " + explain(work, workPr, q, c.token));
    }
  }
  return { text: sb, grammar: work, ll1: cs.length === 0 };
}

// ─────────────────────────────────────────── running the table

// The scanner kinds that appear in the grammar by name.
const TOKEN_CLASSES = ["IDENT", "INT_LIT", "FLOAT_LIT"];

// Scanner token -> grammar terminal. Token classes keep their kind; everything
// else (keywords, operators, punctuation) is the quoted lexeme.
export function terminalOf(tok) {
  if (tok.kind === "EOF") return END;
  if (TOKEN_CLASSES.includes(tok.kind)) return tok.kind;
  return '"' + tok.lexeme + '"';
}

export function readTokens(text) {
  const out = [];
  for (const raw of text.split(/\r?\n/)) {
    if (raw.trim() === "" || raw.startsWith("#")) continue;
    const [at, kind, lexeme = ""] = raw.trim().split(/\s+/);
    const [line, col] = at.split(":").map(Number);
    out.push({ kind, lexeme, line, col });
  }
  return out;
}

// Table-driven LL(1) parse. Returns { ok, steps, expansions, matches, maxDepth, trace, error }.
export function parse(g, table, origin, tokens, traceLimit) {
  const stack = [END, g.start];
  let i = 0, expansions = 0, matches = 0, maxDepth = stack.length, steps = 0;
  const trace = [];
  const note = (top, look, action) => { if (trace.length < traceLimit) trace.push({ top, look, action }); };

  while (stack.length > 0) {
    steps++;
    const top = stack.pop();
    const tok = tokens[i];
    const look = terminalOf(tok);
    if (top === END && look === END) { note(top, look, "accept"); break; }
    if (!g.isNonterminal(top)) {
      if (top !== look)
        return fail(tok, "expected " + top + " but found " + showTok(tok));
      note(top, look, "match");
      matches++;
      i++;
      continue;
    }
    const cell = table.cell(top, look);
    if (cell.length === 0) {
      const expected = table.columns.filter(c => table.cell(top, c).length > 0);
      return fail(tok, "in " + (origin.get(top) ?? top) + ", expected one of " + expected.join(" ")
        + " but found " + showTok(tok));
    }
    const prod = cell[0];
    note(top, look, productionToString(prod));
    expansions++;
    for (let k = prod.rhs.length - 1; k >= 0; k--) stack.push(prod.rhs[k]);
    if (stack.length > maxDepth) maxDepth = stack.length;
  }
  return { ok: true, steps, expansions, matches, maxDepth, trace, error: null };

  function fail(tok, msg) {
    return { ok: false, steps, expansions, matches, maxDepth, trace, error: tok.line + ":" + tok.col + ": " + msg };
  }
}

function showTok(tok) {
  return tok.kind === "EOF" ? "end of input" : '"' + tok.lexeme + '"';
}

export function traceLines(trace) {
  const out = ["    " + pad("top of stack", 22) + pad("lookahead", 12) + "action"];
  for (const s of trace) out.push("    " + pad(s.top, 22) + pad(s.look, 12) + s.action);
  return out;
}
