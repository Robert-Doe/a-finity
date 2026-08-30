import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

/**
 * Module 21 — Nullable and FIRST as LEAST FIXED POINTS.
 *
 * NULLABLE is the smallest set of nonterminals N such that:
 *     A in N   if   A -> epsilon
 *     A in N   if   A -> X1 X2 ... Xk  and every Xi is in N
 * You cannot just read this off — the second rule refers to N on both sides.
 * Start from the empty set (nothing assumed nullable) and apply the rules until
 * a full pass adds nothing. Each pass can only ADD (the rules are monotone) and
 * there are finitely many nonterminals, so it stops. The result is a fixed
 * point, and because we grew from below it is the LEAST one.
 *
 * FIRST is the same shape, over sets of terminals:
 *     FIRST(A) contains  a               if  A -> a ...           (a terminal)
 *     FIRST(A) contains  FIRST(Xi)       if  A -> X1..Xi.. and X1..X(i-1) nullable
 *     FIRST(A) contains  epsilon         if  A -> X1..Xk and all Xi nullable
 *
 * Both computations record every round so `Main` can print the trace.
 */
public final class FirstSets {

    public static final String EPS = "epsilon";

    public final Grammar g;
    public final Set<String> nullable;
    public final List<Set<String>> nullableRounds = new ArrayList<>();
    public final Map<String, Set<String>> first;
    public final List<Map<String, Set<String>>> firstRounds = new ArrayList<>();

    public FirstSets(Grammar g) {
        this.g = g;
        this.nullable = computeNullable();
        this.first = computeFirst();
    }

    // ─────────────────────────────────────────── NULLABLE

    private Set<String> computeNullable() {
        Set<String> n = new LinkedHashSet<>();
        nullableRounds.add(new LinkedHashSet<>(n));         // round 0: {}
        boolean changed = true;
        while (changed) {
            changed = false;
            for (Grammar.Production p : g.productions) {
                if (n.contains(p.lhs())) continue;
                boolean allNull = true;
                for (String s : p.rhs()) {
                    if (!n.contains(s)) { allNull = false; break; }   // a terminal, or a not-yet-nullable NT
                }
                if (allNull) { n.add(p.lhs()); changed = true; }      // empty rhs => allNull stays true
            }
            nullableRounds.add(new LinkedHashSet<>(n));
        }
        return n;
    }

    public boolean nullableSeq(List<String> seq) {
        for (String s : seq) if (!nullable.contains(s)) return false;
        return true;
    }

    // ─────────────────────────────────────────── FIRST

    private Map<String, Set<String>> computeFirst() {
        Map<String, Set<String>> f = new LinkedHashMap<>();
        for (String nt : g.nonterminals) f.put(nt, new LinkedHashSet<>());
        firstRounds.add(snapshot(f));                       // round 0: all empty

        boolean changed = true;
        while (changed) {
            changed = false;
            for (Grammar.Production p : g.productions) {
                Set<String> target = f.get(p.lhs());
                int before = target.size();
                target.addAll(firstOfSeqWith(f, p.rhs()));
                if (target.size() != before) changed = true;
            }
            firstRounds.add(snapshot(f));
        }
        return f;
    }

    private Set<String> firstOfSeqWith(Map<String, Set<String>> f, List<String> seq) {
        Set<String> out = new LinkedHashSet<>();
        boolean allNullable = true;
        for (String sym : seq) {
            if (!g.isNonterminal(sym)) { out.add(sym); allNullable = false; break; }
            for (String x : f.get(sym)) if (!x.equals(EPS)) out.add(x);
            if (!nullable.contains(sym)) { allNullable = false; break; }
        }
        if (allNullable) out.add(EPS);
        return out;
    }

    /** FIRST of a symbol string, using the finished tables. */
    public Set<String> firstOfSeq(List<String> seq) { return firstOfSeqWith(first, seq); }

    public Set<String> firstOf(String sym) {
        if (!g.isNonterminal(sym)) return new LinkedHashSet<>(List.of(sym));
        return new LinkedHashSet<>(first.get(sym));
    }

    // ─────────────────────────────────────────── "is this really a fixed point?"

    /** Run one more FIRST pass; return true iff nothing changes (i.e. we are at a fixed point). */
    public boolean firstIsStable() {
        Map<String, Set<String>> f = snapshot(first);
        for (Grammar.Production p : g.productions)
            f.get(p.lhs()).addAll(firstOfSeqWith(f, p.rhs()));
        for (String nt : g.nonterminals)
            if (!f.get(nt).equals(first.get(nt))) return false;
        return true;
    }

    // ─────────────────────────────────────────── rendering

    private static Map<String, Set<String>> snapshot(Map<String, Set<String>> f) {
        Map<String, Set<String>> s = new LinkedHashMap<>();
        for (var e : f.entrySet()) s.put(e.getKey(), new LinkedHashSet<>(e.getValue()));
        return s;
    }

    public static String setStr(Set<String> s) {
        TreeSet<String> t = new TreeSet<>((a, b) -> {
            boolean ae = a.equals(EPS), be = b.equals(EPS);
            if (ae != be) return ae ? 1 : -1;
            return a.compareTo(b);
        });
        t.addAll(s);
        return "{ " + String.join(" ", t) + " }";
    }
}
