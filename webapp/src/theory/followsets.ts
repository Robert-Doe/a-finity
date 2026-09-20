/**
 * Ported from everything_parsing/22-follow-sets/js/followsets.mjs.
 *
 * FOLLOW as a least fixed point, given FIRST:
 *   1. $ in FOLLOW(start)
 *   2. B -> a A b             =>  FIRST(b) \ {epsilon}  subset of FOLLOW(A)
 *   3. B -> a A b, b nullable =>  FOLLOW(B)             subset of FOLLOW(A)
 * Rule 3 makes FOLLOW circular; start empty, add only, stop when a pass
 * changes nothing.
 */
import { Grammar, EPS, END } from "./grammar";
import { FirstSets } from "./firstsets";

export class FollowSets {
  first: FirstSets;
  follow: Map<string, Set<string>>;

  constructor(public g: Grammar) {
    this.first = new FirstSets(g);
    this.follow = this.compute();
  }

  private compute(): Map<string, Set<string>> {
    const fol = new Map<string, Set<string>>();
    for (const nt of this.g.nonterminals) fol.set(nt, new Set());
    fol.get(this.g.start)!.add(END);

    let changed = true;
    while (changed) {
      changed = false;
      for (const p of this.g.productions) {
        const rhs = p.rhs;
        for (let i = 0; i < rhs.length; i++) {
          const a = rhs[i];
          if (!this.g.isNonterminal(a)) continue;
          const beta = rhs.slice(i + 1);
          const fb = this.first.firstOfSeq(beta);

          const target = fol.get(a)!;
          const before = target.size;
          for (const x of fb) if (x !== EPS) target.add(x);
          if (fb.has(EPS)) for (const x of fol.get(p.lhs)!) target.add(x);
          if (target.size !== before) changed = true;
        }
      }
    }
    return fol;
  }

  followOf(nt: string): Set<string> {
    return new Set(this.follow.get(nt) ?? []);
  }
}
