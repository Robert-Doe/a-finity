/** java -cp java/out RegexTest */
public final class RegexTest {
    public static void main(String[] args) {
        System.out.println("RegexTest");

        // ── parsing: the six node kinds ──
        Assert.equals(Regex.parse("a").tree(), "a", "a single Char");
        Assert.equals(Regex.parse("ab").tree(), "(cat a b)", "juxtaposition is Concat");
        Assert.equals(Regex.parse("a|b").tree(), "(alt a b)", "| is Union");
        Assert.equals(Regex.parse("a*").tree(), "(star a)", "* is Star");

        // ── precedence: * > concat > | ──
        Assert.equals(Regex.parse("ab*").tree(), "(cat a (star b))", "* binds tighter than concat");
        Assert.equals(Regex.parse("a|bc").tree(), "(alt a (cat b c))", "concat binds tighter than |");
        Assert.equals(Regex.parse("a|bc*").tree(), "(alt a (cat b (star c)))", "a | (b(c*))");
        Assert.equals(Regex.parse("(ab)*").tree(), "(star (cat a b))", "parens override: (ab)*");
        Assert.equals(Regex.parse("a|b|c").tree(), "(alt (alt a b) c)", "| is left-associative");

        // ── sugar desugars at parse time ──
        Assert.equals(Regex.parse("a+").tree(), "(cat a (star a))", "a+ = a a*");
        Assert.equals(Regex.parse("a?").tree(), "(alt a eps)", "a? = a | epsilon");

        // ── empty concatenation is epsilon ──
        Assert.equals(Regex.parse("()").tree(), "eps", "() denotes epsilon");
        Assert.equals(Regex.parse("(|a)").tree(), "(alt eps a)", "(|a) = epsilon | a");

        // ── malformed regexes are rejected ──
        Assert.that(rejects("("), "unbalanced (");
        Assert.that(rejects("a)"), "unbalanced )");
        Assert.that(rejects("*a"), "* with nothing to repeat");
        Assert.that(rejects("a**") == false, "a** is legal (star of a star)");

        // ── the meaning as a bounded Language (reuses Module 2) ──
        Assert.equals(Regex.parse("(a|b)a*").toLanguage(4).render(),
            "{ a, b, aa, ba, aaa, baa, aaaa, baaa }", "(a|b)a* up to length 4");
        Assert.equals(Regex.parse("a*").toLanguage(3).render(),
            "{ epsilon, a, aa, aaa }", "a* up to length 3");

        // ── exact matching (not bounded) ──
        Regex e = Regex.parse("(a|b)*abb");
        Assert.that(e.matches("abb"), "(a|b)*abb matches abb");
        Assert.that(e.matches("aabb"), "matches aabb");
        Assert.that(e.matches("babb"), "matches babb");
        Assert.that(e.matches("abababb"), "matches abababb");
        Assert.that(!e.matches("ab"), "does not match ab");
        Assert.that(!e.matches("abba"), "does not match abba");
        Assert.that(!e.matches(""), "does not match the empty string");

        Regex opt = Regex.parse("a?b?c?");
        Assert.that(opt.matches(""), "a?b?c? matches the empty string");
        Assert.that(opt.matches("abc"), "matches abc");
        Assert.that(opt.matches("ac"), "matches ac");
        Assert.that(!opt.matches("acb"), "does not match acb (order fixed)");

        Regex star = Regex.parse("(ab)*");
        Assert.that(star.matches("") && star.matches("ab") && star.matches("abab"), "(ab)* matches even repetitions");
        Assert.that(!star.matches("aba") && !star.matches("a"), "(ab)* rejects partials");

        // matching terminates even when a starred subexpression can match epsilon
        Assert.that(Regex.parse("(a*)*").matches("aaa"), "(a*)* still terminates and matches");
        Assert.that(Regex.parse("(a*)*").matches(""), "(a*)* matches empty");

        Assert.summary();
    }

    private static boolean rejects(String src) {
        try { Regex.parse(src); return false; }
        catch (IllegalArgumentException ex) { return true; }
    }
}
