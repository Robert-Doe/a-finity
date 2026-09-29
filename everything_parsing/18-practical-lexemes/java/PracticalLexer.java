import java.util.ArrayList;
import java.util.List;
import java.util.Set;

/**
 * Module 18 — the messy real-world lexemes a naive regex spec can't handle,
 * as a HAND-CODED scanner (contrast Module 16's generated DFA).
 *
 *   RESERVED WORDS   scan an identifier, then look it up in a keyword table.
 *                    One identifier DFA instead of one per keyword; a new
 *                    keyword is a one-line table edit.
 *
 *   COMMENTS         // to end of line  (regular)
 *                    /* ... *​/          (regular)
 *                    NESTED /* /* *​/ *​/  -- needs a DEPTH COUNTER, which is
 *                    NOT regular. This is the first place a "lexer" reaches
 *                    past finite state on purpose.
 *
 *   STRINGS          "..." with \n \t \" \\ escapes; an unterminated string is
 *                    a lexical error with the opening position.
 *
 *   NUMBERS          123   12.5   1e10   1.5e-3   0xFF
 *                    a small explicit state machine.
 */
public final class PracticalLexer {

    static final Set<String> KEYWORDS = Set.of("let", "in", "if", "then", "else", "true", "false");

    public record Token(String kind, String text) {
        @Override public String toString() { return kind + " " + q(text); }
        private static String q(String s) { return "\"" + s.replace("\n", "\\n").replace("\t", "\\t") + "\""; }
    }

    public record Result(List<Token> tokens, List<String> errors) {}

    private final String src;
    private int pos = 0;
    private final List<Token> tokens = new ArrayList<>();
    private final List<String> errors = new ArrayList<>();

    private PracticalLexer(String src) { this.src = src; }

    public static Result lex(String src) {
        PracticalLexer L = new PracticalLexer(src);
        L.run();
        return new Result(L.tokens, L.errors);
    }

    // ── cursor ──
    private boolean eof()            { return pos >= src.length(); }
    private char peek()              { return eof() ? '\0' : src.charAt(pos); }
    private char peek(int k)         { return pos + k < src.length() ? src.charAt(pos + k) : '\0'; }
    private char advance()           { return src.charAt(pos++); }
    private static boolean isLetter(char c) { return c >= 'a' && c <= 'z' || c >= 'A' && c <= 'Z' || c == '_'; }
    private static boolean isDigit(char c)  { return c >= '0' && c <= '9'; }
    private static boolean isHex(char c)    { return isDigit(c) || c >= 'a' && c <= 'f' || c >= 'A' && c <= 'F'; }

    private void run() {
        while (!eof()) {
            char c = peek();
            if (c == ' ' || c == '\t' || c == '\n') { advance(); continue; }
            if (c == '/' && peek(1) == '/') { skipLineComment(); continue; }
            if (c == '/' && peek(1) == '*') { skipBlockComment(); continue; }
            if (isLetter(c))                { identifierOrKeyword(); continue; }
            if (isDigit(c))                 { number(); continue; }
            if (c == '"')                   { string(); continue; }
            if ("+-*=".indexOf(c) >= 0)     { tokens.add(new Token("OP", String.valueOf(advance()))); continue; }
            errors.add("lexical error at position " + pos + " (char '" + c + "')");
            advance();
        }
    }

    // ── reserved-word trick ──
    private void identifierOrKeyword() {
        int start = pos;
        while (isLetter(peek()) || isDigit(peek())) advance();
        String text = src.substring(start, pos);
        tokens.add(new Token(KEYWORDS.contains(text) ? "KW_" + text.toUpperCase() : "ID", text));
    }

    // ── comments ──
    private void skipLineComment() {
        while (!eof() && peek() != '\n') advance();
    }
    private void skipBlockComment() {
        int start = pos;
        advance(); advance();                  // consume the opening /*
        int depth = 1;                          // <-- the counter: NOT regular
        while (!eof() && depth > 0) {
            if (peek() == '/' && peek(1) == '*') { advance(); advance(); depth++; }
            else if (peek() == '*' && peek(1) == '/') { advance(); advance(); depth--; }
            else advance();
        }
        if (depth > 0)
            errors.add("unterminated block comment opened at position " + start);
    }

    // ── strings with escapes ──
    private void string() {
        int start = pos;
        advance();                              // opening "
        StringBuilder value = new StringBuilder();
        while (!eof() && peek() != '"' && peek() != '\n') {
            char c = advance();
            if (c == '\\') {
                char e = eof() ? '\0' : advance();
                switch (e) {
                    case 'n' -> value.append('\n');
                    case 't' -> value.append('\t');
                    case '"' -> value.append('"');
                    case '\\' -> value.append('\\');
                    case 'u' -> value.append(unicodeEscape(start));
                    default -> errors.add("bad string escape '\\" + e + "' near position " + (pos - 1));
                }
            } else {
                value.append(c);
            }
        }
        if (peek() != '"') {
            errors.add("unterminated string literal opened at position " + start);
            tokens.add(new Token("STRING?", value.toString()));
            return;
        }
        advance();                              // closing "
        tokens.add(new Token("STRING", value.toString()));
    }
    private char unicodeEscape(int strStart) {
        StringBuilder hex = new StringBuilder();
        for (int i = 0; i < 4 && isHex(peek()); i++) hex.append(advance());
        if (hex.length() != 4) { errors.add("bad \\u escape near position " + pos); return '?'; }
        return (char) Integer.parseInt(hex.toString(), 16);
    }

    // ── numbers: 123  12.5  1e10  1.5e-3  0xFF ──
    private void number() {
        int start = pos;
        if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'X')) {
            advance(); advance();
            int h = pos;
            while (isHex(peek())) advance();
            if (pos == h) errors.add("hex literal with no digits at position " + start);
            tokens.add(new Token("HEX", src.substring(start, pos)));
            return;
        }
        while (isDigit(peek())) advance();
        String kind = "INT";
        if (peek() == '.' && isDigit(peek(1))) {
            kind = "FLOAT";
            advance();
            while (isDigit(peek())) advance();
        }
        if (peek() == 'e' || peek() == 'E') {
            char save = peek();
            int mark = pos;
            advance();
            if (peek() == '+' || peek() == '-') advance();
            if (isDigit(peek())) {
                kind = "FLOAT";
                while (isDigit(peek())) advance();
            } else {
                pos = mark;   // 'e' wasn't an exponent -- retract (e.g. "1e" then a letter)
            }
        }
        tokens.add(new Token(kind, src.substring(start, pos)));
    }
}
