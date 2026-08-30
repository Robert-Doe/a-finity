import java.util.List;

/** java -cp java/out DfaTest */
public final class DfaTest {
    public static void main(String[] args) {
        System.out.println("DfaTest");

        Dfa abb = Dfa.parse("""
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

        // ── delta is total: every (state, symbol) maps somewhere ──
        for (String s : abb.states)
            for (String sym : abb.alphabet)
                Assert.that(abb.states.contains(abb.step(s, sym)),
                    "delta(" + s + "," + sym + ") is defined");

        // ── acceptance ──
        Assert.that(abb.acceptsString("abb"), "abb ends in abb");
        Assert.that(abb.acceptsString("aabb"), "aabb ends in abb");
        Assert.that(abb.acceptsString("babbabb"), "babbabb ends in abb");
        Assert.that(!abb.acceptsString(""), "empty does not end in abb");
        Assert.that(!abb.acceptsString("ab"), "ab does not end in abb");
        Assert.that(!abb.acceptsString("abba"), "abba does not end in abb");
        Assert.that(!abb.acceptsString("abc"), "symbol outside the alphabet -> reject");

        // ── trace ──
        Assert.equals(String.join(" ", abb.trace(List.of("a", "b", "b"))), "S A AB ABB", "trace of abb");

        // ── DFA language == regex language, up to a bound ──
        Regex re = Regex.parse("(a|b)*abb");
        for (String w : abb.language(6))
            if (!w.equals("epsilon"))
                Assert.that(re.matches(w), "DFA-accepted \"" + w + "\" is also regex-matched");
        for (String w : re.toLanguage(6).list())
            if (!w.isEmpty())
                Assert.that(abb.acceptsString(w), "regex-matched \"" + w + "\" is also DFA-accepted");

        // ── missing transitions route to an implicit DEAD state ──
        Dfa partial = Dfa.parse("""
            states: X Y
            alphabet: a b
            start: X
            accept: Y
            X a Y
            """);
        Assert.that(partial.states.contains(Dfa.DEAD), "a DEAD state was added");
        Assert.equals(partial.step("X", "b"), Dfa.DEAD, "the missing (X,b) goes to DEAD");
        Assert.equals(partial.step("Y", "a"), Dfa.DEAD, "and (Y,a)");
        Assert.that(partial.acceptsString("a") && !partial.acceptsString("ab") && !partial.acceptsString("b"),
            "DEAD is a trap: once there, never accept");

        // ── even number of a's ──
        Dfa even = Dfa.parse("states: E O\nalphabet: a b\nstart: E\naccept: E\nE a O\nE b E\nO a E\nO b O\n");
        Assert.that(even.acceptsString("") && even.acceptsString("aa") && even.acceptsString("bbaabb"),
            "even # of a's accepted");
        Assert.that(!even.acceptsString("a") && !even.acceptsString("baab" + "a"),
            "odd # of a's rejected");

        // ── the pigeonhole repeat: a DFA cannot count ──
        Dfa asbs = Dfa.parse("""
            states: A B DEAD
            alphabet: a b
            start: A
            accept: A B
            A a A
            A b B
            B a DEAD
            B b B
            DEAD a DEAD
            DEAD b DEAD
            """);
        Assert.that(asbs.acceptsString("aaab"), "a*b* accepts aaab -- it is NOT a^n b^n");
        Assert.that(asbs.acceptsString("abbb"), "a*b* accepts abbb");
        Assert.that(!asbs.acceptsString("ba"), "a*b* rejects ba");
        int[] r = asbs.repeatOn("a");
        Assert.equals(r[0], 0, "state A after 0 a's");
        Assert.equals(r[1], 1, "same state A after 1 a -- no memory of the count");

        // ── malformed DFAs ──
        Assert.that(rejects("alphabet: a\nstart: S\nS a S"), "missing states:");
        Assert.that(rejects("states: S\nalphabet: a\nstart: Z\nS a S"), "start not among states");
        Assert.that(rejects("states: S\nalphabet: a\nstart: S\naccept: Z\nS a S"), "accept not among states");
        Assert.that(rejects("states: S\nalphabet: a\nstart: S\nS a Q"), "transition to unknown state");

        Assert.summary();
    }

    private static boolean rejects(String src) {
        try { Dfa.parse(src); return false; }
        catch (IllegalArgumentException e) { return true; }
    }
}
