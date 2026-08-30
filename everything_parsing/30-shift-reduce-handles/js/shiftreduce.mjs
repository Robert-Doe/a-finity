// Module 30 — shift-reduce parsing, handles, viable prefixes
// (mirror of java/ShiftReduce.java).
//
// SHIFT: push the next input terminal.  REDUCE A -> b: top |b| stack symbols
// are b (a HANDLE); pop, push A.  ACCEPT: stack is [start], input consumed.
// The reductions reversed are a rightmost derivation. No table yet (Module 31):
// handles are found by bounded DFS (try reductions in order, then shift).

export class ShiftReduce {
  constructor(g) { this.g = g; this.nodes = 0; this.cap = 2_000_000; }

  parse(input) {
    this.nodes = 0;
    const steps = [];
    const reductions = [];
    const ok = this._dfs([], 0, input, steps, reductions);
    const note = this._firstConflict([], 0, input, { n: 0 });
    return { ok, steps, reductions, conflict: note != null, conflictNote: note };
  }

  _dfs(stack, pos, input, steps, reductions) {
    if (++this.nodes > this.cap) return false;

    if (stack.length === 1 && stack[0] === this.g.start && pos === input.length) {
      steps.push({ stack: join(stack), action: "ACCEPT", rest: "$" });
      return true;
    }

    for (const p of this.g.productions) {
      if (suffixMatches(stack, p.rhs)) {
        const cut = stack.length - p.rhs.length;
        const ns = [...stack.slice(0, cut), p.lhs];
        steps.push({ stack: join(stack),
          action: "reduce " + compact(p) + "   (handle: " + p.rhs.join(" ") + ")",
          rest: rest(input, pos) });
        reductions.push(p);
        if (this._dfs(ns, pos, input, steps, reductions)) return true;
        reductions.pop();
        steps.pop();
      }
    }

    if (pos < input.length) {
      const ns = [...stack, input[pos]];
      steps.push({ stack: join(stack), action: "shift " + input[pos], rest: rest(input, pos) });
      if (this._dfs(ns, pos + 1, input, steps, reductions)) return true;
      steps.pop();
    }
    return false;
  }

  _firstConflict(stack, pos, input, budget) {
    if (budget.n++ > 200_000) return null;

    const reduces = this.g.productions.filter(p => suffixMatches(stack, p.rhs));
    const canShift = pos < input.length;

    let accepting = 0;
    let kinds = "";
    for (const p of reduces) {
      const cut = stack.length - p.rhs.length;
      const ns = [...stack.slice(0, cut), p.lhs];
      if (this._reachesAccept(ns, pos, input, { n: 0 })) { accepting++; kinds += "reduce " + compact(p) + "; "; }
    }
    if (canShift) {
      const ns = [...stack, input[pos]];
      if (this._reachesAccept(ns, pos + 1, input, { n: 0 })) { accepting++; kinds += "shift " + input[pos] + "; "; }
    }
    if (accepting >= 2)
      return "stack [" + join(stack) + "], input " + rest(input, pos) + "  -> two accepting moves: " + kinds.trim();

    for (const p of reduces) {
      const cut = stack.length - p.rhs.length;
      const ns = [...stack.slice(0, cut), p.lhs];
      if (this._reachesAccept(ns, pos, input, { n: 0 })) {
        const r = this._firstConflict(ns, pos, input, budget);
        if (r != null) return r;
      }
    }
    if (canShift) {
      const ns = [...stack, input[pos]];
      if (this._reachesAccept(ns, pos + 1, input, { n: 0 })) {
        const r = this._firstConflict(ns, pos + 1, input, budget);
        if (r != null) return r;
      }
    }
    return null;
  }

  _reachesAccept(stack, pos, input, budget) {
    if (budget.n++ > 100_000) return false;
    if (stack.length === 1 && stack[0] === this.g.start && pos === input.length) return true;
    for (const p of this.g.productions)
      if (suffixMatches(stack, p.rhs)) {
        const cut = stack.length - p.rhs.length;
        const ns = [...stack.slice(0, cut), p.lhs];
        if (this._reachesAccept(ns, pos, input, budget)) return true;
      }
    if (pos < input.length) {
      const ns = [...stack, input[pos]];
      if (this._reachesAccept(ns, pos + 1, input, budget)) return true;
    }
    return false;
  }
}

export function compact(p) {
  return p.lhs + " -> " + (p.rhs.length === 0 ? "epsilon" : p.rhs.join(" "));
}

function suffixMatches(stack, rhs) {
  if (rhs.length === 0 || rhs.length > stack.length) return false;
  const off = stack.length - rhs.length;
  for (let i = 0; i < rhs.length; i++) if (stack[off + i] !== rhs[i]) return false;
  return true;
}
function join(xs) { return xs.length === 0 ? "(empty)" : xs.join(" "); }
function rest(input, pos) {
  if (pos >= input.length) return "$";
  return input.slice(pos).join(" ") + " $";
}
