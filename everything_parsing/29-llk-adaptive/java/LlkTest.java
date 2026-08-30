import java.util.ArrayList;
import java.util.List;

/** java -cp java/out LlkTest */
public final class LlkTest {

    static LeftRec.G lg(String g) { return LeftRec.G.from(Grammar.parse(g)); }
    static Grammar gr(String g) { return Grammar.parse(g); }

    public static void main(String[] args) {
        System.out.println("LlkTest");

        // ── FIRST_1 == ordinary FIRST ──
        FirstK ek = new FirstK(lg("""
            %start E
            E  -> T Ep
            Ep -> + T Ep | epsilon
            T  -> F Tp
            Tp -> * F Tp | epsilon
            F  -> ( E ) | id | num
            """));
        var t1 = ek.firstKTable(1);
        Assert.equals(FirstK.show(t1.get("F")), "{ ( , id , num }", "FIRST_1(F)");
        Assert.equals(ek.minLL(4, gr("""
            %start E
            E  -> T Ep
            Ep -> + T Ep | epsilon
            T  -> F Tp
            Tp -> * F Tp | epsilon
            F  -> ( E ) | id | num
            """)), 1, "expression grammar is LL(1)");

        // ── label grammar: LL(2) not LL(1) ──
        String label = """
            %start stmt
            stmt  -> label colon stmt | expr semi
            label -> id
            expr  -> id | id lparen rparen
            """;
        FirstK lk = new FirstK(lg(label));
        var lt1 = lk.firstKTable(1);
        // both stmt alternatives predict {id} at k=1
        Assert.equals(FirstK.show(lk.firstKOfSeq(List.of("label", "colon", "stmt"), 1, lt1)), "{ id }",
            "FIRST_1(label colon stmt) = { id }");
        Assert.equals(FirstK.show(lk.firstKOfSeq(List.of("expr", "semi"), 1, lt1)), "{ id }",
            "FIRST_1(expr semi) = { id }  -- collision");
        var lt2 = lk.firstKTable(2);
        Assert.equals(FirstK.show(lk.firstKOfSeq(List.of("label", "colon", "stmt"), 2, lt2)), "{ id colon }",
            "FIRST_2(label colon stmt) = { id colon }");
        Assert.equals(FirstK.show(lk.firstKOfSeq(List.of("expr", "semi"), 2, lt2)), "{ id lparen , id semi }",
            "FIRST_2(expr semi) = { id lparen , id semi }  -- disjoint from { id colon }");
        Assert.equals(lk.minLL(4, gr(label)), 2, "label grammar's minimal k is 2");

        // ── equiv grammar: no finite k ──
        String equiv = """
            %start A
            A -> x B | x C | y
            B -> A
            C -> A
            """;
        FirstK qk = new FirstK(lg(equiv));
        Assert.equals(qk.minLL(4, gr(equiv)), -1, "equiv grammar is not LL(k) for any k <= 4");

        // ── backtracking cost: exponential on equiv, linear on expr ──
        LeftRec.G eG = lg(equiv);
        Backtrack.Result b3 = new Backtrack(eG).parse(List.of("x", "x", "x", "y"));
        Assert.equals(b3.parseCount(), 8L, "x x x y has 2^3 = 8 parse trees");
        Backtrack.Result b5 = new Backtrack(eG).parse(List.of("x", "x", "x", "x", "x", "y"));
        Assert.equals(b5.parseCount(), 32L, "x^5 y has 2^5 = 32 parse trees");
        Assert.that(b5.entries() > 2 * b3.entries(), "work more than doubles from n=3 to n=5");

        LeftRec.G exG = lg("""
            %start E
            E  -> T Ep
            Ep -> + T Ep | epsilon
            T  -> F Tp
            Tp -> * F Tp | epsilon
            F  -> ( E ) | id | num
            """);
        List<String> shortIn = List.of("id", "+", "id");
        List<String> longIn = new ArrayList<>();
        longIn.add("id");
        for (int i = 0; i < 10; i++) { longIn.add("+"); longIn.add("id"); }
        Backtrack.Result e1 = new Backtrack(exG).parse(shortIn);
        Backtrack.Result e2 = new Backtrack(exG).parse(longIn);
        Assert.equals(e1.parseCount(), 1L, "id + id : one parse tree");
        Assert.equals(e2.parseCount(), 1L, "long chain : still one parse tree");
        // linear: ~ 4 entries per extra "+" pair, nowhere near exponential
        Assert.that(e2.entries() < 10 * e1.entries(), "expr grammar backtracking stays linear");

        Assert.summary();
    }
}
