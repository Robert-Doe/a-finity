import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.stream.Collectors;

/**
 *   java -cp java/out Main
 *
 * Runs both recovery strategies over each probe in fixtures/errors.txt,
 * contrasts them with Module 16's stop-at-first-error scan, then recovers
 * through a whole file with typos on several lines.
 */
public final class Main {
    static final StringBuilder sb = new StringBuilder();
    static Scanner sc;

    public static void main(String[] args) throws IOException {
        sc = Scanner.fromSpec(Files.readString(Path.of("fixtures", "ajoda.tokens")));

        p("=== Module 17 - Lexical Errors & Recovery ===");
        p("");
        p("spec: fixtures/ajoda.tokens  (" + sc.rules.size() + " rules, from Module 16)");
        p("");

        for (String raw : Files.readAllLines(Path.of("fixtures", "errors.txt"))) {
            if (raw.isBlank() || raw.startsWith("#")) continue;
            p("--- probe: " + raw + " ---");
            report(raw, Recovery.Strategy.SKIP_ONE, "SKIP_ONE (one ERROR per bad character)");
            report(raw, Recovery.Strategy.SKIP_TO_RESTART, "SKIP_TO_RESTART (one ERROR per bad run)");
            try {
                sc.scan(raw);
                p("  Module 16 alone: scans cleanly");
            } catch (Scanner.ScanError e) {
                p("  Module 16 alone: stops at " + e.getMessage());
            }
            p("");
        }

        p("--- fixtures/typos.ajoda, SKIP_TO_RESTART ---");
        String src = Files.readString(Path.of("fixtures", "typos.ajoda")).replace("\r\n", "\n");
        var r = Recovery.scanRecovering(sc, src, Recovery.Strategy.SKIP_TO_RESTART);
        p("  " + r.tokens().size() + " tokens, " + r.errors().size() + " errors:");
        for (var e : r.errors()) p("    " + e);
        p("  (the comment on line 4 holds @#$? but is discarded whole, so it is not an error)");
        p("");

        p("summary: report + resynchronize | SKIP_ONE: one error per character"
            + " | SKIP_TO_RESTART: one error per run | one scan finds every error, each with line:col");

        System.out.print(sb);
    }

    static void report(String input, Recovery.Strategy strategy, String label) {
        var res = Recovery.scanRecovering(sc, input, strategy);
        p("  " + label + ":");
        p("    " + res.tokens().stream().map(Scanner.Token::brief).collect(Collectors.joining(" ")));
        p("    " + res.errors().size() + " error" + (res.errors().size() == 1 ? "" : "s") + ": "
            + res.errors().stream().map(Object::toString).collect(Collectors.joining(", ")));
    }

    static void p(String s) { sb.append(s).append('\n'); }
}
