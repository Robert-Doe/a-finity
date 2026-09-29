// Module 16 — a scanner GENERATOR: token spec in, working lexer out.
//
//   SPEC     one directive per line ('#' in column 1 starts a comment):
//              token    NAME  pattern     emit a NAME token
//              discard  NAME  pattern     match it, then throw it away
//              keywords FROM  w1 w2 ...   a FROM lexeme spelled like a
//                                          keyword becomes that keyword
//   BUILD    each pattern -> NFA (Thompson), all NFAs joined under one fresh
//            start state with every accept state TAGGED by its rule, then the
//            subset construction -> one combined DFA. A DFA state that holds
//            several tagged accepts resolves to the EARLIEST declared rule.
//   SCAN     longest match: drive the DFA as far as it will go, remember the
//            last accepting (position, rule), back up to it, emit. Keywords
//            are resolved AFTER the match by a table lookup, so `iffy` is one
//            identifier and never `if` + `fy`.
//   CHECK    a pattern that accepts the empty string is refused at load time:
//            the scan loop would match zero characters forever.

import { parsePattern, symbolOf, isSourceChar, PatternError } from "./pattern.mjs";
import { Nfa } from "./nfa.mjs";
import { build as thompsonBuild } from "./thompson.mjs";
import { determinize, legend } from "./subset.mjs";

export class SpecError extends Error {
  constructor(line, msg) { super(`spec line ${line}: ${msg}`); this.line = line; }
}

export class ScanError extends Error {
  constructor(line, col, ch) {
    super(`${line}:${col}: no token can start with ${showChar(ch)}`);
    this.line = line; this.col = col; this.ch = ch;
  }
}

export function showChar(ch) {
  if (ch === "\n") return "'\\n'";
  if (ch === "\t") return "'\\t'";
  if (!isSourceChar(ch)) return "U+" + ch.codePointAt(0).toString(16).toUpperCase().padStart(4, "0");
  return "'" + ch + "'";
}

export class Scanner {
  constructor(rules, keywords, dfa, stateRule, sink) {
    this.rules = rules;          // [{ index, action, name, pattern, line }]
    this.keywords = keywords;    // { from, words: [] } | null
    this.dfa = dfa;
    this.stateRule = stateRule;  // Map: DFA state -> winning rule index
    this.sink = sink;            // the DFA state for "no NFA state left", or null
  }

