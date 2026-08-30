import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

/**
 *   java -cp java/out Main fixtures/regexes.txt
 *
 * For each regex: show the parsed tree (precedence made visible), the bounded
 * language it denotes, and exact match/no-match for each test string.
 * Output buffered + '\n' for byte-parity with the JS build.
 */
public final class Main {
    public static void main(String[] args) throws IOException {
        Path path = Path.of(args.length > 0 ? args[0] : "fixtures/regexes.txt");
        List<String> lines = Files.readAllLines(path);

        StringBuilder sb = new StringBuilder();
        sb.append("=== Module 03 - Regular Expressions: Formal Definition ===\n\n");

        int n = 0;
        for (String raw : lines) {
            String line = raw.strip();
            if (line.isEmpty() || line.startsWith("#")) continue;
            int bar = line.indexOf(" | ");   // space-pipe-space, so regex '|' doesn't collide
            String src = (bar < 0 ? line : line.substring(0, bar)).strip();
            String[] tests = bar < 0 ? new String[0] : line.substring(bar + 3).strip().split("\\s+");
            n++;

            Regex re = Regex.parse(src);
            sb.append("[").append(n).append("] /").append(src).append("/\n");
            sb.append("    tree : ").append(re.tree()).append("\n");

            Language lang = re.toLanguage(6);
            sb.append("    L (up to length 6): ").append(clip(lang, 12)).append("\n");

            for (String t : tests) {
                String w = t.equals("~") ? "" : t;
                String shown = w.isEmpty() ? "\"\"" : "\"" + w + "\"";
                sb.append("    ").append(pad(shown, 10))
                  .append(re.matches(w) ? "match" : "no").append("\n");
            }
            sb.append("\n");
        }

        // precedence: three ways to read "a|bc*", "ab*", "(ab)*"
        sb.append("precedence check (tightest: * , then concatenation, then |):\n");
        for (String s : new String[]{"a|bc*", "ab*", "(ab)*", "a|b|c", "ab|cd"}) {
            sb.append("    /").append(pad(s, 8)).append("  ->  ").append(Regex.parse(s).tree()).append("\n");
        }
        sb.append("\n");

        sb.append("summary: regexes=").append(n)
          .append("  nodeKinds=6 (Empty, Epsilon, Char, Union, Concat, Star)")
          .append("  sugar: + ? desugared at parse time\n");

        System.out.print(sb);
    }

    /** Render a language, truncating to the first `max` strings. */
    private static String clip(Language l, int max) {
        List<String> all = l.list();
        if (all.size() <= max) return l.render();
        StringBuilder b = new StringBuilder("{ ");
        for (int i = 0; i < max; i++) {
            if (i > 0) b.append(", ");
            b.append(all.get(i).isEmpty() ? "epsilon" : all.get(i));
        }
        return b.append(", ... (").append(all.size()).append(" total) }").toString();
    }

    private static String pad(String s, int w) {
        StringBuilder b = new StringBuilder(s);
        while (b.length() < w) b.append(' ');
        return b.toString();
    }
}
