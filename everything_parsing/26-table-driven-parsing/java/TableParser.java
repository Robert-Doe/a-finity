import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.List;

/**
 * Module 26 — Table-Driven Predictive Parsing.
 *
 * An explicit stack + the LL(1) table (Module 25) + a loop. No recursion, no
 * per-grammar code. The stack holds (symbol, tree-node) pairs; it always
 * represents "the rest of the sentential form, left end on top" — exactly what
 * recursive descent kept implicitly in its call frames (Module 19).
 *
 * The sequence of productions applied, read in order, is a LEFTMOST derivation —
 * identical to the one recursive descent produces for the same input. The parse
 * tree is built as a side effect.
 */
public final class TableParser {

    public static final class Node {
        public final String symbol;
        public final List<Node> kids = new ArrayList<>();
        public String lexeme;                       // for matched terminals
        public Node(String symbol) { this.symbol = symbol; }
    }

    public record Result(boolean ok, Node tree, List<String> productions,
                         List<String> trace, String error) {}

    private final Grammar g;
    private final LL1Table tbl;

    public TableParser(Grammar g) { this.g = g; this.tbl = new LL1Table(g); }

    public Result parse(List<String> tokens) {
        List<String> input = new ArrayList<>(tokens);
        input.add(Predict.END);
        List<String> productions = new ArrayList<>();
        List<String> trace = new ArrayList<>();

        Node root = new Node(g.start);
        Deque<Object[]> stack = new ArrayDeque<>();  // [symbol, node]
        stack.push(new Object[]{Predict.END, null});
        stack.push(new Object[]{g.start, root});

        int ip = 0;
        int step = 0;
        while (!stack.isEmpty()) {
            Object[] frame = stack.peek();
            String top = (String) frame[0];
            Node node = (Node) frame[1];
            String look = input.get(ip);

            if (top.equals(Predict.END)) {
                if (look.equals(Predict.END)) {
                    trace.add(row(step++, "$", "$", "ACCEPT"));
                    return new Result(true, root, productions, trace, null);
                }
                trace.add(row(step++, top, look, "error: input left over"));
                return new Result(false, root, productions, trace,
                    "unexpected token '" + look + "' at position " + ip);
            }

            if (!g.isNonterminal(top)) {              // terminal on top: must match
                if (top.equals(look)) {
                    node.lexeme = look;
                    trace.add(row(step++, top, look, "match"));
                    stack.pop();
                    ip++;
                } else {
                    trace.add(row(step++, top, look, "error: expected " + top));
                    return new Result(false, root, productions, trace,
                        "expected '" + top + "' but saw '" + look + "' at position " + ip);
                }
                continue;
            }

            // nonterminal on top: consult the table
            List<Grammar.Production> ps = tbl.cell(top, look);
            if (ps.size() != 1) {
                String why = ps.isEmpty() ? "blank cell" : "CONFLICT";
                trace.add(row(step++, top, look, "error: M[" + top + "][" + look + "] " + why));
                return new Result(false, root, productions, trace,
                    "no production for " + top + " on '" + look + "' at position " + ip);
            }
            Grammar.Production p = ps.get(0);
            productions.add(compact(p));
            trace.add(row(step++, top, look, "expand " + compact(p)));

            stack.pop();
            List<String> rhs = p.rhs();
            List<Node> kids = new ArrayList<>();
            for (String s : rhs) { Node k = new Node(s); node.kids.add(k); kids.add(k); }
            if (rhs.isEmpty()) node.kids.add(new Node("epsilon"));
            for (int i = rhs.size() - 1; i >= 0; i--)
                stack.push(new Object[]{rhs.get(i), kids.get(i)});
        }
        return new Result(false, root, productions, trace, "stack emptied unexpectedly");
    }

    // ─────────────────────────────────────────── leftmost-derivation replay (the proof)

    /** Replay the production list as a leftmost derivation; return the final sentential form. */
    public List<String> replay(List<String> productions) {
        List<String> form = new ArrayList<>(List.of(g.start));
        for (String prod : productions) {
            int i = -1;
            for (int k = 0; k < form.size(); k++)
                if (g.isNonterminal(form.get(k))) { i = k; break; }
            String[] halves = prod.split(" -> ", 2);
            List<String> rhs = new ArrayList<>();
            if (!halves[1].equals("epsilon"))
                for (String s : halves[1].split(" ")) rhs.add(s);
            form.remove(i);
            form.addAll(i, rhs);
        }
        return form;
    }

    static String compact(Grammar.Production p) {
        return p.lhs() + " -> " + (p.rhs().isEmpty() ? "epsilon" : String.join(" ", p.rhs()));
    }
    private static String row(int step, String top, String look, String action) {
        return String.format("  %2d  top %-4s  look %-4s  %s", step, top, look, action);
    }

    // ─────────────────────────────────────────── tree render (matches Module 19)

    public static String render(Node root) {
        StringBuilder sb = new StringBuilder(label(root)).append('\n');
        renderKids(root, "", sb);
        return sb.toString().stripTrailing();
    }
    private static void renderKids(Node n, String prefix, StringBuilder sb) {
        for (int i = 0; i < n.kids.size(); i++) {
            Node k = n.kids.get(i);
            boolean last = i == n.kids.size() - 1;
            sb.append(prefix).append("+- ").append(label(k)).append('\n');
            renderKids(k, prefix + (last ? "   " : "|  "), sb);
        }
    }
    private static String label(Node n) {
        return n.lexeme != null && !n.lexeme.equals(n.symbol) ? n.symbol + " (" + n.lexeme + ")" : n.symbol;
    }
}
