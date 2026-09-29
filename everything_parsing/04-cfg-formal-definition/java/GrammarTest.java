import java.util.List;

/** java -cp java/out GrammarTest */
public final class GrammarTest {
    public static void main(String[] args) {
        System.out.println("GrammarTest");

        // ── parsing a grammar file ──
        Grammar g = Grammar.parse("""
            # a^n b^n
            S -> a S b | epsilon
            """);
        Assert.equals(g.start, "S", "start symbol is the first lhs");
        Assert.equals(g.nonterminals.toString(), "[S]", "one nonterminal");
        Assert.equals(g.terminals().toString(), "[a, b]", "terminals are rhs symbols never on a lhs");
        Assert.equals(g.productions.size(), 2, "two productions (one per alternative)");
        Assert.that(g.productions.get(1).isEpsilon(), "second production is epsilon");
        Assert.equals(g.productions.get(0).toString(), "S -> a S b", "first production renders");

        // ── %start override, multi-rule ──
        Grammar h = Grammar.parse("""
            %start B
            A -> x
            B -> A A
            """);
        Assert.equals(h.start, "B", "%start overrides first-lhs");
        Assert.equals(h.terminals().toString(), "[x]", "A is a nonterminal, not a terminal");

        // ── malformed grammars are rejected ──
        Assert.that(rejects("S -> a\nS b -> c"), "lhs must be a single symbol");
        Assert.that(rejects("S -> -> a"), "two arrows rejected");
        Assert.that(rejects("just some text"), "missing arrow rejected");
        Assert.that(rejects("%start Z\nS -> a"), "start symbol must be a nonterminal");
        Assert.that(rejects("S -> a epsilon b"), "epsilon cannot be mixed with symbols");

        // ── leftmost derivation ──
        var steps = Derivation.leftmostDerivation(g, new int[]{0, 0, 1});
        Assert.equals(steps.size(), 4, "3 steps => 4 sentential forms");
        Assert.equals(String.join(" ", steps.get(0).form()), "S", "form 0 is the start symbol");
        Assert.equals(String.join(" ", steps.get(1).form()), "a S b", "form 1");
        Assert.equals(String.join(" ", steps.get(2).form()), "a a S b b", "form 2");
        Assert.equals(String.join(" ", steps.get(3).form()), "a a b b", "form 3 is all terminals");

        // wrong production for the current leftmost nonterminal is an error
        Assert.that(throwsOn(() -> Derivation.leftmostDerivation(h, new int[]{0})),
            "choosing A -> x when leftmost NT is B is rejected");

        // ── enumerate: the language, and its infiniteness ──
        List<String> lang6 = Derivation.enumerate(g, 6);
        Assert.equals(lang6.toString(), "[epsilon, a b, a a b b, a a a b b b]",
            "a^n b^n up to length 6, in (length, lexicographic) order");
        Assert.that(Derivation.enumerate(g, 8).size() == lang6.size() + 1,
            "raising the bound by one pair adds exactly one string -- infinite");

        // ── derives: membership ──
        Assert.that(Derivation.derives(g, List.of("a", "a", "b", "b")).derivable(), "aabb in L(G)");
        Assert.equals(Derivation.derives(g, List.of("a", "a", "b", "b")).steps(), 3, "aabb in 3 steps");
        Assert.that(!Derivation.derives(g, List.of("a", "a", "b")).derivable(), "aab not in L(G)");
        Assert.that(!Derivation.derives(g, List.of("b", "a")).derivable(), "ba not in L(G)");
        Assert.that(Derivation.derives(g, List.of()).derivable(), "epsilon in L(G)");

        // ── a second grammar: balanced parentheses ──
        Grammar bal = Grammar.parse("S -> ( S ) S | epsilon");
        Assert.that(Derivation.derives(bal, List.of("(", ")")).derivable(), "() balanced");
        Assert.that(Derivation.derives(bal, List.of("(", "(", ")", ")")).derivable(), "(()) balanced");
        Assert.that(!Derivation.derives(bal, List.of("(", ")", ")")).derivable(), "()) not balanced");
        Assert.equals(Derivation.enumerate(bal, 4).toString(),
            "[epsilon, ( ), ( ( ) ), ( ) ( )]", "Dyck language up to length 4 (symbols, then lexicographic)");

        Assert.summary();
    }

    private static boolean rejects(String grammarText) {
        try { Grammar.parse(grammarText); return false; }
        catch (IllegalArgumentException e) { return true; }
    }
    private static boolean throwsOn(Runnable r) {
        try { r.run(); return false; } catch (RuntimeException e) { return true; }
    }
}
