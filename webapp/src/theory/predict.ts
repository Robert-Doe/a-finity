/**
 * Ported from everything_parsing/20-predictive-condition/js/predict.mjs
 * and 25-ll1-table/js/predict.mjs (identical across both modules).
 *
 *   PREDICT(A -> a) = FIRST(a)                            if a not nullable
 *                   = (FIRST(a) \ {epsilon}) U FOLLOW(A)   if a nullable
 *   LL(1)  <=>  every nonterminal's PREDICT sets are pairwise disjoint.
 */
import { Grammar, Production, EPS, END, productionToString } from "./grammar";

export interface Conflict {
  nonterminal: string;
  token: string;
  a: string;
  b: string;
}

export class Predict {
  first = new Map<string, Set<string>>();
  follow = new Map<string, Set<string>>();

  constructor(public g: Grammar) {
    this.computeFirst();
    this.computeFollow();
  }

  private computeFirst(): void {
    for (const nt of this.g.nonterminals) this.first.set(nt, new Set());
    let changed = true;
    while (changed) {
      changed = false;
      for (const p of this.g.productions) {
        const target = this.first.get(p.lhs)!;
        const before = target.size;
        for (const x of this.firstOfSeq(p.rhs)) target.add(x);
        if (target.size !== before) changed = true;
      }
    }
  }

  firstOf(sym: string): Set<string> {
    if (sym === EPS) return new Set([EPS]);
    if (!this.g.isNonterminal(sym)) return new Set([sym]);
    return new Set(this.first.get(sym) ?? []);
  }

  firstOfSeq(seq: string[]): Set<string> {
    const out = new Set<string>();
    let allNullable = true;
    for (const sym of seq) {
      const f = this.firstOf(sym);
      for (const x of f) if (x !== EPS) out.add(x);
      if (!f.has(EPS)) {
        allNullable = false;
        break;
      }
    }
    if (allNullable) out.add(EPS);
    return out;
  }

  nullable(seq: string[]): boolean {
    return this.firstOfSeq(seq).has(EPS);
  }

  private computeFollow(): void {
    for (const nt of this.g.nonterminals) this.follow.set(nt, new Set());
    this.follow.get(this.g.start)!.add(END);

    let changed = true;
    while (changed) {
      changed = false;
      for (const p of this.g.productions) {
        const rhs = p.rhs;
        for (let i = 0; i < rhs.length; i++) {
          const b = rhs[i];
          if (!this.g.isNonterminal(b)) continue;
          const beta = rhs.slice(i + 1);
          const fb = this.firstOfSeq(beta);

          const target = this.follow.get(b)!;
          const before = target.size;
          for (const x of fb) if (x !== EPS) target.add(x);
          if (fb.has(EPS)) for (const x of this.follow.get(p.lhs)!) target.add(x);
          if (target.size !== before) changed = true;
        }
      }
    }
  }

  followOf(nt: string): Set<string> {
    return new Set(this.follow.get(nt) ?? []);
  }

  predict(p: Production): Set<string> {
    const out = new Set<string>();
    const f = this.firstOfSeq(p.rhs);
    for (const x of f) if (x !== EPS) out.add(x);
    if (f.has(EPS)) for (const x of this.followOf(p.lhs)) out.add(x);
    return out;
  }

  conflicts(): Conflict[] {
    const out: Conflict[] = [];
    for (const nt of this.g.nonterminals) {
      const ps = this.g.productionsFor(nt);
      const preds = ps.map((p) => this.predict(p));
      for (let i = 0; i < ps.length; i++) {
        for (let j = i + 1; j < ps.length; j++) {
          for (const tok of preds[i]) {
            if (preds[j].has(tok)) {
              out.push({ nonterminal: nt, token: tok, a: productionToString(ps[i]), b: productionToString(ps[j]) });
            }
          }
        }
      }
    }
    return out;
  }

  isLL1(): boolean {
    return this.conflicts().length === 0;
  }
}

export function setStr(s: Iterable<string>): string {
  const items = [...s].sort((a, b) => {
    const ae = a === EPS || a === END,
      be = b === EPS || b === END;
    if (ae !== be) return ae ? 1 : -1;
    return a < b ? -1 : a > b ? 1 : 0;
  });
  return "{ " + items.join(", ") + " }";
}
