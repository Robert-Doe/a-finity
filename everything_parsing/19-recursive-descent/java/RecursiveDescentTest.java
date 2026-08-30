import java.util.List;

/** java -cp java/out RecursiveDescentTest */
public final class RecursiveDescentTest {

    public static void main(String[] args) {
        System.out.println("RecursiveDescentTest");

        // ── the tiny lexer ──
        Assert.equals(Lexer.lex("1 + 2").size(), 4, "1 + 2 -> num + num EOF");
        Assert.equals(Lexer.lex("(a)").get(1).kind(), "id", "middle token of (a) is id");
        Assert.equals(Lexer.lex("12").get(0).text(), "12", "digits run together into one num");

        // ── the claim: productions fired ARE a leftmost derivation ──
        RecursiveDescent.Result r = RecursiveDescent.parse("1 + 2 * 3");
        Assert.equals(r.rules().get(0), "E -> T E'", "first production is the start rule");
        Assert.equals(r.rules().size(), 11, "1 + 2 * 3 fires 11 productions");
        Assert.equals(Derivation.sentence(r.rules()), "num + num * num",
            "replaying the fired productions as a leftmost derivation yields the token kinds");
        List<Derivation.Step> steps = Derivation.replay(r.rules());
        Assert.equals(steps.size(), r.rules().size() + 1,
            "a derivation has one more sentential form than it has steps");

        // ── replay throws if the production list is not leftmost ──
        boolean threw = false;
        try { Derivation.replay(List.of("E -> T E'", "E' -> epsilon")); }
        catch (IllegalStateException e) { threw = true; }
        Assert.that(threw, "replay rejects a production that does not expand the leftmost nonterminal");

        // ── the tree ──
        Assert.equals(r.tree().symbol, "E", "root is E");
        Assert.equals(r.tree().kids.size(), 2, "E has exactly two children: T and E'");
        Assert.equals(String.join(" ", RecursiveDescent.terminalYield(r.tree())), "num + num * num",
            "the tree's terminal yield is the input");

        // ── precedence: * binds tighter than + ──
        // in  1 + 2 * 3  the '*' sits under the SECOND T (inside E'), not at the top
        RecursiveDescent.Result p = RecursiveDescent.parse("1 + 2 * 3");
        Assert.that(p.rules().contains("T' -> * F T'"), "the * is consumed by a T' (a factor-level rule)");

        // ── associativity: right-recursive E' means a+b+c leans right ──
        RecursiveDescent.Result assoc = RecursiveDescent.parse("a + b + c");
        long plusRules = assoc.rules().stream().filter(x -> x.equals("E' -> + T E'")).count();
        Assert.equals(plusRules, 2L, "a + b + c uses E' -> + T E' twice (nested to the right)");

        // ── parens override precedence ──
        RecursiveDescent.Result par = RecursiveDescent.parse("(1 + 2) * 3");
        Assert.that(par.rules().contains("F -> ( E )"), "parenthesised sub-expression uses F -> ( E )");

        // ── syntax errors, with position ──
        Assert.equals(errPos("1 +"), 3, "'1 +' : error at position 3 (end of input, expecting a factor)");
        Assert.that(errMsg("1 +").contains("factor"), "'1 +' : message mentions a missing factor");
        Assert.equals(errPos("(1 + 2"), 6, "'(1 + 2' : error at position 6, expecting ')'");
        Assert.that(errMsg("(1 + 2").contains(")"), "'(1 + 2' : message mentions ')'");
        Assert.equals(errPos("1 2"), 2, "'1 2' : trailing token at position 2 (expected EOF)");
        Assert.equals(errPos(""), 0, "empty input : error at position 0");

        // ── a well-formed single factor ──
        RecursiveDescent.Result one = RecursiveDescent.parse("a");
        Assert.equals(one.rules().toString(),
            "[E -> T E', T -> F T', F -> id, T' -> epsilon, E' -> epsilon]",
            "'a' : five productions, both primes go to epsilon");

        Assert.summary();
    }

    private static int errPos(String src) {
        try { RecursiveDescent.parse(src); return -1; }
        catch (RecursiveDescent.SyntaxError e) { return e.pos; }
    }
    private static String errMsg(String src) {
        try { RecursiveDescent.parse(src); return ""; }
        catch (RecursiveDescent.SyntaxError e) { return e.getMessage(); }
    }
}
