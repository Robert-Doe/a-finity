import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.stream.Collectors;

/**
 *   java -cp java/out Main
 *
 * Runs both recovery strategies over each input in fixtures/errors.txt, and
 * contrasts with Module 16's throw-on-first-error behaviour.
 */
public final class Main {
    static final StringBuilder sb = new StringBuilder();

    public static void main(String[] args) throws IOException {
        SpecLexer lex = SpecLexer.load(Files.readString(Path.of("fixtures", "mini.spec")));

        p("=== Module 17 - Lexical Errors & Recovery ===");
        p("");
        p("spec: fixtures/mini.spec  (" + lex.rules.size() + " rules)  --  errors on characters not in the alphabet");
        p("");

        for (String raw : Files.readAllLines(Path.of("fixtures", "errors.txt"))) {
            String line = raw.strip();
            if (line.isEmpty() || line.startsWith("#")) continue;
            if (line.length() < 2 || line.charAt(0) != '"' || line.charAt(line.length() - 1) != '"') continue;
            String input = line.substring(1, line.length() - 1);

            p("--- input \"" + input + "\" ---");
            report(lex, input, Recovery.Strategy.PANIC_ONE,     "PANIC_ONE (skip one bad char, ERROR per char)");
            report(lex, input, Recovery.Strategy.PANIC_TO_SYNC, "PANIC_TO_SYNC (skip to a plausible token start)");

            // Module 16: throws at the first error
            try {
                lex.tokenize(input);
                p("  Module 16 (no recovery):  tokenized cleanly (no lexical error)");
            } catch (SpecLexer.LexicalError e) {
                p("  Module 16 (no recovery):  stops -- " + e.getMessage());
            }
            p("");
        }

        p("summary: recovery = report + resync | PANIC_ONE: one ERROR per bad char"
            + " | PANIC_TO_SYNC: one ERROR per garbage run | one scan finds ALL errors");

        System.out.print(sb);
    }

    static void report(SpecLexer lex, String input, Recovery.Strategy s, String label) {
        var r = Recovery.tokenize(lex, input, s);
        p("  strategy " + label + ":");
        for (var t : r.tokens()) p("    " + t);
        p("    -> " + r.errors().size() + " lexical error"
            + (r.errors().size() == 1 ? "" : "s") + ":  "
            + r.errors().stream().map(Object::toString).collect(Collectors.joining(",  ")));
    }

    static void p(String s) { sb.append(s).append('\n'); }
}
