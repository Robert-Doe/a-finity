import java.util.ArrayList;
import java.util.List;

/**
 * Module 17 — lexical error recovery.
 *
 * Module 16's scanner throws at the first bad character. A working compiler
 * reports the error, RESYNCHRONIZES, and keeps going, so one scan finds every
 * lexical error instead of hiding all but the first.
 *
 *   SKIP_ONE         drop one character, emit an ERROR token for it, resume.
 *                    One error per bad character.
 *   SKIP_TO_RESTART  drop characters until some token can start again, emit
 *                    ONE ERROR token for the whole run. One error per run.
 */
public final class Recovery {
    private Recovery() {}

    public enum Strategy { SKIP_ONE, SKIP_TO_RESTART }

    public record Result(List<Scanner.Token> tokens, List<LexError> errors) {}
    public record LexError(int line, int col, String text) {
        @Override public String toString() {
            return line + ":" + col + " \"" + text.replace("\\", "\\\\").replace("\"", "\\\"") + "\"";
        }
    }

    public static Result scanRecovering(Scanner sc, String src, Strategy strategy) {
        List<Scanner.Token> tokens = new ArrayList<>();
        List<LexError> errors = new ArrayList<>();
        int pos = 0;
        int[] lc = { 1, 1 };   // line, col

        while (pos < src.length()) {
            int[] m = sc.longestMatch(src, pos);
            if (m != null) {
                Scanner.Rule rule = sc.rules.get(m[1]);
                String lexeme = src.substring(pos, m[0]);
                if (rule.action().equals("token"))
                    tokens.add(new Scanner.Token(sc.kindOf(rule.name(), lexeme), lexeme, lc[0], lc[1]));
                advance(lc, lexeme);
                pos = m[0];
                continue;
            }
            // no token starts at pos: report, then resynchronize
            int end = pos + 1;
            if (strategy == Strategy.SKIP_TO_RESTART)
                while (end < src.length() && sc.longestMatch(src, end) == null) end++;
            String text = src.substring(pos, end);
            errors.add(new LexError(lc[0], lc[1], text));
            tokens.add(new Scanner.Token("ERROR", text, lc[0], lc[1]));
            advance(lc, text);
            pos = end;
        }
        tokens.add(new Scanner.Token("EOF", "", lc[0], lc[1]));
        return new Result(tokens, errors);
    }

    private static void advance(int[] lc, String text) {
        for (int i = 0; i < text.length(); i++) {
            if (text.charAt(i) == '\n') { lc[0]++; lc[1] = 1; } else lc[1]++;
        }
    }
}
