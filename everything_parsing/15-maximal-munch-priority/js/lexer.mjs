// Module 15 — the two rules that turn token patterns into a scanner.
//
//   RULE 1  Maximal munch: the token is the LONGEST prefix any pattern matches.
//   RULE 2  Priority: on a length tie, the FIRST-declared pattern wins.
//
// Mechanism: regex -> minimal DFA (Modules 12-14). Run it from the current
// position, remember the last position it was in an accepting state, back up
// to that mark, emit a token.

import { parse as parseRegex } from "./regex.mjs";
import { build } from "./thompson.mjs";
import { determinize } from "./subset.mjs";
import { minimal } from "./minimize.mjs";

export function makeRule(name, pattern) {
  return { name, pattern, dfa: minimal(determinize(build(parseRegex(pattern)))) };
}

// Furthest exclusive end index at which `dfa` was accepting, from `pos`; -1 if never.
export function longestAccept(dfa, input, pos) {
  let state = dfa.start;
  let lastAccept = dfa.accept.has(state) ? pos : -1;
  for (let i = pos; i < input.length; i++) {
    const sym = input[i];
    if (!dfa.alphabet.includes(sym)) break;
    state = dfa.step(state, sym);
    if (dfa.accept.has(state)) lastAccept = i + 1;
  }
  return lastAccept;
}

export function nextToken(rules, input, pos) {
  let bestLen = 0, bestRule = -1;
  for (let i = 0; i < rules.length; i++) {
    const end = longestAccept(rules[i].dfa, input, pos);
    const len = end < 0 ? 0 : end - pos;
    if (len > bestLen) { bestLen = len; bestRule = i; }   // strictly greater: RULE 2 keeps the earlier rule
  }
  if (bestRule < 0 || bestLen === 0) return null;
  return { kind: rules[bestRule].name, text: input.slice(pos, pos + bestLen), start: pos };
}

export function tokenize(rules, input) {
  const out = [];
  let pos = 0;
  while (pos < input.length) {
    const t = nextToken(rules, input, pos);
    if (t === null)
      throw new Error(`lexical error at position ${pos} (char '${input[pos]}')`);
    out.push(t);
    pos += t.text.length;
  }
  return out;
}

export function matchLengths(dfa, input, pos) {
  const hits = [];
  let state = dfa.start;
  if (dfa.accept.has(state)) hits.push(0);
  for (let i = pos; i < input.length; i++) {
    const sym = input[i];
    if (!dfa.alphabet.includes(sym)) break;
    state = dfa.step(state, sym);
    if (dfa.accept.has(state)) hits.push(i + 1 - pos);
  }
  return hits;
}

export function tokenStr(t) { return `${t.kind} "${t.text}" @${t.start}`; }
