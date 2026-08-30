import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Module 06 — find every parse tree a grammar gives a string.
 *
 * A grammar is AMBIGUOUS if some string has two or more parse trees. Because a
 * parse tree is in bijection with its leftmost derivation (Module 5), it is
 * enough to enumerate every leftmost derivation that yields the target string.
 *
 * The search is a breadth-first walk over sentential forms. Two prunes keep it
 * finite and fast:
 *   - terminals never disappear, so drop any form with more terminals than |w|
 *   - in a leftmost derivation everything left of the leftmost nonterminal is
 *     final, so that terminal prefix must already match the target
 */
public final class Ambiguity {
    private Ambiguity() {}

    private static final int MAX_STATES = 1_000_000;

    private record State(List<String> form, int[] choices) {}

    /** Every leftmost-derivation choice sequence that yields `target`. */
    static List<int[]> allLeftmostDerivations(Grammar g, List<String> target) {
        List<int[]> out = new ArrayList<>();
        Deque<State> queue = new ArrayDeque<>();
        queue.add(new State(List.of(g.start), new int[0]));

        int budget = MAX_STATES;
        while (!queue.isEmpty() && budget-- > 0) {
            State st = queue.poll();
            int i = Derivation.leftmostNonterminal(g, st.form);
            if (i < 0) {
                if (st.form.equals(target)) out.add(st.choices);
                continue;
            }
            if (i > target.size()) continue;
            if (!st.form.subList(0, i).equals(target.subList(0, i))) continue; // prefix must match

            for (Grammar.Production p : g.productionsFor(st.form.get(i))) {
                List<String> next = splice(st.form, i, p.rhs());
                if (terminalCount(g, next) > target.size()) continue;
                queue.add(new State(next, append(st.choices, p.index())));
            }
        }
        return out;
    }

    /** Distinct parse trees for `target`, keyed by their rendered form. */
    static List<ParseTree> allTrees(Grammar g, List<String> target) {
        Map<String, ParseTree> distinct = new LinkedHashMap<>();
        for (int[] choices : allLeftmostDerivations(g, target)) {
            ParseTree t = ParseTree.build(g, choices, /*leftmost*/ true);
            distinct.putIfAbsent(t.render(), t);
        }
        return new ArrayList<>(distinct.values());
    }

    static boolean isAmbiguousFor(Grammar g, List<String> target) {
        return allTrees(g, target).size() > 1;
    }

    /** Smallest string (by length, then lexicographic) with 2+ trees; null if none up to maxLen. */
    static List<String> smallestAmbiguousString(Grammar g, int maxLen) {
        for (String w : Derivation.enumerate(g, maxLen)) {
            List<String> toks = w.equals("epsilon") ? List.of() : List.of(w.split(" "));
            if (allTrees(g, toks).size() > 1) return toks;
        }
        return null;
    }

    // ── helpers ──

    private static List<String> splice(List<String> form, int i, List<String> rhs) {
        List<String> out = new ArrayList<>(form.size() + rhs.size());
        out.addAll(form.subList(0, i));
        out.addAll(rhs);
        out.addAll(form.subList(i + 1, form.size()));
        return out;
    }
    private static int terminalCount(Grammar g, List<String> form) {
        int n = 0;
        for (String s : form) if (!g.isNonterminal(s)) n++;
        return n;
    }
    private static int[] append(int[] a, int x) {
        int[] b = new int[a.length + 1];
        System.arraycopy(a, 0, b, 0, a.length);
        b[a.length] = x;
        return b;
    }
}
