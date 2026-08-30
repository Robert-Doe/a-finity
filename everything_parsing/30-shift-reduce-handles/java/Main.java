import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public final class Main {

    public static void main(String[] args) throws Exception {
        StringBuilder sb = new StringBuilder();
        sb.append("=== Module 30 - Handles, Shift-Reduce & Viable Prefixes ===\n\n");
        sb.append("SHIFT  : push the next input terminal.\n");
        sb.append("REDUCE A -> b : the top |b| stack symbols are b (a HANDLE); pop them, push A.\n");
        sb.append("ACCEPT : stack is [start], input consumed.\n");
        sb.append("the reductions, reversed, are a RIGHTMOST derivation of the input.\n\n");

        Map<String, Grammar> grammars = new HashMap<>();
        grammars.put("expr", Grammar.parse(Files.readString(Path.of("fixtures/expr.grammar"))));
        grammars.put("ambiguous", Grammar.parse(Files.readString(Path.of("fixtures/ambiguous.grammar"))));

        for (String line : Files.readAllLines(Path.of("fixtures/inputs.txt"))) {
            if (line.isBlank() || line.startsWith("#")) continue;
            int c = line.indexOf(':');
            String gname = line.substring(0, c).trim();
            List<String> toks = Arrays.asList(line.substring(c + 1).trim().split("\\s+"));
            Grammar g = grammars.get(gname);
            ShiftReduce sr = new ShiftReduce(g);

            sb.append("########## ").append(gname).append(" : ").append(String.join(" ", toks)).append('\n');
            for (Grammar.Production p : g.productions) sb.append("  ").append(p).append('\n');
            sb.append('\n');

            ShiftReduce.Result r = sr.parse(toks);
            if (!r.ok()) {
                sb.append("  no parse found\n\n");
                continue;
            }

            sb.append(String.format("  %-26s %-40s %s%n", "STACK", "ACTION", "INPUT"));
            for (ShiftReduce.Step s : r.steps())
                sb.append(String.format("  %-26s %-40s %s%n", s.stack(), s.action(), s.rest()));
            sb.append('\n');

            sb.append("  reductions applied (bottom-up): ");
            List<String> rs = new java.util.ArrayList<>();
            for (Grammar.Production p : r.reductions()) rs.add(ShiftReduce.compact(p));
            sb.append(String.join("  |  ", rs)).append('\n');

            sb.append("  reversed = rightmost derivation:\n    ").append(g.start);
            // rebuild the forms step by step for display
            List<Grammar.Production> rev = new java.util.ArrayList<>(r.reductions());
            java.util.Collections.reverse(rev);
            List<String> form = new java.util.ArrayList<>(List.of(g.start));
            for (Grammar.Production p : rev) {
                int i = -1;
                for (int k = form.size() - 1; k >= 0; k--)
                    if (form.get(k).equals(p.lhs())) { i = k; break; }
                form.remove(i);
                form.addAll(i, p.rhs());
                sb.append("\n    => ").append(String.join(" ", form)).append("   (").append(ShiftReduce.compact(p)).append(")");
            }
            sb.append('\n');
            sb.append("  final form == input? ").append(form.equals(toks)).append('\n');

            if (r.conflict())
                sb.append("  CONFLICT: ").append(r.conflictNote()).append('\n');
            else
                sb.append("  no conflict: every right-sentential form has a unique handle.\n");
            sb.append('\n');
        }

        sb.append("VIABLE PREFIX: any prefix of a right-sentential form that does not run past\n");
        sb.append("the right end of that form's handle. Every STACK column above is a viable\n");
        sb.append("prefix. The set of all viable prefixes of a grammar is REGULAR -- Module 31\n");
        sb.append("builds the DFA that recognizes it, and that DFA is the LR parser's engine.\n");

        System.out.print(sb.toString().replace("\r\n", "\n"));
    }
}
