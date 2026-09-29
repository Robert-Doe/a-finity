import java.util.List;
import java.util.Set;
import java.util.TreeSet;

/** java -cp java/out MinimizeTest */
public final class MinimizeTest {
    public static void main(String[] args) {
        System.out.println("MinimizeTest");

        // ── a DFA with an obviously redundant state ──
        // two accepting states that behave identically
        Dfa redundant = Dfa.parse("""
            states: S A B
            alphabet: a
            start: S
            accept: A B
            S a A
            A a B
            B a A
            """);
        Dfa m = Minimize.minimal(redundant);
        Assert.equals(m.states.size(), 2, "S | {A,B} -> 2 states (A and B are equivalent)");
        Assert.equals(m.language(6), redundant.language(6), "minimization preserves the language");

        // ── the two algorithms agree ──
        for (String r : new String[]{"(a|b)*abb", "a*", "(a|b)*", "a(b|c)*d", "(ab|ba)*", "a?b?c?"}) {
            Dfa d = Minimize.trim(Subset.determinize(Thompson.build(Regex.parse(r))));
            var pr = Minimize.partitionRefinement(d);
            var tf = Minimize.tableFilling(d);
            Assert.that(samePartition(pr, tf),
                "/" + r + "/: partition refinement and table filling give the same classes");
        }

        // ── minimal is idempotent and language-preserving ──
        for (String r : new String[]{"(a|b)*abb", "(a|b)*", "a(b|c)", "(ab)*"}) {
            Dfa full = Subset.determinize(Thompson.build(Regex.parse(r)));
            Dfa min1 = Minimize.minimal(full);
            Dfa min2 = Minimize.minimal(min1);
            Assert.equals(min1.states.size(), min2.states.size(), "/" + r + "/: minimizing twice is idempotent");
            Assert.equals(full.language(6), min1.language(6), "/" + r + "/: language preserved");
            Assert.that(Minimize.isomorphic(min1, min2), "/" + r + "/: the two minimal DFAs are isomorphic");
        }

        // ── the minimal DFA for (a|b)*abb has exactly 4 states ──
        Dfa min = Minimize.minimal(Subset.determinize(Thompson.build(Regex.parse("(a|b)*abb"))));
        Assert.equals(min.states.size(), 4, "minimal DFA for (a|b)*abb has 4 states (Myhill-Nerode)");
        Dfa hand = Dfa.parse("""
            states: S A AB ABB
            alphabet: a b
            start: S
            accept: ABB
            S a A
            S b S
            A a A
            A b AB
            AB a A
            AB b ABB
            ABB a A
            ABB b S
            """);
        Assert.that(Minimize.isomorphic(min, hand), "minimal DFA == Module 10's hand-written DFA (up to renaming)");
        Assert.equals(Minimize.minimal(hand).states.size(), 4, "the hand-written DFA was already minimal");

        // ── regex equivalence ──
        Assert.that(eq("(a|b)*", "(a*b*)*"), "(a|b)* == (a*b*)*  (both Sigma*)");
        Assert.that(eq("a**", "a*"), "a** == a*");
        Assert.that(eq("a(b|c)", "ab|ac"), "a(b|c) == ab|ac  (distributivity)");
        Assert.that(eq("(ab)*", "(ab)*ab|()"), "(ab)* == (ab)*ab | epsilon");
        Assert.that(!eq("a*", "a+"), "a* != a+");
        Assert.that(!eq("ab|ba", "(a|b)(a|b)"), "ab|ba != (a|b)(a|b)");
        Assert.that(!eq("(a|b)*abb", "(a|b)*ab"), "(a|b)*abb != (a|b)*ab");
        Assert.that(eq("(a|b)*abb", "(a|b)*abb"), "a regex equals itself");

        Assert.summary();
    }

    static boolean eq(String x, String y) {
        return Minimize.equivalent(Regex.parse(x), Regex.parse(y));
    }
    static boolean samePartition(List<Set<String>> a, List<Set<String>> b) {
        Set<Set<String>> sa = new java.util.HashSet<>();
        for (var s : a) sa.add(new TreeSet<>(s));
        Set<Set<String>> sb = new java.util.HashSet<>();
        for (var s : b) sb.add(new TreeSet<>(s));
        return sa.equals(sb);
    }
}
