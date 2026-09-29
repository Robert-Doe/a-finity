import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

/**
 * Module 24 — Left Factoring.
 *
 * When two alternatives of a rule begin with the same symbols, one token of
 * lookahead can't tell them apart. Factor the shared prefix out:
 *
 *     A -> a b1 | a b2 | ... | a bk | rest
 *   becomes
 *     A  -> a A' | rest
 *     A' -> b1 | b2 | ... | bk        (an empty bi becomes A' -> epsilon)
 *
 * Repeat until no nonterminal has two alternatives sharing a first symbol. The
 * language is unchanged; every string  a bi ...  is still generated, now via
 * the stem A'.
 *
 * Operates on `LeftRec.G` (shared mutable grammar from Module 23).
 */
public final class LeftFactor {

    public static LeftRec.G factor(Grammar src) {
        LeftRec.G g = LeftRec.G.from(src);
        boolean changed = true;
        while (changed) {
            changed = false;
            for (String nt : new ArrayList<>(g.order)) {
                if (factorOne(g, nt)) { changed = true; break; }   // restart the pass after any change
            }
        }
        return g;
    }

    /** Factor the single longest prefix shared by >= 2 of nt's alternatives. Returns true if it did. */
    private static boolean factorOne(LeftRec.G g, String nt) {
        List<List<String>> alts = g.prods.get(nt);
        List<String> prefix = longestSharedPrefix(alts);
        if (prefix.isEmpty()) return false;

        List<List<String>> withP = new ArrayList<>();
        List<List<String>> rest = new ArrayList<>();
        for (List<String> a : alts) {
            if (startsWith(a, prefix)) withP.add(a);
            else rest.add(a);
        }

        String ap = fresh(g, nt);
        List<List<String>> newNt = new ArrayList<>(rest);
        List<String> stem = new ArrayList<>(prefix);
        stem.add(ap);
        newNt.add(stem);

        List<List<String>> newAp = new ArrayList<>();
        for (List<String> a : withP)
            newAp.add(new ArrayList<>(a.subList(prefix.size(), a.size())));   // suffix; empty -> epsilon

        g.prods.put(nt, dedupe(newNt));
        g.prods.put(ap, dedupe(newAp));
        g.order.add(g.order.indexOf(nt) + 1, ap);
        return true;
    }

    /** The longest symbol sequence (length >= 1) that prefixes at least two alternatives. */
    static List<String> longestSharedPrefix(List<List<String>> alts) {
        List<String> best = List.of();
        for (int i = 0; i < alts.size(); i++)
            for (int j = i + 1; j < alts.size(); j++) {
                List<String> cp = commonPrefix(alts.get(i), alts.get(j));
                if (cp.size() > best.size()) best = cp;
            }
        return best;
    }

    private static List<String> commonPrefix(List<String> a, List<String> b) {
        int n = 0;
        while (n < a.size() && n < b.size() && a.get(n).equals(b.get(n))) n++;
        return new ArrayList<>(a.subList(0, n));
    }

    private static boolean startsWith(List<String> a, List<String> p) {
        if (a.size() < p.size()) return false;
        for (int i = 0; i < p.size(); i++) if (!a.get(i).equals(p.get(i))) return false;
        return true;
    }

    private static String fresh(LeftRec.G g, String base) {
        String name = base + "'";
        while (g.prods.containsKey(name)) name += "'";
        return name;
    }

    private static List<List<String>> dedupe(List<List<String>> in) {
        Set<String> seen = new LinkedHashSet<>();
        List<List<String>> out = new ArrayList<>();
        for (List<String> r : in) if (seen.add(String.join("", r))) out.add(r);
        return out;
    }

    /** True iff some nonterminal still has two alternatives sharing a first symbol. */
    public static boolean needsFactoring(LeftRec.G g) {
        for (String nt : g.order) {
            Set<String> firsts = new LinkedHashSet<>();
            for (List<String> r : g.prods.get(nt)) {
                String key = r.isEmpty() ? "" : r.get(0);
                if (!firsts.add(key)) return true;
            }
        }
        return false;
    }
}
