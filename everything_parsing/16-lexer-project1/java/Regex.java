import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

/**
 * Module 03 — a regular expression is a tree built from exactly six node kinds:
 *
 *   BASE CASES        OPERATORS
 *   ---------         ---------
 *   Empty   (∅)       Union   (r | s)
 *   Epsilon (ε)       Concat  (r s)
 *   Char    (c)       Star    (r *)
 *
 * `+` and `?` are sugar: r+  ->  Concat(r, Star(r)) ;  r?  ->  Union(r, Epsilon).
 *
 * Precedence, tightest first:  * (and + ?)   >   concatenation   >   |
 * Grammar the parser implements (hand-rolled; the *technique* is Module 19):
 *
 *   regex   = concat ('|' concat)*
 *   concat  = repeat*                      (juxtaposition; zero items = ε)
 *   repeat  = atom ('*' | '+' | '?')*
 *   atom    = '(' regex ')' | CHAR
 *
 * The meaning of a node is a Language (Module 2): each node kind maps to exactly
 * one Language operation. Two ways to ask "does w match":
 *   - toLanguage(maxLen).contains(w)   — bounded, via set operations
 *   - matches(w)                       — exact, via the leftover-suffix method
 */
public abstract sealed class Regex
        permits Regex.Empty, Regex.Epsilon, Regex.Char,
                Regex.Union, Regex.Concat, Regex.Star {

    // ── the six node kinds ──
    public static final class Empty   extends Regex {}
    public static final class Epsilon extends Regex {}
    public static final class Char    extends Regex { final char c; Char(char c) { this.c = c; } }
    public static final class Union   extends Regex { final Regex l, r; Union(Regex l, Regex r) { this.l = l; this.r = r; } }
    public static final class Concat  extends Regex { final Regex l, r; Concat(Regex l, Regex r) { this.l = l; this.r = r; } }
    public static final class Star    extends Regex { final Regex x;    Star(Regex x)            { this.x = x; } }

    // ─────────────────────────────────────────── the meaning: a Language

    /**
     * Every node kind is one Language operation. This is the whole semantics.
     * Each operator result is truncated to length <= maxLen so intermediate
     * languages stay finite and small; Star is already bounded by construction.
     */
    public Language toLanguage(int maxLen) {
        if (this instanceof Empty)        return Language.EMPTY;
        if (this instanceof Epsilon)      return Language.EPSILON;
        if (this instanceof Char ch)      return Language.of(String.valueOf(ch.c));
        if (this instanceof Union u)      return u.l.toLanguage(maxLen).union(u.r.toLanguage(maxLen)).truncate(maxLen);
        if (this instanceof Concat co)    return co.l.toLanguage(maxLen).concat(co.r.toLanguage(maxLen)).truncate(maxLen);
        if (this instanceof Star s)       return s.x.toLanguage(maxLen).star(maxLen);
        throw new IllegalStateException();
    }

    // ─────────────────────────────────────────── the meaning: exact matching

    /** w matches this regex iff, after consuming a prefix, "" is a possible leftover. */
    public boolean matches(String w) {
        return leftovers(w).contains("");
    }

    /** The set of suffixes of w that remain after this regex consumes a prefix. */
    Set<String> leftovers(String w) {
        Set<String> out = new LinkedHashSet<>();
        if (this instanceof Empty) {
            // nothing
        } else if (this instanceof Epsilon) {
            out.add(w);
        } else if (this instanceof Char ch) {
            if (!w.isEmpty() && w.charAt(0) == ch.c) out.add(w.substring(1));
        } else if (this instanceof Union u) {
            out.addAll(u.l.leftovers(w));
            out.addAll(u.r.leftovers(w));
        } else if (this instanceof Concat co) {
            for (String mid : co.l.leftovers(w)) out.addAll(co.r.leftovers(mid));
        } else if (this instanceof Star s) {
            out.add(w);                                   // zero repetitions
            for (String rest : s.x.leftovers(w))
                if (!rest.equals(w))                      // guard: subexpr must consume something
                    out.addAll(new Star(s.x).leftovers(rest));
        }
        return out;
    }

    // ─────────────────────────────────────────── rendering the tree

    /** Fully parenthesised s-expression, so precedence is visible. ASCII only. */
    public String tree() {
        if (this instanceof Empty)     return "EMPTY";
        if (this instanceof Epsilon)   return "eps";
        if (this instanceof Char ch)   return String.valueOf(ch.c);
        if (this instanceof Union u)   return "(alt " + u.l.tree() + " " + u.r.tree() + ")";
        if (this instanceof Concat co) return "(cat " + co.l.tree() + " " + co.r.tree() + ")";
        if (this instanceof Star s)    return "(star " + s.x.tree() + ")";
        throw new IllegalStateException();
    }

    // ─────────────────────────────────────────── the parser

    public static Regex parse(String src) {
        Parser p = new Parser(src);
        Regex r = p.regex();
        if (!p.eof()) throw new IllegalArgumentException(
            "unexpected '" + p.peek() + "' at position " + p.pos + " in /" + src + "/");
        return r;
    }

    private static final class Parser {
        final String s;
        int pos = 0;
        Parser(String s) { this.s = s; }

        boolean eof()   { return pos >= s.length(); }
        char peek()     { return eof() ? '\0' : s.charAt(pos); }
        char advance()  { return s.charAt(pos++); }
        boolean take(char c) { if (peek() == c) { pos++; return true; } return false; }

        // regex = concat ('|' concat)*
        Regex regex() {
            Regex left = concat();
            while (take('|')) left = new Union(left, concat());
            return left;
        }

        // concat = repeat*   (juxtaposition; zero items = ε)
        Regex concat() {
            if (endsConcat()) return new Epsilon();
            Regex left = repeat();
            while (!endsConcat()) left = new Concat(left, repeat());
            return left;
        }

        private boolean endsConcat() {
            char c = peek();
            return eof() || c == '|' || c == ')';
        }

        // repeat = atom ('*' | '+' | '?')*
        Regex repeat() {
            Regex base = atom();
            while (true) {
                if (take('*'))      base = new Star(base);
                else if (take('+')) base = new Concat(base, new Star(base)); // r+ = r r*
                else if (take('?')) base = new Union(base, new Epsilon());   // r? = r | ε
                else return base;
            }
        }

        // atom = '(' regex ')' | CHAR
        Regex atom() {
            if (take('(')) {
                Regex inner = regex();
                if (!take(')')) throw new IllegalArgumentException(
                    "missing ')' in /" + s + "/ at position " + pos);
                return inner;
            }
            if (eof())
                throw new IllegalArgumentException("unexpected end of regex /" + s + "/");
            char c = peek();
            if (c == '*' || c == '+' || c == '?' || c == ')' || c == '|')
                throw new IllegalArgumentException(
                    "'" + c + "' has nothing to apply to at position " + pos + " in /" + s + "/");
            advance();
            return new Char(c);
        }
    }
}
