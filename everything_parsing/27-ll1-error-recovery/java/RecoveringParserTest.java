import java.util.Arrays;
import java.util.List;

/** java -cp java/out RecoveringParserTest */
public final class RecoveringParserTest {

    static List<String> toks(String s) { return Arrays.asList(s.split(" ")); }

    public static void main(String[] args) {
        System.out.println("RecoveringParserTest");

        Grammar g = Grammar.parse("""
            %start E
            E  -> T Ep
            Ep -> + T Ep | epsilon
            T  -> F Tp
            Tp -> * F Tp | epsilon
            F  -> ( E ) | id | num
            """);
        RecoveringParser p = new RecoveringParser(g);

        // ── a clean parse has zero errors ──
        RecoveringParser.Result ok = p.parse(toks("id + id * id"));
        Assert.that(ok.clean(), "id + id * id: accepted, no errors");

        // ── panic mode: discard a stray token ──
        RecoveringParser.Result r1 = p.parse(toks("id + * id"));
        Assert.that(r1.accepted(), "id + * id recovers to end of input");
        Assert.equals(r1.errors().size(), 1, "one error");
        Assert.that(r1.errors().get(0).message().contains("discarding '*'"), "the stray '*' is discarded");
        Assert.equals(r1.errors().get(0).pos(), 2, "error reported at position 2");

        // ── panic mode: sync on FOLLOW ──
        RecoveringParser.Result r2 = p.parse(toks("( id + )"));
        Assert.that(r2.accepted(), "( id + ) recovers");
        Assert.that(r2.errors().get(0).message().contains("skipping T"), "T is skipped (')' in FOLLOW(T))");

        // ── phrase-level: insert a missing terminal ──
        RecoveringParser.Result r3 = p.parse(toks("( id + id"));
        Assert.that(r3.accepted(), "( id + id recovers");
        Assert.that(r3.errors().get(0).message().contains("inserted missing ')'"), "the ')' is inserted");

        // ── leading operator ──
        RecoveringParser.Result r4 = p.parse(toks("+ id"));
        Assert.that(r4.accepted(), "+ id recovers");
        Assert.equals(r4.errors().get(0).pos(), 0, "error at position 0");

        // ── extra operand (missing operator) ──
        RecoveringParser.Result r5 = p.parse(toks("id id + id"));
        Assert.that(r5.accepted(), "id id + id recovers");
        Assert.equals(r5.errors().size(), 1, "one error: the second id has no operator before it");

        // ── recovery always terminates (guard never trips for these) ──
        RecoveringParser.Result r6 = p.parse(toks("* * * *"));
        Assert.that(r6 != null, "pathological input still returns");
        Assert.that(!r6.errors().isEmpty(), "and reports errors");

        // ── sync sets are FOLLOW sets ──
        Assert.that(p.syncSet("T").contains(")"), "FOLLOW(T) contains )");
        Assert.that(p.syncSet("T").contains("+"), "FOLLOW(T) contains +");

        Assert.summary();
    }
}
