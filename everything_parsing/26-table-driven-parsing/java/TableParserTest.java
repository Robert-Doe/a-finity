import java.util.Arrays;
import java.util.List;

/** java -cp java/out TableParserTest */
public final class TableParserTest {

    static List<String> toks(String s) { return Arrays.asList(s.split(" ")); }

    public static void main(String[] args) {
        System.out.println("TableParserTest");

        Grammar g = Grammar.parse("""
            %start E
            E  -> T Ep
            Ep -> + T Ep | epsilon
            T  -> F Tp
            Tp -> * F Tp | epsilon
            F  -> ( E ) | id | num
            """);
        TableParser p = new TableParser(g);

        // ── accepts ──
        TableParser.Result r = p.parse(toks("id + id * id"));
        Assert.that(r.ok(), "id + id * id accepted");
        Assert.equals(r.productions().get(0), "E -> T Ep", "first production is the start rule");
        Assert.equals(r.productions().size(), 11, "11 productions applied");

        // ── the production sequence IS a leftmost derivation of the input ──
        Assert.equals(p.replay(r.productions()), toks("id + id * id"),
            "replaying the productions leftmost yields exactly the input");

        // ── tree ──
        Assert.equals(r.tree().symbol, "E", "root is E");
        Assert.equals(r.tree().kids.size(), 2, "E has two children (T, Ep)");

        // ── nested parens ──
        Assert.that(p.parse(toks("( id + num ) * id")).ok(), "( id + num ) * id accepted");
        Assert.that(p.parse(toks("num")).ok(), "num accepted");

        // ── rejects, with position ──
        TableParser.Result e1 = p.parse(toks("id +"));
        Assert.that(!e1.ok(), "id + rejected (trailing operator)");
        Assert.that(e1.error().contains("position 2"), "error at position 2");

        TableParser.Result e2 = p.parse(toks("id id"));
        Assert.that(!e2.ok(), "id id rejected");
        Assert.that(e2.error().contains("Tp"), "blank cell for Tp on 'id'");

        TableParser.Result e3 = p.parse(toks("( id + id"));
        Assert.that(!e3.ok(), "( id + id rejected (missing close paren)");
        Assert.that(e3.error().contains(")"), "error mentions the expected ')'");

        // ── partial productions still recorded before the error ──
        Assert.that(e1.productions().contains("Ep -> + T Ep"),
            "productions applied before the error are kept");

        Assert.summary();
    }
}
