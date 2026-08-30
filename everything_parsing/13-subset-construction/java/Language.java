import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.Deque;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.NavigableSet;
import java.util.Set;
import java.util.TreeSet;

/**
 * Module 02 — a language is a (here: finite) set of strings, and the operations
 * on languages are set operations plus string glue.
 *
 *   Sigma      an alphabet — a finite set of symbols (here: characters)
 *   ""         the empty string, epsilon; length 0; a perfectly good string
 *   Language   any set of strings over Sigma
 *
 * Closure result this module demonstrates: for finite L1, L2, the languages
 * L1 ∪ L2, L1 · L2, and L^n are all finite and computable. L* is infinite, so
 * we compute it up to a length bound.
 *
 * Strings are stored in a canonical order — shorter first, then lexicographic —
 * so every rendering is deterministic and the Java and JS builds match.
 */
public final class Language {

    static final Comparator<String> ORDER = (x, y) ->
        x.length() != y.length() ? Integer.compare(x.length(), y.length()) : x.compareTo(y);

    final NavigableSet<String> strings;

    private Language(Set<String> s) {
        this.strings = new TreeSet<>(ORDER);
        this.strings.addAll(s);
    }

    static Language of(String... ws) {
        Set<String> s = new LinkedHashSet<>();
        for (String w : ws) s.add(w);
        return new Language(s);
    }

    static final Language EMPTY   = of();      // the empty language, ∅
    static final Language EPSILON = of("");    // { epsilon }

    int size()               { return strings.size(); }
    boolean contains(String w) { return strings.contains(w); }
    List<String> list()      { return new ArrayList<>(strings); }

    // ── the operations ──

    Language union(Language o) {
        Set<String> s = new HashSet<>(strings);
        s.addAll(o.strings);
        return new Language(s);
    }

    /** { xy : x in this, y in o } — the cartesian product, glued. */
    Language concat(Language o) {
        Set<String> s = new HashSet<>();
        for (String x : strings)
            for (String y : o.strings)
                s.add(x + y);
        return new Language(s);
    }

    /** this concatenated with itself n times; power(0) = { epsilon }. */
    Language power(int n) {
        if (n < 0) throw new IllegalArgumentException("power exponent must be >= 0");
        Language result = EPSILON;
        for (int i = 0; i < n; i++) result = result.concat(this);
        return result;
    }

    /**
     * Kleene star, bounded: every string of L* whose length is <= maxLen.
     * Breadth-first closure from { epsilon }; the seen-set makes it terminate
     * even when this language itself contains epsilon.
     */
    Language star(int maxLen) {
        Set<String> acc = new HashSet<>();
        acc.add("");
        Deque<String> work = new ArrayDeque<>();
        work.add("");
        while (!work.isEmpty()) {
            String prefix = work.poll();
            for (String s : strings) {
                String next = prefix + s;
                if (next.length() > maxLen) continue;
                if (acc.add(next)) work.add(next);
            }
        }
        return new Language(acc);
    }

    Language intersect(Language o) {
        Set<String> s = new HashSet<>(strings);
        s.retainAll(o.strings);
        return new Language(s);
    }

    Language minus(Language o) {
        Set<String> s = new HashSet<>(strings);
        s.removeAll(o.strings);
        return new Language(s);
    }

    /** Keep only strings of length <= maxLen. */
    Language truncate(int maxLen) {
        Set<String> s = new HashSet<>();
        for (String w : strings) if (w.length() <= maxLen) s.add(w);
        return new Language(s);
    }

    /** Sigma* up to a length bound: the bounded Kleene star of the alphabet. */
    static Language sigmaStar(Set<Character> alphabet, int maxLen) {
        Set<String> singles = new LinkedHashSet<>();
        for (char c : alphabet) singles.add(String.valueOf(c));
        return new Language(singles).star(maxLen);
    }

    /** "{ }" for empty; "{ epsilon, a, ab }" otherwise (epsilon stands in for ""). */
    String render() {
        if (strings.isEmpty()) return "{ }";
        StringBuilder b = new StringBuilder("{ ");
        boolean first = true;
        for (String w : strings) {
            if (!first) b.append(", ");
            b.append(w.isEmpty() ? "epsilon" : w);
            first = false;
        }
        return b.append(" }").toString();
    }
}
