import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Module 25 — The LL(1) Parsing Table.
 *
 * M[A][t] = the one production to apply when the parser is expanding
 * nonterminal A and the lookahead token is t.
 *
 * Build rule: for each production p = A -> alpha, for each terminal t in
 * PREDICT(p), set M[A][t] = p. If M[A][t] is already set, that cell is a
 * CONFLICT and the grammar is not LL(1) — the two productions competing for the
 * cell are a concrete witness.
 *
 * PREDICT / FIRST / FOLLOW come from Module 20's Predict.
 */
public final class LL1Table {

    public final Grammar g;
    public final Predict pr;
    public final List<String> columns;                        // terminals + $
    public final Map<String, Map<String, List<Grammar.Production>>> M = new LinkedHashMap<>();

    public LL1Table(Grammar g) {
        this.g = g;
        this.pr = new Predict(g);

        Set<String> cols = new LinkedHashSet<>(g.terminals());
        cols.add(Predict.END);
        this.columns = new ArrayList<>(cols);

        for (String nt : g.nonterminals) M.put(nt, new LinkedHashMap<>());
        for (Grammar.Production p : g.productions)
            for (String t : pr.predict(p))
                M.get(p.lhs()).computeIfAbsent(t, k -> new ArrayList<>()).add(p);
    }

    public List<Grammar.Production> cell(String nt, String t) {
        return M.getOrDefault(nt, Map.of()).getOrDefault(t, List.of());
    }

    public record Conflict(String nt, String token, List<Grammar.Production> ps) {}

    public List<Conflict> conflicts() {
        List<Conflict> out = new ArrayList<>();
        for (String nt : g.nonterminals)
            for (var e : M.get(nt).entrySet())
                if (e.getValue().size() > 1) out.add(new Conflict(nt, e.getKey(), e.getValue()));
        return out;
    }

    public boolean isLL1() { return conflicts().isEmpty(); }

    /**
     * Grid render: one row per nonterminal, one column per terminal (+ $).
     * Cells hold the production INDEX (see the numbered legend printed above);
     * "." = blank (a syntax error at parse time), "!" = conflict.
     */
    public String render() {
        int w = 4;
        for (String c : columns) w = Math.max(w, c.length() + 1);

        StringBuilder sb = new StringBuilder();
        sb.append(pad("", 6));
        for (String c : columns) sb.append(pad(c, w));
        sb.append('\n');
        for (String nt : g.nonterminals) {
            sb.append(pad(nt, 6));
            for (String c : columns) {
                List<Grammar.Production> ps = cell(nt, c);
                String txt = ps.isEmpty() ? "." :
                    ps.size() == 1 ? Integer.toString(ps.get(0).index()) : "!";
                sb.append(pad(txt, w));
            }
            sb.append('\n');
        }
        return sb.toString();
    }

    static String compact(Grammar.Production p) {
        return "(" + p.index() + ") " + p.lhs() + " -> "
             + (p.rhs().isEmpty() ? "epsilon" : String.join(" ", p.rhs()));
    }
    private static String pad(String s, int w) {
        StringBuilder b = new StringBuilder(s);
        while (b.length() < w) b.append(' ');
        return b.toString();
    }
}
