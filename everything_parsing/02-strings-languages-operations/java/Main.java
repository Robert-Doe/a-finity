import java.util.LinkedHashSet;
import java.util.Set;

/**
 *   java -cp java/out Main
 *
 * No fixture file — this module's "input" is the algebra itself. The demo
 * exercises every operation and prints the results; output is buffered and
 * printed once with '\n' for byte-parity with the JS build.
 */
public final class Main {
    public static void main(String[] args) {
        StringBuilder sb = new StringBuilder();
        sb.append("=== Module 02 - Alphabets, Strings, Languages, Operations ===\n\n");

        // ── the alphabet, and Sigma* ──
        Set<Character> sigma = new LinkedHashSet<>();
        sigma.add('a');
        sigma.add('b');
        sb.append("alphabet Sigma = { a, b }\n");
        Language sigmaStar3 = Language.sigmaStar(sigma, 3);
        sb.append("Sigma* up to length 3  (bounded Kleene star of the alphabet):\n");
        sb.append("  ").append(sigmaStar3.render())
          .append("   [").append(sigmaStar3.size()).append(" strings]\n\n");

        // ── union / concat ──
        Language l1 = Language.of("a", "b");
        Language l2 = Language.of("c");
        sb.append("L1 = ").append(l1.render()).append("\n");
        sb.append("L2 = ").append(l2.render()).append("\n");
        sb.append("L1 union L2  = ").append(l1.union(l2).render()).append("\n");
        sb.append("L1 concat L2 = ").append(l1.concat(l2).render()).append("\n");
        sb.append("L2 concat L1 = ").append(l2.concat(l1).render()).append("\n\n");

        // ── closure: results of finite operands are finite ──
        Language p3 = l1.power(3);
        sb.append("closure of finite languages:\n");
        sb.append("  L1 union L2 : ").append(l1.union(l2).size()).append(" strings\n");
        sb.append("  L1 concat L2: ").append(l1.concat(l2).size()).append(" strings\n");
        sb.append("  L1 power 3  : ").append(p3.render()).append("  (")
          .append(p3.size()).append(")\n\n");

        // ── {epsilon} is not {} ──
        sb.append("the empty string is not the empty language:\n");
        sb.append("  {}          size ").append(Language.EMPTY.size()).append("\n");
        sb.append("  { epsilon }  size ").append(Language.EPSILON.size()).append("\n");
        sb.append("  {} concat L1        = ").append(Language.EMPTY.concat(l1).render())
          .append("   (annihilator)\n");
        sb.append("  { epsilon } concat L1 = ").append(Language.EPSILON.concat(l1).render())
          .append("   (identity)\n\n");

        // ── Kleene star of a single-string language ──
        Language aStar = Language.of("a").star(4);
        sb.append("Kleene star of { a } up to length 4:  ").append(aStar.render())
          .append("   [").append(aStar.size()).append(" strings]\n\n");

        // ── a regular expression, rebuilt from set operations ──
        // (a|b) a*   , everything of length <= 4
        Language firstChar = Language.of("a").union(Language.of("b"));
        Language tail      = Language.of("a").star(3);
        Language re        = firstChar.concat(tail);
        // keep only length <= 4
        Language re4 = re.intersect(Language.sigmaStar(sigma, 4));
        sb.append("regex as operations:  (a|b) a*   up to length 4\n");
        sb.append("  built as: ( {a} union {b} ) concat ( {a}.star(3) )\n");
        sb.append("  = ").append(re4.render()).append("   [").append(re4.size()).append(" strings]\n");
        sb.append("  membership:  ")
          .append(q("b", re4)).append("   ")
          .append(q("ba", re4)).append("   ")
          .append(q("ab", re4)).append("   ")
          .append(q("", re4)).append("\n\n");

        sb.append("summary: operations=union,concat,power,star  ")
          .append("epsilonLang!=emptyLang=").append(Language.EPSILON.size() != Language.EMPTY.size())
          .append("  regexReproduced=").append(re4.contains("b") && re4.contains("ba") && !re4.contains("ab"))
          .append("\n");

        System.out.print(sb);
    }

    private static String q(String w, Language l) {
        String shown = w.isEmpty() ? "\"\"" : "\"" + w + "\"";
        return shown + (l.contains(w) ? " in L" : " not in L");
    }
}
