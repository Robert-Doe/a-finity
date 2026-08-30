import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.List;
import java.util.Set;
import java.util.TreeSet;

/**
 * Bounded string enumeration for a `LeftRec.G` grammar.
 *
 * BFS over sentential forms, always expanding the leftmost nonterminal. A form
 * whose terminal count already exceeds `maxLen` is pruned (terminals only
 * increase). All-terminal forms of length &lt;= maxLen are collected.
 *
 * Used to check that left-recursion elimination preserves the language: the two
 * grammars must generate the identical set of strings up to some length.
 */
public final class Language {

    public static Set<String> upTo(LeftRec.G g, int maxLen) {
        Set<String> out = new TreeSet<>();
        Set<String> visited = new java.util.HashSet<>();
        Deque<List<String>> queue = new ArrayDeque<>();
        queue.add(List.of(g.start));
        int budget = 400_000;

        while (!queue.isEmpty() && budget-- > 0) {
            List<String> form = queue.poll();

            int lm = -1;
            for (int i = 0; i < form.size(); i++)
                if (g.isNT(form.get(i))) { lm = i; break; }

            if (lm < 0) {                                   // all terminals
                if (form.size() <= maxLen) out.add(String.join(" ", form));
                continue;
            }
            long terminals = form.stream().filter(s -> !g.isNT(s)).count();
            if (terminals > maxLen) continue;               // can only grow

            for (List<String> rhs : g.prods.get(form.get(lm))) {
                List<String> next = new ArrayList<>(form.subList(0, lm));
                next.addAll(rhs);
                next.addAll(form.subList(lm + 1, form.size()));
                String key = String.join("", next);
                if (next.size() <= 4 * maxLen + 4 && visited.add(key)) queue.add(next);
            }
        }
        return out;
    }
}
