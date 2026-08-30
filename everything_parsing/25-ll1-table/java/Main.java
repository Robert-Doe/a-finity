import java.nio.file.Files;
import java.nio.file.Path;

public final class Main {

    static final String[] FILES = {
        "fixtures/expr.grammar",
        "fixtures/stmts.grammar",
        "fixtures/danglingelse.grammar",
    };

    public static void main(String[] args) throws Exception {
        StringBuilder sb = new StringBuilder();
        sb.append("=== Module 25 - The LL(1) Parsing Table ===\n\n");
        sb.append("M[A][t] = the production to apply when expanding A with lookahead t.\n");
        sb.append("build: for each A -> alpha, put it in M[A][t] for every t in PREDICT(A -> alpha).\n");
        sb.append("a cell that gets two productions is a CONFLICT  <=>  grammar is not LL(1).\n\n");

        for (String file : FILES) {
            Grammar g = Grammar.parse(Files.readString(Path.of(file)));
            LL1Table tbl = new LL1Table(g);

            sb.append("########## ").append(file).append("\n\n");
            sb.append("productions:\n");
            for (Grammar.Production p : g.productions)
                sb.append("  ").append(LL1Table.compact(p)).append('\n');
            sb.append('\n');

            sb.append("PREDICT sets:\n");
            for (Grammar.Production p : g.productions)
                sb.append(String.format("  (%d) %-20s %s%n", p.index(),
                    p.lhs() + " -> " + (p.rhs().isEmpty() ? "epsilon" : String.join(" ", p.rhs())),
                    Predict.setStr(tbl.pr.predict(p))));
            sb.append('\n');

            sb.append("table (cells = production index; . = blank, ! = conflict):\n");
            for (String line : tbl.render().split("\n")) sb.append("  ").append(line).append('\n');
            sb.append('\n');

            if (tbl.isLL1()) {
                sb.append("VERDICT: LL(1) = YES   every cell has at most one production\n");
            } else {
                sb.append("VERDICT: LL(1) = NO    ").append(tbl.conflicts().size()).append(" conflicted cell(s):\n");
                for (LL1Table.Conflict c : tbl.conflicts()) {
                    sb.append("  M[").append(c.nt()).append("][").append(c.token()).append("] wants:\n");
                    for (Grammar.Production p : c.ps())
                        sb.append("      ").append(LL1Table.compact(p)).append('\n');
                }
            }
            sb.append('\n');
        }

        // a tiny parse-table walk, to show the table is executable
        sb.append("using the expr table to parse  id + id * id :\n");
        traceParse(sb, Grammar.parse(Files.readString(Path.of("fixtures/expr.grammar"))),
            new String[]{"id", "+", "id", "*", "id"});

        System.out.print(sb.toString().replace("\r\n", "\n"));
    }

    /** Stack-driven predictive parse using the table (a preview of Module 26). */
    static void traceParse(StringBuilder sb, Grammar g, String[] input) {
        LL1Table tbl = new LL1Table(g);
        java.util.Deque<String> stack = new java.util.ArrayDeque<>();
        stack.push(Predict.END);
        stack.push(g.start);
        int ip = 0;
        String[] in = java.util.Arrays.copyOf(input, input.length + 1);
        in[input.length] = Predict.END;

        int step = 0;
        while (!stack.isEmpty()) {
            String top = stack.peek();
            String look = in[ip];
            if (top.equals(Predict.END) && look.equals(Predict.END)) {
                sb.append(String.format("  %2d  stack top %-4s  look %-4s  ACCEPT%n", step++, top, look));
                break;
            }
            if (!g.isNonterminal(top)) {                       // terminal: must match
                sb.append(String.format("  %2d  match %s%n", step++, top));
                stack.pop(); ip++;
                continue;
            }
            var ps = tbl.cell(top, look);
            if (ps.size() != 1) {
                sb.append(String.format("  %2d  M[%s][%s] empty -> syntax error%n", step++, top, look));
                break;
            }
            Grammar.Production p = ps.get(0);
            sb.append(String.format("  %2d  expand %s%n", step++, LL1Table.compact(p)));
            stack.pop();
            var rhs = p.rhs();
            for (int k = rhs.size() - 1; k >= 0; k--) stack.push(rhs.get(k));
        }
    }
}
