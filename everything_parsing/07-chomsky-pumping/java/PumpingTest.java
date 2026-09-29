/** java -cp java/out PumpingTest */
public final class PumpingTest {
    public static void main(String[] args) {
        System.out.println("PumpingTest");

        // ── grammar classification ──
        Assert.equals(GrammarClass.classify(Grammar.parse("S -> a S | b")),
            GrammarClass.Kind.RIGHT_LINEAR, "S -> a S | b is right-linear");
        Assert.equals(GrammarClass.classify(Grammar.parse("S -> S a | b")),
            GrammarClass.Kind.LEFT_LINEAR, "S -> S a | b is left-linear");
        Assert.equals(GrammarClass.classify(Grammar.parse("S -> a S b | epsilon")),
            GrammarClass.Kind.CONTEXT_FREE, "S -> a S b | epsilon is context-free (nonterminal in the middle)");
        Assert.equals(GrammarClass.classify(Grammar.parse("E -> E + E | id")),
            GrammarClass.Kind.CONTEXT_FREE, "E -> E + E | id is context-free (two nonterminals)");
        Assert.that(GrammarClass.isRegular(Grammar.parse("S -> a S | b S | b")),
            "a grammar with only A -> wB / A -> w is regular");
        Assert.that(!GrammarClass.isRegular(Grammar.parse("S -> a S b | epsilon")),
            "a^n b^n grammar is not regular");

        // ── the language predicates ──
        Assert.that(Pumping.inAnBn("") && Pumping.inAnBn("ab") && Pumping.inAnBn("aabb"), "a^n b^n members");
        Assert.that(!Pumping.inAnBn("aab") && !Pumping.inAnBn("ba") && !Pumping.inAnBn("abab"), "a^n b^n non-members");
        Assert.that(Pumping.inAnBnCn("") && Pumping.inAnBnCn("abc") && Pumping.inAnBnCn("aabbcc"), "a^n b^n c^n members");
        Assert.that(!Pumping.inAnBnCn("aabbc") && !Pumping.inAnBnCn("abcabc"), "a^n b^n c^n non-members");

        // ── regular pumping lemma: NO valid pumping length for a^n b^n ──
        for (int p = 1; p <= 8; p++) {
            var w = Pumping.regularRefutation(p);
            Assert.that(w.allEscaped(),
                "p=" + p + ": every x y z split of a^p b^p escapes under pumping");
            Assert.that(Pumping.inAnBn(w.s()), "the chosen string a^p b^p is in the language");
            Assert.that(!Pumping.inAnBn(w.pumped()),
                "the pumped representative is not in a^n b^n");
            Assert.equals(w.decompositionsTried(), p * (p + 1) / 2,
                "checked all p(p+1)/2 valid splits");
        }

        // ── context-free pumping lemma: NO valid pumping length for a^n b^n c^n ──
        for (int p = 1; p <= 4; p++) {
            var w = Pumping.cflRefutation(p);
            Assert.that(w.allEscaped(),
                "p=" + p + ": every u v w x y split of a^p b^p c^p escapes under pumping");
            Assert.that(Pumping.inAnBnCn(w.s()), "the chosen string a^p b^p c^p is in the language");
            Assert.that(!Pumping.inAnBnCn(w.pumped()), "the pumped representative is not in a^n b^n c^n");
        }

        Assert.summary();
    }
}
