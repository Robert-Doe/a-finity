import java.util.ArrayList;
import java.util.List;

/**
 * Module 30 — shift-reduce parsing, handles, and viable prefixes.
 *
 * A shift-reduce parser holds a STACK of grammar symbols and reads the input
 * left to right. Two moves:
 *   SHIFT  — push the next input terminal onto the stack.
 *   REDUCE A -> b — the top |b| symbols of the stack are exactly b; pop them,
 *                   push A. That b is a HANDLE.
 * Accept when the stack is just the start symbol and the input is consumed.
 *
 * The sequence of reductions, read in reverse, is a RIGHTMOST derivation of the
 * input. For an unambiguous grammar every step has exactly one handle; an
 * ambiguous grammar has a state where two different moves both lead to accept
 * (a shift-reduce or reduce-reduce conflict).
 *
 * This module has no parsing table yet (that's Module 31). It finds handles by
 * bounded DFS: try each reduction (in production order), then shift; backtrack
 * from dead ends. Module 31 replaces the search with a DFA over viable prefixes.
 */
public final class ShiftReduce {

    public record Step(String stack, String action, String rest) {}
    public record Result(boolean ok, List<Step> steps, List<Grammar.Production> reductions,
                         boolean conflict, String conflictNote) {}

    private final Grammar g;
    private long nodes;
    private final long cap = 2_000_000;

    public ShiftReduce(Grammar g) { this.g = g; }

    public Result parse(List<String> input) {
        nodes = 0;
        List<Step> steps = new ArrayList<>();
        List<Grammar.Production> reductions = new ArrayList<>();
        boolean ok = dfs(new ArrayList<>(), 0, input, steps, reductions);

        // conflict probe: is there a reachable state with two accepting moves?
        String note = firstConflict(new ArrayList<>(), 0, input, new int[]{0});
        return new Result(ok, steps, reductions, note != null, note);
    }

    private boolean dfs(List<String> stack, int pos, List<String> input,
                        List<Step> steps, List<Grammar.Production> reductions) {
        if (++nodes > cap) return false;

        if (stack.size() == 1 && stack.get(0).equals(g.start) && pos == input.size()) {
            steps.add(new Step(join(stack), "ACCEPT", "$"));
            return true;
        }

        // try every reduction whose RHS is a suffix of the stack
        for (Grammar.Production p : g.productions) {
            if (suffixMatches(stack, p.rhs())) {
                int cut = stack.size() - p.rhs().size();
                List<String> ns = new ArrayList<>(stack.subList(0, cut));
                ns.add(p.lhs());
                steps.add(new Step(join(stack), "reduce " + compact(p)
                    + "   (handle: " + handleText(p) + ")", rest(input, pos)));
                reductions.add(p);
                if (dfs(ns, pos, input, steps, reductions)) return true;
                reductions.remove(reductions.size() - 1);
                steps.remove(steps.size() - 1);
            }
        }

        // then shift
        if (pos < input.size()) {
            List<String> ns = new ArrayList<>(stack);
            ns.add(input.get(pos));
            steps.add(new Step(join(stack), "shift " + input.get(pos), rest(input, pos)));
            if (dfs(ns, pos + 1, input, steps, reductions)) return true;
            steps.remove(steps.size() - 1);
        }
        return false;
    }

    /** Find a reachable state where both "reduce" and "shift" (or two reduces) lead to accept. */
    private String firstConflict(List<String> stack, int pos, List<String> input, int[] budget) {
        if (budget[0]++ > 200_000) return null;

        List<Grammar.Production> reduces = new ArrayList<>();
        for (Grammar.Production p : g.productions)
            if (suffixMatches(stack, p.rhs())) reduces.add(p);
        boolean canShift = pos < input.size();

        int accepting = 0;
        String kinds = "";
        for (Grammar.Production p : reduces) {
            int cut = stack.size() - p.rhs().size();
            List<String> ns = new ArrayList<>(stack.subList(0, cut));
            ns.add(p.lhs());
            if (reachesAccept(ns, pos, input, new int[]{0})) { accepting++; kinds += "reduce " + compact(p) + "; "; }
        }
        if (canShift) {
            List<String> ns = new ArrayList<>(stack); ns.add(input.get(pos));
            if (reachesAccept(ns, pos + 1, input, new int[]{0})) { accepting++; kinds += "shift " + input.get(pos) + "; "; }
        }
        if (accepting >= 2)
            return "stack [" + join(stack) + "], input " + rest(input, pos) + "  -> two accepting moves: " + kinds.trim();

        // descend one accepting move (any) to look deeper
        for (Grammar.Production p : reduces) {
            int cut = stack.size() - p.rhs().size();
            List<String> ns = new ArrayList<>(stack.subList(0, cut));
            ns.add(p.lhs());
            if (reachesAccept(ns, pos, input, new int[]{0})) {
                String r = firstConflict(ns, pos, input, budget);
                if (r != null) return r;
            }
        }
        if (canShift) {
            List<String> ns = new ArrayList<>(stack); ns.add(input.get(pos));
            if (reachesAccept(ns, pos + 1, input, new int[]{0})) {
                String r = firstConflict(ns, pos + 1, input, budget);
                if (r != null) return r;
            }
        }
        return null;
    }

    private boolean reachesAccept(List<String> stack, int pos, List<String> input, int[] budget) {
        if (budget[0]++ > 100_000) return false;
        if (stack.size() == 1 && stack.get(0).equals(g.start) && pos == input.size()) return true;
        for (Grammar.Production p : g.productions)
            if (suffixMatches(stack, p.rhs())) {
                int cut = stack.size() - p.rhs().size();
                List<String> ns = new ArrayList<>(stack.subList(0, cut));
                ns.add(p.lhs());
                if (reachesAccept(ns, pos, input, budget)) return true;
            }
        if (pos < input.size()) {
            List<String> ns = new ArrayList<>(stack); ns.add(input.get(pos));
            if (reachesAccept(ns, pos + 1, input, budget)) return true;
        }
        return false;
    }

    /** Reverse the reductions to get the rightmost derivation; return its sentential forms. */
    public List<String> rightmostDerivation(List<Grammar.Production> reductions) {
        List<String> form = new ArrayList<>(List.of(g.start));
        List<Grammar.Production> rev = new ArrayList<>(reductions);
        java.util.Collections.reverse(rev);
        for (Grammar.Production p : rev) {
            int i = -1;
            for (int k = form.size() - 1; k >= 0; k--)
                if (form.get(k).equals(p.lhs())) { i = k; break; }   // rightmost occurrence
            form.remove(i);
            form.addAll(i, p.rhs());
        }
        return form;
    }

    // ─────────────────────────────────────────── helpers

    private static boolean suffixMatches(List<String> stack, List<String> rhs) {
        if (rhs.isEmpty() || rhs.size() > stack.size()) return false;   // no epsilon rules in these fixtures
        int off = stack.size() - rhs.size();
        for (int i = 0; i < rhs.size(); i++)
            if (!stack.get(off + i).equals(rhs.get(i))) return false;
        return true;
    }
    static String compact(Grammar.Production p) {
        return p.lhs() + " -> " + (p.rhs().isEmpty() ? "epsilon" : String.join(" ", p.rhs()));
    }
    private static String handleText(Grammar.Production p) {
        return String.join(" ", p.rhs());
    }
    private static String join(List<String> xs) { return xs.isEmpty() ? "(empty)" : String.join(" ", xs); }
    private static String rest(List<String> input, int pos) {
        if (pos >= input.size()) return "$";
        return String.join(" ", input.subList(pos, input.size())) + " $";
    }
}
