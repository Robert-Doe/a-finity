import java.util.stream.Collectors;

/** java -cp java/out RecoveryTest */
public final class RecoveryTest {
    public static void main(String[] args) {
        System.out.println("RecoveryTest");

        SpecLexer lex = SpecLexer.load("""
            KW_LET   let
            KW_IN    in
            ID       (e|i|l|n|t|x|y)(e|i|l|n|t|x|y)*
            NUM      (0|1|2|3|4|5|6|7|8|9)(0|1|2|3|4|5|6|7|8|9)*
            ASSIGN   =
            WS       .
            %skip WS
            """);

        // ── PANIC_ONE: one ERROR per bad character ──
        var a = Recovery.tokenize(lex, "x@y#x", Recovery.Strategy.PANIC_ONE);
        Assert.equals(kinds(a), "ID ERROR ID ERROR ID", "x@y#x -> ID ERROR ID ERROR ID");
        Assert.equals(a.errors().size(), 2, "two lexical errors");
        Assert.equals(a.errors().get(0).from(), 1, "first error at position 1");
        Assert.equals(a.errors().get(1).from(), 3, "second error at position 3");

        // ── PANIC_ONE on a run: one ERROR per char ──
        var b = Recovery.tokenize(lex, "x@@@#y", Recovery.Strategy.PANIC_ONE);
        Assert.equals(b.errors().size(), 4, "@@@# -> 4 separate errors under PANIC_ONE");

        // ── PANIC_TO_SYNC on a run: one ERROR spanning it ──
        var c = Recovery.tokenize(lex, "x@@@#y", Recovery.Strategy.PANIC_TO_SYNC);
        Assert.equals(kinds(c), "ID ERROR ID", "x@@@#y -> ID ERROR ID under PANIC_TO_SYNC");
        Assert.equals(c.errors().size(), 1, "one error for the whole garbage run");
        Assert.equals(c.errors().get(0).from(), 1, "spans from position 1");
        Assert.equals(c.errors().get(0).to(), 5, "to position 5 (exclusive)");
        Assert.equals(c.tokens().get(1).lexeme(), "@@@#", "the ERROR token's text is the run");

        // ── all-garbage input ──
        var d = Recovery.tokenize(lex, "@@@", Recovery.Strategy.PANIC_TO_SYNC);
        Assert.equals(kinds(d), "ERROR", "@@@ -> one ERROR token");
        var e = Recovery.tokenize(lex, "@@@", Recovery.Strategy.PANIC_ONE);
        Assert.equals(e.errors().size(), 3, "@@@ -> 3 errors under PANIC_ONE");

        // ── error in the middle of good tokens ──
        var f = Recovery.tokenize(lex, "let.x=@=9", Recovery.Strategy.PANIC_ONE);
        Assert.equals(kinds(f), "KW_LET ID ASSIGN ERROR ASSIGN NUM",
            "recovery lets scanning continue past the '@'");
        Assert.equals(f.errors().size(), 1, "one error");

        // ── one scan finds ALL errors (vs Module 16 stopping at the first) ──
        var g = Recovery.tokenize(lex, "@x@y@", Recovery.Strategy.PANIC_ONE);
        Assert.equals(g.errors().size(), 3, "one recovering scan reports all 3 errors");
        Assert.that(throwsLexical(() -> lex.tokenize("@x@y@")), "Module 16's tokenize still stops at the first");

        // ── a clean input: no errors, no ERROR tokens ──
        var h = Recovery.tokenize(lex, "let.x=9", Recovery.Strategy.PANIC_TO_SYNC);
        Assert.equals(h.errors().size(), 0, "clean input -> no errors");
        Assert.that(h.tokens().stream().noneMatch(t -> t.kind().equals("ERROR")), "and no ERROR tokens");

        Assert.summary();
    }

    static String kinds(Recovery.Result r) {
        return r.tokens().stream().map(SpecLexer.Token::kind).collect(Collectors.joining(" "));
    }
    static boolean throwsLexical(Runnable run) {
        try { run.run(); return false; } catch (SpecLexer.LexicalError e) { return true; }
    }
}