  static fromSpec(text) {
    const rules = [];
    let keywords = null;
    const names = new Set();
    text.split(/\r?\n/).forEach((raw, i) => {
      const lineNo = i + 1;
      const line = raw.trim();
      if (line === "" || line.startsWith("#")) return;
      const m = /^(\S+)\s+(\S+)\s*(.*)$/.exec(line);
      if (!m) throw new SpecError(lineNo, "expected '<directive> <name> ...'");
      const [, directive, name, rest] = m;
      if (directive === "keywords") {
        if (keywords) throw new SpecError(lineNo, "only one 'keywords' line is allowed");
        const words = rest.split(/\s+/).filter(Boolean);
        if (words.length === 0) throw new SpecError(lineNo, "'keywords' needs at least one word");
        keywords = { from: name, words, line: lineNo };
        return;
      }
      if (directive !== "token" && directive !== "discard")
        throw new SpecError(lineNo, `expected 'token', 'discard' or 'keywords', found '${directive}'`);
      if (rest === "") throw new SpecError(lineNo, `${name} has no pattern`);
      if (names.has(name)) throw new SpecError(lineNo, `${name} is declared twice`);
      names.add(name);
      rules.push({ index: rules.length, action: directive, name, pattern: rest, line: lineNo });
    });
    if (rules.length === 0) throw new SpecError(0, "the spec declares no tokens");
    if (keywords && !rules.some(r => r.name === keywords.from && r.action === "token"))
      throw new SpecError(keywords.line, `keywords come from '${keywords.from}', which is not a token`);

    // Compile each pattern once; refuse any that accepts the empty string.
    const nfas = rules.map(r => {
      let re;
      try { re = parsePattern(r.pattern); }
      catch (e) { if (e instanceof PatternError) throw new SpecError(r.line, `${r.name}: ${e.message}`); throw e; }
      const n = thompsonBuild(re);
      const start = n.epsilonClosure(new Set([n.start]));
      if ([...start].some(s => n.accept.has(s)))
        throw new SpecError(r.line,
          `${r.name} accepts the empty string; a token must consume at least one character`);
      return n;
    });

    // One combined NFA: a fresh START with an epsilon edge into each rule's NFA.
    let states = "states: START";
    const alphabet = new Set();
    const acceptRule = new Map();
    const edges = [];
    rules.forEach((r, i) => {
      const n = nfas[i];
      const p = "r" + r.index + "_";
      for (const st of n.states) states += " " + p + st;
      edges.push("START epsilon " + p + n.start);
      for (const acc of n.accept) acceptRule.set(p + acc, r.index);
      for (const [from, bySym] of n.delta)
        for (const [sym, tos] of bySym)
          for (const to of tos) edges.push(p + from + " " + sym + " " + p + to);
      for (const a of n.alphabet) alphabet.add(a);
    });
    const text2 = states
      + "\nalphabet: " + [...alphabet].sort().join(" ")
      + "\nstart: START\naccept: " + [...acceptRule.keys()].join(" ") + "\n"
      + edges.join("\n") + "\n";

    const combined = Nfa.parse(text2);
    const dfa = determinize(combined);

    const stateRule = new Map();
    let sink = null;
    for (const [dState, nfaStates] of legend(combined)) {
      if (nfaStates.length === 0) sink = dState;
      let best = Infinity;
      for (const s of nfaStates) {
        const ri = acceptRule.get(s);
        if (ri !== undefined && ri < best) best = ri;
      }
      if (best !== Infinity) stateRule.set(dState, best);
    }
    return new Scanner(rules, keywords, dfa, stateRule, sink);
  }

  // Longest match starting at `pos`: { end, rule } or null if nothing matches.
  longestMatch(src, pos) {
    let state = this.dfa.start;
    let end = -1, rule = -1;
    for (let i = pos; i < src.length; i++) {
      const sym = symbolOf(src[i]);
      if (!this.dfa.alphabet.includes(sym)) break;
      state = this.dfa.step(state, sym);
      if (state === this.sink) break;          // no rule can extend this match
      const ri = this.stateRule.get(state);
      if (ri !== undefined) { end = i + 1; rule = ri; }
    }
    return end > pos ? { end, rule } : null;
  }

  // Tokens with 1-based line:col, ending in EOF. Discarded matches still move
  // the position, so every later token keeps its true location.
  scan(src) {
    const out = [];
    let pos = 0, line = 1, col = 1;
    while (pos < src.length) {
      const m = this.longestMatch(src, pos);
      if (m === null) {
        const err = new ScanError(line, col, src[pos]);
        err.partial = out;                     // everything scanned before the bad character
        throw err;
      }
      const rule = this.rules[m.rule];
      const lexeme = src.slice(pos, m.end);
      if (rule.action === "token") out.push({ kind: this.kindOf(rule.name, lexeme), lexeme, line, col });
      for (const ch of lexeme) {
        if (ch === "\n") { line++; col = 1; } else col++;
      }
      pos = m.end;
    }
    out.push({ kind: "EOF", lexeme: "", line, col });
    return out;
  }

  kindOf(ruleName, lexeme) {
    if (this.keywords && ruleName === this.keywords.from && this.keywords.words.includes(lexeme))
      return lexeme.toUpperCase();
    return ruleName;
  }

  get dfaStates() { return this.dfa.states.length; }
  get alphabetSize() { return this.dfa.alphabet.length; }
}

export function tokenLine(t) {
  const at = (t.line + ":" + t.col).padEnd(7);
  return at + t.kind.padEnd(10) + (t.kind === "EOF" ? "" : t.lexeme);
}
