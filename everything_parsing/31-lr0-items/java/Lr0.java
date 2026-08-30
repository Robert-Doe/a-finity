import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

/**
 * Module 31 — LR(0) items and the canonical collection.
 *
 * An LR(0) item is a production with a dot: E -> E . plus T . The dot marks how
 * much of the RHS has been seen.
 *
 *   CLOSURE(I): while some item  A -> a . B b  is in I and B is a nonterminal,
 *               add  B -> . g  for every production B -> g.
 *   GOTO(I, X): CLOSURE of { A -> a X . b : A -> a . X b in I }.
 *
 * Start from CLOSURE({ S' -> . S }) on the augmented grammar; apply GOTO for
 * every grammar symbol; repeat to a fixed point. The resulting item sets are the
 * STATES of a DFA, and that DFA recognizes exactly the viable prefixes
 * (Module 30).
 *
 * LR(0) parsing decisions come straight from the items: a state with
 * A -> a .  (dot at the end) says "reduce by A -> a"; a state with an outgoing
 * edge on a terminal says "shift". A state with both, or with two complete
 * items, is an LR(0) CONFLICT.
 */
public final class Lr0 {

    /** item = production index into `prods` + dot position (0..rhs.size()). */
    public record Item(int prod, int dot) {}

    public final List<Grammar.Production> prods = new ArrayList<>();   // augmented; prod 0 = S' -> S
    public final String startAug;
    public final Set<String> nonterminals = new LinkedHashSet<>();
    public final List<Set<Item>> states = new ArrayList<>();
    public final Map<Integer, Map<String, Integer>> trans = new LinkedHashMap<>();

    public Lr0(Grammar g) {
        this.startAug = g.start + "'";
        prods.add(new Grammar.Production(0, startAug, List.of(g.start)));
        for (Grammar.Production p : g.productions)
            prods.add(new Grammar.Production(prods.size(), p.lhs(), p.rhs()));
        nonterminals.add(startAug);
        nonterminals.addAll(g.nonterminals);
        build();
    }

    boolean isNT(String s) { return nonterminals.contains(s); }

    List<String> rhsOf(int prod) { return prods.get(prod).rhs(); }
    String lhsOf(int prod) { return prods.get(prod).lhs(); }
    String afterDot(Item it) {
        List<String> r = rhsOf(it.prod());
        return it.dot() < r.size() ? r.get(it.dot()) : null;
    }

    Set<Item> closure(Set<Item> kernel) {
        Set<Item> I = new LinkedHashSet<>(kernel);
        boolean changed = true;
        while (changed) {
            changed = false;
            for (Item it : new ArrayList<>(I)) {
                String B = afterDot(it);
                if (B != null && isNT(B))
                    for (int p = 0; p < prods.size(); p++)
                        if (lhsOf(p).equals(B) && I.add(new Item(p, 0))) changed = true;
            }
        }
        return I;
    }

    Set<Item> gotoSet(Set<Item> I, String X) {
        Set<Item> kernel = new LinkedHashSet<>();
        for (Item it : I)
            if (X.equals(afterDot(it))) kernel.add(new Item(it.prod(), it.dot() + 1));
        return kernel.isEmpty() ? kernel : closure(kernel);
    }

    void build() {
        Set<Item> start = closure(Set.of(new Item(0, 0)));
        states.add(start);
        Map<String, Integer> index = new LinkedHashMap<>();
        index.put(key(start), 0);

        List<Integer> work = new ArrayList<>(List.of(0));
        while (!work.isEmpty()) {
            int si = work.remove(0);
            Set<Item> I = states.get(si);

            Set<String> symbols = new LinkedHashSet<>();
            for (Item it : I) { String s = afterDot(it); if (s != null) symbols.add(s); }

            for (String X : symbols) {
                Set<Item> J = gotoSet(I, X);
                if (J.isEmpty()) continue;
                Integer target = index.get(key(J));
                if (target == null) {
                    target = states.size();
                    states.add(J);
                    index.put(key(J), target);
                    work.add(target);
                }
                trans.computeIfAbsent(si, k -> new TreeMap<>()).put(X, target);
            }
        }
    }

    // ─────────────────────────────────────────── LR(0) analysis

    public record Conflict(int state, String kind, String detail) {}

    public List<Conflict> conflicts() {
        List<Conflict> out = new ArrayList<>();
        for (int s = 0; s < states.size(); s++) {
            List<Integer> complete = new ArrayList<>();
            for (Item it : states.get(s))
                if (afterDot(it) == null && it.prod() != 0) complete.add(it.prod());
            boolean canShift = trans.getOrDefault(s, Map.of()).keySet().stream().anyMatch(x -> !isNT(x));

            if (complete.size() >= 2)
                out.add(new Conflict(s, "reduce/reduce",
                    "productions " + complete + " both reduce here"));
            if (!complete.isEmpty() && canShift)
                out.add(new Conflict(s, "shift/reduce",
                    "reduce by " + itemStr(new Item(complete.get(0), rhsOf(complete.get(0)).size()))
                    + " vs shift " + trans.get(s).keySet().stream().filter(x -> !isNT(x)).toList()));
        }
        return out;
    }

    public boolean isLR0() { return conflicts().isEmpty(); }

    // ─────────────────────────────────────────── rendering

    public String itemStr(Item it) {
        List<String> r = rhsOf(it.prod());
        StringBuilder sb = new StringBuilder(lhsOf(it.prod())).append(" ->");
        for (int i = 0; i <= r.size(); i++) {
            if (i == it.dot()) sb.append(" .");
            if (i < r.size()) sb.append(' ').append(r.get(i));
        }
        if (r.isEmpty() && it.dot() == 0) sb.append(" .");
        return sb.toString();
    }

    public String render() {
        StringBuilder sb = new StringBuilder();
        sb.append("augmented grammar:\n");
        for (Grammar.Production p : prods)
            sb.append("  (").append(p.index()).append(") ").append(p.lhs()).append(" -> ")
              .append(p.rhs().isEmpty() ? "epsilon" : String.join(" ", p.rhs())).append('\n');
        sb.append('\n');
        for (int s = 0; s < states.size(); s++) {
            sb.append("I").append(s).append(":\n");
            for (Item it : states.get(s)) sb.append("   ").append(itemStr(it)).append('\n');
            var t = trans.getOrDefault(s, Map.of());
            if (!t.isEmpty()) {
                sb.append("   ---");
                for (var e : t.entrySet()) sb.append("  ").append(e.getKey()).append(" -> I").append(e.getValue());
                sb.append('\n');
            }
        }
        return sb.toString();
    }

    private String key(Set<Item> I) {
        List<String> ks = new ArrayList<>();
        for (Item it : I) ks.add(it.prod() + "." + it.dot());
        java.util.Collections.sort(ks);
        return String.join(",", ks);
    }
}
