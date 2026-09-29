// Module 13 — the subset (powerset) construction: NFA -> equivalent DFA.
//
// A DFA state is a SET of NFA states — "every state a clone could be in."
//   DFA start        = epsilonClosure({ nfa.start })
//   DFA delta(S, a)  = epsilonClosure( union of nfa.move(s, a) for s in S )
//   DFA state S accepting  iff  S meets nfa.accept
//
// Only reachable subsets are built (worklist), so the DFA is usually much
// smaller than 2^|NFA states| — but the worst case is exactly that big.

import { Nfa } from "./nfa.mjs";
import { Dfa } from "./dfa.mjs";

const key = (set) => [...set].sort().join("");

function explore(nfa) {
  // returns { order: [ [name, sortedSet] ], deltaByName: Map, acceptNames: [] }
  const nameOf = new Map();               // key -> name
  const setOf = new Map();                // name -> sorted array
  const order = [];
  const work = [];

  const start = [...nfa.epsilonClosure(new Set([nfa.start]))].sort();
  nameOf.set(key(start), "D0");
  setOf.set("D0", start);
  order.push("D0");
  work.push(start);

  const deltaByName = new Map();
  const acceptNames = [];

  while (work.length) {
    const S = work.shift();
    const sName = nameOf.get(key(S));
    if (S.some(s => nfa.accept.has(s))) acceptNames.push(sName);

    for (const a of nfa.alphabet) {
      const moved = new Set();
      for (const s of S) for (const t of nfa.move(s, a)) moved.add(t);
      const T = [...nfa.epsilonClosure(moved)].sort();
      const k = key(T);
      let tName = nameOf.get(k);
      if (tName === undefined) {
        tName = "D" + nameOf.size;
        nameOf.set(k, tName);
        setOf.set(tName, T);
        order.push(tName);
        work.push(T);
      }
      if (!deltaByName.has(sName)) deltaByName.set(sName, new Map());
      deltaByName.get(sName).set(a, tName);
    }
  }
  return { order, setOf, deltaByName, acceptNames };
}

export function determinize(nfa) {
  const { order, deltaByName, acceptNames } = explore(nfa);
  let text = "states: " + order.join(" ") + "\n";
  text += "alphabet: " + nfa.alphabet.join(" ") + "\n";
  text += "start: D0\n";
  text += "accept: " + acceptNames.join(" ") + "\n";
  for (const [from, m] of deltaByName)
    for (const [a, to] of m)
      text += from + " " + a + " " + to + "\n";
  return Dfa.parse(text);
}

export function legend(nfa) {
  const { order, setOf } = explore(nfa);
  const out = new Map();
  for (const n of order) out.set(n, setOf.get(n));
  return out;
}

// NFA over {a,b} for "the k-th symbol from the end is 'a'": k+1 states, DFA 2^k.
export function kthFromEndNfa(k) {
  let text = "states:";
  for (let i = 0; i <= k; i++) text += " s" + i;
  text += "\nalphabet: a b\nstart: s0\naccept: s" + k + "\n";
  text += "s0 a s0\ns0 b s0\ns0 a s1\n";
  for (let i = 1; i < k; i++) {
    text += "s" + i + " a s" + (i + 1) + "\n";
    text += "s" + i + " b s" + (i + 1) + "\n";
  }
  return Nfa.parse(text);
}
