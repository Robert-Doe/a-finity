// Module 10 — a deterministic finite automaton.
//
//   Q      finite states        delta  Q x Sigma -> Q   (TOTAL)
//   Sigma  input alphabet        q0     start state       F  accepting states
//
// Run: start at q0, apply delta once per symbol, accept iff the final state
// is in F. Accepts exactly a regular language.
//
// File format:
//   states: S A B
//   alphabet: a b
//   start: S
//   accept: A B
//   S a A            <- fromState symbol toState
// Missing (state, symbol) pairs route to an implicit DEAD state.

const DEAD = "<dead>";

export class Dfa {
  constructor(states, alphabet, delta, start, accept) {
    this.states = states;        // string[]
    this.alphabet = alphabet;    // string[]
    this.delta = delta;          // Map<from, Map<symbol, to>>
    this.start = start;
    this.accept = new Set(accept);
  }

  step(state, symbol) {
    return this.delta.get(state)?.get(symbol) ?? DEAD;
  }

  trace(input) {
    const out = [this.start];
    let s = this.start;
    for (const sym of input) { s = this.step(s, sym); out.push(s); }
    return out;
  }

  accepts(input) {
    let s = this.start;
    for (const sym of input) {
      if (!this.alphabet.includes(sym)) return false;
      s = this.step(s, sym);
    }
    return this.accept.has(s);
  }

  acceptsString(w) {
    return this.accepts([...w]);
  }

  language(maxLen) {
    const out = [];
    const q = [""];
    while (q.length) {
      const cur = q.shift();
      if (this.acceptsString(cur)) out.push(cur === "" ? "epsilon" : cur);
      if (cur.length < maxLen) for (const sym of this.alphabet) q.push(cur + sym);
    }
    out.sort((x, y) => {
      const lx = x === "epsilon" ? 0 : x.length;
      const ly = y === "epsilon" ? 0 : y.length;
      return lx !== ly ? lx - ly : (x < y ? -1 : x > y ? 1 : 0);
    });
    return out;
  }

  // First (i, j) after which the DFA is in the same state on `symbol` repeated.
  repeatOn(symbol) {
    const firstSeen = new Map([[this.start, 0]]);
    let s = this.start;
    for (let len = 1; len <= this.states.length + 1; len++) {
      s = this.step(s, symbol);
      if (firstSeen.has(s)) return [firstSeen.get(s), len];
      firstSeen.set(s, len);
    }
    return [-1, -1];
  }

  transitionTable() {
    const syms = this.alphabet;
    let sb = "  " + pad("state", 10);
    for (const s of syms) sb += " " + pad(s, 10);
    sb += "\n";
    for (const st of this.states) {
      const tag = (st === this.start ? "->" : "  ") + (this.accept.has(st) ? "*" : " ");
      sb += "  " + tag + pad(st, 8);
      for (const s of syms) sb += " " + pad(this.step(st, s), 10);
      sb += "\n";
    }
    return sb.replace(/\s+$/, "");   // matches Java String.stripTrailing()
  }

  static parse(text) {
    const states = [], alphabet = [], accept = [];
    let start = null;
    const delta = new Map();
    const add = (arr, xs) => { for (const x of xs) if (!arr.includes(x)) arr.push(x); };

    for (const raw of text.split(/\r?\n/)) {
      const h = raw.indexOf("#");
      const line = (h >= 0 ? raw.slice(0, h) : raw).trim();
      if (line === "") continue;
      if (line.startsWith("states:")) add(states, rest(line));
      else if (line.startsWith("alphabet:")) add(alphabet, rest(line));
      else if (line.startsWith("start:")) start = rest(line)[0];
      else if (line.startsWith("accept:")) add(accept, rest(line));
      else {
        const p = line.split(/\s+/);
        if (p.length !== 3) throw new Error("bad transition line: " + line);
        if (!delta.has(p[0])) delta.set(p[0], new Map());
        delta.get(p[0]).set(p[1], p[2]);
      }
    }
    if (!start || states.length === 0 || alphabet.length === 0)
      throw new Error("DFA needs states, alphabet, and a start state");
    if (!states.includes(start)) throw new Error("start state not in states");
    for (const a of accept) if (!states.includes(a)) throw new Error(`accept state '${a}' not in states`);

    let needDead = false;
    for (const s of states) for (const sym of alphabet)
      if (!delta.get(s)?.has(sym)) needDead = true;
    if (needDead) {
      if (!states.includes(DEAD)) states.push(DEAD);
      for (const s of states) {
        if (!delta.has(s)) delta.set(s, new Map());
        for (const sym of alphabet) if (!delta.get(s).has(sym)) delta.get(s).set(sym, DEAD);
      }
    }
    for (const m of delta.values()) for (const to of m.values())
      if (!states.includes(to)) throw new Error(`transition to unknown state '${to}'`);

    return new Dfa(states, alphabet, delta, start, accept);
  }
}

function rest(line) { return line.slice(line.indexOf(":") + 1).trim().split(/\s+/); }
function pad(s, w) { s = String(s); return s.length >= w ? s : s + " ".repeat(w - s.length); }
