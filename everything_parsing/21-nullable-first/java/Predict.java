import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Module 20 — The Predictive Parsing Condition.
 *
 * Backtrack-free recursive descent works iff, for every nonterminal A with
 * productions  A -> a1 | a2 | ... | ak , the PREDICT sets are pairwise disjoint:
 *
 *     PREDICT(A -> ai) = FIRST(ai)                              if ai not nullable
 *                      = (FIRST(ai) \ {epsilon}) U FOLLOW(A)    if ai nullable
 *
 * Current token in exactly one PREDICT set  ->  the parser knows which
 * alternative to take, no guessing. In two  ->  the grammar is not LL(1) on that
 * token, and the pair of productions is the witness.
 *
 * FIRST / FOLLOW here are computed by iterating the standard rules to a fixed
 * point — enough to state and check the condition. Module 21 proves FIRST is a
 * *least* fixed point (and pins down `nullable`); Module 22 does the same for
 * FOLLOW and the `$` end-marker. This module is about the CONDITION.
 */
public final class Predict {

    public static final String EPS = "epsilon";
    public static final String END = "$";

    public final Grammar g;
    private final Map<String, Set<String>> first = new HashMap<>();
    private final Map<String, Set<String>> follow = new HashMap<>();

    public Predict(Grammar g) {
        this.g = g;
        computeFirst();
        computeFollow();
    }

    // ─────────────────────────────────────────── FIRST (fixed point)

    private void computeFirst() {
        for (String nt : g.nonterminals) first.put(nt, new LinkedHashSet<>());
        boolean changed = true;
        while (changed) {
            changed = false;
            for (Grammar.Production p : g.productions) {
                Set<String> target = first.get(p.lhs());
                int before = target.size();
                target.addAll(firstOfSeq(p.rhs()));
                if (target.size() != before) changed = true;
            }
        }
    }

    /** FIRST of a symbol: terminals map to themselves; nonterminals look up the table. */
    public Set<String> firstOf(String sym) {
        if (sym.equals(EPS)) return new LinkedHashSet<>(List.of(EPS));
        if (!g.isNonterminal(sym)) return new LinkedHashSet<>(List.of(sym));
        return new LinkedHashSet<>(first.getOrDefault(sym, Set.of()));
    }

    /** FIRST of a sequence. Contains EPS iff every symbol in it is nullable. */
    public Set<String> firstOfSeq(List<String> seq) {
        Set<String> out = new LinkedHashSet<>();
        boolean allNullable = true;
        for (String sym : seq) {
            Set<String> f = firstOf(sym);
            for (String x : f) if (!x.equals(EPS)) out.add(x);
            if (!f.contains(EPS)) { allNullable = false; break; }
        }
        if (allNullable) out.add(EPS);
        return out;
    }

    public boolean nullable(List<String> seq) { return firstOfSeq(seq).contains(EPS); }

    // ─────────────────────────────────────────── FOLLOW (fixed point)

    private void computeFollow() {
        for (String nt : g.nonterminals) follow.put(nt, new LinkedHashSet<>());
        follow.get(g.start).add(END);

        boolean changed = true;
        while (changed) {
            changed = false;
            for (Grammar.Production p : g.productions) {
                List<String> rhs = p.rhs();
                for (int i = 0; i < rhs.size(); i++) {
                    String b = rhs.get(i);
                    if (!g.isNonterminal(b)) continue;
                    List<String> beta = rhs.subList(i + 1, rhs.size());
                    Set<String> fb = firstOfSeq(beta);

                    Set<String> target = follow.get(b);
                    int before = target.size();
                    for (String x : fb) if (!x.equals(EPS)) target.add(x);
                    if (fb.contains(EPS)) target.addAll(follow.get(p.lhs()));
                    if (target.size() != before) changed = true;
                }
            }
        }
    }

    public Set<String> followOf(String nt) {
        return new LinkedHashSet<>(follow.getOrDefault(nt, Set.of()));
    }

    // ─────────────────────────────────────────── PREDICT + the LL(1) verdict

    public Set<String> predict(Grammar.Production p) {
        Set<String> out = new LinkedHashSet<>();
        Set<String> f = firstOfSeq(p.rhs());
        for (String x : f) if (!x.equals(EPS)) out.add(x);
        if (f.contains(EPS)) out.addAll(followOf(p.lhs()));
        return out;
    }

    public record Conflict(String nonterminal, String token,
                           Grammar.Production a, Grammar.Production b) {
        @Override public String toString() {
            return "nonterminal " + nonterminal + " on token '" + token + "':  ["
                 + a + "]  vs  [" + b + "]";
        }
    }

    /** Every pair of same-nonterminal productions whose PREDICT sets overlap. */
    public List<Conflict> conflicts() {
        List<Conflict> out = new ArrayList<>();
        for (String nt : g.nonterminals) {
            List<Grammar.Production> ps = g.productionsFor(nt);
            List<Set<String>> preds = new ArrayList<>();
            for (Grammar.Production p : ps) preds.add(predict(p));
            for (int i = 0; i < ps.size(); i++)
                for (int j = i + 1; j < ps.size(); j++) {
                    Set<String> both = new LinkedHashSet<>(preds.get(i));
                    both.retainAll(preds.get(j));
                    for (String tok : both) out.add(new Conflict(nt, tok, ps.get(i), ps.get(j)));
                }
        }
        return out;
    }

    public boolean isLL1() { return conflicts().isEmpty(); }

    // ─────────────────────────────────────────── rendering

    public static String setStr(Set<String> s) {
        List<String> items = new ArrayList<>(s);
        items.sort((a, b) -> {
            boolean ae = a.equals(EPS) || a.equals(END);
            boolean be = b.equals(EPS) || b.equals(END);
            if (ae != be) return ae ? 1 : -1;
            return a.compareTo(b);
        });
        return "{ " + String.join(", ", items) + " }";
    }
}
