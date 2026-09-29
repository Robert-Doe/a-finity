/** java -cp java/out SubsetTest */
public final class SubsetTest {
    public static void main(String[] args) {
        System.out.println("SubsetTest");

        // ── determinize a Thompson NFA and check the language ──
        Regex re = Regex.parse("(a|b)*abb");
        Nfa nfa = Thompson.build(re);
        Dfa dfa = Subset.determinize(nfa);

        Assert.equals(nfa.language(7), dfa.language(7),
            "L(NFA) == L(DFA), up to length 7");
        for (String w : re.toLanguage(7).list())
            if (!w.isEmpty())
                Assert.that(dfa.acceptsString(w), "regex-matched \"" + w + "\" is DFA-accepted");
        for (String w : dfa.language(7))
            if (!w.equals("epsilon"))
                Assert.that(re.matches(w), "DFA-accepted \"" + w + "\" is regex-matched");

        // ── the DFA start state is the epsilon-closure of the NFA start ──
        var leg = Subset.legend(nfa);
        Assert.equals(leg.get("D0"), nfa.epsilonClosure(java.util.Set.of(nfa.start)),
            "D0 is epsilonClosure({nfa.start})");

        // ── an accepting DFA state is exactly one whose set meets the NFA's accept set ──
        for (var e : leg.entrySet()) {
            boolean meets = !Nfa.intersect(e.getValue(), nfa.accept).isEmpty();
            Assert.equals(dfa.accept.contains(e.getKey()), meets,
                e.getKey() + " accepting iff its set contains an NFA accepting state");
        }

        // ── the exponential blow-up ──
        for (int k = 1; k <= 6; k++) {
            Nfa n = Subset.kthFromEndNfa(k);
            Dfa d = Subset.determinize(n);
            Assert.equals(n.states.size(), k + 1, "k=" + k + ": NFA has k+1 states");
            Assert.equals(d.states.size(), 1 << k, "k=" + k + ": DFA has exactly 2^k states");
        }

        // ── the k-th-from-end DFA decides correctly ──
        Dfa d3 = Subset.determinize(Subset.kthFromEndNfa(3));
        Assert.that(d3.acceptsString("baab"), "3rd from end of baab is 'a'");
        Assert.that(!d3.acceptsString("abaa"), "3rd from end of abaa is 'b'");
        Assert.that(d3.acceptsString("aaa"), "3rd from end of aaa is 'a'");
        Assert.that(!d3.acceptsString("ab"), "too short: cannot have a 3rd-from-end 'a'");

        // ── the empty NFA determinizes to a single non-accepting state ──
        Nfa none = Thompson.build(new Regex.Empty());
        Dfa noneD = Subset.determinize(none);
        Assert.that(noneD.accept.isEmpty(), "L(∅) DFA has no accepting state");
        Assert.that(!noneD.acceptsString("") && !noneD.acceptsString("a"), "and accepts nothing");

        // ── determinizing an already-deterministic NFA is (near) idempotent in language ──
        Nfa evenA = Nfa.parse("states: E O\nalphabet: a b\nstart: E\naccept: E\nE a O\nE b E\nO a E\nO b O\n");
        Dfa evenD = Subset.determinize(evenA);
        Assert.equals(evenA.language(6), evenD.language(6), "even-a: NFA and its DFA agree");

        Assert.summary();
    }
}
