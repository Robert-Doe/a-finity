import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

/**
 *   java -cp java/out Main
 *
 * Loads fixtures/mini.spec, tokenizes every program in fixtures/inputs.txt,
 * and shows the "epsilon is not a token" rejection on fixtures/bad.spec.
 */
public final class Main {
    static final StringBuilder sb = new StringBuilder();

    public static void main(String[] args) throws IOException {
        p("=== Module 16 - The Lexical Analyzer (Project 1) ===");
        p("");

        String specText = Files.readString(Path.of("fixtures", "mini.spec"));
        SpecLexer lex = SpecLexer.load(specText);

        p("spec: fixtures/mini.spec");
        for (SpecLexer.Rule r : lex.rules)
            p(String.format("  (%d) %-8s = %s%s", r.index(), r.name(), squash(r.pattern()),
                lex.skip.contains(r.name()) ? "   [skipped]" : ""));
        p("");
        p("epsilon check:  no token can match the empty string  ->  OK");
        p("combined DFA:  " + lex.combinedDfaStates() + " states  (union of "
            + lex.rules.size() + " pattern NFAs, tagged, determinized)");
        p("");

        List<String> lines = Files.readAllLines(Path.of("fixtures", "inputs.txt"));
        p("--- fixtures/inputs.txt ---");
        for (String raw : lines) {
            String line = raw.strip();
            if (line.isEmpty() || line.startsWith("#")) continue;
            if (line.length() < 2 || line.charAt(0) != '"' || line.charAt(line.length() - 1) != '"') continue;
            String input = line.substring(1, line.length() - 1);
            p("");
            p("\"" + input + "\"");
            try {
                var toks = lex.tokenize(input);
                for (var t : toks) p("  " + t);
                int skipped = countSkipped(lex, input);
                if (skipped > 0) p("  (" + skipped + " WS token" + (skipped == 1 ? "" : "s") + " skipped)");
            } catch (SpecLexer.LexicalError e) {
                p("  " + e.getMessage());
            }
        }
        p("");

        p("--- bad spec: a token that matches epsilon ---");
        try {
            SpecLexer.load(Files.readString(Path.of("fixtures", "bad.spec")));
            p("  fixtures/bad.spec  ->  (unexpectedly accepted?!)");
        } catch (IllegalArgumentException e) {
            p("  fixtures/bad.spec  ->  rejected:  " + e.getMessage());
        }
        p("");

        p("summary: " + lex.rules.size() + " rules -> 1 combined DFA (" + lex.combinedDfaStates()
            + " states) | maximal munch + priority in one pass | WS skipped | epsilon-token rejected at load");

        System.out.print(sb);
    }

    static int countSkipped(SpecLexer lex, String input) {
        // re-scan counting how many produced tokens are skip tokens
        int pos = 0, skipped = 0;
        while (pos < input.length()) {
            String state = lex.dfa.start;
            int lastPos = -1, lastRule = -1;
            Integer at = lex.stateToken.get(state);
            if (at != null) { lastPos = pos; lastRule = at; }
            for (int i = pos; i < input.length(); i++) {
                String sym = String.valueOf(input.charAt(i));
                if (!lex.dfa.alphabet.contains(sym)) break;
                state = lex.dfa.step(state, sym);
                Integer ri = lex.stateToken.get(state);
                if (ri != null) { lastPos = i + 1; lastRule = ri; }
            }
            if (lastPos <= pos) break;
            if (lex.skip.contains(lex.rules.get(lastRule).name())) skipped++;
            pos = lastPos;
        }
        return skipped;
    }

    /** Shorten a long (0|1|...|9) style pattern for display. */
    static String squash(String p) {
        return p.replace("(0|1|2|3|4|5|6|7|8|9)", "(0..9)");
    }
    static void p(String s) { sb.append(s).append('\n'); }
}
