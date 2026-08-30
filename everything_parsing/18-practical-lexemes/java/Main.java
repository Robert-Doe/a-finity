import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.stream.Collectors;

/**
 *   java -cp java/out Main
 *
 * Tokenizes each program in fixtures/programs.txt with the hand-coded scanner,
 * then shows the "nested comment needs a counter" point and the reserved-word
 * table.
 */
public final class Main {
    static final StringBuilder sb = new StringBuilder();

    public static void main(String[] args) throws IOException {
        p("=== Module 18 - Practical Lexemes: Numbers, Strings, Comments, Keywords ===");
        p("");
        p("keyword table: " + PracticalLexer.KEYWORDS.stream().sorted().collect(Collectors.joining(" ")));
        p("  -> scan an identifier, then look it up.  A new keyword is a one-line table edit.");
        p("");

        for (String raw : Files.readAllLines(Path.of("fixtures", "programs.txt"))) {
            String line = raw.stripTrailing();
            if (line.strip().isEmpty() || line.strip().startsWith("#")) continue;
            String program = line.replace("~", "\n");

            p("--- " + line + " ---");
            var r = PracticalLexer.lex(program);
            p("  tokens:  " + r.tokens().stream().map(Object::toString).collect(Collectors.joining("  ")));
            for (String e : r.errors()) p("  ERROR:   " + e);
            p("");
        }

        // ── nesting is not regular ──
        p("nesting is beyond regular:");
        String s1 = "/* a /* b */ c */";
        var n1 = PracticalLexer.lex(s1 + " x");
        p("  \"" + s1 + " x\"  ->  " + n1.tokens().stream().map(Object::toString).collect(Collectors.joining(" ")));
        p("  a flat regex /\\*.*\\*/ would stop at the FIRST '*/' and leave  ' c */ x'  unparsed.");
        p("  the hand-coded scanner uses a depth counter -- one integer of memory the DFA doesn't have.");
        p("");

        String s2 = "/* /* only one close */ y";
        var n2 = PracticalLexer.lex(s2);
        p("  \"" + s2 + "\"  ->  " + (n2.errors().isEmpty() ? "(no error?!)" : n2.errors().get(0)));
        p("");

        p("summary: reserved-word trick (1 DFA + lookup) | // and /* */ comments | NESTED comments need a counter"
            + " (not regular) | strings with \\n \\t \\\" \\\\ \\uXXXX | numbers: INT FLOAT exp HEX");

        System.out.print(sb);
    }

    static void p(String s) { sb.append(s).append('\n'); }
}
