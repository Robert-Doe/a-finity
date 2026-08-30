// Module 14 — DFA minimization.
//
// Two states are EQUIVALENT if the DFA accepts the same continuations from
// either. Merge each equivalence class → the MINIMAL DFA, unique by Myhill-Nerode.
//
//   partitionRefinement — {accepting}|{non-accepting}, split any block whose
//                         members disagree on where a symbol sends them (Moore)
//   tableFilling         — mark distinguishable pairs to a fixed point

import { Dfa } from "./dfa.mjs";
import { build } from "./thompson.mjs";
import { determinize } from "./subset.mjs";

// ── remove unreachable states, keep original names ──

export function trim(d) {
  const reach = new Set([d.start]);
  const q = [d.start];
  while (q.length) {
    const s = q.shift();
    for (const a of d.alphabet) {
      const t = d.step(s, a);
      if (!reach.has(t)) { reach.add(t); q.push(t); }
    }
  }
  if (reach.size === d.states.length) return d;

  let text = "states: " + [...reach].join(" ") + "\n";
  text += "alphabet: " + d.alphabet.join(" ") + "\n";
  text += "start: " + d.start + "\n";
  text += "accept: " + [...reach].filter(s => d.accept.has(s)).join(" ") + "\n";
  for (const s of reach)
    for (const a of d.alphabet)
      text += s + " " + a + " " + d.step(s, a) + "\n";
  return Dfa.parse(text);
}

// ── partition refinement (Moore) ──

export function partitionRefinement(d0) {
  const d = trim(d0);
  const states = d.states;
  const block = new Map();
  for (const s of states) block.set(s, d.accept.has(s) ? 1 : 0);
  let count = new Set(block.values()).size;

  for (;;) {
    const sigId = new Map();
    const next = new Map();
    for (const s of states) {
      const sig = [block.get(s), ...d.alphabet.map(a => block.get(d.step(s, a)))].join(",");
      if (!sigId.has(sig)) sigId.set(sig, sigId.size);
      next.set(s, sigId.get(sig));
    }
    const nc = new Set(next.values()).size;
    for (const [k, v] of next) block.set(k, v);
    if (nc === count) break;
    count = nc;
  }
  return blocksToSets(states, block);
}

// ── table filling ──

export function tableFilling(d0) {
  const d = trim(d0);
  const states = d.states;
  const n = states.length;
  const idx = new Map(states.map((s, i) => [s, i]));
  const marked = Array.from({ length: n }, () => new Array(n).fill(false));

  for (let i = 0; i < n; i++)
    for (let j = i + 1; j < n; j++)
      marked[i][j] = d.accept.has(states[i]) !== d.accept.has(states[j]);

  let changed = true;
  while (changed) {
    changed = false;
    for (let i = 0; i < n; i++)
      for (let j = i + 1; j < n; j++) {
        if (marked[i][j]) continue;
        for (const a of d.alphabet) {
          const pi = idx.get(d.step(states[i], a));
          const pj = idx.get(d.step(states[j], a));
          const lo = Math.min(pi, pj), hi = Math.max(pi, pj);
          if (lo !== hi && marked[lo][hi]) { marked[i][j] = true; changed = true; break; }
        }
      }
  }

  const parent = states.map((_, i) => i);
  const find = x => (parent[x] === x ? x : (parent[x] = find(parent[x])));
  for (let i = 0; i < n; i++)
    for (let j = i + 1; j < n; j++)
      if (!marked[i][j]) parent[find(i)] = find(j);

  const classes = new Map();
  for (let i = 0; i < n; i++) {
    const r = find(i);
    if (!classes.has(r)) classes.set(r, new Set());
    classes.get(r).add(states[i]);
  }
  return [...classes.values()].map(s => [...s].sort());
}

// ── quotient (minimal) DFA ──

export function minimal(d0) {
  const d = trim(d0);
  let classes = partitionRefinement(d).map(s => [...s].sort());
  classes.sort((x, y) => {
    const xs = x.includes(d.start), ys = y.includes(d.start);
    if (xs !== ys) return xs ? -1 : 1;
    return x[0] < y[0] ? -1 : x[0] > y[0] ? 1 : 0;
  });

  const nameOf = new Map();
  classes.forEach((c, i) => c.forEach(s => nameOf.set(s, "M" + i)));

  let text = "states: " + classes.map((_, i) => "M" + i).join(" ") + "\n";
  text += "alphabet: " + d.alphabet.join(" ") + "\n";
  text += "start: " + nameOf.get(d.start) + "\n";
  text += "accept: " + classes.map((c, i) => (d.accept.has(c[0]) ? "M" + i : null)).filter(Boolean).join(" ") + "\n";
  classes.forEach((c, i) => {
    for (const a of d.alphabet) text += "M" + i + " " + a + " " + nameOf.get(d.step(c[0], a)) + "\n";
  });
  return Dfa.parse(text);
}

// ── regex equivalence via minimal-DFA isomorphism ──

export function equivalent(a, b) {
  return isomorphic(minimal(determinize(build(a))), minimal(determinize(build(b))));
}

export function isomorphic(a, b) {
  if (a.states.length !== b.states.length) return false;
  if (a.alphabet.join(",") !== b.alphabet.join(",")) return false;
  const map = new Map([[a.start, b.start]]);
  const q = [[a.start, b.start]];
  while (q.length) {
    const [sa, sb] = q.shift();
    if (a.accept.has(sa) !== b.accept.has(sb)) return false;
    for (const sym of a.alphabet) {
      const ta = a.step(sa, sym), tb = b.step(sb, sym);
      if (!map.has(ta)) { map.set(ta, tb); q.push([ta, tb]); }
      else if (map.get(ta) !== tb) return false;
    }
  }
  return true;
}

// ── helpers ──

function blocksToSets(states, block) {
  const g = new Map();
  for (const s of states) {
    const b = block.get(s);
    if (!g.has(b)) g.set(b, new Set());
    g.get(b).add(s);
  }
  return [...g.values()].map(s => [...s].sort());
}
