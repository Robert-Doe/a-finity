/**
 * Ported from everything_parsing/21-nullable-first/js/firstsets.mjs.
 *
 * NULLABLE = smallest set N with: A in N if A -> epsilon; A in N if
 * A -> X1..Xk and every Xi in N. Start empty, apply until a pass adds
 * nothing (a least fixed point). FIRST is the same shape over sets of
 * terminals. Both record every round, for showing the fixed-point climb.
 */
import { Grammar, EPS } from "./grammar";

export class FirstSets {
  nullableRounds: Set<string>[] = [];
  firstRounds: Map<string, Set<string>>[] = [];
  nullable: Set<string>;
  first: Map<string, Set<string>>;

  constructor(public g: Grammar) {
    this.nullable = this.computeNullable();
    this.first = this.computeFirst();
  }

  private computeNullable(): Set<string> {
    const n = new Set<string>();
    this.nullableRounds.push(new Set(n));
    let changed = true;
    while (changed) {
      changed = false;
      for (const p of this.g.productions) {
        if (n.has(p.lhs)) continue;
        let allNull = true;
        for (const s of p.rhs) {
          if (!n.has(s)) {
            allNull = false;
            break;
          }
        }
        if (allNull) {
          n.add(p.lhs);
          changed = true;
        }
      }
      this.nullableRounds.push(new Set(n));
    }
    return n;
  }

  nullableSeq(seq: string[]): boolean {
    return seq.every((s) => this.nullable.has(s));
  }

  private computeFirst(): Map<string, Set<string>> {
    const f = new Map<string, Set<string>>();
    for (const nt of this.g.nonterminals) f.set(nt, new Set());
    this.firstRounds.push(snapshot(f));

    let changed = true;
    while (changed) {
      changed = false;
      for (const p of this.g.productions) {
        const target = f.get(p.lhs)!;
        const before = target.size;
        for (const x of this.firstOfSeqWith(f, p.rhs)) target.add(x);
        if (target.size !== before) changed = true;
      }
      this.firstRounds.push(snapshot(f));
    }
    return f;
  }

  private firstOfSeqWith(f: Map<string, Set<string>>, seq: string[]): Set<string> {
    const out = new Set<string>();
    let allNullable = true;
    for (const sym of seq) {
      if (!this.g.isNonterminal(sym)) {
        out.add(sym);
        allNullable = false;
        break;
      }
      for (const x of f.get(sym) ?? []) if (x !== EPS) out.add(x);
      if (!this.nullable.has(sym)) {
        allNullable = false;
        break;
      }
    }
    if (allNullable) out.add(EPS);
    return out;
  }

  firstOfSeq(seq: string[]): Set<string> {
    return this.firstOfSeqWith(this.first, seq);
  }

  firstOf(sym: string): Set<string> {
    if (!this.g.isNonterminal(sym)) return new Set([sym]);
    return new Set(this.first.get(sym));
  }
}

function snapshot(f: Map<string, Set<string>>): Map<string, Set<string>> {
  const s = new Map<string, Set<string>>();
  for (const [k, v] of f) s.set(k, new Set(v));
  return s;
}

export function setStr(s: Iterable<string>): string {
  const items = [...s].sort((a, b) => {
    const ae = a === EPS,
      be = b === EPS;
    if (ae !== be) return ae ? 1 : -1;
    return a < b ? -1 : a > b ? 1 : 0;
  });
  return "{ " + items.join(", ") + " }";
}
