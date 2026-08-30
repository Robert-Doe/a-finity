import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

/**
 * Module 14 — DFA minimization.
 *
 * Two states are EQUIVALENT if the DFA accepts exactly the same continuation
 * strings from either. Merge every equivalence class into one state → the
 * MINIMAL DFA, which by the Myhill-Nerode theorem is UNIQUE for the language.
 *
 *   partitionRefinement  — start {accepting} | {non-accepting}, keep splitting
 *                          a block whose members disagree on which block some
 *                          symbol sends them to, until stable (Moore's method)
 *   tableFilling         — mark "obviously distinguishable" pairs (one accepting
 *                          one not), then mark a pair if some symbol sends it to
 *                          an already-marked pair, to a fixed point
 *
 * Both yield the same partition. `minimal()` builds the quotient DFA.
 */
public final class Minimize {
    private Minimize() {}

    // ── remove states unreachable from the start (keeps original names) ──

    static Dfa trim(Dfa d) {
        Set<String> reach = new LinkedHashSet<>();
        Deque<String> q = new ArrayDeque<>(List.of(d.start));
        reach.add(d.start);
        while (!q.isEmpty()) {
            String s = q.poll();
            for (String a : d.alphabet) {
                String t = d.step(s, a);
                if (reach.add(t)) q.add(t);
            }
        }
        if (reach.size() == d.states.size()) return d;

        StringBuilder sb = new StringBuilder("states:");
        for (String s : reach) sb.append(" ").append(s);
        sb.append("\nalphabet:");
        for (String a : d.alphabet) sb.append(" ").append(a);
        sb.append("\nstart: ").append(d.start).append("\naccept:");
        for (String s : reach) if (d.accept.contains(s)) sb.append(" ").append(s);
        sb.append("\n");
        for (String s : reach)
            for (String a : d.alphabet)
                sb.append(s).append(" ").append(a).append(" ").append(d.step(s, a)).append("\n");
        return Dfa.parse(sb.toString());
    }

    // ── partition refinement (Moore) ──

    static List<Set<String>> partitionRefinement(Dfa d) {
        d = trim(d);
        List<String> states = new ArrayList<>(d.states);
        List<String> alpha = new ArrayList<>(d.alphabet);

        Map<String, Integer> block = new HashMap<>();
        for (String s : states) block.put(s, d.accept.contains(s) ? 1 : 0);
        int blockCount = distinct(block);

        while (true) {
            Map<List<Integer>, Integer> sigId = new LinkedHashMap<>();
            Map<String, Integer> next = new HashMap<>();
            for (String s : states) {
                List<Integer> sig = new ArrayList<>();
                sig.add(block.get(s));
                for (String a : alpha) sig.add(block.get(d.step(s, a)));
                next.put(s, sigId.computeIfAbsent(sig, k -> sigId.size()));
            }
            int nc = distinct(next);
            block = next;
            if (nc == blockCount) break;   // no block was split this round
            blockCount = nc;
        }
        return blocksToSets(states, block);
    }

    // ── table filling ──

    static List<Set<String>> tableFilling(Dfa d) {
        d = trim(d);
        List<String> states = new ArrayList<>(d.states);
        int n = states.size();
        Map<String, Integer> idx = new HashMap<>();
        for (int i = 0; i < n; i++) idx.put(states.get(i), i);
        boolean[][] marked = new boolean[n][n];

        for (int i = 0; i < n; i++)
            for (int j = i + 1; j < n; j++)
                marked[i][j] = d.accept.contains(states.get(i)) != d.accept.contains(states.get(j));

        boolean changed = true;
        while (changed) {
            changed = false;
            for (int i = 0; i < n; i++)
                for (int j = i + 1; j < n; j++) {
                    if (marked[i][j]) continue;
                    for (String a : d.alphabet) {
                        int pi = idx.get(d.step(states.get(i), a));
                        int pj = idx.get(d.step(states.get(j), a));
                        int lo = Math.min(pi, pj), hi = Math.max(pi, pj);
                        if (lo != hi && marked[lo][hi]) { marked[i][j] = true; changed = true; break; }
                    }
                }
        }

        int[] parent = new int[n];
        for (int i = 0; i < n; i++) parent[i] = i;
        for (int i = 0; i < n; i++)
            for (int j = i + 1; j < n; j++)
                if (!marked[i][j]) union(parent, i, j);

        Map<Integer, Set<String>> classes = new LinkedHashMap<>();
        for (int i = 0; i < n; i++)
            classes.computeIfAbsent(find(parent, i), k -> new TreeSet<>()).add(states.get(i));
        return new ArrayList<>(classes.values());
    }

