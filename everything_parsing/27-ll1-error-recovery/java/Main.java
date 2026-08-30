import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.List;

public final class Main {

    public static void main(String[] args) throws Exception {
        StringBuilder sb = new StringBuilder();
        sb.append("=== Module 27 - LL(1) Error Recovery: Panic Mode & Phrase-Level ===\n\n");
        sb.append("terminal mismatch on top   -> phrase-level: insert the expected token, keep the input.\n");
        sb.append("M[A][t] blank/conflicted   -> panic mode:\n");
        sb.append("    t in FOLLOW(A) (or $)   -> pop A (skip the nonterminal)\n");
        sb.append("    otherwise              -> discard t, retry\n");
        sb.append("one run, every error. every branch pops the stack or advances the input -> it stops.\n\n");

        Grammar g = Grammar.parse(Files.readString(Path.of("fixtures/expr.grammar")));
        RecoveringParser parser = new RecoveringParser(g);

        sb.append("grammar:\n");
        for (Grammar.Production p : g.productions) sb.append("  ").append(RecoveringParser.compact(p)).append('\n');
        sb.append('\n');
        sb.append("synchronizing sets (= FOLLOW):\n");
        for (String nt : g.nonterminals)
            sb.append("  FOLLOW(").append(nt).append(") = ").append(Predict.setStr(parser.syncSet(nt))).append('\n');
        sb.append('\n');

        for (String line : Files.readAllLines(Path.of("fixtures/inputs.txt"))) {
            if (line.isBlank() || line.startsWith("#")) continue;
            List<String> toks = Arrays.asList(line.trim().split("\\s+"));

            sb.append("--- ").append(line).append(" ---\n");
            RecoveringParser.Result r = parser.parse(toks);
            for (String t : r.trace()) sb.append(t).append('\n');

            if (r.clean()) {
                sb.append("  RESULT: accepted, no errors\n");
            } else {
                sb.append("  RESULT: ").append(r.errors().size()).append(" error(s), ")
                  .append(r.accepted() ? "recovered to end of input" : "gave up").append(":\n");
                for (RecoveringParser.Err e : r.errors())
                    sb.append("    @").append(e.pos()).append("  ").append(e.message()).append('\n');
            }
            sb.append('\n');
        }

        System.out.print(sb.toString().replace("\r\n", "\n"));
    }
}
