import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

/** java -cp java/out WorkbenchTest */
public final class WorkbenchTest {
    public static void main(String[] args) throws IOException {
        System.out.println("WorkbenchTest");

        // ── desugaring ──
        var d1 = Ebnf.desugar("A ::= \"x\" { B } [ \"y\" ]\nB ::= \"b\"");
        Assert.equals(rhs(d1.grammar(), "A"), "\"x\" A_rep1 A_opt2", "brackets become named helpers");
        Assert.equals(rhs(d1.grammar(), "A_rep1"), "B A_rep1 | epsilon", "{ X } -> X rep | epsilon");
        Assert.equals(rhs(d1.grammar(), "A_opt2"), "\"y\" | epsilon", "[ X ] -> X | epsilon");
        Assert.equals(rhs(Ebnf.desugar("A ::= ( \"a\" \"b\" ) \"c\"").grammar(), "A"), "\"a\" \"b\" \"c\"",
            "a one-branch group is inlined");
        Assert.equals(d1.origin().get("A_rep1"), "A", "helpers remember their rule");

        // ── names ──
        Assert.that(throwsEbnf("A ::= Bee"), "an undefined rule name is an error");
        Assert.that(!throwsEbnf("A ::= IDENT"), "an UPPER_CASE name is a token class");
        Assert.that(throwsEbnf("A ::= \"x\"\nA ::= \"y\""), "a rule defined twice is an error");
        Assert.that(throwsEbnf("A ::= { \"x\""), "an unclosed bracket is an error");

        // ── Ajoda as written: exactly one conflict, explained ──
        var raw = Ebnf.desugar(read("fixtures/ajoda.ebnf"));
        Predict prRaw = new Predict(raw.grammar());
        var cs = Workbench.conflicts(raw.grammar(), prRaw);
        Assert.equals(cs.size(), 1, "one conflict");
        Assert.equals(cs.get(0).nt() + " " + cs.get(0).token(), "Statement IDENT", "Statement on IDENT");
        Assert.that(Workbench.explain(raw.grammar(), prRaw, cs.get(0).prods().get(1), "IDENT")
            .endsWith("Call -> Primary -> IDENT"), "the ExprStmt path is traced down to Primary");

        // ── the repaired grammar is LL(1) and parses real tokens ──
        var fixed = Ebnf.desugar(read("fixtures/ajoda-fixed.ebnf"));
        LL1Table table = new LL1Table(fixed.grammar());
        Assert.equals(Workbench.conflicts(fixed.grammar(), new Predict(fixed.grammar())).size(), 0,
            "the repair removes the conflict");
        var ok = Workbench.parse(fixed.grammar(), table, fixed.origin(),
            Workbench.readTokens(read("fixtures/factorial.tokens")), 0);
        Assert.that(ok.ok(), "factorial.ajoda parses");
        var bad = Workbench.parse(fixed.grammar(), table, fixed.origin(),
            Workbench.readTokens(read("fixtures/missing-semi.tokens")), 0);
        Assert.that(!bad.ok() && bad.error().startsWith("3:5:"), "a missing ';' is caught at the next token, 3:5");

        // ── left recursion ──
        var arith = Ebnf.desugar(read("fixtures/arith-leftrec.ebnf"));
        Assert.that(LeftRec.hasLeftRecursion(LeftRec.G.from(arith.grammar())), "Expr ::= Expr ... is left-recursive");
        Grammar noLR = Workbench.fromG(LeftRec.paull(arith.grammar()));
        Assert.that(!LeftRec.hasLeftRecursion(LeftRec.G.from(noLR)), "Paull removes it");
        Assert.equals(Workbench.conflicts(noLR, new Predict(noLR)).size(), 0, "and the result is LL(1)");

        Assert.summary();
    }

    static String rhs(Grammar g, String nt) {
        List<String> alts = new ArrayList<>();
        for (Grammar.Production p : g.productionsFor(nt))
            alts.add(p.rhs().isEmpty() ? "epsilon" : String.join(" ", p.rhs()));
        return String.join(" | ", alts);
    }
    static String read(String f) throws IOException { return Files.readString(Path.of(f)); }
    static boolean throwsEbnf(String text) {
        try { Ebnf.desugar(text); return false; } catch (Ebnf.EbnfError e) { return true; }
    }
}
