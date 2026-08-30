import java.util.List;

/** java -cp java/out EbnfTest */
public final class EbnfTest {
    public static void main(String[] args) {
        System.out.println("EbnfTest");

        // ── parsing EBNF ──
        Ebnf.Grammar g = Ebnf.parse("a = 'x' 'y' ;");
        Assert.equals(Ebnf.isoRule(g.rules.get("a")), "'x' 'y'", "sequence of two terminals");
        Assert.equals(Ebnf.isoRule(Ebnf.parse("a = 'x' | 'y' ;").rules.get("a")), "'x' | 'y'", "alternation");
        Assert.equals(Ebnf.isoRule(Ebnf.parse("a = 'x'? ;").rules.get("a")), "[ 'x' ]", "optional -> [ ]");
        Assert.equals(Ebnf.isoRule(Ebnf.parse("a = 'x'* ;").rules.get("a")), "{ 'x' }", "star -> { }");
        Assert.equals(Ebnf.isoRule(Ebnf.parse("a = 'x'+ ;").rules.get("a")), "'x' { 'x' }", "plus -> x { x }");
        Assert.equals(Ebnf.isoRule(Ebnf.parse("a = ( 'x' | 'y' ) 'z' ;").rules.get("a")),
            "( 'x' | 'y' ) 'z'", "grouping");

        // ── desugaring the operators ──
        String bnf = Ebnf.toBnfText(Ebnf.parse("a = 'x'? ; "));
        Assert.that(bnf.contains("a -> opt_1") && bnf.contains("opt_1 -> x | epsilon"),
            "x? becomes  Opt -> x | epsilon");

        bnf = Ebnf.toBnfText(Ebnf.parse("a = 'x'* ;"));
        Assert.that(bnf.contains("a -> rep_1") && bnf.contains("rep_1 -> x rep_1 | epsilon"),
            "x* becomes  Rep -> x Rep | epsilon");

        bnf = Ebnf.toBnfText(Ebnf.parse("a = 'x'+ ;"));
        Assert.that(bnf.contains("plus_1 -> x plus_1 | x"), "x+ becomes  Plus -> x Plus | x");

        // ── round trip: EBNF and desugared BNF describe the same language ──
        Ebnf.Grammar expr = Ebnf.parse("""
            expr   = term (('+' | '-') term)* ;
            term   = factor (('*' | '/') factor)* ;
            factor = '(' expr ')' | NUM ;
            """);
        Grammar be = Grammar.parse(Ebnf.toBnfText(expr));
        var lang = Derivation.enumerate(be, 7);
        boolean allAgree = true;
        for (String w : lang) {
            List<String> toks = w.equals("epsilon") ? List.of() : List.of(w.split(" "));
            if (!Ebnf.accepts(expr, toks)) allAgree = false;
        }
        Assert.that(allAgree, "every string L(BNF) generates is accepted by the EBNF matcher");
        Assert.that(lang.size() > 50, "the enumeration is non-trivial (" + lang.size() + " strings)");

        // ── specific strings ──
        Assert.that(Ebnf.accepts(expr, List.of("NUM")), "NUM");
        Assert.that(Ebnf.accepts(expr, List.of("NUM", "+", "NUM", "*", "NUM")), "NUM + NUM * NUM");
        Assert.that(Ebnf.accepts(expr, List.of("(", "NUM", "+", "NUM", ")")), "( NUM + NUM )");
        Assert.that(!Ebnf.accepts(expr, List.of("NUM", "+")), "NUM + is rejected");
        Assert.that(!Ebnf.accepts(expr, List.of("+", "NUM")), "+ NUM is rejected");
        Assert.that(!Ebnf.accepts(expr, List.of("(", "NUM")), "( NUM is rejected (unbalanced)");

        // ── signed: ? and + together ──
        Ebnf.Grammar num = Ebnf.parse("number = '-'? digit+ ; digit = '0' | '1' | '2' ;");
        Assert.that(Ebnf.accepts(num, List.of("0")), "0");
        Assert.that(Ebnf.accepts(num, List.of("-", "1", "2")), "-12");
        Assert.that(Ebnf.accepts(num, List.of("1", "0", "1", "2")), "1012");
        Assert.that(!Ebnf.accepts(num, List.of("-")), "- alone is rejected (digit+ needs one)");
        Assert.that(!Ebnf.accepts(num, List.of()), "empty is rejected");
        Assert.that(!Ebnf.accepts(num, List.of("-", "-", "1")), "-- is rejected (only one optional -)");

        // ── malformed EBNF ──
        Assert.that(rejects("a = 'x'"), "missing ';'");
        Assert.that(rejects("a 'x' ;"), "missing '='");
        Assert.that(rejects("a = ( 'x' ;"), "unbalanced (");

        Assert.summary();
    }

    private static boolean rejects(String src) {
        try { Ebnf.parse(src); return false; }
        catch (IllegalArgumentException e) { return true; }
    }
}
