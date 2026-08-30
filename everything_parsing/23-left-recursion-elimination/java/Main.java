import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Set;
import java.util.TreeSet;

public final class Main {

    public static void main(String[] args) throws Exception {
        StringBuilder sb = new StringBuilder();
        sb.append("=== Module 23 - Left Recursion Elimination ===\n\n");
        sb.append("direct:   A -> A a | b        ==>   A -> b A'    A' -> a A' | epsilon\n");
        sb.append("indirect: Paull's algorithm -- fix an order, substitute lower nonterminals\n");
        sb.append("          into higher ones, then remove direct left recursion.\n\n");

        // ── direct: the expression grammar ──
        section(sb, "fixtures/expr.grammar", 6);

        // ── indirect: needs Paull ──
        section(sb, "fixtures/indirect.grammar", 6);

        System.out.print(sb.toString().replace("\r\n", "\n"));
    }

    static void section(StringBuilder sb, String file, int maxLen) throws Exception {
        Grammar src = Grammar.parse(Files.readString(Path.of(file)));
        LeftRec.G before = LeftRec.G.from(src);
        LeftRec.G after = LeftRec.paull(src);

        sb.append("########## ").append(file).append("\n\n");
        sb.append("BEFORE:\n");
        for (String line : before.text().split("\n")) sb.append("  ").append(line).append('\n');
        sb.append("  left-recursive? ").append(LeftRec.hasLeftRecursion(before)).append('\n');
        sb.append('\n');

        sb.append("AFTER (Paull):\n");
        for (String line : after.text().split("\n")) sb.append("  ").append(line).append('\n');
        sb.append("  left-recursive? ").append(LeftRec.hasLeftRecursion(after)).append('\n');
        sb.append("  start symbol: ").append(after.start).append('\n');
        sb.append('\n');

        Set<String> lb = Language.upTo(before, maxLen);
        Set<String> la = Language.upTo(after, maxLen);
        sb.append("language check (all strings up to length ").append(maxLen).append("):\n");
        sb.append("  before: ").append(lb.size()).append(" strings\n");
        sb.append("  after : ").append(la.size()).append(" strings\n");
        sb.append("  identical? ").append(lb.equals(la)).append('\n');
        if (!lb.equals(la)) {
            Set<String> onlyBefore = new TreeSet<>(lb); onlyBefore.removeAll(la);
            Set<String> onlyAfter = new TreeSet<>(la); onlyAfter.removeAll(lb);
            sb.append("  only before: ").append(onlyBefore).append('\n');
            sb.append("  only after : ").append(onlyAfter).append('\n');
        } else {
            sb.append("  sample: ");
            int n = 0;
            for (String s : la) { if (n++ == 8) break; sb.append('"').append(s).append("\"  "); }
            sb.append('\n');
        }
        sb.append('\n');
    }
}
