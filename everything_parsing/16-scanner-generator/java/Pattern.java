import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Module 16 — the pattern language a real token spec needs, as SUGAR over
 * Module 3's six regex node kinds.
 *
 * Module 3's syntax has no escaping and no character classes, so it cannot
 * spell `(`, `|`, a space or "any digit" without pain. This layer adds exactly
 * what a working token spec needs and compiles every piece of it down to
 * Empty / Epsilon / Char / Union / Concat / Star. Nothing downstream changes:
 * Thompson (Module 12) and the subset construction (Module 13) run unmodified.
 *
 *   pattern = seq ('|' seq)*
 *   seq     = item*                          (zero items = epsilon)
 *   item    = atom ('*' | '+' | '?')*
 *   atom    = '(' pattern ')'
 *           | '[' '^'? classItem+ ']'        character class, ranges a-z
 *           | '"' literalChar* '"'           quoted literal, no escaping needed
 *           | '\' escape                     \n \t \s(space) or any metachar
 *           | plainChar
 *
 * SYMBOLS. The automaton file format (Module 10) separates fields with
 * whitespace and treats '#' as a comment, so four characters get a symbol
 * NAME instead of standing for themselves: space=SP, tab=TAB, newline=NL,
 * '#'=HASH. Every other printable ASCII character is its own symbol.
 *
 * Regex.Char holds one Java char, so a named symbol is carried in a
 * {@link Sym} node that Thompson treats exactly like a Char.
 */
public final class Pattern {
    private Pattern() {}

    /** The source alphabet: tab, newline, and printable ASCII 0x20..0x7E. */
    public static final List<Character> SOURCE_CHARS;
    static {
        List<Character> cs = new ArrayList<>();
        cs.add('\t');
        cs.add('\n');
        for (char c = 0x20; c <= 0x7e; c++) cs.add(c);
        SOURCE_CHARS = List.copyOf(cs);
    }

    private static final Map<Character, String> NAMED =
        Map.of(' ', "SP", '\t', "TAB", '\n', "NL", '#', "HASH");

    public static String symbolOf(char ch) {
        String n = NAMED.get(ch);
        return n != null ? n : String.valueOf(ch);
    }

    public static boolean isSourceChar(char ch) {
        return ch == '\t' || ch == '\n' || (ch >= ' ' && ch <= '~');
    }

    private static final String META = "()|*+?[]\"\\";

    public static final class PatternError extends RuntimeException {
        PatternError(String msg) { super(msg); }
    }

    public static Regex parse(String src) {
        Parser p = new Parser(src);
        Regex node = p.pattern();
        if (!p.eof()) throw new PatternError("unexpected '" + p.peek() + "' at column " + (p.pos + 1));
        return node;
    }

    static Regex sym(char ch) { return Regex.symbol(symbolOf(ch)); }

    private static Regex unionOf(List<Character> chars) {
        if (chars.isEmpty()) throw new PatternError("empty character class");
        Regex node = sym(chars.get(0));
        for (int i = 1; i < chars.size(); i++) node = new Regex.Union(node, sym(chars.get(i)));
        return node;
    }

    private static final class Parser {
        final String s;
        int pos = 0;
        Parser(String s) { this.s = s; }

        boolean eof() { return pos >= s.length(); }
        char peek()   { return s.charAt(pos); }
        boolean take(char c) { if (!eof() && peek() == c) { pos++; return true; } return false; }

        Regex pattern() {
            Regex left = seq();
            while (take('|')) left = new Regex.Union(left, seq());
            return left;
        }

        Regex seq() {
            Regex left = null;
            while (!eof() && peek() != '|' && peek() != ')') {
                Regex next = item();
                left = left == null ? next : new Regex.Concat(left, next);
            }
            return left != null ? left : new Regex.Epsilon();
        }

        Regex item() {
            Regex base = atom();
            while (true) {
                if (take('*'))      base = new Regex.Star(base);
                else if (take('+')) base = new Regex.Concat(base, new Regex.Star(base));
                else if (take('?')) base = new Regex.Union(base, new Regex.Epsilon());
                else return base;
            }
        }

        Regex atom() {
            int col = pos + 1;
            char c = peek();
            if (take('(')) {
                Regex inner = pattern();
                if (!take(')')) throw new PatternError("'(' at column " + col + " is never closed");
                return inner;
            }
            if (take('[')) return charClass(col);
            if (take('"')) return literal(col);
            if (take('\\')) return sym(escape(col));
            if (c == ' ' || c == '\t')
                throw new PatternError("bare whitespace at column " + col + "; write \\s, \\t or a quoted \" \"");
            if (META.indexOf(c) >= 0)
                throw new PatternError("'" + c + "' at column " + col + " has nothing to apply to");
            pos++;
            if (!isSourceChar(c))
                throw new PatternError("character at column " + col + " is outside the source alphabet");
            return sym(c);
        }

        char escape(int col) {
            if (eof()) throw new PatternError("pattern ends inside an escape at column " + col);
            char e = s.charAt(pos++);
            if (e == 'n') return '\n';
            if (e == 't') return '\t';
            if (e == 's') return ' ';
            if (META.indexOf(e) >= 0 || e == '^' || e == '-') return e;
            throw new PatternError("unknown escape '\\" + e + "' at column " + col);
        }

        char classChar(int col) {
            if (eof()) throw new PatternError("'[' at column " + col + " is never closed");
            if (take('\\')) return escape(pos);
            return s.charAt(pos++);
        }

        Regex charClass(int col) {
            boolean negate = take('^');
            Set<Character> picked = new LinkedHashSet<>();
            while (!take(']')) {
                char lo = classChar(col);
                if (!eof() && peek() == '-' && pos + 1 < s.length() && s.charAt(pos + 1) != ']') {
                    pos++;
                    char hi = classChar(col);
                    if (hi < lo) throw new PatternError(
                        "backwards range " + lo + "-" + hi + " in class at column " + col);
                    for (char k = lo; k <= hi; k++) picked.add(k);
                } else {
                    picked.add(lo);
                }
            }
            List<Character> chars = new ArrayList<>();
            for (char ch : SOURCE_CHARS) if (negate != picked.contains(ch)) chars.add(ch);
            return unionOf(chars);
        }

        Regex literal(int col) {
            Regex node = null;
            while (!take('"')) {
                if (eof()) throw new PatternError("quoted literal at column " + col + " is never closed");
                Regex next = sym(s.charAt(pos++));
                node = node == null ? next : new Regex.Concat(node, next);
            }
            if (node == null) throw new PatternError("empty quoted literal at column " + col);
            return node;
        }
    }
}
