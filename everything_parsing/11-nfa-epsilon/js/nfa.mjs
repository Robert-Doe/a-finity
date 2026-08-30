// Module 11 — a nondeterministic finite automaton, with epsilon transitions.
//
//   1. delta(q, a) is a SET of states  (0, 1, or many)
//   2. epsilon transitions move without consuming input
//
// A string is accepted if SOME path ends accepting. The simulator tracks the
// whole SET of states the NFA could be in. epsilonClosure adds every state
// reachable by epsilon alone; run it before the first symbol and after each.
//
// removeEpsilon() produces an equivalent NFA with no epsilon transitions —
// proving epsilon adds no power. (Module 13 removes the nondeterminism.)

export const EPS = "epsilon";

export class Nfa {
  constructor(states, alphabet, delta, start, accept) {
    this.states = states;         // string[]
    this.alphabet = alphabet;     // string[]  (no epsilon)
    this.delta = delta;           // Map<from, Map<symbol, Set<to>>>
    this.start = start;
    this.accept = new Set(accept);
  }

  move(state, symbol) {
    return this.delta.get(state)?.get(symbol) ?? new Set();
  }

  epsilonClosure(set) {
    const closure = new Set(set);
    const work = [...set];
    while (work.length) {
      const s = work.shift();
      for (const t of this.move(s, EPS)) if (!closure.has(t)) { closure.add(t); work.push(t); }
    }
    return closure;
  }

  trace(input) {
    const out = [];
    let cur = this.epsilonClosure(new Set([this.start]));
    out.push(cur);
    for (const sym of input) {
      const moved = new Set();
      for (const s of cur) for (const t of this.move(s, sym)) moved.add(t);
      cur = this.epsilonClosure(moved);
      out.push(cur);
    }
    return out;
  }

  accepts(input) {
    let cur = this.epsilonClosure(new Set([this.start]));
    for (const sym of input) {
      if (!this.alphabet.includes(sym)) return false;
      const moved = new Set();
      for (const s of cur) for (const t of this.move(s, sym)) moved.add(t);
      cur = this.epsilonClosure(moved);
      if (cur.size === 0) return false;
    }
    for (const s of cur) if (this.accept.has(s)) return true;
    return false;
  }

  acceptsString(w) { return this.accepts([...w]); }

  language(maxLen) {
    const out = [];
    const q = [""];
    while (q.length) {
      const s = q.shift();
      if (this.acceptsString(s)) out.push(s === "" ? "epsilon" : s);
      if (s.length < maxLen) for (const sym of this.alphabet) q.push(s + sym);
    }
    out.sort((x, y) => {
      const lx = x === "epsilon" ? 0 : x.length, ly = y === "epsilon" ? 0 : y.length;
      return lx !== ly ? lx - ly : (x < y ? -1 : x > y ? 1 : 0);
    });
    return out;
  }

  removeEpsilon() {
    const nd = new Map();
    const nAccept = [];
    for (const q of this.states) {
      const cq = this.epsilonClosure(new Set([q]));
      if ([...cq].some(s => this.accept.has(s))) nAccept.push(q);
      for (const a of this.alphabet) {
        const moved = new Set();
        for (const p of cq) for (const t of this.move(p, a)) moved.add(t);
        const target = this.epsilonClosure(moved);
        if (target.size > 0) {
          if (!nd.has(q)) nd.set(q, new Map());
          nd.get(q).set(a, target);
        }
      }
    }
    return new Nfa([...this.states], [...this.alphabet], nd, this.start, nAccept);
  }

  hasEpsilon() {
    for (const m of this.delta.values()) if (m.has(EPS)) return true;
    return false;
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
        if (!delta.get(p[0]).has(p[1])) delta.get(p[0]).set(p[1], new Set());
        delta.get(p[0]).get(p[1]).add(p[2]);
      }
    }
    if (!start || states.length === 0 || alphabet.length === 0)
      throw new Error("NFA needs states, alphabet, and a start state");
    if (!states.includes(start)) throw new Error("start state not in states");
    for (const a of accept) if (!states.includes(a)) throw new Error(`accept '${a}' not in states`);
    for (const m of delta.values()) for (const set of m.values()) for (const to of set)
      if (!states.includes(to)) throw new Error(`transition to unknown state '${to}'`);

    return new Nfa(states, alphabet, delta, start, accept);
  }
}

export function showSet(s) {
  return s.size === 0 ? "{}" : "{" + [...s].sort().join(",") + "}";
}

function rest(line) { return line.slice(line.indexOf(":") + 1).trim().split(/\s+/); }
