import java.util.List;
import java.util.stream.Collectors;

/** java -cp java/out SpecLexerTest */
public final class SpecLexerTest {
    public static void main(String[] args) {
        System.out.println("SpecLexerTest");

        String spec = """
            KW_LET   let
            KW_IN    in
            ID       (e|i|l|n|t|x|y)(e|i|l|n|t|x|y)*
            NUM      (0|1|2|3|4|5|6|7|8|9)(0|1|2|3|4|5|6|7|8|9)*
            LE       <=
            LT       <
            ASSIGN   =
            WS       .
            %skip WS
            """;
        SpecLexer lex = SpecLexer.load(spec);

        // ── maximal munch + priority in one combined pass ──
        Assert.equals(kinds(lex, "let.x=12.in.x"), "KW_LET ID ASSIGN NUM KW_IN ID",
            "keywords win the tie; numbers and identifiers munch");
        Assert.equals(kinds(lex, "let.let.in"), "KW_LET KW_LET KW_IN",
            "each 'let' is the keyword, not an identifier");
        Assert.equals(kinds(lex, "x<=99.in.y"), "ID LE NUM KW_IN ID",
            "<= munches as one token, not < then =");
        Assert.equals(kinds(lex, "in<x"), "KW_IN LT ID", "< alone is LT");

        // ── an identifier that merely CONTAINS a keyword ──
        Assert.equals(kinds(lex, "inlet"), "ID", "'inlet' is one identifier (munch beats keyword)");
        Assert.equals(lexemes(lex, "inlet"), "inlet", "and its lexeme is the whole thing");

        // ── WS skipped, but positions still count the skipped characters ──
        var toks = lex.tokenize("x.in");
        Assert.equals(toks.size(), 2, "x.in -> 2 tokens (the '.' is skipped)");
        Assert.equals(toks.get(1).pos(), 2, "'in' is at position 2, not 1 -- the '.' occupied position 1");

        // ── lexical error with a position ──
        try {
            lex.tokenize("x=q");
            Assert.that(false, "should have thrown");
        } catch (SpecLexer.LexicalError e) {
            Assert.equals(e.pos, 2, "'q' is not in the alphabet -> error at position 2");
        }

        // ── "epsilon IS NOOOOOT A TOKEN" ──
        Assert.that(rejects("A ab\nB (a|b)*"), "a token pattern matching '' is rejected");
        Assert.that(rejects("A a?"), "a? matches the empty string -> rejected");
        Assert.that(rejects("A ()"), "() (epsilon) as a whole token -> rejected");
        Assert.that(!rejects("A a\nB ab"), "non-nullable patterns are fine");

        // ── empty input ──
        Assert.equals(lex.tokenize("").size(), 0, "empty input -> no tokens");

        // ── combined DFA is a single automaton ──
        Assert.that(lex.combinedDfaStates() > 0 && lex.combinedDfaStates() < 200,
            "the 8 patterns became one DFA of " + lex.combinedDfaStates() + " states");

        Assert.summary();
    }

    static String kinds(SpecLexer lex, String s) {
        return lex.tokenize(s).stream().map(SpecLexer.Token::kind).collect(Collectors.joining(" "));
    }
    static String lexemes(SpecLexer lex, String s) {
        return lex.tokenize(s).stream().map(SpecLexer.Token::lexeme).collect(Collectors.joining(" "));
    }
    static boolean rejects(String spec) {
        try { SpecLexer.load(spec); return false; }
        catch (IllegalArgumentException e) { return true; }
    }
}
