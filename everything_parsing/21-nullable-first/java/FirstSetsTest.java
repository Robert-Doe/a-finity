import java.util.List;
import java.util.Set;
import java.util.TreeSet;

/** java -cp java/out FirstSetsTest */
public final class FirstSetsTest {

    static FirstSets load(String g) { return new FirstSets(Grammar.parse(g)); }
    static String s(Set<String> set) { return new TreeSet<>(set).toString(); }

    public static void main(String[] args) {
        System.out.println("FirstSetsTest");

        // ── indirect nullability, discovered one layer per round ──
        FirstSets ind = load("""
            %start X
            X -> Y
            Y -> Z
            Z -> z | epsilon
            """);
        Assert.equals(s(ind.nullable), "[X, Y, Z]", "X, Y, Z all nullable (indirectly)");
        // round 0 {}, round 1 {Z}, round 2 {Y,Z}, round 3 {X,Y,Z}, round 4 {X,Y,Z}
        Assert.equals(ind.nullableRounds.size(), 5, "nullable took 3 productive rounds + 1 confirming pass + round 0");
        Assert.equals(s(ind.nullableRounds.get(1)), "[Z]", "round 1 finds only Z");
        Assert.equals(s(ind.nullableRounds.get(2)), "[Y, Z]", "round 2 adds Y");
        Assert.equals(s(ind.firstOf("X")), "[epsilon, z]", "FIRST(X) = { z epsilon }");
        Assert.that(ind.firstIsStable(), "the FIRST result is a fixed point");

        // ── nullable cascade: FIRST(S) reaches past the nullable prefix ──
        FirstSets cas = load("""
            %start S
            S -> A B C d
            A -> a | epsilon
            B -> b | epsilon
            C -> c | epsilon
            """);
        Assert.equals(s(cas.nullable), "[A, B, C]", "A B C nullable; S is not");
        Assert.that(!cas.nullable.contains("S"), "S not nullable (the d)");
        Assert.equals(s(cas.firstOf("S")), "[a, b, c, d]", "FIRST(S) collects a,b,c AND d");
        Assert.that(!cas.firstOf("S").contains("epsilon"), "epsilon NOT in FIRST(S)");

        // ── nullableSeq / firstOfSeq ──
        Assert.that(cas.nullableSeq(List.of("A", "B", "C")), "A B C is a nullable sequence");
        Assert.that(!cas.nullableSeq(List.of("A", "B", "C", "d")), "A B C d is not (d is a terminal)");
        Assert.equals(s(cas.firstOfSeq(List.of("A", "B", "C"))), "[a, b, c, epsilon]",
            "FIRST(A B C) includes epsilon (all nullable)");
        Assert.equals(s(cas.firstOfSeq(List.of("C", "d"))), "[c, d]", "FIRST(C d): c, then d because C nullable");

        // ── expression grammar: circular FIRST needs the iteration ──
        FirstSets e = load("""
            %start E
            E  -> T Ep
            Ep -> + T Ep | epsilon
            T  -> F Tp
            Tp -> * F Tp | epsilon
            F  -> ( E ) | num | id
            """);
        Assert.equals(s(e.nullable), "[Ep, Tp]", "only Ep and Tp are nullable");
        Assert.equals(s(e.firstOf("E")), "[(, id, num]", "FIRST(E) resolved through the E->T->F->(E) cycle");
        Assert.that(e.firstRounds.size() >= 4, "FIRST(E) is empty until round 3 (E<-T<-F chain)");
        Assert.equals(s(e.firstRounds.get(1).get("E")), "[]", "FIRST(E) still empty after round 1");

        // ── cross-check against Module 20's independent implementation ──
        Predict pr = new Predict(e.g);
        for (String nt : e.g.nonterminals)
            Assert.equals(s(e.firstOf(nt)), s(pr.firstOf(nt)), "FIRST(" + nt + ") agrees with Module 20");

        Assert.summary();
    }
}
