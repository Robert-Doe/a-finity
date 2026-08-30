import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

/**
 * Module 13 — the subset (powerset) construction: turn an NFA into an equivalent
 * DFA.
 *
 * A DFA state is a SET of NFA states — "every state a clone could be in."
 *
 *   DFA start   = epsilonClosure({ nfa.start })
 *   DFA delta(S, a) = epsilonClosure( union of nfa.move(s, a) for s in S )
 *   DFA state S is accepting  iff  S contains an NFA accepting state
 *
 * Only the *reachable* subsets are built (worklist), so the DFA is often far
 * smaller than 2^|NFA states| — but in the worst case it is exactly that big,
 * which `kthFromEndNfa` demonstrates.
 */
public final class Subset {
    private Subset() {}

    public static Dfa determinize(Nfa nfa) {
        // reachable subsets, in discovery order; each gets a name D0, D1, ...
        Map<Set<String>, String> name = new LinkedHashMap<>();
        Deque<Set<String>> work = new ArrayDeque<>();

        Set<String> startSet = nfa.epsilonClosure(Set.of(nfa.start));
        name.put(startSet, "D0");
        work.add(startSet);

        Map<String, Map<String, String>> delta = new LinkedHashMap<>();
        List<String> accepting = new ArrayList<>();

        while (!work.isEmpty()) {
            Set<String> S = work.poll();
            String sName = name.get(S);
            if (!Nfa.intersect(S, nfa.accept).isEmpty()) accepting.add(sName);

            for (String a : nfa.alphabet) {
                Set<String> moved = new TreeSet<>();
                for (String s : S) moved.addAll(nfa.move(s, a));
                Set<String> T = new TreeSet<>(nfa.epsilonClosure(moved));
                String tName = name.get(T);
                if (tName == null) {
                    tName = "D" + name.size();
                    name.put(T, tName);
                    work.add(T);
                }
                delta.computeIfAbsent(sName, k -> new LinkedHashMap<>()).put(a, tName);
            }
        }

        // emit .dfa text and reuse Module 10's loader
        StringBuilder sb = new StringBuilder("states:");
        for (String n : name.values()) sb.append(" ").append(n);
        sb.append("\nalphabet:");
        for (String a : nfa.alphabet) sb.append(" ").append(a);
        sb.append("\nstart: D0");
        sb.append("\naccept:");
        for (String n : accepting) sb.append(" ").append(n);
        sb.append("\n");
        for (var e : delta.entrySet())
            for (var t : e.getValue().entrySet())
                sb.append(e.getKey()).append(" ").append(t.getKey()).append(" ").append(t.getValue()).append("\n");
        return Dfa.parse(sb.toString());
    }

    /** The legend: which set of NFA states each DFA state name stands for. */
    public static Map<String, Set<String>> legend(Nfa nfa) {
        Map<Set<String>, String> name = new LinkedHashMap<>();
        Deque<Set<String>> work = new ArrayDeque<>();
        Set<String> startSet = nfa.epsilonClosure(Set.of(nfa.start));
        name.put(startSet, "D0");
        work.add(startSet);
        while (!work.isEmpty()) {
            Set<String> S = work.poll();
            for (String a : nfa.alphabet) {
                Set<String> moved = new TreeSet<>();
                for (String s : S) moved.addAll(nfa.move(s, a));
                Set<String> T = new TreeSet<>(nfa.epsilonClosure(moved));
                if (!name.containsKey(T)) { name.put(T, "D" + name.size()); work.add(T); }
            }
        }
        Map<String, Set<String>> out = new LinkedHashMap<>();
        for (var e : name.entrySet()) out.put(e.getValue(), e.getKey());
        return out;
    }

    /**
     * An NFA over {a,b} accepting "the k-th symbol from the end is a".
     * It has k+1 states; its minimal DFA has 2^k.
     */
    public static Nfa kthFromEndNfa(int k) {
        StringBuilder sb = new StringBuilder("states:");
        for (int i = 0; i <= k; i++) sb.append(" s").append(i);
        sb.append("\nalphabet: a b\nstart: s0\naccept: s").append(k).append("\n");
        sb.append("s0 a s0\ns0 b s0\n");          // loop on anything
        sb.append("s0 a s1\n");                    // guess: this 'a' is k-th from the end
        for (int i = 1; i < k; i++) {
            sb.append("s").append(i).append(" a s").append(i + 1).append("\n");
            sb.append("s").append(i).append(" b s").append(i + 1).append("\n");
        }
        return Nfa.parse(sb.toString());
    }
}
