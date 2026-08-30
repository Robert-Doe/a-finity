import java.util.List;

/** java -cp java/out AmbiguityTest */
public final class AmbiguityTest {
    public static void main(String[] args) {
        System.out.println("AmbiguityTest");

        Grammar amb = Grammar.parse("E -> E + E | E * E | ( E ) | id");
        Grammar unamb = Grammar.parse("""
            E -> E + T | T
            T -> T * F | F
            F -> ( E ) | id
            """);

        List<String> mul = List.of("id", "+", "id", "*", "id");
        List<String> add = List.of("id", "+", "id", "+", "id");
        List<String> one = List.of("id");
        List<String> two = List.of("id", "+", "id");

        // ── the ambiguous grammar ──
        Assert.equals(Ambiguity.allTrees(amb, one).size(), 1, "id has one tree even in the ambiguous grammar");
        Assert.equals(Ambiguity.allTrees(amb, two).size(), 1, "id + id has one tree (only one operator)");
        Assert.equals(Ambiguity.allTrees(amb, mul).size(), 2, "id + id * id has TWO trees");
        Assert.equals(Ambiguity.allTrees(amb, add).size(), 2, "id + id + id has TWO trees");
        Assert.that(Ambiguity.isAmbiguousFor(amb, mul), "grammar is ambiguous for id + id * id");

        // the two trees for id + id * id mean different numbers (id = 2)
        var t = Ambiguity.allTrees(amb, mul);
        long v0 = Main.evalExpr(t.get(0));
        long v1 = Main.evalExpr(t.get(1));
        Assert.that((v0 == 6 && v1 == 8) || (v0 == 8 && v1 == 6),
            "the two trees evaluate to 6 and 8");
        Assert.that(v0 != v1, "ambiguity here means two different values");

        // the two trees for id + id + id mean the SAME number but group differently
        var a = Ambiguity.allTrees(amb, add);
        Assert.equals(Main.evalExpr(a.get(0)), 6L, "left-assoc value");
        Assert.equals(Main.evalExpr(a.get(1)), 6L, "right-assoc value (addition is associative)");
        Assert.that(!Main.grouping(a.get(0)).equals(Main.grouping(a.get(1))),
            "but the groupings differ");

        // ── the disambiguated grammar ──
        Assert.equals(Ambiguity.allTrees(unamb, mul).size(), 1, "disambiguated: id + id * id has ONE tree");
        Assert.equals(Ambiguity.allTrees(unamb, add).size(), 1, "disambiguated: id + id + id has ONE tree");
        Assert.equals(Main.grouping(Ambiguity.allTrees(unamb, mul).get(0)), "(id + (id * id))",
            "disambiguated grammar groups * before +");
        Assert.equals(Main.grouping(Ambiguity.allTrees(unamb, add).get(0)), "((id + id) + id)",
            "disambiguated grammar makes + left-associative");

        // ── same language ──
        Assert.equals(Derivation.enumerate(amb, 7), Derivation.enumerate(unamb, 7),
            "ambiguous and disambiguated grammars generate the same language (up to length 7)");

        // ── smallest ambiguous string ──
        Assert.equals(Ambiguity.smallestAmbiguousString(amb, 7), List.of("id", "*", "id", "*", "id"),
            "smallest ambiguous string is id * id * id");
        Assert.that(Ambiguity.smallestAmbiguousString(unamb, 9) == null,
            "the disambiguated grammar has no ambiguous string up to length 9");

        // ── dangling else ──
        Grammar dang = Grammar.parse("S -> if x then S | if x then S else S | a | b");
        var ie = List.of("if", "x", "then", "if", "x", "then", "a", "else", "b");
        Assert.equals(Ambiguity.allTrees(dang, ie).size(), 2, "dangling else: two trees");

        Assert.summary();
    }
}
