/** java -cp java/out LanguageTest */
public final class LanguageTest {
    public static void main(String[] args) {
        System.out.println("LanguageTest");

        // ── the empty string vs the empty language ──
        Assert.equals(Language.EMPTY.size(), 0, "{} has no strings");
        Assert.equals(Language.EPSILON.size(), 1, "{ epsilon } has exactly one string");
        Assert.that(Language.EPSILON.contains(""), "{ epsilon } contains the empty string");
        Assert.that(!Language.EMPTY.contains(""), "{} does not contain the empty string");

        // ── canonical order: shorter first, then lexicographic ──
        Assert.equals(Language.of("bb", "a", "ab", "").render(),
            "{ epsilon, a, ab, bb }", "render is (length, lexicographic)");

        // ── union ──
        Assert.equals(Language.of("a", "b").union(Language.of("b", "c")).render(),
            "{ a, b, c }", "union deduplicates");

        // ── concat ──
        Assert.equals(Language.of("a", "b").concat(Language.of("x", "y")).render(),
            "{ ax, ay, bx, by }", "concat is the glued cartesian product");
        Assert.equals(Language.EMPTY.concat(Language.of("a")).render(),
            "{ }", "empty language annihilates under concat");
        Assert.equals(Language.EPSILON.concat(Language.of("a", "b")).render(),
            "{ a, b }", "{ epsilon } is the identity under concat");

        // ── power ──
        Assert.equals(Language.of("a", "b").power(0).render(), "{ epsilon }", "L^0 = { epsilon }");
        Assert.equals(Language.of("a", "b").power(1).render(), "{ a, b }", "L^1 = L");
        Assert.equals(Language.of("a", "b").power(2).render(),
            "{ aa, ab, ba, bb }", "L^2");
        Assert.equals(Language.of("ab").power(3).render(), "{ ababab }", "{ab}^3");

        // ── bounded Kleene star ──
        Assert.equals(Language.of("a").star(4).render(),
            "{ epsilon, a, aa, aaa, aaaa }", "{a}* up to length 4");
        Assert.equals(Language.EMPTY.star(5).render(), "{ epsilon }", "empty language star = { epsilon }");
        // star of a language that contains epsilon must still terminate
        Assert.equals(Language.of("", "a").star(3).render(),
            "{ epsilon, a, aa, aaa }", "star terminates when L contains epsilon");

        // ── Sigma* ──
        Assert.equals(Language.sigmaStar(setOf('a', 'b'), 2).render(),
            "{ epsilon, a, b, aa, ab, ba, bb }", "Sigma* over {a,b} up to length 2");
        Assert.equals(Language.sigmaStar(setOf('a', 'b'), 3).size(), 15, "|Sigma*<=3| = 1+2+4+8");

        // ── a regular expression, rebuilt from operations ──
        Language re = Language.of("a").union(Language.of("b"))
                        .concat(Language.of("a").star(3))
                        .intersect(Language.sigmaStar(setOf('a', 'b'), 4));
        Assert.equals(re.render(),
            "{ a, b, aa, ba, aaa, baa, aaaa, baaa }", "(a|b)a* up to length 4");
        Assert.that(re.contains("b") && re.contains("ba") && re.contains("baaa"),
            "(a|b)a* accepts b, ba, baaa");
        Assert.that(!re.contains("ab") && !re.contains(""),
            "(a|b)a* rejects ab and the empty string");

        // ── intersect / minus ──
        Assert.equals(Language.of("a", "b", "c").intersect(Language.of("b", "c", "d")).render(),
            "{ b, c }", "intersection");
        Assert.equals(Language.of("a", "b", "c").minus(Language.of("b")).render(),
            "{ a, c }", "difference");

        Assert.summary();
    }

    private static java.util.Set<Character> setOf(char... cs) {
        java.util.Set<Character> s = new java.util.LinkedHashSet<>();
        for (char c : cs) s.add(c);
        return s;
    }
}
