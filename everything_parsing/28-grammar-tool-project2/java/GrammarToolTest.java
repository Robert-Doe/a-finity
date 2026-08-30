import java.util.Set;
import java.util.TreeSet;

/** java -cp java/out GrammarToolTest */
public final class GrammarToolTest {

    static String s(Set<String> set) { return new TreeSet<>(set).toString(); }

    public static void main(String[] args) {
        System.out.println("GrammarToolTest");

        // ── the LL(1) statement grammar: all seven sections agree it's ready ──
        Grammar st = Grammar.parse("""
            %start P
            P -> L
            L -> S L | epsilon
            S -> id assign E semi
            E -> id | num
            """);
        GrammarTool t1 = new GrammarTool(st);
        Assert.equals(String.join(" ", st.nonterminals), "P L S E", "nonterminals in appearance order");
        Assert.equals(s(t1.fs.nullable), "[L, P]", "L and P nullable");
        Assert.equals(s(t1.fs.firstOf("E")), "[id, num]", "FIRST(E)");
        Assert.equals(s(t1.fol.followOf("L")), "[$]", "FOLLOW(L)");
        Assert.that(new Predict(st).isLL1(), "already LL(1)");
        Assert.that(!LeftRec.hasLeftRecursion(LeftRec.paull(st)), "Paull leaves it LL-friendly");
        Assert.that(!LeftFactor.needsFactoring(LeftFactor.factor(st)), "nothing to factor");

        // ── the natural expression grammar: NOT LL(1), fixable ──
        Grammar ex = Grammar.parse("""
            %start E
            E -> E + T | T
            T -> T * F | F
            F -> ( E ) | id | num
            """);
        GrammarTool t2 = new GrammarTool(ex);
        Assert.that(!new Predict(ex).isLL1(), "expression grammar is not LL(1)");
        Assert.that(LeftRec.hasLeftRecursion(LeftRec.G.from(ex)), "it is left-recursive");

        LeftRec.G noLR = LeftRec.paull(ex);
        Assert.equals(noLR.text().strip(),
            String.join("\n",
                "E -> T E'",
                "E' -> + T E' | epsilon",
                "T -> F T'",
                "T' -> * F T' | epsilon",
                "F -> ( E ) | id | num"),
            "section 5 gives the Module 19 grammar");
        Grammar fixed = GrammarTool.toGrammar(noLR);
        Assert.that(new Predict(fixed).isLL1(), "and that IS LL(1)");

        // ── the JSON grammar: not LL(1) (shared prefixes), left factoring fixes it ──
        Grammar js = Grammar.parse("""
            %start value
            value    -> obj | arr | str | num | tru | fls | nul
            obj      -> lbrace rbrace | lbrace members rbrace
            members  -> pair | pair comma members
            pair     -> str colon value
            arr      -> lbrack rbrack | lbrack elements rbrack
            elements -> value | value comma elements
            """);
        Assert.that(!new Predict(js).isLL1(), "JSON grammar as written is not LL(1)");
        Assert.that(!LeftRec.hasLeftRecursion(LeftRec.G.from(js)), "but it has no left recursion");
        // section 5 must NOT mangle the non-left-recursive elements rule
        LeftRec.G jsNoLR = LeftRec.paull(js);
        Assert.that(jsNoLR.text().contains("elements -> value | value comma elements"),
            "Paull leaves 'elements' untouched (no left recursion to remove)");
        LeftRec.G jsFactored = LeftFactor.factor(js);
        Assert.that(!LeftFactor.needsFactoring(jsFactored), "left factoring clears the shared prefixes");
        Grammar jsFixed = GrammarTool.toGrammar(LeftFactor.factor(GrammarTool.toGrammar(LeftRec.paull(js))));
        Assert.that(new Predict(jsFixed).isLL1(), "transformed JSON grammar is LL(1)");

        // ── the report runs end to end ──
        Assert.that(t2.report().contains("7. LL(1) VERDICT"), "report has all sections");
        Assert.that(t2.report().contains("after left-recursion removal + left factoring: LL(1) = YES"),
            "report confirms the fix works");

        Assert.summary();
    }
}
