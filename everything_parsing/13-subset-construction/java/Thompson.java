import java.util.ArrayList;
import java.util.List;
import java.util.TreeSet;

/**
 * Module 12 — Thompson's construction: compile a regex tree (Module 3) into an
 * epsilon-NFA (Module 11), ONE small gadget per node.
 *
 *   Empty (∅)      s --(nothing)--> e            accepts nothing
 *   Epsilon (ε)    s --ε--> e
 *   Char c         s --c--> e
 *   Concat(l, r)   l.end --ε--> r.start ;  start = l.start,  end = r.end
 *   Union(l, r)    new s --ε--> l.start, r.start ;  l.end, r.end --ε--> new e
 *   Star(x)        new s --ε--> x.start, e ;  x.end --ε--> x.start, e
 *
 * State budget: a leaf (Char/ε/∅) costs 2; Union and Star add 2; Concat adds 0.
 * So the NFA has at most 2 * (number of regex nodes) states — and, counting the
 * regex without its parentheses, at most 2 * |regex| states.
 */
public final class Thompson {
    private Thompson() {}

    /** A partially built NFA: one entry state, one accept state, some transition lines. */
    private record Frag(String start, String end, List<String> lines) {}

    private static final class Counter {
        int n = 0;
        String fresh() { return "q" + (n++); }
    }

    public static Nfa build(Regex re) {
        Counter c = new Counter();
        Frag f = frag(re, c);

        TreeSet<String> alphabet = new TreeSet<>();
        collectAlphabet(re, alphabet);

        StringBuilder sb = new StringBuilder("states:");
        for (int i = 0; i < c.n; i++) sb.append(" q").append(i);
        sb.append("\nalphabet:");
        for (String a : alphabet) sb.append(" ").append(a);
        sb.append("\nstart: ").append(f.start);
        sb.append("\naccept: ").append(f.end).append("\n");
        for (String line : f.lines) sb.append(line).append("\n");
        return Nfa.parse(sb.toString());
    }

    /** How many states the built NFA has (without building it). */
    public static int stateCount(Regex re) {
        Counter c = new Counter();
        frag(re, c);
        return c.n;
    }

    public static int nodeCount(Regex re) {
        if (re instanceof Regex.Union u)   return 1 + nodeCount(u.l) + nodeCount(u.r);
        if (re instanceof Regex.Concat co) return 1 + nodeCount(co.l) + nodeCount(co.r);
        if (re instanceof Regex.Star s)    return 1 + nodeCount(s.x);
        return 1; // Empty, Epsilon, Char
    }

    private static Frag frag(Regex re, Counter c) {
        if (re instanceof Regex.Empty) {
            return new Frag(c.fresh(), c.fresh(), new ArrayList<>());   // no transitions at all
        }
        if (re instanceof Regex.Epsilon) {
            String s = c.fresh(), e = c.fresh();
            return new Frag(s, e, list(s + " epsilon " + e));
        }
        if (re instanceof Regex.Char ch) {
            String s = c.fresh(), e = c.fresh();
            return new Frag(s, e, list(s + " " + ch.c + " " + e));
        }
        if (re instanceof Regex.Concat co) {
            Frag l = frag(co.l, c), r = frag(co.r, c);
            List<String> lines = new ArrayList<>(l.lines);
            lines.addAll(r.lines);
            lines.add(l.end + " epsilon " + r.start);
            return new Frag(l.start, r.end, lines);          // +0 states
        }
        if (re instanceof Regex.Union u) {
            Frag l = frag(u.l, c), r = frag(u.r, c);
            String s = c.fresh(), e = c.fresh();             // +2 states
            List<String> lines = list(s + " epsilon " + l.start, s + " epsilon " + r.start);
            lines.addAll(l.lines);
            lines.addAll(r.lines);
            lines.add(l.end + " epsilon " + e);
            lines.add(r.end + " epsilon " + e);
            return new Frag(s, e, lines);
        }
        if (re instanceof Regex.Star st) {
            Frag x = frag(st.x, c);
            String s = c.fresh(), e = c.fresh();             // +2 states
            List<String> lines = list(s + " epsilon " + x.start, s + " epsilon " + e);
            lines.addAll(x.lines);
            lines.add(x.end + " epsilon " + x.start);
            lines.add(x.end + " epsilon " + e);
            return new Frag(s, e, lines);
        }
        throw new IllegalStateException();
    }

    private static void collectAlphabet(Regex re, TreeSet<String> out) {
        if (re instanceof Regex.Char ch)   out.add(String.valueOf(ch.c));
        else if (re instanceof Regex.Union u)   { collectAlphabet(u.l, out); collectAlphabet(u.r, out); }
        else if (re instanceof Regex.Concat co) { collectAlphabet(co.l, out); collectAlphabet(co.r, out); }
        else if (re instanceof Regex.Star s)    collectAlphabet(s.x, out);
    }

    private static List<String> list(String... xs) {
        List<String> l = new ArrayList<>();
        for (String x : xs) l.add(x);
        return l;
    }
}
