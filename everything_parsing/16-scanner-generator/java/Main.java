import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.stream.Collectors;

/**
 *   java -cp java/out Main
 *
 * Generates a scanner from fixtures/ajoda.tokens, scans a whole program and a
 * set of one-line probes, and shows a spec being refused at load time.
 */
public final class Main {
    static final StringBuilder sb = new StringBuilder();

    public static void main(String[] args) throws IOException {
        p("=== Module 16 - Scanner Generator: a token spec in, a working lexer out ===");
        p("");

        Scanner sc = Scanner.fromSpec(Files.readString(Path.of("fixtures", "ajoda.tokens")));

        p("spec: fixtures/ajoda.tokens");
        p("  #   action   name       pattern");
        for (Scanner.Rule r : sc.rules)
            p(String.format("  %-3s %-8s %-10s %s", r.index(), r.action(), r.name(), r.pattern()));
        p("  keywords (from " + sc.keywords.from() + "): " + String.join(" ", sc.keywords.words()));
        p("");
        p("load checks: " + sc.rules.size() + " patterns parsed, none accepts the empty string");
        p("combined DFA: " + sc.dfaStates() + " states over " + sc.alphabetSize() + " input symbols");
        p("");

        p("--- fixtures/factorial.ajoda ---");
        String prog = Files.readString(Path.of("fixtures", "factorial.ajoda")).replace("\r\n", "\n");
        for (Scanner.Token t : sc.scan(prog)) p("  " + t);
        p("");

        p("--- fixtures/probes.txt (each line scanned on its own) ---");
        for (String raw : Files.readAllLines(Path.of("fixtures", "probes.txt"))) {
            if (raw.isBlank() || raw.startsWith("#")) continue;
            p("");
            p("> " + raw);
            try {
                p("  " + brief(sc.scan(raw)));
            } catch (Scanner.ScanError e) {
                if (!e.partial.isEmpty()) p("  " + brief(e.partial));
                p("  error " + e.getMessage());
            }
        }
        p("");

        p("--- fixtures/broken.tokens ---");
        try {
            Scanner.fromSpec(Files.readString(Path.of("fixtures", "broken.tokens")));
            p("  (unexpectedly accepted)");
        } catch (Scanner.SpecError e) {
            p("  refused: " + e.getMessage());
        }
        p("");

        p("summary: " + sc.rules.size() + " rules -> 1 DFA of " + sc.dfaStates()
            + " states | longest match, earliest rule breaks ties | keywords by table lookup"
            + " | line:col on every token | empty-string patterns refused at load");

        System.out.print(sb);
    }

    static String brief(List<Scanner.Token> ts) {
        return ts.stream().map(Scanner.Token::brief).collect(Collectors.joining(" "));
    }
    static void p(String s) { sb.append(s).append('\n'); }
}