    // ── build the quotient (minimal) DFA ──

    static Dfa minimal(Dfa d) {
        d = trim(d);
        List<Set<String>> classes = new ArrayList<>(partitionRefinement(d));
        // order: the class with the start state first, then by smallest member
        final Dfa dd = d;
        classes.sort((x, y) -> {
            boolean xs = x.contains(dd.start), ys = y.contains(dd.start);
            if (xs != ys) return xs ? -1 : 1;
            return new TreeSet<>(x).first().compareTo(new TreeSet<>(y).first());
        });

        Map<String, String> nameOf = new HashMap<>();
        for (int i = 0; i < classes.size(); i++)
            for (String s : classes.get(i)) nameOf.put(s, "M" + i);

        StringBuilder sb = new StringBuilder("states:");
        for (int i = 0; i < classes.size(); i++) sb.append(" M").append(i);
        sb.append("\nalphabet:");
        for (String a : d.alphabet) sb.append(" ").append(a);
        sb.append("\nstart: ").append(nameOf.get(d.start)).append("\naccept:");
        for (int i = 0; i < classes.size(); i++) {
            String rep = new TreeSet<>(classes.get(i)).first();
            if (d.accept.contains(rep)) sb.append(" M").append(i);
        }
        sb.append("\n");
        for (int i = 0; i < classes.size(); i++) {
            String rep = new TreeSet<>(classes.get(i)).first();
            for (String a : d.alphabet)
                sb.append("M").append(i).append(" ").append(a).append(" ").append(nameOf.get(d.step(rep, a))).append("\n");
        }
        return Dfa.parse(sb.toString());
    }

    // ── regex equivalence: minimal DFAs are isomorphic iff the languages are equal ──

    static boolean equivalent(Regex a, Regex b) {
        return isomorphic(
            minimal(Subset.determinize(Thompson.build(a))),
            minimal(Subset.determinize(Thompson.build(b))));
    }

    static boolean isomorphic(Dfa a, Dfa b) {
        if (a.states.size() != b.states.size()) return false;
        if (!a.alphabet.equals(b.alphabet)) return false;
        Map<String, String> map = new HashMap<>();
        Deque<String[]> q = new ArrayDeque<>();
        map.put(a.start, b.start);
        q.add(new String[]{a.start, b.start});
        while (!q.isEmpty()) {
            String[] pr = q.poll();
            if (a.accept.contains(pr[0]) != b.accept.contains(pr[1])) return false;
            for (String sym : a.alphabet) {
                String ta = a.step(pr[0], sym), tb = b.step(pr[1], sym);
                String seen = map.get(ta);
                if (seen == null) { map.put(ta, tb); q.add(new String[]{ta, tb}); }
                else if (!seen.equals(tb)) return false;
            }
        }
        return true;
    }

    // ── helpers ──

    private static int distinct(Map<String, Integer> m) { return new HashSet<>(m.values()).size(); }
    private static List<Set<String>> blocksToSets(List<String> states, Map<String, Integer> block) {
        Map<Integer, Set<String>> g = new LinkedHashMap<>();
        for (String s : states) g.computeIfAbsent(block.get(s), k -> new TreeSet<>()).add(s);
        return new ArrayList<>(g.values());
    }
    private static int find(int[] p, int x) { return p[x] == x ? x : (p[x] = find(p, p[x])); }
    private static void union(int[] p, int a, int b) { p[find(p, a)] = find(p, b); }
}
