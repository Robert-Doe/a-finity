import java.util.List;

/** java -cp java/out RpnTest */
public final class RpnTest {
    public static void main(String[] args) {
        System.out.println("RpnTest");

        // ── tokenize: columns are 1-based, whitespace is skipped ──
        List<Rpn.Token> t = Rpn.tokenize("  3   40 +");
        Assert.equals(t.size(), 3, "three tokens");
        Assert.equals(t.get(0).text(), "3", "first lexeme");
        Assert.equals(t.get(0).col(), 3, "first lexeme starts at col 3");
        Assert.equals(t.get(1).text(), "40", "second lexeme");
        Assert.equals(t.get(1).col(), 7, "second lexeme starts at col 7");
        Assert.equals(t.get(2).col(), 10, "operator starts at col 10");

        // ── classification ──
        Assert.that(Rpn.isInteger("0") && Rpn.isInteger("1234"), "digits are integers");
        Assert.that(!Rpn.isInteger("") && !Rpn.isInteger("-5") && !Rpn.isInteger("1a"),
                "empty, signed, and mixed are not integers");
        Assert.that(Rpn.isOperator("+") && Rpn.isOperator("/"), "+ and / are operators");
        Assert.that(!Rpn.isOperator("++") && !Rpn.isOperator("%"), "++ and % are not operators");

        // ── syntax: well-formed cases ──
        Assert.that(syntaxOk("3 4 +"), "3 4 + is well formed");
        Assert.that(syntaxOk("10 2 - 3 *"), "10 2 - 3 * is well formed");
        Assert.that(syntaxOk("12 0 /"), "12 0 / is well formed (syntax cannot see the zero)");

        // ── syntax: the four failure shapes ──
        Assert.equals(syntaxMsg(""), "empty expression", "empty input");
        Assert.equals(syntaxMsg("3 x +"), "unknown token 'x'", "unknown lexeme");
        Assert.equals(syntaxMsg("3 4 + +"),
                "operator '+' needs 2 operands, stack has 1", "operand underflow");
        Assert.equals(syntaxMsg("3 4"),
                "2 values left on stack, expected 1", "leftover operands");

        // ── the column of each syntax error ──
        Assert.equals(syntaxCol("3 x +"), 3, "unknown token column");
        Assert.equals(syntaxCol("3 4 + +"), 7, "underflow points at the offending operator");
        Assert.equals(syntaxCol("3 4"), 0, "leftover-operands error has no single column");

        // ── semantics: value and the one real error ──
        Assert.equals(eval("3 4 +"), 7L, "3 + 4");
        Assert.equals(eval("10 2 - 3 *"), 24L, "(10 - 2) * 3");
        Assert.equals(eval("7 2 /"), 3L, "7 / 2 truncates toward zero");
        Assert.equals(eval("0 5 - 2 /"), -2L, "(-5) / 2 truncates toward zero, not down");

        Rpn.SemanticResult div0 = Rpn.evaluate(Rpn.tokenize("12 0 /"));
        Assert.that(!div0.ok(), "12 0 / fails semantically");
        Assert.equals(div0.message(), "division by zero", "the semantic error message");
        Assert.equals(div0.col(), 6, "the '/' is at column 6");

        // ── the headline claim: syntax PASS and semantics FAIL on the same string ──
        var toks = Rpn.tokenize("12 0 /");
        Assert.that(Rpn.checkSyntax(toks).ok() && !Rpn.evaluate(toks).ok(),
                "12 0 / : syntactically valid, semantically invalid");

        Assert.summary();
    }

    private static boolean syntaxOk(String s)  { return Rpn.checkSyntax(Rpn.tokenize(s)).ok(); }
    private static String  syntaxMsg(String s) { return Rpn.checkSyntax(Rpn.tokenize(s)).message(); }
    private static int     syntaxCol(String s) { return Rpn.checkSyntax(Rpn.tokenize(s)).col(); }
    private static long    eval(String s)      { return Rpn.evaluate(Rpn.tokenize(s)).value(); }
}
