import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

/**
 * The derivation relation, made runnable.
 *
 *   ⇒    one step: replace one nonterminal by the rhs of one of its productions
 *   ⇒*   zero or more steps
 *   L(G) = { w in Sigma* : S ⇒* w }
 *
 * Three operations:
 *   leftmostDerivation — apply a caller-chosen sequence of productions, always to
 *                        the leftmost nonterminal, and return every sentential form
 *   enumerate          — every terminal string of length <= maxLen (breadth-first)
 *   derives            — is a given string in L(G)? (bounded breadth-first search)
 */
public final class Derivation {
    private Derivation() {}

    private static final String SEP = "";
    private static final int MAX_FORMS = 200_000; // safety valve for pathological grammars

    /** One recorded step of a derivation. */
    record Step(List<String> form, Grammar.Production applied) {}

    static int leftmostNonterminal(Grammar g, List<String> form) {
        for (int i = 0; i < form.size(); i++)
            if (g.isNonterminal(form.get(i))) return i;
        return -1;
    }

    /** Expand form at position i by replacing that one symbol with `rhs`. */
    private static List<String> substitute(List<String> form, int i, List<String> rhs) {
        List<String> out = new ArrayList<>(form.size() + rhs.size());
        out.addAll(form.subList(0, i));
        out.addAll(rhs);
        out.addAll(form.subList(i + 1, form.size()));
        return out;
    }

    /**
     * Leftmost derivation driven by explicit production indices.
     * Returns the list of steps (the initial form [S] is the implicit step 0).
     * Throws if a chosen production does not match the current leftmost nonterminal.
     */
    static List<Step> leftmostDerivation(Grammar g, int[] choices) {
        List<Step> steps = new ArrayList<>();
        List<String> form = new ArrayList<>(List.of(g.start));
        steps.add(new Step(form, null));

        for (int c : choices) {
            int i = leftmostNonterminal(g, form);
            if (i < 0)
                throw new IllegalArgumentException(
                    "derivation already complete: no nonterminal left to expand");
            Grammar.Production p = g.productions.get(c);
            if (!p.lhs().equals(form.get(i)))
                throw new IllegalArgumentException(
                    "production " + c + " is " + p + ", but the leftmost nonterminal is '"
                        + form.get(i) + "'");
            form = substitute(form, i, p.rhs());
            steps.add(new Step(form, p));
        }
        return steps;
    }

    /** Every terminal string of length <= maxLen, sorted by (length, lexicographic). */
    static List<String> enumerate(Grammar g, int maxLen) {
        // sort by number of terminal symbols, then lexicographically
        TreeSet<String> results = new TreeSet<>((x, y) -> {
            int sx = symbolCount(x), sy = symbolCount(y);
            return sx != sy ? Integer.compare(sx, sy) : x.compareTo(y);
        });
        Deque<List<String>> queue = new ArrayDeque<>();
        Set<String> seen = new HashSet<>();

        List<String> startForm = List.of(g.start);
        queue.add(startForm);
        seen.add(key(startForm));

        int budget = MAX_FORMS;
        while (!queue.isEmpty() && budget-- > 0) {
            List<String> form = queue.poll();
            if (terminalCount(g, form) > maxLen) continue;

            int i = leftmostNonterminal(g, form);
            if (i < 0) {                       // fully terminal
                results.add(render(form));
                continue;
            }
            for (Grammar.Production p : g.productionsFor(form.get(i))) {
                List<String> next = substitute(form, i, p.rhs());
                if (terminalCount(g, next) > maxLen) continue;
                if (seen.add(key(next))) queue.add(next);
            }
        }
        return new ArrayList<>(results);
    }

    record Membership(boolean derivable, int steps, String note) {}

    /** Bounded breadth-first search for `target` in L(G). */
    static Membership derives(Grammar g, List<String> target) {
        int bound = target.size();
        Deque<List<String>> queue = new ArrayDeque<>();
        Map<String, Integer> depth = new HashMap<>();

        List<String> startForm = List.of(g.start);
        queue.add(startForm);
        depth.put(key(startForm), 0);

        int budget = MAX_FORMS;
        while (!queue.isEmpty() && budget-- > 0) {
            List<String> form = queue.poll();
            int d = depth.get(key(form));

            int i = leftmostNonterminal(g, form);
            if (i < 0) {
                if (form.equals(target))
                    return new Membership(true, d,
                        "S =>* " + display(target) + " in " + d + (d == 1 ? " step" : " steps"));
                continue;
            }
            for (Grammar.Production p : g.productionsFor(form.get(i))) {
                List<String> next = substitute(form, i, p.rhs());
                if (terminalCount(g, next) > bound) continue;   // terminals never disappear
                String k = key(next);
                if (!depth.containsKey(k)) {
                    depth.put(k, d + 1);
                    queue.add(next);
                }
            }
        }
        return new Membership(false, -1,
            "searched every sentential form with <= " + bound + " terminal symbols");
    }

    // ─────────────────────────────────────────── helpers

    private static int terminalCount(Grammar g, List<String> form) {
        int n = 0;
        for (String s : form) if (!g.isNonterminal(s)) n++;
        return n;
    }

    private static String key(List<String> form)  { return String.join(SEP, form); }

    private static int symbolCount(String rendered) {
        if (rendered.equals("epsilon")) return 0;
        int n = 1;
        for (int i = 0; i < rendered.length(); i++) if (rendered.charAt(i) == ' ') n++;
        return n;
    }

    /** Language listing: the empty string prints as "epsilon". */
    private static String render(List<String> form) {
        return form.isEmpty() ? "epsilon" : String.join(" ", form);
    }

    /** Membership / derivation display: the empty string prints as "". */
    static String display(List<String> form) {
        return form.isEmpty() ? "\"\"" : String.join(" ", form);
    }
}
