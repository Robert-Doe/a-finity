import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Set;
import java.util.TreeSet;

public final class Main {

    public static void main(String[] args) throws Exception {
        StringBuilder sb = new StringBuilder();
        sb.append("=== Module 24 - Left Factoring ===\n\n");
        sb.append("A -> a b1 | a b2 | rest      ==>      A  -> a A' | rest\n");
        sb.append("                                     A' -> b1 | b2      (empty bi -> A' -> epsilon)\n");
        sb.append("repeat until no nonterminal has two alternatives with the same first symbol.\n\n");

        section(sb, "fixtures/danglingelse.grammar", 8);
        section(sb, "fixtures/nested.grammar", 4);
        section(sb, "fixtures/decl.grammar", 6);

        System.out.print(sb.toString().replace("\r\n", "\n"));
    }

    static void section(StringBuilder sb, String file, int maxLen) throws Exception {
        Grammar src = Grammar.parse(Files.readString(Path.of(file)));
        LeftRec.G before = LeftRec.G.from(src);
        LeftRec.G after = LeftFactor.factor(src);

        sb.append("########## ").append(file).append("\n\n");
        sb.append("BEFORE:\n");
        for (String line : before.text().split("\n")) sb.append("  ").append(line).append('\n');
        sb.append("  needs factoring? ").append(LeftFactor.needsFactoring(before)).append('\n');
        sb.append('\n');

        sb.append("AFTER:\n");
        for (String line : after.text().split("\n")) sb.append("  ").append(line).append('\n');
        sb.append("  needs factoring? ").append(LeftFactor.needsFactoring(after)).append('\n');
        sb.append('\n');

        Set<String> lb = Language.upTo(before, maxLen);
        Set<String> la = Language.upTo(after, maxLen);
        sb.append("language check (all strings up to length ").append(maxLen).append("):\n");
        sb.append("  before: ").append(lb.size()).append("   after: ").append(la.size())
          .append("   identical? ").append(lb.equals(la)).append('\n');
        if (!lb.equals(la)) {
            Set<String> ob = new TreeSet<>(lb); ob.removeAll(la);
            Set<String> oa = new TreeSet<>(la); oa.removeAll(lb);
            sb.append("  only before: ").append(ob).append('\n');
            sb.append("  only after : ").append(oa).append('\n');
        }
        sb.append('\n');
    }
}
