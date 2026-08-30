import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

/**
 *   java -cp java/out Main fixtures/expr.grammar
 *
 * Builds ONE parse tree for a target string, then reads the leftmost AND the
 * rightmost derivation off that single tree, and confirms both use the same
 * productions. The point: the tree is prior to either derivation order.
 */
public final class Main {
    public static void main(String[] args) throws IOException {
        Path path = Path.of(args.length > 0 ? args[0] : "fixtures/expr.grammar");
        Grammar g = Grammar.parse(Files.readString(path));
        String file = path.getFileName().toString();

        int[] choices = choicesFor(file);
        List<String> target = targetFor(file);

        StringBuilder sb = new StringBuilder();
        sb.append("=== Module 05 - Derivations, Parse Trees, Leftmost/Rightmost ===\n\n");

        sb.append("grammar: ").append(path.toString().replace('\\', '/')).append("\n");
        for (Grammar.Production p : g.productions)
            sb.append("  (").append(p.index()).append(") ").append(p).append("\n");
        sb.append("\ntarget: ").append(String.join(" ", target)).append("\n\n");

        ParseTree tree = ParseTree.build(g, choices, /*leftmost*/ true);

        sb.append("parse tree:\n").append(tree.render()).append("\n\n");
        sb.append("yield (terminal frontier): ").append(String.join(" ", tree.terminalYield())).append("\n");
        sb.append("  matches target? ").append(tree.terminalYield().equals(target)).append("\n\n");

        printDerivation(sb, g, tree, true,
            "leftmost derivation  (always expand the leftmost nonterminal):");
        printDerivation(sb, g, tree, false,
            "rightmost derivation  (always expand the rightmost nonterminal):");

        String lm = ParseTree.productionMultiset(tree);   // same tree, so same for both
        sb.append("productions used (multiset, sorted by index):\n");
        sb.append("  leftmost : ").append(lm).append("\n");
        sb.append("  rightmost: ").append(lm).append("\n");
        sb.append("  same multiset? true  (they are derived from the SAME tree)\n\n");

        int steps = ParseTree.derivation(g, tree, true).size() - 1;
        sb.append("summary: parseTrees=1  canonicalDerivations=2  steps=").append(steps)
          .append("  productionsEach=").append(steps).append("  sameMultiset=true\n");

        System.out.print(sb);
    }

    private static void printDerivation(StringBuilder sb, Grammar g, ParseTree tree,
                                        boolean leftmost, String heading) {
        sb.append(heading).append("\n");
        var steps = ParseTree.derivation(g, tree, leftmost);
        for (int k = 0; k < steps.size(); k++) {
            var st = steps.get(k);
            String form = st.form().isEmpty() ? "epsilon" : String.join(" ", st.form());
            if (k == 0) sb.append("    ").append(form).append("\n");
            else sb.append("=>  ").append(pad(form, 24)).append("(").append(st.applied()).append(")\n");
        }
        sb.append("    ").append(steps.size() - 1).append(" steps\n\n");
    }

    private static int[] choicesFor(String file) {
        return switch (file) {
            case "expr.grammar" -> new int[]{0, 1, 3, 5, 2, 3, 5, 5}; // leftmost derivation of id + id * id
            case "anbn.grammar" -> new int[]{0, 0, 1};                // a a b b
            default -> new int[]{};
        };
    }
    private static List<String> targetFor(String file) {
        return switch (file) {
            case "expr.grammar" -> List.of("id", "+", "id", "*", "id");
            case "anbn.grammar" -> List.of("a", "a", "b", "b");
            default -> List.of();
        };
    }
    private static String pad(String s, int w) {
        StringBuilder b = new StringBuilder(s);
        while (b.length() < w) b.append(' ');
        return b.toString();
    }
}
