import java.util.Set;

/** java -cp java/out LeftFactorTest */
public final class LeftFactorTest {

    static Grammar parse(String g) { return Grammar.parse(g); }

    public static void main(String[] args) {
        System.out.println("LeftFactorTest");

        // ── dangling else: long shared prefix -> the Module 20/22 grammar ──
        Grammar de = parse("""
            %start S
            S -> if b then S | if b then S else S | other
            """);
        LeftRec.G deBefore = LeftRec.G.from(de);
        Assert.that(LeftFactor.needsFactoring(deBefore), "before: two alts share 'if'");

        LeftRec.G deAfter = LeftFactor.factor(de);
        Assert.that(!LeftFactor.needsFactoring(deAfter), "after: no shared first symbol");
        Assert.equals(deAfter.text().strip(),
            String.join("\n",
                "S -> other | if b then S S'",
                "S' -> epsilon | else S"),
            "produces the dangling-else grammar Modules 20/22 analysed");
        Assert.equals(Language.upTo(deBefore, 9), Language.upTo(deAfter, 9),
            "same language up to length 9");

        // ── nested prefixes: two passes ──
        Grammar ne = parse("""
            %start A
            A -> a b c | a b d | a e
            """);
        LeftRec.G neBefore = LeftRec.G.from(ne);
        LeftRec.G neAfter = LeftFactor.factor(ne);
        Assert.that(!LeftFactor.needsFactoring(neAfter), "nested: fully factored");
        Assert.equals(neAfter.text().strip(),
            String.join("\n",
                "A -> a A''",
                "A'' -> e | b A'",
                "A' -> c | d"),
            "a factored out first, then b");
        Assert.equals(Language.upTo(neBefore, 4), Language.upTo(neAfter, 4), "nested: language preserved");

        // ── longestSharedPrefix picks the longest, not just the first symbol ──
        var alts = java.util.List.of(
            java.util.List.of("a", "b", "c"),
            java.util.List.of("a", "b", "d"),
            java.util.List.of("a", "e"));
        Assert.equals(LeftFactor.longestSharedPrefix(alts).toString(), "[a, b]",
            "longest shared prefix of the three alts is [a, b]");

        // ── realistic: three id-prefixed statements ──
        Grammar decl = parse("""
            %start stmt
            stmt -> id lp args rp semi | id assign expr semi | id colon type semi
            args -> expr | epsilon
            expr -> id | num
            type -> id
            """);
        LeftRec.G declAfter = LeftFactor.factor(decl);
        Assert.that(declAfter.text().contains("stmt -> id stmt'"), "id factored out of stmt");
        Assert.that(!LeftFactor.needsFactoring(declAfter), "decl fully factored");
        Assert.equals(Language.upTo(LeftRec.G.from(decl), 6), Language.upTo(declAfter, 6),
            "decl: language preserved");

        // ── nothing to do ──
        Grammar clean = parse("""
            %start S
            S -> a X | b Y
            X -> x
            Y -> y
            """);
        Assert.equals(LeftFactor.factor(clean).text().strip(),
            String.join("\n", "S -> a X | b Y", "X -> x", "Y -> y"),
            "already-factored grammar is unchanged");

        Assert.summary();
    }
}
