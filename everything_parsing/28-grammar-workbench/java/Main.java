import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

/**
 *   java -cp java/out Main
 *
 * Runs the workbench on Ajoda's grammar as written, on the repaired grammar
 * (then parses real Module 16 token streams with its table), and on a
 * left-recursive grammar.
 */
public final class Main {
    static final StringBuilder sb = new StringBuilder();

    public static void main(String[] args) throws IOException {
        p("=== Module 28 - Grammar Workbench: from a language's EBNF to a working LL(1) parser ===");
        p("");

        // 1. Ajoda as specified
        p("--- 1. the grammar as the spec writes it ---");
        sb.append(Workbench.report("fixtures/ajoda.ebnf", load("fixtures/ajoda.ebnf")).text());
        p("");

        // 2. the repaired grammar, then real tokens from Module 16
        p("--- 2. one repair: fold assignment into ExprStmt ---");
        Ebnf.Desugared fixed = load("fixtures/ajoda-fixed.ebnf");
        Workbench.Report r = Workbench.report("fixtures/ajoda-fixed.ebnf", fixed);
        sb.append(r.text());
        p("");

        LL1Table table = new LL1Table(r.grammar());
        for (String f : List.of("fixtures/factorial.tokens", "fixtures/missing-semi.tokens")) {
            List<Workbench.Token> toks = Workbench.readTokens(Files.readString(Path.of(f)));
            var res = Workbench.parse(r.grammar(), table, fixed.origin(), toks, f.contains("factorial") ? 12 : 0);
            p("parse " + f + " (" + toks.size() + " tokens from Module 16):");
            if (!res.trace().isEmpty()) {
                p("  first " + res.trace().size() + " steps:");
                for (String line : Workbench.traceLines(res.trace())) p(line);
            }
            if (res.ok())
                p("  ACCEPTED: " + res.expansions() + " expansions, " + res.matches() + " matches, stack depth up to "
                    + res.maxDepth());
            else
                p("  REJECTED at " + res.error());
            p("");
        }

        // 3. left recursion, found and removed
        p("--- 3. a left-recursive grammar ---");
        sb.append(Workbench.report("fixtures/arith-leftrec.ebnf", load("fixtures/arith-leftrec.ebnf")).text());
        p("");

        p("summary: EBNF read as written | brackets desugared to named helpers | conflicts traced to the"
            + " rules that cause them | left recursion removed | the finished table parses Module 16's tokens");

        System.out.print(sb);
    }

    static Ebnf.Desugared load(String f) throws IOException {
        return Ebnf.desugar(Files.readString(Path.of(f)));
    }
    static void p(String s) { sb.append(s).append('\n'); }
}
