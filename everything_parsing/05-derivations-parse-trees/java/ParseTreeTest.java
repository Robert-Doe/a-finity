import java.util.List;

/** java -cp java/out ParseTreeTest */
public final class ParseTreeTest {
    public static void main(String[] args) {
        System.out.println("ParseTreeTest");

        Grammar expr = Grammar.parse("""
            E -> E + T | T
            T -> T * F | F
            F -> ( E ) | id
            """);
        int[] lmChoices = {0, 1, 3, 5, 2, 3, 5, 5}; // leftmost derivation of id + id * id

        ParseTree tree = ParseTree.build(expr, lmChoices, true);

        // ── the tree yields the target string ──
        Assert.equals(String.join(" ", tree.terminalYield()), "id + id * id",
            "tree yields id + id * id");

        // ── leftmost and rightmost derivations off the SAME tree ──
        var lm = ParseTree.derivation(expr, tree, true);
        var rm = ParseTree.derivation(expr, tree, false);

        Assert.equals(lm.size(), 9, "leftmost: 8 steps + initial form");
        Assert.equals(rm.size(), 9, "rightmost: 8 steps + initial form");
        Assert.equals(lm.size(), rm.size(), "same number of steps");

        // both start at [E] and end at the yield
        Assert.equals(String.join(" ", lm.get(0).form()), "E", "leftmost starts at E");
        Assert.equals(String.join(" ", rm.get(0).form()), "E", "rightmost starts at E");
        Assert.equals(String.join(" ", lm.get(lm.size() - 1).form()), "id + id * id", "leftmost ends at the yield");
        Assert.equals(String.join(" ", rm.get(rm.size() - 1).form()), "id + id * id", "rightmost ends at the yield");

        // the SECOND sentential form differs between the two orders
        Assert.equals(String.join(" ", lm.get(2).form()), "T + T", "leftmost step 2: expand the left E");
        Assert.equals(String.join(" ", rm.get(2).form()), "E + T * F", "rightmost step 2: expand the right T");
        Assert.that(!lm.get(2).form().equals(rm.get(2).form()), "the intermediate forms genuinely differ");

        // ── but the productions used are the same multiset ──
        String multiset = ParseTree.productionMultiset(tree);
        Assert.equals(multiset, "[E->E+T, E->T, T->T*F, T->F, T->F, F->id, F->id, F->id]",
            "8 productions, sorted by index");
        Assert.equals(lmProductions(lm), sorted(multiset), "leftmost uses exactly this multiset");
        Assert.equals(rmProductions(rm), sorted(multiset), "rightmost uses exactly this multiset");

        // ── a small nested tree: a^n b^n ──
        Grammar anbn = Grammar.parse("S -> a S b | epsilon");
        ParseTree t2 = ParseTree.build(anbn, new int[]{0, 0, 1}, true);
        Assert.equals(String.join(" ", t2.terminalYield()), "a a b b", "anbn tree yields a a b b");
        Assert.equals(t2.render(),
            String.join("\n",
                "S",
                "+- a",
                "+- S",
                "|  +- a",
                "|  +- S",
                "|  |  +- epsilon",
                "|  +- b",
                "+- b"),
            "the nested S tree renders as expected");

        // rightmost derivation of a^n b^n expands inner S before... actually S is the only NT each level
        var d = ParseTree.derivation(anbn, t2, false);
        Assert.equals(String.join(" ", d.get(1).form()), "a S b", "anbn step 1");
        Assert.equals(String.join(" ", d.get(3).form()), "a a b b", "anbn ends at a a b b");

        // ── build rejects mismatched choices ──
        Assert.that(rejects(() -> ParseTree.build(expr, new int[]{5}, true)),
            "choosing F->id when the frontier nonterminal is E is rejected");
        Assert.that(rejects(() -> ParseTree.build(expr, new int[]{1}, true)),
            "choices that finish the tree early... actually E->T leaves T unexpanded → 'choices ran out'");

        Assert.summary();
    }

    private static String lmProductions(List<ParseTree.Step> steps) {
        return productionsSorted(steps);
    }
    private static String rmProductions(List<ParseTree.Step> steps) {
        return productionsSorted(steps);
    }
    private static String productionsSorted(List<ParseTree.Step> steps) {
        List<Grammar.Production> ps = new java.util.ArrayList<>();
        for (var s : steps) if (s.applied() != null) ps.add(s.applied());
        ps.sort((a, b) -> Integer.compare(a.index(), b.index()));
        List<String> parts = new java.util.ArrayList<>();
        for (var p : ps) parts.add(p.lhs() + "->" + (p.rhs().isEmpty() ? "eps" : String.join("", p.rhs())));
        return "[" + String.join(", ", parts) + "]";
    }
    private static String sorted(String multiset) { return multiset; }

    private static boolean rejects(Runnable r) {
        try { r.run(); return false; } catch (RuntimeException e) { return true; }
    }
}
