import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

public final class Main {

    static final String[] FILES = {
        "fixtures/indirect.grammar",
        "fixtures/cascade.grammar",
        "fixtures/expr.grammar",
    };

    public static void main(String[] args) throws Exception {
        StringBuilder sb = new StringBuilder();
        sb.append("=== Module 21 - Nullable & FIRST Sets (least fixed points) ===\n\n");
        sb.append("NULLABLE: start empty; add A when  A -> epsilon  or  A -> X1..Xk with every Xi\n");
        sb.append("          already nullable; repeat until a pass adds nothing.\n");
        sb.append("FIRST   : start empty; fold FIRST(rhs) into FIRST(lhs) for every production;\n");
        sb.append("          repeat to convergence. Circular by nature -> needs the iteration.\n\n");

        for (String file : FILES) {
            Grammar g = Grammar.parse(Files.readString(Path.of(file)));
            FirstSets fs = new FirstSets(g);

            sb.append("########## ").append(file).append("\n\n");
            for (Grammar.Production p : g.productions)
                sb.append("  (").append(p.index()).append(") ").append(p).append('\n');
            sb.append('\n');

            // nullable trace
            sb.append("NULLABLE iteration:\n");
            for (int i = 0; i < fs.nullableRounds.size(); i++) {
                boolean last = i == fs.nullableRounds.size() - 1;
                sb.append("  round ").append(i).append(": ")
                  .append(FirstSets.setStr(fs.nullableRounds.get(i)))
                  .append(last ? "   (stable -- one more pass adds nothing)" : "")
                  .append('\n');
            }
            sb.append("  => nullable nonterminals: ").append(FirstSets.setStr(fs.nullable)).append('\n');
            sb.append('\n');

            // first trace
            sb.append("FIRST iteration:\n");
            for (int i = 0; i < fs.firstRounds.size(); i++) {
                sb.append("  round ").append(i).append(":\n");
                var round = fs.firstRounds.get(i);
                for (String nt : g.nonterminals)
                    sb.append(String.format("      FIRST(%-2s) = %s%n", nt, FirstSets.setStr(round.get(nt))));
            }
            sb.append("  one more pass changes nothing? ").append(fs.firstIsStable()).append('\n');
            sb.append('\n');

            // cross-check against Module 20's independent implementation
            Predict pr = new Predict(g);
            boolean agree = true;
            for (String nt : g.nonterminals)
                if (!new java.util.TreeSet<>(fs.firstOf(nt)).equals(new java.util.TreeSet<>(pr.firstOf(nt))))
                    agree = false;
            sb.append("cross-check vs Module 20 Predict.firstOf: ").append(agree ? "MATCH" : "MISMATCH").append('\n');
            sb.append('\n');
        }

        // leastness, spelled out
        sb.append("why the LEAST fixed point?\n");
        sb.append("  - the empty assignment is NOT a fixed point: the rules force elements in.\n");
        sb.append("  - our result IS a fixed point: one more pass adds nothing (checked above).\n");
        sb.append("  - we only ever ADD, starting from empty, so every element we put in was\n");
        sb.append("    forced by a rule. Any other fixed point must also contain those elements.\n");
        sb.append("    Therefore ours is contained in every fixed point = the least one.\n");
        sb.append("  - the LEAST one is the right answer: a bigger set would claim a token can\n");
        sb.append("    start a nonterminal when no derivation actually produces it there.\n");

        System.out.print(sb.toString().replace("\r\n", "\n"));
    }
}
