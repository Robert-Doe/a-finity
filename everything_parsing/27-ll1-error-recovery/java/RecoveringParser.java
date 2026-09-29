import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.List;
import java.util.Set;

/**
 * Module 27 — LL(1) error recovery: panic mode + phrase-level.
 *
 * When the table parser (Module 26) hits a bad spot it normally stops. Here it
 * recovers and keeps going, so one run reports every error:
 *
 *   - terminal on top != lookahead  -> PHRASE-LEVEL: assume the expected token
 *     was there (pop it, don't consume input). "inserted 'X'".
 *
 *   - M[A][lookahead] is blank (or conflicted) -> PANIC MODE using a
 *     synchronizing set:
 *       lookahead in FOLLOW(A) (or $)  -> pop A: skip this nonterminal entirely.
 *       otherwise                      -> discard the lookahead token and retry.
 *
 * Every branch either pops the stack or advances the input, so it terminates.
 */
public final class RecoveringParser {

    public record Err(int pos, String message) {}
    public record Result(boolean accepted, List<Err> errors, List<String> trace) {
        public boolean clean() { return accepted && errors.isEmpty(); }
    }

    private final Grammar g;
    private final LL1Table tbl;
    private final Predict pr;

    public RecoveringParser(Grammar g) {
        this.g = g; this.tbl = new LL1Table(g); this.pr = new Predict(g);
    }

    public Set<String> syncSet(String nt) { return pr.followOf(nt); }

    public Result parse(List<String> tokens) {
        List<String> input = new ArrayList<>(tokens);
        input.add(Predict.END);
        List<Err> errors = new ArrayList<>();
        List<String> trace = new ArrayList<>();

        Deque<String> stack = new ArrayDeque<>();
        stack.push(Predict.END);
        stack.push(g.start);

        int ip = 0, step = 0, guard = 1000;
        boolean accepted = false;

        while (!stack.isEmpty() && guard-- > 0) {
            String top = stack.peek();
            String look = input.get(ip);

            if (top.equals(Predict.END)) {
                if (look.equals(Predict.END)) {
                    trace.add(row(step++, top, look, "ACCEPT"));
                    accepted = true;
                } else {
                    errors.add(new Err(ip, "extra input: discarding '" + look + "'"));
                    trace.add(row(step++, top, look, "discard (extra input)"));
                    ip++;
                    continue;
                }
                break;
            }

            if (!g.isNonterminal(top)) {                       // terminal on top
                if (top.equals(look)) {
                    trace.add(row(step++, top, look, "match"));
                    stack.pop(); ip++;
                } else {
                    errors.add(new Err(ip, "inserted missing '" + top + "' before '" + look + "'"));
                    trace.add(row(step++, top, look, "insert '" + top + "' (phrase-level)"));
                    stack.pop();                               // pretend it was there
                }
                continue;
            }

            List<Grammar.Production> ps = tbl.cell(top, look);
            if (ps.size() == 1) {
                Grammar.Production p = ps.get(0);
                trace.add(row(step++, top, look, "expand " + compact(p)));
                stack.pop();
                List<String> rhs = p.rhs();
                for (int i = rhs.size() - 1; i >= 0; i--) stack.push(rhs.get(i));
                continue;
            }

            // panic mode
            Set<String> sync = pr.followOf(top);
            if (look.equals(Predict.END) || sync.contains(look)) {
                errors.add(new Err(ip, "no rule for " + top + " on '" + look
                    + "'; skipping " + top + " (lookahead in FOLLOW)"));
                trace.add(row(step++, top, look, "pop " + top + " (sync on FOLLOW)"));
                stack.pop();
            } else {
                errors.add(new Err(ip, "no rule for " + top + " on '" + look
                    + "'; discarding '" + look + "'"));
                trace.add(row(step++, top, look, "discard '" + look + "'"));
                ip++;
            }
        }
        return new Result(accepted, errors, trace);
    }

    static String compact(Grammar.Production p) {
        return p.lhs() + " -> " + (p.rhs().isEmpty() ? "epsilon" : String.join(" ", p.rhs()));
    }
    private static String row(int step, String top, String look, String action) {
        return String.format("  %2d  top %-4s  look %-4s  %s", step, top, look, action);
    }
}
