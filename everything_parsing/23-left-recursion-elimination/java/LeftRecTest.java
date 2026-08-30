import java.util.Set;

/** java -cp java/out LeftRecTest */
public final class LeftRecTest {

    static Grammar parse(String g) { return Grammar.parse(g); }

    public static void main(String[] args) {
        System.out.println("LeftRecTest");

        // ── direct: the classic expression grammar ──
        Grammar expr = parse("""
            %start E
            E -> E + T | T
            T -> T * F | F
            F -> ( E ) | id | num
            """);
        LeftRec.G before = LeftRec.G.from(expr);
        Assert.that(LeftRec.hasLeftRecursion(before), "the natural expression grammar is left-recursive");

        LeftRec.G after = LeftRec.paull(expr);
        Assert.that(!LeftRec.hasLeftRecursion(after), "after elimination it is not");
        Assert.equals(after.text().strip(),
            String.join("\n",
                "E -> T E'",
                "E' -> + T E' | epsilon",
                "T -> F T'",
                "T' -> * F T' | epsilon",
                "F -> ( E ) | id | num"),
            "produces exactly the Module 19 grammar");
        Assert.equals(after.start, "E", "start symbol preserved");

        // ── language preserved (direct) ──
        Set<String> lb = Language.upTo(before, 7);
        Set<String> la = Language.upTo(after, 7);
        Assert.equals(la, lb, "same language up to length 7");
        Assert.that(la.contains("id + id * id"), "id + id * id still derivable");

        // ── indirect: Paull with order S, A ──
        Grammar ind = parse("""
            %start S
            S -> A a | b
            A -> A c | S d | e
            """);
        LeftRec.G indBefore = LeftRec.G.from(ind);
        LeftRec.G indAfter = LeftRec.paull(ind);
        Assert.that(LeftRec.hasLeftRecursion(indBefore), "indirect grammar is (indirectly) left-recursive");
        Assert.that(!LeftRec.hasLeftRecursion(indAfter), "Paull removes the indirect left recursion");
        Assert.equals(indAfter.text().strip(),
            String.join("\n",
                "S -> A a | b",
                "A -> b d A' | e A'",
                "A' -> c A' | a d A' | epsilon"),
            "matches the Dragon Book 4.18 result");

        Set<String> ib = Language.upTo(indBefore, 6);
        Set<String> ia = Language.upTo(indAfter, 6);
        Assert.equals(ia, ib, "indirect: same language up to length 6");

        // ── a grammar with no left recursion is untouched ──
        Grammar clean = parse("""
            %start S
            S -> a S b | c
            """);
        LeftRec.G cleanAfter = LeftRec.paull(clean);
        Assert.equals(cleanAfter.text().strip(), "S -> a S b | c", "non-left-recursive grammar unchanged");

        Assert.summary();
    }
}
