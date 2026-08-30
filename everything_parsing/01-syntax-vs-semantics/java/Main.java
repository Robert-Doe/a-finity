import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

/**
 * Runs both checkers over every input in a fixture file and prints a report.
 *
 * Fixture format: one input per line, wrapped in double quotes. Lines that are
 * blank or start with '#' are ignored. "" is the empty expression.
 *
 *   java -cp java/out Main fixtures/inputs.txt
 *
 * Output is built with explicit '\n' and printed once, so the bytes are the
 * same on Windows and POSIX (and identical to the JavaScript build's output).
 */
public final class Main {
    public static void main(String[] args) throws IOException {
        Path path = Path.of(args.length > 0 ? args[0] : "fixtures/inputs.txt");
        List<String> lines = Files.readAllLines(path);

        StringBuilder sb = new StringBuilder();
        sb.append("=== Module 01 - Syntax vs. Semantics - RPN checker ===\n\n");

        int n = 0, syntaxPass = 0, semanticsChecked = 0, semanticsPass = 0, syntaxOkButSemanticFail = 0;

        for (String raw : lines) {
            String line = raw.strip();
            if (line.isEmpty() || line.startsWith("#")) continue;
            if (line.length() < 2 || line.charAt(0) != '"' || line.charAt(line.length() - 1) != '"') {
                continue; // not a quoted input line
            }
            String input = line.substring(1, line.length() - 1);
            n++;

            sb.append("[").append(n).append("] \"").append(input).append("\"\n");

            var tokens = Rpn.tokenize(input);
            Rpn.SyntaxResult syn = Rpn.checkSyntax(tokens);

            if (syn.ok()) {
                syntaxPass++;
                sb.append("    syntax   : PASS\n");
            } else {
                sb.append("    syntax   : FAIL  ").append(at(syn.col(), syn.message())).append("\n");
            }

            if (!syn.ok()) {
                sb.append("    semantics: (skipped: syntax failed)\n");
            } else {
                semanticsChecked++;
                Rpn.SemanticResult sem = Rpn.evaluate(tokens);
                if (sem.ok()) {
                    semanticsPass++;
                    sb.append("    semantics: PASS  value = ").append(sem.value()).append("\n");
                } else {
                    syntaxOkButSemanticFail++;
                    sb.append("    semantics: FAIL  ").append(at(sem.col(), sem.message())).append("\n");
                }
            }
            sb.append("\n");
        }

        sb.append("summary: inputs=").append(n)
          .append("  syntaxPass=").append(syntaxPass)
          .append("  semanticsChecked=").append(semanticsChecked)
          .append("  semanticsPass=").append(semanticsPass)
          .append("  syntaxOkButSemanticFail=").append(syntaxOkButSemanticFail)
          .append("\n");

        System.out.print(sb);
    }

    /** "col N: message" when a column is known, otherwise just the message. */
    private static String at(int col, String message) {
        return col > 0 ? "col " + col + ": " + message : message;
    }
}
