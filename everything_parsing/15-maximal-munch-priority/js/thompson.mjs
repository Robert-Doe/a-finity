// Module 12 — Thompson's construction: regex tree (Module 3) -> epsilon-NFA.
//
//   Empty (∅)    s --(nothing)--> e
//   Epsilon (ε)  s --ε--> e
//   Char c       s --c--> e
//   Concat(l,r)  l.end --ε--> r.start ;  start=l.start, end=r.end        (+0 states)
//   Union(l,r)   new s --ε--> l.start,r.start ;  l.end,r.end --ε--> new e (+2)
//   Star(x)      new s --ε--> x.start,e ;  x.end --ε--> x.start,e         (+2)
//
// So the NFA has <= 2 * (number of regex nodes) states.

import { K } from "./regex.mjs";
import { Nfa } from "./nfa.mjs";

export function build(re) {
  const c = { n: 0 };
  const f = frag(re, c);
  const alphabet = new Set();
  collectAlphabet(re, alphabet);

  let text = "states:";
  for (let i = 0; i < c.n; i++) text += " q" + i;
  text += "\nalphabet:";
  for (const a of [...alphabet].sort()) text += " " + a;
  text += "\nstart: " + f.start + "\naccept: " + f.end + "\n";
  for (const line of f.lines) text += line + "\n";
  return Nfa.parse(text);
}

export function stateCount(re) {
  const c = { n: 0 };
  frag(re, c);
  return c.n;
}

export function nodeCount(re) {
  if (re.k === K.ALT || re.k === K.CAT) return 1 + nodeCount(re.l) + nodeCount(re.r);
  if (re.k === K.STAR) return 1 + nodeCount(re.x);
  return 1;
}

function fresh(c) { return "q" + (c.n++); }

function frag(re, c) {
  switch (re.k) {
    case K.EMPTY: {
      const s = fresh(c), e = fresh(c);
      return { start: s, end: e, lines: [] };
    }
    case K.EPS: {
      const s = fresh(c), e = fresh(c);
      return { start: s, end: e, lines: [`${s} epsilon ${e}`] };
    }
    case K.CHAR: {
      const s = fresh(c), e = fresh(c);
      return { start: s, end: e, lines: [`${s} ${re.c} ${e}`] };
    }
    case K.CAT: {
      const l = frag(re.l, c), r = frag(re.r, c);
      return { start: l.start, end: r.end, lines: [...l.lines, ...r.lines, `${l.end} epsilon ${r.start}`] };
    }
    case K.ALT: {
      const l = frag(re.l, c), r = frag(re.r, c);
      const s = fresh(c), e = fresh(c);
      return {
        start: s, end: e,
        lines: [
          `${s} epsilon ${l.start}`, `${s} epsilon ${r.start}`,
          ...l.lines, ...r.lines,
          `${l.end} epsilon ${e}`, `${r.end} epsilon ${e}`,
        ],
      };
    }
    case K.STAR: {
      const x = frag(re.x, c);
      const s = fresh(c), e = fresh(c);
      return {
        start: s, end: e,
        lines: [
          `${s} epsilon ${x.start}`, `${s} epsilon ${e}`,
          ...x.lines,
          `${x.end} epsilon ${x.start}`, `${x.end} epsilon ${e}`,
        ],
      };
    }
    default: throw new Error("bad node " + re.k);
  }
}

function collectAlphabet(re, out) {
  if (re.k === K.CHAR) out.add(re.c);
  else if (re.k === K.ALT || re.k === K.CAT) { collectAlphabet(re.l, out); collectAlphabet(re.r, out); }
  else if (re.k === K.STAR) collectAlphabet(re.x, out);
}
