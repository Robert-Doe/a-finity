import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

/**
 * Module 22 — FOLLOW as a LEAST FIXED POINT, given FIRST.
 *
 * FOLLOW(A) = the terminals that can appear immediately after A in some
 * sentential form, plus $ if A can be the last thing in the input.
 *
 * Rules (iterate to convergence):
 *   1.  $  in  FOLLOW(start)
 *   2.  for every production  B -> alpha A beta :
 *          FIRST(beta) \ {epsilon}   subset of   FOLLOW(A)
 *   3.  for every production  B -> alpha A beta  where beta is nullable
 *       (this includes beta empty, i.e. A is the last symbol):
 *          FOLLOW(B)   subset of   FOLLOW(A)
 *
 * Rule 3 is what makes FOLLOW circular: FOLLOW(A) can depend on FOLLOW(B) which
 * can depend back on FOLLOW(A). Same fix as Module 21 — start empty, add only,
 * stop when a pass changes nothing; the result is the least fixed point.
 *
 * FIRST comes from Module 21's FirstSets, used here as a fixed input.
 */
public final class FollowSets {

    public static final String EPS = "epsilon";
    public static final String END = "$";

    public final Grammar g;
    public final FirstSets first;
    public final Map<String, Set<String>> follow;
    public final List<Map<String, Set<String>>> rounds = new ArrayList<>();

    public FollowSets(Grammar g) {
        this.g = g;
        this.first = new FirstSets(g);
        this.follow = compute();
    }

    private Map<String, Set<String>> compute() {
        Map<String, Set<String>> fol = new LinkedHashMap<>();
        for (String nt : g.nonterminals) fol.put(nt, new LinkedHashSet<>());
        fol.get(g.start).add(END);                       // rule 1
        rounds.add(snapshot(fol));

        boolean changed = true;
        while (changed) {
            changed = false;
            for (Grammar.Production p : g.productions) {
                List<String> rhs = p.rhs();
                for (int i = 0; i < rhs.size(); i++) {
                    String a = rhs.get(i);
                    if (!g.isNonterminal(a)) continue;
                    List<String> beta = rhs.subList(i + 1, rhs.size());
                    Set<String> fb = first.firstOfSeq(beta);

                    Set<String> target = fol.get(a);
                    int before = target.size();
                    for (String x : fb) if (!x.equals(EPS)) target.add(x);   // rule 2
                    if (fb.contains(EPS)) target.addAll(fol.get(p.lhs()));   // rule 3
                    if (target.size() != before) changed = true;
                }
            }
            rounds.add(snapshot(fol));
        }
        return fol;
    }

    public Set<String> followOf(String nt) {
        return new LinkedHashSet<>(follow.getOrDefault(nt, Set.of()));
    }

    /** One more pass; true iff nothing changes. */
    public boolean isStable() {
        Map<String, Set<String>> f = snapshot(follow);
        for (Grammar.Production p : g.productions) {
            List<String> rhs = p.rhs();
            for (int i = 0; i < rhs.size(); i++) {
                String a = rhs.get(i);
                if (!g.isNonterminal(a)) continue;
                List<String> beta = rhs.subList(i + 1, rhs.size());
                Set<String> fb = first.firstOfSeq(beta);
                for (String x : fb) if (!x.equals(EPS)) f.get(a).add(x);
                if (fb.contains(EPS)) f.get(a).addAll(f.get(p.lhs()));
            }
        }
        for (String nt : g.nonterminals)
            if (!f.get(nt).equals(follow.get(nt))) return false;
        return true;
    }

    private static Map<String, Set<String>> snapshot(Map<String, Set<String>> f) {
        Map<String, Set<String>> s = new LinkedHashMap<>();
        for (var e : f.entrySet()) s.put(e.getKey(), new LinkedHashSet<>(e.getValue()));
        return s;
    }

    public static String setStr(Set<String> s) {
        TreeSet<String> t = new TreeSet<>((a, b) -> {
            boolean ae = a.equals(END), be = b.equals(END);
            if (ae != be) return ae ? 1 : -1;      // $ sorts last
            return a.compareTo(b);
        });
        t.addAll(s);
        return "{ " + String.join(" ", t) + " }";
    }
}
