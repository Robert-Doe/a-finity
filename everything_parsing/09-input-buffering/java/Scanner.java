import java.util.ArrayList;
import java.util.List;

/**
 * A deliberately tiny scanner, built ON TOP of Buffer, to exercise the buffer's
 * peek / advance / mark / retract. Recognises:
 *
 *   WORD   [a-z]+
 *   NUM    [0-9]+
 *   FLOAT  [0-9]+ '.' [0-9]+
 *   PLUS   '+'
 *   (spaces are skipped)
 *
 * The FLOAT rule is where retract earns its keep: after reading "12." the
 * scanner must peek one more character; if it isn't a digit, the '.' belongs to
 * the next token and `forward` retracts one.
 */
public final class Scanner {
    public record Tok(String kind, String text) {}

    static boolean isDigit(int c) { return c >= '0' && c <= '9'; }
    static boolean isLower(int c) { return c >= 'a' && c <= 'z'; }

    /** The next token, or null at end of input. `retracts` is incremented on each retract. */
    static Tok next(Buffer b, int[] retracts) {
        while (b.peek() == ' ') b.advance();
        b.mark();
        int c = b.peek();
        if (c == Buffer.EOF) return null;

        if (isLower(c)) {
            do { b.advance(); } while (isLower(b.peek()));
            return new Tok("WORD", b.lexeme());
        }
        if (isDigit(c)) {
            do { b.advance(); } while (isDigit(b.peek()));
            if (b.peek() == '.') {
                b.advance();                       // tentatively take the '.'
                if (isDigit(b.peek())) {
                    do { b.advance(); } while (isDigit(b.peek()));
                    return new Tok("FLOAT", b.lexeme());
                }
                b.retract(1);                      // '.' was not part of the number
                retracts[0]++;
                return new Tok("NUM", b.lexeme());
            }
            return new Tok("NUM", b.lexeme());
        }
        if (c == '+') { b.advance(); return new Tok("PLUS", b.lexeme()); }

        b.advance();
        return new Tok("ERR", b.lexeme());
    }

    static List<Tok> scanAll(Buffer b, int[] retracts) {
        List<Tok> out = new ArrayList<>();
        Tok t;
        while ((t = next(b, retracts)) != null) out.add(t);
        return out;
    }
}
