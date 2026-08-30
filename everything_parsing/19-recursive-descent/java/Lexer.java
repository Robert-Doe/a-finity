import java.util.ArrayList;
import java.util.List;

/**
 * Module 19 — a deliberately tiny tokenizer.
 *
 * Part II (Modules 9–18) built a real lexer. This module is about PARSING, so it
 * uses the smallest scanner that produces a clean token stream: skip spaces,
 * run digits into `num`, letters into `id`, and treat `+ * ( )` as one-char
 * tokens. Every token carries its start position for error messages. The stream
 * always ends with an explicit `EOF` token so the parser never indexes past the
 * end.
 */
public final class Lexer {

    public record Token(String kind, String text, int pos) {
        @Override public String toString() {
            if (kind.equals("EOF")) return "EOF";
            return kind.equals(text) ? kind : kind + "(" + text + ")";
        }
    }

    public static final class LexError extends RuntimeException {
        public final int pos;
        LexError(String msg, int pos) { super(msg); this.pos = pos; }
    }

    public static List<Token> lex(String src) {
        List<Token> out = new ArrayList<>();
        int i = 0;
        while (i < src.length()) {
            char c = src.charAt(i);
            if (c == ' ' || c == '\t') { i++; continue; }
            if (Character.isDigit(c)) {
                int s = i;
                while (i < src.length() && Character.isDigit(src.charAt(i))) i++;
                out.add(new Token("num", src.substring(s, i), s));
            } else if (Character.isLetter(c)) {
                int s = i;
                while (i < src.length() && Character.isLetterOrDigit(src.charAt(i))) i++;
                out.add(new Token("id", src.substring(s, i), s));
            } else if (c == '+' || c == '*' || c == '(' || c == ')') {
                out.add(new Token(String.valueOf(c), String.valueOf(c), i));
                i++;
            } else {
                throw new LexError("stray character '" + c + "' at position " + i, i);
            }
        }
        out.add(new Token("EOF", "", src.length()));
        return out;
    }
}
