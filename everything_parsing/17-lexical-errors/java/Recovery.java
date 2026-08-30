import java.util.ArrayList;
import java.util.List;

/**
 * Module 17 — lexical error recovery.
 *
 * Module 16's lexer THROWS at the first bad character. A real scanner reports
 * the error, RESYNCHRONIZES, and keeps going, so one scan finds every lexical
 * error instead of hiding all but the first.
 *
 * Two resync strategies:
 *
 *   PANIC_ONE     skip exactly one character, emit an ERROR token for it,
 *                 resume. One ERROR per bad character.
 *
 *   PANIC_TO_SYNC skip characters until one that could plausibly start a token
 *                 (here: in the alphabet). Emit one ERROR token spanning the
 *                 whole garbage run. One ERROR per run.
 *
 * "Plausible token start" is the classic panic-mode idea: a small set of
 * "safe" characters/classes you can trust to realign the scanner.
 */
public final class Recovery {
    private Recovery() {}

    public enum Strategy { PANIC_ONE, PANIC_TO_SYNC }

    public record Result(List<SpecLexer.Token> tokens, List<LexError> errors) {}
    public record LexError(int from, int to, String text) {
        @Override public String toString() {
            return from + 1 == to
                ? "position " + from + " ('" + text + "')"
                : "positions " + from + ".." + (to - 1) + " (\"" + text + "\")";
        }
    }

    public static Result tokenize(SpecLexer lex, String input, Strategy strategy) {
        List<SpecLexer.Token> tokens = new ArrayList<>();
        List<LexError> errors = new ArrayList<>();
        int pos = 0;

        while (pos < input.length()) {
            int[] match = longestToken(lex, input, pos);   // {endPos, ruleIndex} or {-1, -1}
            if (match[0] > pos) {
                String kind = lex.rules.get(match[1]).name();
                if (!lex.skip.contains(kind))
                    tokens.add(new SpecLexer.Token(kind, input.substring(pos, match[0]), pos));
                pos = match[0];
                continue;
            }
            // ── lexical error at `pos`: recover ──
            int errStart = pos;
            int errEnd;
            if (strategy == Strategy.PANIC_ONE) {
                errEnd = pos + 1;
            } else {
                errEnd = pos + 1;
                while (errEnd < input.length()) {
                    String c = String.valueOf(input.charAt(errEnd));
                    // a plausible restart: in the alphabet AND the DFA can move from start on it
                    if (lex.dfa.alphabet.contains(c)
                        && longestToken(lex, input, errEnd)[0] > errEnd) break;
                    errEnd++;
                }
            }
            String garbage = input.substring(errStart, errEnd);
            errors.add(new LexError(errStart, errEnd, garbage));
            tokens.add(new SpecLexer.Token("ERROR", garbage, errStart));
            pos = errEnd;
        }
        return new Result(tokens, errors);
    }

    /** Maximal-munch match at `pos` over the combined DFA. {endPos, ruleIndex}; {-1,-1} if none. */
    static int[] longestToken(SpecLexer lex, String input, int pos) {
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
        return new int[]{lastPos, lastRule};
    }
}
