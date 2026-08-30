import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Module 29 — a backtracking recursive-descent recognizer that COUNTS.
 *
 * `deriveSym` tries every production of a nonterminal in order, following all of
 * them (no lookahead pruning). It returns, for each end position, the number of
 * distinct ways to derive the consumed span — so the total for
 * position == input.length is the number of parse trees.
 *
 * `entries` counts production attempts: the raw work. With no memoization this
 * is exponential for a grammar whose alternatives overlap (see equiv.grammar).
 * Packrat parsing (Appendix X2) memoizes (nonterminal, position) to make it
 * linear.
 *
 * Grammars must be free of left recursion (checked by the caller) or this
 * recurses forever.
 */
public final class Backtrack {

    public static final class Capped extends RuntimeException {}

    public record Result(long parseCount, long entries, boolean capped) {}

    private final LeftRec.G g;
    private final long cap;
    private long entries;

    public Backtrack(LeftRec.G g) { this(g, 20_000_000L); }
    public Backtrack(LeftRec.G g, long cap) { this.g = g; this.cap = cap; }

    public Result parse(List<String> input) {
        entries = 0;
        try {
            Map<Integer, Long> ends = deriveSym(g.start, 0, input);
            long full = ends.getOrDefault(input.size(), 0L);
            return new Result(full, entries, false);
        } catch (Capped c) {
            return new Result(-1, entries, true);
        }
    }

    private Map<Integer, Long> deriveSym(String sym, int pos, List<String> input) {
        if (!g.isNT(sym)) {
            Map<Integer, Long> out = new HashMap<>();
            if (pos < input.size() && input.get(pos).equals(sym)) out.put(pos + 1, 1L);
            return out;
        }
        Map<Integer, Long> out = new HashMap<>();
        for (List<String> rhs : g.prods.get(sym)) {
            if (++entries > cap) throw new Capped();
            Map<Integer, Long> r = deriveSeq(rhs, pos, input);
            for (var e : r.entrySet()) out.merge(e.getKey(), e.getValue(), Long::sum);
        }
        return out;
    }

    private Map<Integer, Long> deriveSeq(List<String> seq, int pos, List<String> input) {
        Map<Integer, Long> cur = new HashMap<>();
        cur.put(pos, 1L);
        for (String sym : seq) {
            Map<Integer, Long> nxt = new HashMap<>();
            for (var pe : cur.entrySet()) {
                Map<Integer, Long> r = deriveSym(sym, pe.getKey(), input);
                for (var re : r.entrySet())
                    nxt.merge(re.getKey(), pe.getValue() * re.getValue(), Long::sum);
            }
            cur = nxt;
            if (cur.isEmpty()) break;
        }
        return cur;
    }
}
