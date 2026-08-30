import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Module 23 — Left Recursion Elimination.
 *
 * DIRECT:   A -> A a1 | ... | A ak | b1 | ... | bm      (no bi starts with A)
 *   becomes A  -> b1 A' | ... | bm A'
 *           A' -> a1 A' | ... | ak A' | epsilon
 *   Same language; the recursion now grows on the RIGHT, which recursive
 *   descent can parse (a token is consumed before A' recurses).
 *
 * INDIRECT (Paull's algorithm): fix an order A1..An of the nonterminals. For
 * i = 1..n:  for j = 1..i-1, replace every  Ai -> Aj g  by  Ai -> d g  for each
 * Aj -> d ; then eliminate direct left recursion in Ai. After step i, no Ai
 * production starts with A1..Ai, so by the end none is left-recursive.
 *
 * Language preservation is checked empirically by `Language` (bounded string
 * enumeration) in Main and the tests.
 */
public final class LeftRec {

    /** A mutable grammar: ordered nonterminals + ordered productions (RHS = list of symbols, [] = epsilon). */
    public static final class G {
        public final List<String> order = new ArrayList<>();
        public final Map<String, List<List<String>>> prods = new LinkedHashMap<>();
        public String start;

        public boolean isNT(String s) { return prods.containsKey(s); }

        public static G from(Grammar src) {
            G g = new G();
            g.start = src.start;
            for (String nt : src.nonterminals) { g.order.add(nt); g.prods.put(nt, new ArrayList<>()); }
            for (Grammar.Production p : src.productions)
                g.prods.get(p.lhs()).add(new ArrayList<>(p.rhs()));
            return g;
        }

        public String text() {
            StringBuilder sb = new StringBuilder();
            for (String nt : order) {
                List<String> alts = new ArrayList<>();
                for (List<String> r : prods.get(nt)) alts.add(r.isEmpty() ? "epsilon" : String.join(" ", r));
                sb.append(nt).append(" -> ").append(String.join(" | ", alts)).append('\n');
            }
            return sb.toString();
        }
    }

    // ─────────────────────────────────────────── direct

    public static void eliminateDirect(G g, String a) {
        List<List<String>> rules = g.prods.get(a);
        List<List<String>> alpha = new ArrayList<>();   // A -> A alpha
        List<List<String>> beta = new ArrayList<>();    // A -> beta
        for (List<String> r : rules) {
            if (!r.isEmpty() && r.get(0).equals(a)) alpha.add(new ArrayList<>(r.subList(1, r.size())));
            else beta.add(new ArrayList<>(r));
        }
        if (alpha.isEmpty()) return;                    // no direct left recursion

        String ap = fresh(g, a);
        List<List<String>> newA = new ArrayList<>();
        for (List<String> b : beta) { List<String> nb = new ArrayList<>(b); nb.add(ap); newA.add(nb); }
        List<List<String>> newAp = new ArrayList<>();
        for (List<String> al : alpha) { List<String> na = new ArrayList<>(al); na.add(ap); newAp.add(na); }
        newAp.add(new ArrayList<>());                   // A' -> epsilon

        g.prods.put(a, dedupe(newA));
        g.prods.put(ap, dedupe(newAp));
        g.order.add(g.order.indexOf(a) + 1, ap);
    }

    // ─────────────────────────────────────────── indirect (Paull)

    public static G paull(Grammar src) {
        G g = G.from(src);
        List<String> a = new ArrayList<>(g.order);      // snapshot: original nonterminals only
        for (int i = 0; i < a.size(); i++) {
            String ai = a.get(i);
            for (int j = 0; j < i; j++) {
                String aj = a.get(j);
                // Only substitute Aj into Ai -> Aj g when Aj can reach Ai as a
                // leftmost symbol — i.e. when doing so exposes (indirect) left
                // recursion for eliminateDirect to remove. Otherwise the rule is
                // fine as written and substituting only bloats the grammar.
                if (!leftmostReaches(g, aj).contains(ai)) continue;
                List<List<String>> replaced = new ArrayList<>();
                for (List<String> r : g.prods.get(ai)) {
                    if (!r.isEmpty() && r.get(0).equals(aj)) {
                        List<String> tail = r.subList(1, r.size());
                        for (List<String> d : g.prods.get(aj)) {
                            List<String> combined = new ArrayList<>(d);
                            combined.addAll(tail);
                            replaced.add(combined);
                        }
                    } else {
                        replaced.add(new ArrayList<>(r));
                    }
                }
                g.prods.put(ai, dedupe(replaced));
            }
            eliminateDirect(g, ai);
        }
        return g;
    }

    // ─────────────────────────────────────────── helpers

    /** Nonterminals reachable as the leftmost symbol of a form derived from `from`. */
    private static Set<String> leftmostReaches(G g, String from) {
        Set<String> seen = new LinkedHashSet<>();
        java.util.Deque<String> stack = new java.util.ArrayDeque<>();
        for (List<String> r : g.prods.get(from))
            if (!r.isEmpty() && g.isNT(r.get(0))) stack.push(r.get(0));
        while (!stack.isEmpty()) {
            String x = stack.pop();
            if (!seen.add(x)) continue;
            for (List<String> r : g.prods.get(x))
                if (!r.isEmpty() && g.isNT(r.get(0))) stack.push(r.get(0));
        }
        return seen;
    }

    private static String fresh(G g, String base) {
        String name = base + "'";
        while (g.prods.containsKey(name)) name += "'";
        return name;
    }

    private static List<List<String>> dedupe(List<List<String>> in) {
        Set<String> seen = new LinkedHashSet<>();
        List<List<String>> out = new ArrayList<>();
        for (List<String> r : in) {
            String key = String.join("", r);
            if (seen.add(key)) out.add(r);
        }
        return out;
    }

    /**
     * Syntactic left-recursion check: is any nonterminal reachable as the
     * leftmost symbol of a form it derives? (Ignores nullable-prefix cases,
     * which neither fixture uses.)
     */
    public static boolean hasLeftRecursion(G g) {
        for (String nt : g.order) {
            Set<String> seen = new LinkedHashSet<>();
            java.util.Deque<String> stack = new java.util.ArrayDeque<>();
            for (List<String> r : g.prods.get(nt))
                if (!r.isEmpty() && g.isNT(r.get(0))) stack.push(r.get(0));
            while (!stack.isEmpty()) {
                String x = stack.pop();
                if (x.equals(nt)) return true;
                if (!seen.add(x)) continue;
                for (List<String> r : g.prods.get(x))
                    if (!r.isEmpty() && g.isNT(r.get(0))) stack.push(r.get(0));
            }
        }
        return false;
    }
}
