import java.util.List;
import java.util.Set;
import java.util.TreeSet;

/** java -cp java/out PredictTest */
public final class PredictTest {

    static Predict load(String g) { return new Predict(Grammar.parse(g)); }
    static String s(Set<String> set) { return new TreeSet<>(set).toString(); }

    public static void main(String[] args) {
        System.out.println("PredictTest");

        String expr = """
            %start E
            E  -> T Ep
            Ep -> + T Ep | epsilon
            T  -> F Tp
            Tp -> * F Tp | epsilon
            F  -> ( E ) | num | id
            """;
        Predict e = load(expr);

        // ── FIRST ──
        Assert.equals(s(e.firstOf("E")), "[(, id, num]", "FIRST(E) = { ( id num }");
        Assert.equals(s(e.firstOf("Ep")), "[+, epsilon]", "FIRST(Ep) = { + epsilon }");
        Assert.equals(s(e.firstOf("Tp")), "[*, epsilon]", "FIRST(Tp) = { * epsilon }");
        Assert.that(e.nullable(List.of("Ep")), "Ep is nullable");
        Assert.that(!e.nullable(List.of("F")), "F is not nullable");

        // ── FOLLOW ──
        Assert.equals(s(e.followOf("E")), "[$, )]", "FOLLOW(E) = { $ ) }");
        Assert.equals(s(e.followOf("T")), "[$, ), +]", "FOLLOW(T) = { $ ) + }");
        Assert.equals(s(e.followOf("F")), "[$, ), *, +]", "FOLLOW(F) = { $ ) * + }");

        // ── PREDICT + verdict ──
        Assert.that(e.isLL1(), "the Module 19 expression grammar IS LL(1)");
        Assert.equals(e.conflicts().size(), 0, "no conflicts in the expression grammar");

        // ── nullable list rule stays LL(1) ──
        Predict st = load("""
            %start P
            P -> L
            L -> S L | epsilon
            S -> id assign E semi
            E -> id | num
            """);
        Assert.that(st.isLL1(), "statement-list grammar with a nullable L is LL(1)");
        Assert.equals(s(st.followOf("L")), "[$]", "FOLLOW(L) = { $ }");
        Assert.equals(s(st.predict(st.g.productionsFor("L").get(0))), "[id]", "PREDICT(L -> S L) = { id }");

        // ── common prefix: NOT LL(1) ──
        Predict pf = load("S -> a b c | a b d\n");
        Assert.that(!pf.isLL1(), "S -> a b c | a b d is not LL(1)");
        Assert.equals(pf.conflicts().size(), 1, "exactly one conflict");
        Assert.equals(pf.conflicts().get(0).token(), "a", "the conflict token is 'a'");

        // ── dangling else: nullable/FOLLOW conflict on 'else' ──
        Predict de = load("""
            %start S
            S -> if b then S X | other
            X -> else S | epsilon
            """);
        Assert.equals(s(de.followOf("X")), "[$, else]", "FOLLOW(X) = { $ else }");
        Assert.that(!de.isLL1(), "dangling-else grammar is not LL(1)");
        Assert.equals(de.conflicts().get(0).token(), "else", "the conflict is on 'else'");
        Assert.equals(de.conflicts().get(0).nonterminal(), "X", "the conflicted nonterminal is X");

        // ── left recursion: FIRST self-loop, and it is never LL(1) ──
        Predict lr = load("A -> A x | y\n");
        Assert.equals(s(lr.firstOf("A")), "[y]", "FIRST(A) for A -> A x | y is still { y } (fixed point)");
        Assert.that(!lr.isLL1(), "a left-recursive grammar is not LL(1) (A -> A x vs A -> y both predict 'y')");

        Assert.summary();
    }
}
