import java.util.List;

/** java -cp java/out LexerTest */
public final class LexerTest {
    public static void main(String[] args) {
        System.out.println("LexerTest");

        List<Lexer.Rule> rules = List.of(
            Lexer.Rule.of("KW_IF",  "if"),
            Lexer.Rule.of("ID",     "(i|f|x|y)(i|f|x|y)*"),
            Lexer.Rule.of("ASSIGN", "=="),
            Lexer.Rule.of("EQ",     "=")
        );

        // ── RULE 1: longest match wins ──
        Assert.equals(tok(rules, "iffy"), "ID \"iffy\" @0", "iffy -> ID (4 > 2), not KW_IF");
        Assert.equals(tok(rules, "ifx"),  "ID \"ifx\" @0",  "ifx -> ID");
        Assert.equals(tok(rules, "=="),   "ASSIGN \"==\" @0", "== -> ASSIGN (2 > 1), not EQ");
        Assert.equals(tok(rules, "ifff"), "ID \"ifff\" @0", "ifff -> ID");

        // ── RULE 2: tie -> earlier rule ──
        Assert.equals(tok(rules, "if"), "KW_IF \"if\" @0", "if -> KW_IF (rule 0), even though ID also matches len 2");
        Assert.equals(tok(rules, "="),  "EQ \"=\" @0",     "= -> EQ (ASSIGN needs two)");

        // swap the rule order: now ID beats KW_IF on the tie
        List<Lexer.Rule> swapped = List.of(
            Lexer.Rule.of("ID",    "(i|f|x|y)(i|f|x|y)*"),
            Lexer.Rule.of("KW_IF", "if")
        );
        Assert.equals(tok(swapped, "if"), "ID \"if\" @0", "with ID declared first, 'if' lexes as ID");

        // ── longestAccept: the last-accept mark ──
        Assert.equals(Lexer.longestAccept(rules.get(0).dfa(), "iffy", 0), 2, "KW_IF last-accepts at 2 in 'iffy'");
        Assert.equals(Lexer.longestAccept(rules.get(1).dfa(), "iffy", 0), 4, "ID last-accepts at 4 in 'iffy'");
        Assert.equals(Lexer.longestAccept(rules.get(2).dfa(), "iffy", 0), -1, "ASSIGN never accepts a prefix of 'iffy'");
        Assert.equals(Lexer.longestAccept(rules.get(1).dfa(), "x=y", 0), 1, "ID accepts only 'x' before '='");

        // ── full tokenization ──
        Assert.equals(toks(rules, "iffy==x=y"),
            "[ID \"iffy\" @0, ASSIGN \"==\" @4, ID \"x\" @6, EQ \"=\" @7, ID \"y\" @8]",
            "iffy==x=y tokenizes correctly");
        Assert.equals(toks(rules, "if==x"),
            "[KW_IF \"if\" @0, ASSIGN \"==\" @2, ID \"x\" @4]",
            "if==x: the 'if' is a keyword here (nothing longer matches)");
        Assert.equals(toks(rules, "ifif"),
            "[ID \"ifif\" @0]",
            "ifif is ONE identifier, not KW_IF KW_IF");

        // ── lexical error ──
        Assert.that(throwsOn(() -> Lexer.tokenize(rules, "if z")), "space is not in the alphabet -> error");
        Assert.equals(toks(rules, "==="), "[ASSIGN \"==\" @0, EQ \"=\" @2]",
            "=== -> ASSIGN (maximal munch) then EQ, no error");

        // ── empty input ──
        Assert.equals(Lexer.tokenize(rules, "").toString(), "[]", "empty input -> no tokens");

        Assert.summary();
    }

    static String tok(List<Lexer.Rule> rules, String s) {
        var t = Lexer.nextToken(rules, s, 0);
        return t == null ? "null" : t.toString();
    }
    static String toks(List<Lexer.Rule> rules, String s) {
        return Lexer.tokenize(rules, s).toString();
    }
    static boolean throwsOn(Runnable r) {
        try { r.run(); return false; } catch (RuntimeException e) { return true; }
    }
}
