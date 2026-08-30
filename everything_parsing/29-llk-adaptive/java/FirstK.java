import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Module 29 — FIRST_k sets and the minimal-k question.
 *
 * FIRST_k(alpha) = every terminal string of length <= k that can BEGIN a string
 * derived from alpha (shorter than k only if alpha can derive something that
 * short). FIRST_1 is the ordinary FIRST set with each element wrapped as a
 * one-element list.
 *
 * A grammar is LL(k) (by the FIRST_k test used here) when, for every nonterminal
 * with two or more productions, the FIRST_k sets of its alternatives are
 * pairwise disjoint. (The full condition also folds FOLLOW_k in for nullable
 * alternatives; none of this module's LL(k) fixtures need that, and k=1 is
 * delegated to Module 20's exact PREDICT check.)
 */
public final class FirstK {

    private final LeftRec.G g;
    public FirstK(LeftRec.G g) { this.g = g; }

    /** FIRST_k of every nonterminal, as a fixed point. */
    public Map<String, Set<List<String>>> firstKTable(int k) {
        Map<String, Set<List<String>>> t = new LinkedHashMap<>();
        for (String nt : g.order) t.put(nt, new LinkedHashSet<>());
        boolean changed = true;
        while (changed) {
            changed = false;
            for (String nt : g.order) {
                Set<List<String>> tgt = t.get(nt);
                int before = tgt.size();
                for (List<String> rhs : g.prods.get(nt)) tgt.addAll(firstKOfSeq(rhs, k, t));
                if (tgt.size() != before) changed = true;
            }
        }
        return t;
    }

    /** FIRST_k of a symbol sequence, using a (possibly partial) nonterminal table. */
    public Set<List<String>> firstKOfSeq(List<String> seq, int k, Map<String, Set<List<String>>> t) {
        Set<List<String>> acc = new LinkedHashSet<>();
        acc.add(new ArrayList<>());                          // the empty prefix
        for (String sym : seq) {
            Set<List<String>> next = new LinkedHashSet<>();
            Set<List<String>> symSet = g.isNT(sym)
                ? t.getOrDefault(sym, Set.of())
                : Set.of(List.of(sym));
            for (List<String> pre : acc) {
                if (pre.size() == k) { next.add(pre); continue; }   // already full
                for (List<String> s : symSet) {
                    List<String> joined = new ArrayList<>(pre);
                    joined.addAll(s);
                    if (joined.size() > k) joined = joined.subList(0, k);
                    next.add(joined);
                }
                // an empty symSet means "not computed yet" -> contributes nothing.
                // a genuinely nullable nonterminal carries [] IN its set, so the
                // loop above already handles it.
            }
            acc = next;
        }
        return acc;
    }

    /** Minimal k in 1..maxK for which the grammar is LL(k); -1 if none. */
    public int minLL(int maxK, Grammar original) {
        if (new Predict(original).isLL1()) return 1;
        for (int k = 2; k <= maxK; k++) if (isLLk(k)) return k;
        return -1;
    }

    public boolean isLLk(int k) {
        Map<String, Set<List<String>>> t = firstKTable(k);
        for (String nt : g.order) {
            List<List<String>> prods = g.prods.get(nt);
            if (prods.size() < 2) continue;
            List<Set<List<String>>> sets = new ArrayList<>();
            for (List<String> rhs : prods) {
                if (nullable(rhs)) return false;             // FOLLOW_k needed; out of scope here
                sets.add(firstKOfSeq(rhs, k, t));
            }
            for (int i = 0; i < sets.size(); i++)
                for (int j = i + 1; j < sets.size(); j++) {
                    Set<List<String>> inter = new LinkedHashSet<>(sets.get(i));
                    inter.retainAll(sets.get(j));
                    if (!inter.isEmpty()) return false;
                }
        }
        return true;
    }

    private boolean nullable(List<String> seq) {
        // conservative: a sequence is "nullable" here iff it is literally empty
        // (an epsilon production). Deep nullability is Module 21's job; the
        // LL(k) fixtures don't exercise it in a conflicted rule.
        return seq.isEmpty();
    }

    // ─────────────────────────────────────────── rendering

    public static String show(Set<List<String>> set) {
        List<String> items = new ArrayList<>();
        for (List<String> s : set) items.add(s.isEmpty() ? "()" : String.join(" ", s));
        java.util.Collections.sort(items);
        return "{ " + String.join(" , ", items) + " }";
    }
}
