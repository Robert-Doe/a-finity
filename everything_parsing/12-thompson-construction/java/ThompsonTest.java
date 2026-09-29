/** java -cp java/out ThompsonTest */
public final class ThompsonTest {
    public static void main(String[] args) {
        System.out.println("ThompsonTest");

        // ── gadget sizes ──
        Assert.equals(Thompson.stateCount(Regex.parse("a")), 2, "Char -> 2 states");
        Assert.equals(Thompson.stateCount(Regex.parse("ab")), 4, "Concat adds 0 states");
        Assert.equals(Thompson.stateCount(Regex.parse("a|b")), 6, "Union adds 2 states");
        Assert.equals(Thompson.stateCount(Regex.parse("a*")), 4, "Star adds 2 states");
        Assert.equals(Thompson.stateCount(Regex.parse("()")), 2, "Epsilon -> 2 states");

        // ── the bound: states <= 2 * nodes, always ──
        for (String r : new String[]{"a", "ab", "a|b", "a*", "(a|b)*", "(a|b)*abb",
                "a(b|c)*d", "((a|b)(c|d))*", "a**", "(a|b|c|d)*"}) {
            Regex x = Regex.parse(r);
            int states = Thompson.stateCount(x), nodes = Thompson.nodeCount(x);
            Assert.that(states <= 2 * nodes, "/" + r + "/: " + states + " states <= 2*" + nodes + " nodes");
            int chars = r.replaceAll("[()]", "").length();
            Assert.that(states <= 2 * chars, "/" + r + "/: " + states + " states <= 2*" + chars + " chars");
        }

        // ── the built NFA matches the regex ──
        Regex re = Regex.parse("(a|b)*abb");
        Nfa nfa = Thompson.build(re);
        for (String w : nfa.language(7)) {
            if (w.equals("epsilon")) continue;
            Assert.that(re.matches(w), "Thompson NFA accepts \"" + w + "\" and so does the regex");
        }
        for (String w : re.toLanguage(7).list())
            if (!w.isEmpty())
                Assert.that(nfa.acceptsString(w), "regex matches \"" + w + "\" and so does the Thompson NFA");

        // ── specific runs ──
        Nfa abc = Thompson.build(Regex.parse("a(b|c)"));
        Assert.that(abc.acceptsString("ab") && abc.acceptsString("ac"), "a(b|c) accepts ab, ac");
        Assert.that(!abc.acceptsString("a") && !abc.acceptsString("abc"), "a(b|c) rejects a, abc");

        Nfa star = Thompson.build(Regex.parse("(ab)*"));
        Assert.that(star.acceptsString("") && star.acceptsString("ab") && star.acceptsString("abab"),
            "(ab)* accepts even repetitions");
        Assert.that(!star.acceptsString("a") && !star.acceptsString("aba"), "(ab)* rejects partials");

        // ── Empty accepts nothing; Epsilon accepts only "" ──
        Nfa empty = Thompson.build(new Regex.Empty());
        Assert.that(!empty.acceptsString("") && !empty.acceptsString("a"), "∅ accepts nothing");
        Nfa eps = Thompson.build(new Regex.Epsilon());
        Assert.that(eps.acceptsString("") && !eps.acceptsString("a"), "ε accepts only the empty string");

        // ── nested stars still build and match ──
        Nfa nested = Thompson.build(Regex.parse("(a*)*"));
        Assert.that(nested.acceptsString("") && nested.acceptsString("aaa"), "(a*)* accepts a-strings");
        Assert.that(!nested.acceptsString("b"), "(a*)* rejects b");

        // ── the Thompson NFA -> epsilon-free NFA preserves the language ──
        Nfa free = nfa.removeEpsilon();
        Assert.equals(nfa.language(6), free.language(6), "removeEpsilon preserves the language");
        Assert.that(!free.hasEpsilon(), "and leaves no epsilon transitions");

        Assert.summary();
    }
}
