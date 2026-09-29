import java.util.Set;
import java.util.TreeSet;

/** java -cp java/out FollowSetsTest */
public final class FollowSetsTest {

    static FollowSets load(String g) { return new FollowSets(Grammar.parse(g)); }
    static String s(Set<String> set) { return new TreeSet<>(set).toString(); }

    public static void main(String[] args) {
        System.out.println("FollowSetsTest");

        // ── FOLLOW flows through a nullable tail ──
        FollowSets abc = load("""
            %start S
            S -> A B c
            A -> a | epsilon
            B -> b | epsilon
            """);
        Assert.equals(s(abc.followOf("S")), "[$]", "FOLLOW(S) = { $ }");
        Assert.equals(s(abc.followOf("A")), "[b, c]", "FOLLOW(A) = { b c }  (B nullable -> add c)");
        Assert.equals(s(abc.followOf("B")), "[c]", "FOLLOW(B) = { c }");
        Assert.that(abc.isStable(), "FOLLOW result is a fixed point");

        // ── mutually-dependent FOLLOW: the dangling else ──
        FollowSets de = load("""
            %start S
            S -> if b then S X | other
            X -> else S | epsilon
            """);
        Assert.equals(s(de.followOf("S")), "[$, else]", "FOLLOW(S) = { $ else }");
        Assert.equals(s(de.followOf("X")), "[$, else]", "FOLLOW(X) = { $ else }  (cycle resolved)");
        // round 0 seeds $ into FOLLOW(S); round 1 propagates; round 2 confirms
        Assert.equals(de.rounds.size(), 3, "converges in 1 productive round + confirm + round 0");

        // ── expression grammar ──
        FollowSets e = load("""
            %start E
            E  -> T Ep
            Ep -> + T Ep | epsilon
            T  -> F Tp
            Tp -> * F Tp | epsilon
            F  -> ( E ) | num | id
            """);
        Assert.equals(s(e.followOf("E")), "[$, )]", "FOLLOW(E) = { $ ) }");
        Assert.equals(s(e.followOf("Ep")), "[$, )]", "FOLLOW(Ep) = FOLLOW(E) (Ep is last in E -> T Ep)");
        Assert.equals(s(e.followOf("T")), "[$, ), +]", "FOLLOW(T) picks up + from Ep -> + T Ep");
        Assert.equals(s(e.followOf("F")), "[$, ), *, +]", "FOLLOW(F) picks up * from Tp -> * F Tp");
        Assert.that(!e.followOf("F").contains("epsilon"), "epsilon is never in a FOLLOW set");

        // ── start symbol always has $ ──
        Assert.that(e.followOf("E").contains("$"), "$ seeded into FOLLOW(start)");

        // ── cross-check vs Module 20 ──
        Predict pr = new Predict(e.g);
        for (String nt : e.g.nonterminals)
            Assert.equals(s(e.followOf(nt)), s(pr.followOf(nt)), "FOLLOW(" + nt + ") agrees with Module 20");

        Assert.summary();
    }
}
