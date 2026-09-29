import java.util.List;

/** java -cp java/out LL1TableTest */
public final class LL1TableTest {

    static LL1Table load(String g) { return new LL1Table(Grammar.parse(g)); }

    public static void main(String[] args) {
        System.out.println("LL1TableTest");

        LL1Table e = load("""
            %start E
            E  -> T Ep
            Ep -> + T Ep | epsilon
            T  -> F Tp
            Tp -> * F Tp | epsilon
            F  -> ( E ) | id | num
            """);

        // ── the table is single-valued ──
        Assert.that(e.isLL1(), "the expression grammar's table has no conflicts");
        Assert.equals(e.conflicts().size(), 0, "zero conflicted cells");

        // ── specific cells ──
        Assert.equals(e.cell("E", "id").get(0).index(), 0, "M[E][id] = production 0 (E -> T Ep)");
        Assert.equals(e.cell("E", "(").get(0).index(), 0, "M[E][(] = 0");
        Assert.equals(e.cell("Ep", "+").get(0).index(), 1, "M[Ep][+] = 1 (Ep -> + T Ep)");
        Assert.equals(e.cell("Ep", ")").get(0).index(), 2, "M[Ep][)] = 2 (Ep -> epsilon), from FOLLOW");
        Assert.equals(e.cell("Ep", "$").get(0).index(), 2, "M[Ep][$] = 2");
        Assert.that(e.cell("E", "+").isEmpty(), "M[E][+] is blank -> syntax error on '+' where E is expected");
        Assert.that(e.cell("Tp", "*").get(0).index() == 4, "M[Tp][*] = 4 (Tp -> * F Tp)");

        // ── columns are terminals + $ ──
        Assert.that(e.columns.contains("$"), "$ is a column");
        Assert.that(!e.columns.contains("E"), "nonterminals are not columns");

        // ── dangling else: one conflicted cell ──
        LL1Table de = load("""
            %start S
            S  -> other | if b then S Sp
            Sp -> else S | epsilon
            """);
        Assert.that(!de.isLL1(), "left-factored dangling else is still not LL(1)");
        Assert.equals(de.conflicts().size(), 1, "exactly one conflicted cell");
        LL1Table.Conflict c = de.conflicts().get(0);
        Assert.equals(c.nt(), "Sp", "conflict is in row Sp");
        Assert.equals(c.token(), "else", "conflict is in column else");
        Assert.equals(c.ps().size(), 2, "two productions compete: Sp -> else S and Sp -> epsilon");

        // ── nullable list rule ──
        LL1Table st = load("""
            %start P
            P -> L
            L -> S L | epsilon
            S -> id assign E semi
            E -> id | num
            """);
        Assert.that(st.isLL1(), "statement grammar is LL(1)");
        Assert.equals(st.cell("L", "$").get(0).index(), 2, "M[L][$] = L -> epsilon (from FOLLOW(L))");
        Assert.equals(st.cell("L", "id").get(0).index(), 1, "M[L][id] = L -> S L");

        Assert.summary();
    }
}
