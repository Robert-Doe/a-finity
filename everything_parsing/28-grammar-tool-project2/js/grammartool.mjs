// Module 28 — Grammar Analysis Tool (CSE 340 Project 2), mirror of java/GrammarTool.java.
//
// Reads a CFG and reports: symbols | nullable | FIRST | FOLLOW |
// left-recursion-free | left-factored | LL(1) verdict + table.
// Every section is an earlier module's component, composed. No new algorithm.

import { Grammar } from "./grammar.mjs";
import { FirstSets, setStr as firstSetStr } from "./firstsets.mjs";
import { FollowSets, setStr as followSetStr } from "./followsets.mjs";
import { G, paull, hasLeftRecursion } from "./leftrec.mjs";
import { factor, needsFactoring } from "./leftfactor.mjs";
import { Predict } from "./predict.mjs";
import { LL1Table } from "./ll1table.mjs";

const padEnd = (s, w) => { s = String(s); while (s.length < w) s += " "; return s; };

function toGrammar(lg) {
  return Grammar.parse("%start " + lg.start + "\n" + lg.text());
}

export class GrammarTool {
  constructor(g) {
    this.g = g;
    this.fs = new FirstSets(g);
    this.fol = new FollowSets(g);
    this.table = new LL1Table(g);
  }

  report() {
    const g = this.g;
    let sb = "";
    const p = s => { sb += s + "\n"; };

    // 1
    p("1. SYMBOLS (in order of first appearance)");
    p("   nonterminals: " + [...g.nonterminals].join(" "));
    p("   terminals   : " + [...g.terminals()].join(" "));
    p("   start       : " + g.start);
    p("");

    // 2
    p("2. NULLABLE");
    p("   " + (this.fs.nullable.size === 0 ? "(none)" : [...this.fs.nullable].join(" ")));
    p("");

    // 3
    p("3. FIRST");
    for (const nt of g.nonterminals)
      p("   FIRST(" + padEnd(nt, 9) + ") = " + firstSetStr(this.fs.firstOf(nt)));
    p("");

    // 4
    p("4. FOLLOW");
    for (const nt of g.nonterminals)
      p("   FOLLOW(" + padEnd(nt, 9) + ") = " + followSetStr(this.fol.followOf(nt)));
    p("");

    // 5
    const noLR = paull(g);
    p("5. LEFT-RECURSION-FREE GRAMMAR (Paull)");
    p("   input left-recursive? " + hasLeftRecursion(G.from(g)));
    for (const line of noLR.text().split("\n").filter(l => l !== "")) p("   " + line);
    p("   still left-recursive? " + hasLeftRecursion(noLR));
    p("");

    // 6
    const factored = factor(g);
    p("6. LEFT-FACTORED GRAMMAR");
    p("   input needs factoring? " + needsFactoring(G.from(g)));
    for (const line of factored.text().split("\n").filter(l => l !== "")) p("   " + line);
    p("   still needs factoring? " + needsFactoring(factored));
    p("");

    // 7
    p("7. LL(1) VERDICT");
    const pr = new Predict(g);
    if (pr.isLL1()) {
      p("   LL(1) = YES");
      p("   parsing table (production index; . = blank):");
      for (const line of this.table.render().split("\n").filter(l => l !== "")) p("     " + line);
    } else {
      const cs = pr.conflicts();
      p("   LL(1) = NO   " + cs.length + " conflict(s):");
      for (const c of cs) p("     " + c.toString());
      p("   -> apply sections 5 and 6, then re-check.");
      const fixedG = toGrammar(factor(toGrammar(paull(g))));
      const pr2 = new Predict(fixedG);
      const c2 = pr2.conflicts();
      p("   after left-recursion removal + left factoring: LL(1) = " +
        (pr2.isLL1() ? "YES" : "NO (" + c2.length + " conflict(s) remain)"));
    }

    return sb;
  }
}
