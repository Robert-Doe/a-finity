import java.util.List;
import java.util.Set;

/** java -cp java/out NfaTest */
public final class NfaTest {
    public static void main(String[] args) {
        System.out.println("NfaTest");

        // ── nondeterminism: two transitions on the same symbol ──
        Nfa abb = Nfa.parse("""
            states: S S1 S2 S3
            alphabet: a b
            start: S
            accept: S3
            S a S
            S b S
            S a S1
            S1 b S2
            S2 b S3
            """);
        Assert.equals(abb.move("S", "a").size(), 2, "S has two targets on 'a'");
        Assert.that(abb.acceptsString("abb"), "abb");
        Assert.that(abb.acceptsString("aabb"), "aabb");
        Assert.that(abb.acceptsString("bababb"), "bababb");
        Assert.that(!abb.acceptsString("ab"), "ab is not accepted");
        Assert.that(!abb.acceptsString("abba"), "abba is not accepted");
        Assert.that(!abb.acceptsString(""), "empty is not accepted");

        // trace: the state SET grows and shrinks
        var tr = abb.trace(List.of("a", "a", "b", "b"));
        Assert.equals(Nfa.showSet(tr.get(0)), "{S}", "start set");
        Assert.equals(Nfa.showSet(tr.get(1)), "{S,S1}", "after first a");
        Assert.equals(Nfa.showSet(tr.get(4)), "{S,S3}", "after aabb -- S3 reached");

        // ── epsilon closure ──
        Nfa eps = Nfa.parse("""
            states: I U A1 A2 B1 B2 W F
            alphabet: a b
            start: I
            accept: F
            I epsilon U
            I epsilon F
            U epsilon A1
            U epsilon B1
            A1 a A2
            B1 b B2
            A2 epsilon W
            B2 epsilon W
            W epsilon U
            W epsilon F
            """);
        Assert.that(eps.hasEpsilon(), "the NFA has epsilon transitions");
        Assert.equals(Nfa.showSet(eps.epsilonClosure(Set.of("I"))), "{A1,B1,F,I,U}",
            "epsilon-closure of the start includes F (so '' is accepted)");
        Assert.that(eps.acceptsString(""), "(a|b)* accepts the empty string");
        Assert.that(eps.acceptsString("ab") && eps.acceptsString("ba") && eps.acceptsString("abbaab"),
            "(a|b)* accepts mixed strings");

        // ── epsilon elimination preserves the language ──
        Nfa noEps = eps.removeEpsilon();
        Assert.that(!noEps.hasEpsilon(), "removeEpsilon() leaves no epsilon transitions");
        Assert.equals(eps.language(6), noEps.language(6),
            "the epsilon-free NFA accepts exactly the same language (up to length 6)");
        Assert.that(noEps.accept.contains("I"), "I is accepting now (its closure reached F)");

        // ── NFA for (a|b)*abb == DFA (Module 10) == regex ──
        Dfa dfa = Dfa.parse("""
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
        Regex re = Regex.parse("(a|b)*abb");
        for (String w : abb.language(7)) {
            if (w.equals("epsilon")) continue;
            Assert.that(dfa.acceptsString(w), "DFA agrees on \"" + w + "\"");
            Assert.that(re.matches(w), "regex agrees on \"" + w + "\"");
        }
        for (String w : dfa.language(7))
            if (!w.equals("epsilon"))
                Assert.that(abb.acceptsString(w), "NFA agrees on \"" + w + "\"");

        // ── malformed ──
        Assert.that(rejects("states: S\nstart: S\nS a S"), "missing alphabet:");
        Assert.that(rejects("states: S\nalphabet: a\nstart: S\nS a Q"), "transition to unknown state");

        Assert.summary();
    }

    private static boolean rejects(String s) {
        try { Nfa.parse(s); return false; } catch (IllegalArgumentException e) { return true; }
    }
}
