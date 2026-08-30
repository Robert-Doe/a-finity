// Module 16 — a complete lexical analyzer, ASU CSE 340 Project 1 style.
//
//   INPUT   a token-spec: one "NAME  regex" per line, priority order, plus
//           optional "%skip NAME" lines
//   BUILD   each pattern's regex -> NFA (Thompson), UNION under a fresh start
//           with a tag on each accept state, then DETERMINIZE. Each combined-DFA
//           state knows the highest-priority token it accepts.
//   SCAN    one pass, maximal munch: run the DFA, remember the last (position,
//           token) accepted, back up to it, emit; skip %skip tokens; a stuck
//           position with no accept is a lexical error with a column.
//   CHECK   reject any pattern that matches the empty string.

import { parse as parseRegex } from "./regex.mjs";
import { Nfa } from "./nfa.mjs";
import { build as thompsonBuild } from "./thompson.mjs";
import { determinize, legend } from "./subset.mjs";

export class LexicalError extends Error {
  constructor(pos, ch) {
    super(`lexical error at position ${pos} (char '${ch}')`);
    this.pos = pos; this.ch = ch;
  }
}

export class SpecLexer {
  constructor(rules, skip, dfa, stateToken) {
    this.rules = rules;              // [{ index, name, pattern }]
    this.skip = skip;               // Set of names
    this.dfa = dfa;
    this.stateToken = stateToken;   // Map: combined-DFA state -> winning rule index
  }

  static load(specText) {
    const rules = [];
    const skip = new Set();
    for (const raw of specText.split(/\r?\n/)) {
      const h = raw.indexOf("#");
      const line = (h >= 0 ? raw.slice(0, h) : raw).replace(/\s+$/, "");
      if (line.trim() === "") continue;
      if (line.trim().startsWith("%skip")) { skip.add(line.trim().split(/\s+/)[1]); continue; }
      const s = line.trim();
      const sp = s.search(/\s/);
      if (sp < 0) throw new Error("spec line has no regex: " + s);
      rules.push({ index: rules.length, name: s.slice(0, sp), pattern: s.slice(sp).trim() });
    }
    if (rules.length === 0) throw new Error("spec has no token rules");

    for (const r of rules) {
      const n = thompsonBuild(parseRegex(r.pattern));
      const close = n.epsilonClosure(new Set([n.start]));
      if ([...close].some(s => n.accept.has(s)))
        throw new Error(`token '${r.name}' can match the empty string  (epsilon IS NOOOOOT A TOKEN)`);
    }

    // combined tagged NFA
    let text = "states: START";
    const alphabet = new Set();
    const acceptRule = new Map();
    const trans = [];
    for (const r of rules) {
      const n = thompsonBuild(parseRegex(r.pattern));
      const p = "r" + r.index + "_";
      for (const st of n.states) text += " " + p + st;
      trans.push("START epsilon " + p + n.start);
      for (const acc of n.accept) acceptRule.set(p + acc, r.index);
      for (const [from, m] of n.delta)
        for (const [sym, tos] of m)
          for (const to of tos) trans.push(p + from + " " + sym + " " + p + to);
      for (const a of n.alphabet) alphabet.add(a);
    }
    text += "\nalphabet: " + [...alphabet].sort().join(" ");
    text += "\nstart: START\naccept: " + [...acceptRule.keys()].join(" ") + "\n";
    for (const t of trans) text += t + "\n";

    const combined = Nfa.parse(text);
    const dfa = determinize(combined);
    const leg = legend(combined);

    const stateToken = new Map();
    for (const [name, set] of leg) {
      let best = Infinity;
      for (const s of set) {
        const ri = acceptRule.get(s);
        if (ri !== undefined && ri < best) best = ri;
      }
      if (best !== Infinity) stateToken.set(name, best);
    }
    return new SpecLexer(rules, skip, dfa, stateToken);
  }

  tokenize(input) {
    const out = [];
    let pos = 0;
    while (pos < input.length) {
      let state = this.dfa.start;
      let lastPos = -1, lastRule = -1;
      const at = this.stateToken.get(state);
      if (at !== undefined) { lastPos = pos; lastRule = at; }
      for (let i = pos; i < input.length; i++) {
        const sym = input[i];
        if (!this.dfa.alphabet.includes(sym)) break;
        state = this.dfa.step(state, sym);
        const ri = this.stateToken.get(state);
        if (ri !== undefined) { lastPos = i + 1; lastRule = ri; }
      }
      if (lastPos <= pos) throw new LexicalError(pos, input[pos]);
      const lexeme = input.slice(pos, lastPos);
      const kind = this.rules[lastRule].name;
      if (!this.skip.has(kind)) out.push({ kind, lexeme, pos });
      pos = lastPos;
    }
    return out;
  }

  countSkipped(input) {
    let pos = 0, skipped = 0;
    while (pos < input.length) {
      let state = this.dfa.start;
      let lastPos = -1, lastRule = -1;
      const at = this.stateToken.get(state);
      if (at !== undefined) { lastPos = pos; lastRule = at; }
      for (let i = pos; i < input.length; i++) {
        const sym = input[i];
        if (!this.dfa.alphabet.includes(sym)) break;
        state = this.dfa.step(state, sym);
        const ri = this.stateToken.get(state);
        if (ri !== undefined) { lastPos = i + 1; lastRule = ri; }
      }
      if (lastPos <= pos) break;
      if (this.skip.has(this.rules[lastRule].name)) skipped++;
      pos = lastPos;
    }
    return skipped;
  }

  get combinedDfaStates() { return this.dfa.states.length; }
}

export function tokenStr(t) {
  return t.kind.padEnd(8) + ' "' + t.lexeme + '" @' + t.pos;
}
