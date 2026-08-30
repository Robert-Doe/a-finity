import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.List;

public final class Main {

    public static void main(String[] args) throws Exception {
        StringBuilder sb = new StringBuilder();
        sb.append("=== Module 26 - Table-Driven Predictive Parsing ===\n\n");
        sb.append("stack + LL(1) table + a loop. no recursion, no per-grammar code.\n");
        sb.append("stack top = nonterminal -> M[top][lookahead] gives the production; pop, push RHS reversed.\n");
        sb.append("stack top = terminal    -> must equal lookahead; pop and advance.\n");
        sb.append("stack top = $ and lookahead = $ -> ACCEPT.\n\n");

        Grammar g = Grammar.parse(Files.readString(Path.of("fixtures/expr.grammar")));
        sb.append("grammar:\n");
        for (Grammar.Production p : g.productions) sb.append("  ").append(TableParser.compact(p)).append('\n');
        sb.append('\n');

        TableParser parser = new TableParser(g);

        for (String line : Files.readAllLines(Path.of("fixtures/inputs.txt"))) {
            if (line.isBlank() || line.startsWith("#")) continue;
            List<String> toks = Arrays.asList(line.trim().split("\\s+"));

            sb.append("--- ").append(line).append(" ---\n");
            TableParser.Result r = parser.parse(toks);

            for (String t : r.trace()) sb.append(t).append('\n');

            if (r.ok()) {
                sb.append("  productions (a leftmost derivation):\n");
                for (String prod : r.productions()) sb.append("      ").append(prod).append('\n');
                List<String> sform = parser.replay(r.productions());
                sb.append("  replay -> ").append(String.join(" ", sform)).append('\n');
                sb.append("  equals input? ").append(sform.equals(toks)).append('\n');
                sb.append("  parse tree:\n");
                for (String tl : TableParser.render(r.tree()).split("\n"))
                    sb.append("    ").append(tl).append('\n');
            } else {
                sb.append("  REJECTED: ").append(r.error()).append('\n');
            }
            sb.append('\n');
        }

        sb.append("table-driven == recursive descent: the production sequence above is the\n");
        sb.append("same leftmost derivation Module 19's call stack produces for the same input.\n");
        sb.append("the explicit stack here IS that call stack, made into data.\n");

        System.out.print(sb.toString().replace("\r\n", "\n"));
    }
}
