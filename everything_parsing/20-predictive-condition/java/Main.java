import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Set;

public final class Main {

    static final String[] FILES = {
        "fixtures/expr.grammar",
        "fixtures/stmts.grammar",
        "fixtures/prefix.grammar",
        "fixtures/danglingelse.grammar",
    };

    public static void main(String[] args) throws Exception {
        StringBuilder sb = new StringBuilder();
        sb.append("=== Module 20 - The Predictive Parsing Condition ===\n\n");
        sb.append("PREDICT(A -> a) = FIRST(a)                        if a is not nullable\n");
        sb.append("               = FIRST(a)\\{epsilon} U FOLLOW(A)   if a is nullable\n");
        sb.append("LL(1)  <=>  for every nonterminal, the PREDICT sets of its productions\n");
        sb.append("            are pairwise disjoint.\n\n");

        for (String file : FILES) {
            Grammar g = Grammar.parse(Files.readString(Path.of(file)));
            Predict pr = new Predict(g);

            sb.append("########## ").append(file).append("\n\n");
            sb.append("grammar:\n");
            for (Grammar.Production p : g.productions)
                sb.append("  (").append(p.index()).append(") ").append(p).append('\n');
            sb.append("  start: ").append(g.start).append('\n');
            sb.append("  terminals: ").append(Predict.setStr(new java.util.LinkedHashSet<>(g.terminals()))).append('\n');
            sb.append('\n');

            sb.append(String.format("%-6s %-26s %s%n", "NT", "FIRST", "FOLLOW"));
            for (String nt : g.nonterminals)
                sb.append(String.format("%-6s %-26s %s%n",
                    nt, Predict.setStr(pr.firstOf(nt)), Predict.setStr(pr.followOf(nt))));
            sb.append('\n');

            sb.append("PREDICT sets:\n");
            for (Grammar.Production p : g.productions) {
                Set<String> pred = pr.predict(p);
                String tag = pr.nullable(p.rhs()) ? "  (nullable: adds FOLLOW)" : "";
                sb.append(String.format("  (%d) %-22s  %s%s%n",
                    p.index(), p, Predict.setStr(pred), tag));
            }
            sb.append('\n');

            List<Predict.Conflict> cs = pr.conflicts();
            if (cs.isEmpty()) {
                sb.append("VERDICT: LL(1) = YES   every nonterminal's PREDICT sets are disjoint\n");
                sb.append("         -> predictive recursive descent works, no backtracking\n");
            } else {
                sb.append("VERDICT: LL(1) = NO    ").append(cs.size()).append(" conflict(s):\n");
                for (Predict.Conflict c : cs) sb.append("  - ").append(c).append('\n');
                sb.append(hintFor(file));
            }
            sb.append('\n');
        }

        System.out.print(sb.toString().replace("\r\n", "\n"));
    }

    static String hintFor(String file) {
        if (file.contains("prefix"))
            return "  -> the alternatives share a prefix; LEFT-FACTOR them (Module 24):\n"
                 + "        S -> a b S'      S' -> c | d\n";
        if (file.contains("danglingelse"))
            return "  -> classic dangling-else. A predictive parser resolves it by always\n"
                 + "     choosing  X -> else S  when the token is 'else' (else binds to the\n"
                 + "     nearest if). Modules 25/35 formalise this as a resolved conflict.\n";
        return "";
    }
}
