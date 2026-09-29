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
 * Module 11 — a nondeterministic finite automaton, with epsilon transitions.
 *
 * Two freedoms a DFA doesn't have:
 *   1. delta(q, a) is a SET of states — zero, one, or many
 *   2. epsilon transitions move without consuming any input
 *
 * A string is accepted if SOME path through the choices ends in an accepting
 * state. The simulator tracks the whole SET of states the NFA could be in.
 *
 * The key operation is epsilon-closure: given a set of states, add every state
 * reachable from it by epsilon transitions alone. Run it before the first
 * symbol and after every symbol.
 *
 * This module proves epsilon adds no power: removeEpsilon() produces an
 * equivalent NFA with no epsilon transitions at all. (Module 13 removes the
 * nondeterminism.)
 *
 * File format: like the DFA format, but 'epsilon' is a legal symbol and a state
 * may have several transition lines for the same symbol.
 */
public final class Nfa {

    public static final String EPS = "epsilon";

    final Set<String> states;
    final Set<String> alphabet;                          // does NOT include epsilon
    final Map<String, Map<String, Set<String>>> delta;   // from -> (symbol -> {to})
    final String start;
    final Set<String> accept;

    private Nfa(Set<String> states, Set<String> alphabet,
               Map<String, Map<String, Set<String>>> delta, String start, Set<String> accept) {
        this.states = states;
        this.alphabet = alphabet;
        this.delta = delta;
        this.start = start;
        this.accept = accept;
    }

    Set<String> move(String state, String symbol) {
        return delta.getOrDefault(state, Map.of()).getOrDefault(symbol, Set.of());
    }

    /** All states reachable from `set` using only epsilon transitions (includes `set`). */
    Set<String> epsilonClosure(Set<String> set) {
        Set<String> closure = new LinkedHashSet<>(set);
        Deque<String> work = new ArrayDeque<>(set);
        while (!work.isEmpty()) {
            String s = work.poll();
            for (String t : move(s, EPS))
                if (closure.add(t)) work.add(t);
        }
        return closure;
    }

    // ── simulation ──

    List<Set<String>> trace(List<String> input) {
        List<Set<String>> out = new ArrayList<>();
        Set<String> cur = epsilonClosure(Set.of(start));
        out.add(cur);
        for (String sym : input) {
            Set<String> moved = new LinkedHashSet<>();
            for (String s : cur) moved.addAll(move(s, sym));
            cur = epsilonClosure(moved);
            out.add(cur);
        }
        return out;
    }

    boolean accepts(List<String> input) {
        Set<String> cur = epsilonClosure(Set.of(start));
        for (String sym : input) {
            if (!alphabet.contains(sym)) return false;
            Set<String> moved = new LinkedHashSet<>();
            for (String s : cur) moved.addAll(move(s, sym));
            cur = epsilonClosure(moved);
            if (cur.isEmpty()) return false;
        }
        return !intersect(cur, accept).isEmpty();
    }

    boolean acceptsString(String w) {
        List<String> cs = new ArrayList<>();
        for (int i = 0; i < w.length(); i++) cs.add(String.valueOf(w.charAt(i)));
        return accepts(cs);
    }

    List<String> language(int maxLen) {
        List<String> out = new ArrayList<>();
        Deque<String> q = new ArrayDeque<>();
        q.add("");
        while (!q.isEmpty()) {
            String s = q.poll();
            if (acceptsString(s)) out.add(s.isEmpty() ? "epsilon" : s);
            if (s.length() < maxLen) for (String sym : alphabet) q.add(s + sym);
        }
        out.sort((x, y) -> {
            int lx = x.equals("epsilon") ? 0 : x.length(), ly = y.equals("epsilon") ? 0 : y.length();
            return lx != ly ? Integer.compare(lx, ly) : x.compareTo(y);
        });
        return out;
    }

    // ── epsilon elimination: an equivalent NFA with no epsilon transitions ──

    Nfa removeEpsilon() {
        Map<String, Map<String, Set<String>>> nd = new LinkedHashMap<>();
        Set<String> nAccept = new LinkedHashSet<>();

        for (String q : states) {
            Set<String> cq = epsilonClosure(Set.of(q));
            // q is accepting in the new NFA if its epsilon-closure hits an old accepting state
            if (!intersect(cq, accept).isEmpty()) nAccept.add(q);
            for (String a : alphabet) {
                // new delta(q, a) = epsilonClosure( union of delta(p, a) for p in epsilonClosure(q) )
                Set<String> moved = new LinkedHashSet<>();
                for (String p : cq) moved.addAll(move(p, a));
                Set<String> target = epsilonClosure(moved);
                if (!target.isEmpty())
                    nd.computeIfAbsent(q, k -> new LinkedHashMap<>()).put(a, target);
            }
        }
        return new Nfa(new LinkedHashSet<>(states), new LinkedHashSet<>(alphabet), nd, start, nAccept);
    }

    boolean hasEpsilon() {
        for (var m : delta.values()) if (m.containsKey(EPS)) return true;
        return false;
    }

    // ── loader ──

    static Nfa parse(String text) {
        Set<String> states = new LinkedHashSet<>(), alphabet = new LinkedHashSet<>(), accept = new LinkedHashSet<>();
        String start = null;
        Map<String, Map<String, Set<String>>> delta = new LinkedHashMap<>();

        for (String raw : text.split("\r?\n")) {
            int h = raw.indexOf('#');
            String line = (h >= 0 ? raw.substring(0, h) : raw).strip();
            if (line.isEmpty()) continue;
            if (line.startsWith("states:")) for (String s : rest(line)) states.add(s);
            else if (line.startsWith("alphabet:")) for (String s : rest(line)) alphabet.add(s);
            else if (line.startsWith("start:")) start = rest(line)[0];
            else if (line.startsWith("accept:")) for (String s : rest(line)) accept.add(s);
            else {
                String[] p = line.split("\\s+");
                if (p.length != 3) throw new IllegalArgumentException("bad transition line: " + line);
                delta.computeIfAbsent(p[0], k -> new LinkedHashMap<>())
                     .computeIfAbsent(p[1], k -> new LinkedHashSet<>()).add(p[2]);
            }
        }
        if (start == null || states.isEmpty() || alphabet.isEmpty())
            throw new IllegalArgumentException("NFA needs states, alphabet, and a start state");
        if (!states.contains(start)) throw new IllegalArgumentException("start state not in states");
        for (String a : accept) if (!states.contains(a)) throw new IllegalArgumentException("accept '" + a + "' not in states");
        for (var e : delta.entrySet())
            for (var bySym : e.getValue().entrySet())
                for (String to : bySym.getValue())
                    if (!states.contains(to)) throw new IllegalArgumentException("transition to unknown state '" + to + "'");

        return new Nfa(states, alphabet, delta, start, accept);
    }

    private static String[] rest(String line) {
        return line.substring(line.indexOf(':') + 1).strip().split("\\s+");
    }

    // ── helpers ──

    static Set<String> intersect(Set<String> a, Set<String> b) {
        Set<String> r = new LinkedHashSet<>(a);
        r.retainAll(b);
        return r;
    }

    static String showSet(Set<String> s) {
        return s.isEmpty() ? "{}" : "{" + String.join(",", new TreeSet<>(s)) + "}";
    }
}
