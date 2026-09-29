import java.util.Arrays;
import java.util.List;

/** java -cp java/out ShiftReduceTest */
public final class ShiftReduceTest {

    static Grammar expr() {
        return Grammar.parse("""
            %start E
            E -> E + T | T
            T -> T * F | F
            F -> ( E ) | id
            """);
    }
    static List<String> toks(String s) { return Arrays.asList(s.split(" ")); }

    public static void main(String[] args) {
        System.out.println("ShiftReduceTest");

        Grammar g = expr();
        ShiftReduce sr = new ShiftReduce(g);

        // ── a full parse ──
        ShiftReduce.Result r = sr.parse(toks("id + id * id"));
        Assert.that(r.ok(), "id + id * id parses bottom-up");
        Assert.that(!r.conflict(), "unambiguous grammar: no shift-reduce conflict");
        Assert.equals(r.steps().get(r.steps().size() - 1).action(), "ACCEPT", "ends in ACCEPT");

        // ── reductions reversed = a rightmost derivation of the input ──
        List<String> form = sr.rightmostDerivation(r.reductions());
        Assert.equals(form, toks("id + id * id"),
            "reversing the reductions yields a rightmost derivation ending at the input");

        // ── the LAST reduction is the FIRST step of the rightmost derivation: E -> E + T ──
        Assert.equals(ShiftReduce.compact(r.reductions().get(r.reductions().size() - 1)), "E -> E + T",
            "the final reduction corresponds to the top of the rightmost derivation");

        // ── * binds tighter: T * F is reduced before E + T ──
        List<String> red = new java.util.ArrayList<>();
        for (Grammar.Production p : r.reductions()) red.add(ShiftReduce.compact(p));
        Assert.that(red.indexOf("T -> T * F") < red.indexOf("E -> E + T"),
            "T -> T * F is reduced before E -> E + T (precedence shows up in reduction order)");

        // ── parens ──
        ShiftReduce.Result r2 = sr.parse(toks("( id + id ) * id"));
        Assert.that(r2.ok(), "( id + id ) * id parses");
        Assert.equals(sr.rightmostDerivation(r2.reductions()), toks("( id + id ) * id"), "and reverses correctly");
        boolean sawParenHandle = r2.steps().stream().anyMatch(s -> s.action().contains("F -> ( E )"));
        Assert.that(sawParenHandle, "( E ) is reduced as a handle");

        // ── single id ──
        ShiftReduce.Result r3 = sr.parse(toks("id"));
        Assert.equals(r3.reductions().size(), 3, "id needs 3 reductions: F->id, T->F, E->T");

        // ── ambiguous grammar: shift-reduce conflict ──
        Grammar amb = Grammar.parse("""
            %start E
            E -> E + E | id
            """);
        ShiftReduce sr2 = new ShiftReduce(amb);
        ShiftReduce.Result ra = sr2.parse(toks("id + id + id"));
        Assert.that(ra.ok(), "the ambiguous grammar still finds A parse");
        Assert.that(ra.conflict(), "and reports a shift-reduce conflict");
        Assert.that(ra.conflictNote().contains("E + E"), "the conflict state stack is E + E");
        Assert.that(ra.conflictNote().contains("shift") && ra.conflictNote().contains("reduce"),
            "both shift and reduce lead to accept");

        Assert.summary();
    }
}
