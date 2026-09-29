import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

/**
 * Module 10 — a deterministic finite automaton.
 *
 *   Q      finite set of states
 *   Sigma  the input alphabet
 *   delta  Q x Sigma -> Q      TOTAL: defined for every (state, symbol) pair
 *   q0     the start state
 *   F      the accepting states
 *
 * Run: start at q0, apply delta once per input symbol, accept iff the final
 * state is in F. The language accepted is exactly a REGULAR language — no more,
 * no less (Module 7's pumping lemma is the "no more" half).
 *
 * File format (see fixtures/*.dfa):
 *   states: S A B
 *   alphabet: a b
 *   start: S
 *   accept: A B
 *   S a A            <- one transition per line: fromState symbol toState
 * Any (state, symbol) pair with no line gets a transition to an implicit
 * DEAD state, so delta is always total.
 */
public final class Dfa {

    static final String DEAD = "<dead>";

    final Set<String> states;                       // insertion order
    final Set<String> alphabet;
    final Map<String, Map<String, String>> delta;   // from -> (symbol -> to)
    final String start;
    final Set<String> accept;

    private Dfa(Set<String> states, Set<String> alphabet,
                Map<String, Map<String, String>> delta, String start, Set<String> accept) {
        this.states = states;
        this.alphabet = alphabet;
        this.delta = delta;
        this.start = start;
        this.accept = accept;
    }

    // ── simulation ──

    String step(String state, String symbol) {
        return delta.getOrDefault(state, Map.of()).getOrDefault(symbol, DEAD);
    }

    /** The sequence of states visited, starting with the start state. */
    List<String> trace(List<String> input) {
        List<String> out = new ArrayList<>();
        String s = start;
        out.add(s);
        for (String sym : input) {
            s = step(s, sym);
            out.add(s);
        }
        return out;
    }

    boolean accepts(List<String> input) {
        String s = start;
        for (String sym : input) {
            if (!alphabet.contains(sym)) return false;   // symbol outside Sigma
            s = step(s, sym);
        }
        return accept.contains(s);
    }

    /** Every accepted string of length <= maxLen, in (length, lexicographic) order. */
    List<String> language(int maxLen) {
        List<String> syms = new ArrayList<>(alphabet);
        List<String> out = new ArrayList<>();
        Deque<String> q = new ArrayDeque<>();       // every string over the alphabet, breadth-first
        q.add("");
        while (!q.isEmpty()) {
            String cur = q.poll();
            if (acceptsString(cur)) out.add(cur.isEmpty() ? "epsilon" : cur);
            if (cur.length() < maxLen) for (String sym : syms) q.add(cur + sym);
        }
        out.sort((x, y) -> {
            int lx = x.equals("epsilon") ? 0 : x.length();
            int ly = y.equals("epsilon") ? 0 : y.length();
            return lx != ly ? Integer.compare(lx, ly) : x.compareTo(y);
        });
        return out;
    }

    boolean acceptsString(String w) {
        String state = start;
        for (int i = 0; i < w.length(); i++) {
            String sym = String.valueOf(w.charAt(i));
            if (!alphabet.contains(sym)) return false;
            state = step(state, sym);
        }
        return accept.contains(state);
    }

    /**
     * Pumping in action: run the DFA on `symbol` repeated, and report the first
     * pair of prefix lengths (i, j) after which it is in the SAME state — the
     * pigeonhole repeat that makes counting impossible.
     */
    int[] repeatOn(String symbol) {
        Map<String, Integer> firstSeen = new LinkedHashMap<>();
        String s = start;
        firstSeen.put(s, 0);
        for (int len = 1; len <= states.size() + 1; len++) {
            s = step(s, symbol);
            if (firstSeen.containsKey(s)) return new int[]{firstSeen.get(s), len};
            firstSeen.put(s, len);
        }
        return new int[]{-1, -1};
    }

    // ── loader ──

    static Dfa parse(String text) {
        Set<String> states = new LinkedHashSet<>(), alphabet = new LinkedHashSet<>(), accept = new LinkedHashSet<>();
        String start = null;
        Map<String, Map<String, String>> delta = new LinkedHashMap<>();

        for (String raw : text.split("\r?\n")) {
            int h = raw.indexOf('#');
            String line = (h >= 0 ? raw.substring(0, h) : raw).strip();
            if (line.isEmpty()) continue;
            if (line.startsWith("states:"))   { for (String s : rest(line)) states.add(s); }
            else if (line.startsWith("alphabet:")) { for (String s : rest(line)) alphabet.add(s); }
            else if (line.startsWith("start:")) { start = rest(line)[0]; }
            else if (line.startsWith("accept:")) { for (String s : rest(line)) accept.add(s); }
            else {
                String[] p = line.split("\\s+");
                if (p.length != 3) throw new IllegalArgumentException("bad transition line: " + line);
                delta.computeIfAbsent(p[0], k -> new LinkedHashMap<>()).put(p[1], p[2]);
            }
        }
        if (start == null || states.isEmpty())
            throw new IllegalArgumentException("DFA needs states and a start state");
        if (!states.contains(start)) throw new IllegalArgumentException("start state not in states");
        for (String a : accept)
            if (!states.contains(a)) throw new IllegalArgumentException("accept state '" + a + "' not in states");

        // complete delta to total, routing missing pairs to DEAD
        boolean needDead = false;
        for (String s : states)
            for (String sym : alphabet)
                if (!delta.getOrDefault(s, Map.of()).containsKey(sym)) needDead = true;
        if (needDead) {
            states.add(DEAD);
            for (String s : states)
                for (String sym : alphabet)
                    delta.computeIfAbsent(s, k -> new LinkedHashMap<>()).putIfAbsent(sym, DEAD);
        }
        // validate transition targets
        for (var e : delta.entrySet())
            for (String to : e.getValue().values())
                if (!states.contains(to)) throw new IllegalArgumentException("transition to unknown state '" + to + "'");

        return new Dfa(states, alphabet, delta, start, accept);
    }

    private static String[] rest(String line) {
        String r = line.substring(line.indexOf(':') + 1).strip();
        return r.isEmpty() ? new String[0] : r.split("\\s+");
    }

    // ── rendering ──

    String transitionTable() {
        List<String> syms = new ArrayList<>(alphabet);
        StringBuilder sb = new StringBuilder();
        sb.append(String.format("  %-10s", "state"));
        for (String s : syms) sb.append(String.format(" %-10s", s));
        sb.append("\n");
        for (String st : states) {
            String tag = (st.equals(start) ? "->" : "  ") + (accept.contains(st) ? "*" : " ");
            sb.append(String.format("  %s%-8s", tag, st));
            for (String s : syms) sb.append(String.format(" %-10s", step(st, s)));
            sb.append("\n");
        }
        return sb.toString().stripTrailing();
    }
}
