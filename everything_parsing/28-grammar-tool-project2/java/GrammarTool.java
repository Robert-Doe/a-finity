import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.TreeSet;

/**
 * Module 28 — Grammar Analysis Tool  (CSE 340 Project 2).
 *
 * One program that reads any CFG and reports, in order:
 *   1. terminals and nonterminals, in order of first appearance
 *   2. nullable nonterminals                          (Module 21)
 *   3. FIRST sets                                     (Module 21)
 *   4. FOLLOW sets                                    (Module 22)
 *   5. left-recursion-free grammar (Paull)            (Module 23)
 *   6. left-factored grammar                          (Module 24)
 *   7. the LL(1) verdict + the parsing table          (Modules 20, 25)
 *
 * Every section is one of the earlier modules' components, composed. This module
 * writes no new algorithm — it is the integration.
 */
public final class GrammarTool {

    public final Grammar g;
    public final FirstSets fs;
    public final FollowSets fol;
    public final LL1Table table;

    public GrammarTool(Grammar g) {
        this.g = g;
        this.fs = new FirstSets(g);
        this.fol = new FollowSets(g);
        this.table = new LL1Table(g);
    }

    public String report() {
        StringBuilder sb = new StringBuilder();

        // 1 — symbols
        sb.append("1. SYMBOLS (in order of first appearance)\n");
        sb.append("   nonterminals: ").append(String.join(" ", g.nonterminals)).append('\n');
        sb.append("   terminals   : ").append(String.join(" ", g.terminals())).append('\n');
        sb.append("   start       : ").append(g.start).append('\n');
        sb.append('\n');

        // 2 — nullable
        sb.append("2. NULLABLE\n   ");
        sb.append(fs.nullable.isEmpty() ? "(none)" : String.join(" ", fs.nullable)).append('\n');
        sb.append('\n');

        // 3 — FIRST
        sb.append("3. FIRST\n");
        for (String nt : g.nonterminals)
            sb.append(String.format("   FIRST(%-9s) = %s%n", nt, FirstSets.setStr(fs.firstOf(nt))));
        sb.append('\n');

        // 4 — FOLLOW
        sb.append("4. FOLLOW\n");
        for (String nt : g.nonterminals)
            sb.append(String.format("   FOLLOW(%-9s) = %s%n", nt, FollowSets.setStr(fol.followOf(nt))));
        sb.append('\n');

        // 5 — left-recursion-free
        LeftRec.G noLR = LeftRec.paull(g);
        sb.append("5. LEFT-RECURSION-FREE GRAMMAR (Paull)\n");
        sb.append("   input left-recursive? ").append(LeftRec.hasLeftRecursion(LeftRec.G.from(g))).append('\n');
        for (String line : noLR.text().split("\n")) sb.append("   ").append(line).append('\n');
        sb.append("   still left-recursive? ").append(LeftRec.hasLeftRecursion(noLR)).append('\n');
        sb.append('\n');

        // 6 — left-factored
        LeftRec.G factored = LeftFactor.factor(g);
        sb.append("6. LEFT-FACTORED GRAMMAR\n");
        sb.append("   input needs factoring? ").append(LeftFactor.needsFactoring(LeftRec.G.from(g))).append('\n');
        for (String line : factored.text().split("\n")) sb.append("   ").append(line).append('\n');
        sb.append("   still needs factoring? ").append(LeftFactor.needsFactoring(factored)).append('\n');
        sb.append('\n');

        // 7 — LL(1) verdict
        sb.append("7. LL(1) VERDICT\n");
        Predict pr = new Predict(g);
        if (pr.isLL1()) {
            sb.append("   LL(1) = YES\n");
            sb.append("   parsing table (production index; . = blank):\n");
            for (String line : table.render().split("\n")) sb.append("     ").append(line).append('\n');
        } else {
            sb.append("   LL(1) = NO   ").append(pr.conflicts().size()).append(" conflict(s):\n");
            for (Predict.Conflict c : pr.conflicts())
                sb.append("     ").append(c).append('\n');
            sb.append("   -> apply sections 5 and 6, then re-check.\n");
            LeftRec.G fixed = LeftFactor.factor(toGrammar(LeftRec.paull(g)));
            Grammar fixedG = toGrammar(fixed);
            Predict pr2 = new Predict(fixedG);
            sb.append("   after left-recursion removal + left factoring: LL(1) = ")
              .append(pr2.isLL1() ? "YES" : "NO (" + pr2.conflicts().size() + " conflict(s) remain)")
              .append('\n');
        }

        return sb.toString();
    }

    /** Serialise a mutable grammar back to a Grammar (so later stages can re-parse it). */
    static Grammar toGrammar(LeftRec.G lg) {
        StringBuilder txt = new StringBuilder("%start ").append(lg.start).append('\n');
        txt.append(lg.text());
        return Grammar.parse(txt.toString());
    }
}
