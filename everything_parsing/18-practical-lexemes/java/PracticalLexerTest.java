import java.util.stream.Collectors;

/** java -cp java/out PracticalLexerTest */
public final class PracticalLexerTest {
    public static void main(String[] args) {
        System.out.println("PracticalLexerTest");

        // ── reserved-word trick ──
        Assert.equals(kinds("let x in y"), "KW_LET ID KW_IN ID", "keywords looked up, rest are ID");
        Assert.equals(kinds("lettuce letters"), "ID ID", "words that merely start with a keyword are ID");
        Assert.equals(kinds("if1 then2"), "ID ID", "if1 is an identifier, not KW_IF + 1");

        // ── numbers ──
        Assert.equals(one("0"), "INT \"0\"", "0");
        Assert.equals(one("42"), "INT \"42\"", "42");
        Assert.equals(one("3.14"), "FLOAT \"3.14\"", "3.14");
        Assert.equals(one("1e10"), "FLOAT \"1e10\"", "1e10");
        Assert.equals(one("2.5E-3"), "FLOAT \"2.5E-3\"", "2.5E-3");
        Assert.equals(one("0xDEAD"), "HEX \"0xDEAD\"", "0xDEAD");
        // "1e" is not an exponent -> INT 1, then ID e
        Assert.equals(kinds("1e"), "INT ID", "1e retracts: INT then ID");
        // "5.x": INT 5 (the '.' is not consumed because 'x' isn't a digit), then '.' is a
        // lexical error (no rule for it), then ID x
        var dotcase = PracticalLexer.lex("5.x");
        Assert.equals(kinds2(dotcase), "INT ID", "5.x -> INT then ID; the '.' is skipped");
        Assert.equals(dotcase.errors().size(), 1, "the stray '.' is one lexical error");

        // ── strings + escapes ──
        Assert.equals(text("\"hi\""), "hi", "plain string");
        Assert.equals(text("\"a\\nb\""), "a\nb", "\\n becomes a real newline");
        Assert.equals(text("\"tab\\there\""), "tab\there", "\\t");
        Assert.equals(text("\"quote \\\" done\""), "quote \" done", "\\\" is an embedded quote");
        Assert.equals(text("\"back \\\\ slash\""), "back \\ slash", "\\\\ is one backslash");
        var uni = PracticalLexer.lex("\"\\u0041\\u0042\"");
        Assert.equals(uni.tokens().get(0).text(), "AB", "\\u0041\\u0042 -> AB");

        var bad = PracticalLexer.lex("\"oops");
        Assert.equals(bad.errors().size(), 1, "unterminated string -> 1 error");
        Assert.that(bad.errors().get(0).contains("position 0"), "error names the opening position");
        Assert.equals(bad.tokens().get(0).kind(), "STRING?", "and yields a STRING? token");

        var badEsc = PracticalLexer.lex("\"a\\qb\"");
        Assert.that(badEsc.errors().stream().anyMatch(e -> e.contains("\\q")), "bad escape reported");

        // ── comments ──
        Assert.equals(kinds("a // ignore this\nb"), "ID ID", "// comment to end of line");
        Assert.equals(kinds("a /* c */ b"), "ID ID", "/* */ block comment");
        Assert.equals(kinds("a /* /* nested */ still */ b"), "ID ID",
            "NESTED /* */ fully consumed -- the depth counter");
        Assert.equals(kinds("a /* /* not closed */ b"), "ID",
            "one close for two opens -> comment runs to EOF, only 'a' survives");
        var unclosed = PracticalLexer.lex("x /* /*");
        Assert.that(unclosed.errors().stream().anyMatch(e -> e.contains("unterminated block comment")),
            "unterminated nested comment is reported");

        // ── the "not regular" point: a nested comment defeats a flat regex ──
        // /* a /* b */ c */  -- 2 opens, 2 closes; a regex /\*.*?\*/ closes at the first */
        var deep = PracticalLexer.lex("/* a /* b */ c */ done");
        Assert.equals(kinds2(deep), "ID", "the whole nested comment is skipped; only 'done' remains");
        Assert.equals(deep.tokens().get(0).text(), "done", "and it's 'done'");

        // ── operators + full program ──
        Assert.equals(kinds("let pi = 3.14 * r"), "KW_LET ID OP FLOAT OP ID", "a small program");

        Assert.summary();
    }

    static String kinds(String s) {
        return PracticalLexer.lex(s).tokens().stream()
            .map(PracticalLexer.Token::kind).collect(Collectors.joining(" "));
    }
    static String kinds2(PracticalLexer.Result r) {
        return r.tokens().stream().map(PracticalLexer.Token::kind).collect(Collectors.joining(" "));
    }
    static String one(String s) {
        var t = PracticalLexer.lex(s).tokens();
        return t.isEmpty() ? "(none)" : t.get(0).toString();
    }
    static String text(String s) {
        return PracticalLexer.lex(s).tokens().get(0).text();
    }
}
