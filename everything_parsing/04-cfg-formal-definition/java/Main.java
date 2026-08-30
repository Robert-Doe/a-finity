import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.List;

/**
 *   java -cp java/out Main fixtures/anbn.grammar
 *
 * Demonstrates, for one grammar:
 *   1. what the grammar is (N, Sigma, P, S)
 *   2. one concrete leftmost derivation, sentential form by sentential form
 *   3. the language enumerated up to a length bound  — and why it never ends
 *   4. membership decided by bounded derivation search
 *
 * Output is buffered and printed once with '\n' so Java and JS emit identical bytes.
 */
public final class Main {
    public static void main(String[] args) throws IOException {
        Path path = Path.of(args.length > 0 ? args[0] : "fixtures/anbn.grammar");
        Grammar g = Grammar.parse(Files.readString(path));

        StringBuilder sb = new StringBuilder();
        sb.append("=== Module 04 - Context-Free Grammars ===\n\n");

        // 1. the grammar
        sb.append("grammar: ").append(path.toString().replace('\\', '/')).append("\n");
        sb.append("  nonterminals: ").append(String.join(" ", g.nonterminals)).append("\n");
        sb.append("  terminals   : ").append(String.join(" ", g.terminals())).append("\n");
        sb.append("  start       : ").append(g.start).append("\n");
        sb.append("  productions :\n");
        for (Grammar.Production p : g.productions)
            sb.append("    (").append(p.index()).append(") ").append(p).append("\n");
        sb.append("\n");

        // 2. a leftmost derivation chosen per grammar
        int[] choices = derivationChoicesFor(path.getFileName().toString());
        List<String> target = leftmostTargetFor(path.getFileName().toString());
        sb.append("leftmost derivation of ").append(Derivation.display(target))
          .append("  [choices: ").append(join(choices)).append("]\n");
        var steps = Derivation.leftmostDerivation(g, choices);
        for (int k = 0; k < steps.size(); k++) {
            var st = steps.get(k);
            String form = st.form().isEmpty() ? "epsilon" : String.join(" ", st.form());
            if (k == 0) sb.append("    ").append(form).append("\n");
            else        sb.append("=>  ").append(pad(form, 16)).append("(")
                          .append(st.applied().index()).append(") ").append(st.applied()).append("\n");
        }
        sb.append("    ").append(steps.size()).append(" sentential forms, ")
          .append(steps.size() - 1).append(" steps\n\n");

        // 3. the language, up to a bound
        int bound = 6;
        List<String> lang = Derivation.enumerate(g, bound);
        sb.append("language up to length ").append(bound).append(":\n");
        int shownLen = -1;
        for (String w : lang) {
            int len = w.equals("epsilon") ? 0 : w.split(" ").length;
            if (len != shownLen) { sb.append("    len ").append(len).append(": "); shownLen = len; }
            else sb.append("  |  ");
            sb.append(w).append("\n");
        }
        sb.append("    ").append(lang.size()).append(" strings of length <= ").append(bound)
          .append(" -- raise the bound and the list always grows (the grammar is infinite)\n\n");

        // 4. membership
        sb.append("membership (bounded derivation search):\n");
        List<List<String>> tests = testStringsFor(path.getFileName().toString());
        int derivable = 0;
        for (List<String> t : tests) {
            var m = Derivation.derives(g, t);
            if (m.derivable()) derivable++;
            sb.append("    ").append(pad(Derivation.display(t), 14))
              .append(m.derivable() ? "DERIVABLE      " : "NOT DERIVABLE  ")
              .append("(").append(m.note()).append(")\n");
        }
        sb.append("\n");

        sb.append("summary: productions=").append(g.productions.size())
          .append("  language=infinite")
          .append("  testsDerivable=").append(derivable).append("/").append(tests.size())
          .append("\n");

        System.out.print(sb);
    }

    // Per-fixture scripts so the demo is deterministic and readable.
    private static int[] derivationChoicesFor(String file) {
        return switch (file) {
            case "anbn.grammar"     -> new int[]{0, 0, 1};       // S=>aSb=>aaSbb=>aabb
            case "balanced.grammar" -> new int[]{0, 1, 0, 1, 1}; // S=>(S)S=>()S=>()(S)S=>()()S=>()()
            default -> new int[]{};
        };
    }
    private static List<String> leftmostTargetFor(String file) {
        return switch (file) {
            case "anbn.grammar"     -> List.of("a", "a", "b", "b");
            case "balanced.grammar" -> List.of("(", ")", "(", ")");
            default -> List.of();
        };
    }
    private static List<List<String>> testStringsFor(String file) {
        return switch (file) {
            case "anbn.grammar" -> List.of(
                List.of("a", "a", "b", "b"),
                List.of("a", "a", "b"),
                List.of("b", "a"),
                List.of());
            case "balanced.grammar" -> List.of(
                List.of("(", ")"),
                List.of("(", "(", ")", ")"),
                List.of("(", ")", ")"),
                List.of());
            default -> List.of();
        };
    }

    private static String join(int[] a) {
        StringBuilder s = new StringBuilder();
        for (int i = 0; i < a.length; i++) { if (i > 0) s.append(' '); s.append(a[i]); }
        return s.toString();
    }
    private static String pad(String s, int w) {
        StringBuilder b = new StringBuilder(s);
        while (b.length() < w) b.append(' ');
        return b.toString();
    }
}
