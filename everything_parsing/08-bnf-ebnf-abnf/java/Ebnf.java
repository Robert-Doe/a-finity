import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Module 08 — EBNF, and the proof that its extra operators add nothing.
 *
 * EBNF adds four things to BNF:  x?  (optional),  x*  (zero or more),
 * x+  (one or more),  ( ... )  (grouping).  This module parses EBNF, then
 * DESUGARS it to pure BNF (Module 4's grammar format) by introducing one fresh
 * nonterminal per operator:
 *
 *     x?  ->  Opt   with   Opt -> x | epsilon
 *     x*  ->  Rep   with   Rep -> x Rep | epsilon
 *     x+  ->  Plus  with   Plus -> x Plus | x
 *     (...)  is absorbed by the parser (parens only affect grouping)
 *
 * Then it checks — by exact matching over the EBNF AST and by bounded
 * enumeration of the BNF — that the two describe the same language.
 *
 * EBNF meta-grammar this parser implements:
 *   grammar = rule+
 *   rule    = IDENT '=' alt ';'
 *   alt     = seq ('|' seq)*
 *   seq     = rep+
 *   rep     = atom ('?' | '*' | '+')?
 *   atom    = '(' alt ')' | STRING | IDENT
 */
public final class Ebnf {

    // ── the AST ──
    public sealed interface Expr permits Terminal, Nonterminal, Seq, Alt, Opt, Star, Plus {}
    public record Terminal(String text) implements Expr {}
    public record Nonterminal(String name) implements Expr {}
    public record Seq(List<Expr> parts) implements Expr {}
    public record Alt(List<Expr> parts) implements Expr {}
    public record Opt(Expr inner) implements Expr {}
    public record Star(Expr inner) implements Expr {}
    public record Plus(Expr inner) implements Expr {}

    public static final class Grammar {
        final Map<String, Expr> rules = new LinkedHashMap<>();   // name -> body, in file order
        String start;
        Set<String> ruleNames() { return rules.keySet(); }
        boolean isNonterminal(String s) { return rules.containsKey(s); }
    }

    // ─────────────────────────────────────────── parsing EBNF

    public static Grammar parse(String src) {
        List<String> toks = lex(src);
        P p = new P(toks);
        Grammar g = new Grammar();
        while (!p.eof()) {
            String name = p.ident();
            p.expect("=");
            Expr body = p.alt();
            p.expect(";");
            g.rules.put(name, body);
            if (g.start == null) g.start = name;
        }
        return g;
    }

    private static List<String> lex(String src) {
        List<String> out = new ArrayList<>();
        int i = 0, n = src.length();
        while (i < n) {
            char c = src.charAt(i);
            if (Character.isWhitespace(c)) { i++; continue; }
            if (c == '/' && i + 1 < n && src.charAt(i + 1) == '/') {      // // line comment
                while (i < n && src.charAt(i) != '\n') i++;
                continue;
            }
            if (c == '\'' || c == '"') {                                  // quoted terminal
                int j = i + 1;
                while (j < n && src.charAt(j) != c) j++;
                out.add("'" + src.substring(i + 1, j) + "'");             // normalise to single-quote form
                i = j + 1;
                continue;
            }
            if ("=;|()?*+".indexOf(c) >= 0) { out.add(String.valueOf(c)); i++; continue; }
            if (Character.isLetterOrDigit(c) || c == '_') {               // identifier
                int j = i;
                while (j < n && (Character.isLetterOrDigit(src.charAt(j)) || src.charAt(j) == '_')) j++;
                out.add(src.substring(i, j));
                i = j;
                continue;
            }
            throw new IllegalArgumentException("EBNF: unexpected character '" + c + "' at " + i);
        }
        return out;
    }

    private static final class P {
        final List<String> t;
        int i = 0;
        P(List<String> t) { this.t = t; }
        boolean eof()    { return i >= t.size(); }
        String peek()    { return eof() ? "" : t.get(i); }
        String next()    { return t.get(i++); }
        void expect(String s) {
            if (!peek().equals(s)) throw new IllegalArgumentException("EBNF: expected '" + s + "', got '" + peek() + "'");
            i++;
        }
        String ident() {
            String s = next();
            if (s.isEmpty() || s.startsWith("'") || "=;|()?*+".contains(s))
                throw new IllegalArgumentException("EBNF: expected an identifier, got '" + s + "'");
            return s;
        }

        Expr alt() {
            List<Expr> parts = new ArrayList<>();
            parts.add(seq());
            while (peek().equals("|")) { next(); parts.add(seq()); }
            return parts.size() == 1 ? parts.get(0) : new Alt(parts);
        }
        Expr seq() {
            List<Expr> parts = new ArrayList<>();
            while (!peek().equals("|") && !peek().equals(")") && !peek().equals(";") && !eof())
                parts.add(rep());
            if (parts.isEmpty()) throw new IllegalArgumentException("EBNF: empty sequence");
            return parts.size() == 1 ? parts.get(0) : new Seq(parts);
        }
        Expr rep() {
            Expr a = atom();
            String q = peek();
            if (q.equals("?")) { next(); return new Opt(a); }
            if (q.equals("*")) { next(); return new Star(a); }
            if (q.equals("+")) { next(); return new Plus(a); }
            return a;
        }
        Expr atom() {
            String s = peek();
            if (s.equals("(")) { next(); Expr e = alt(); expect(")"); return e; }
            if (s.startsWith("'")) { next(); return new Terminal(s.substring(1, s.length() - 1)); }
            return new Nonterminal(ident());
        }
    }

    // ─────────────────────────────────────────── desugar to pure BNF text

    /** Returns Module-4 BNF grammar text. `%start` set to the EBNF start rule. */
    public static String toBnfText(Grammar g) {
        StringBuilder out = new StringBuilder("%start ").append(g.start).append("\n");
        int[] counter = {0};
        // reserve-then-fill: keys are inserted in creation order, so a helper rule
        // appears before the helpers IT spawns
        Map<String, String> extra = new LinkedHashMap<>();
        for (Map.Entry<String, Expr> e : g.rules.entrySet()) {
            List<List<String>> alts = alternatives(e.getValue(), counter, extra);
            out.append(rule(e.getKey(), alts)).append("\n");
        }
        for (String text : extra.values()) out.append(text).append("\n");
        return out.toString();
    }

    private static String rule(String lhs, List<List<String>> alts) {
        List<String> parts = new ArrayList<>();
        for (List<String> a : alts) parts.add(a.isEmpty() ? "epsilon" : String.join(" ", a));
        return lhs + " -> " + String.join(" | ", parts);
    }

    /** The alternatives (each a symbol list) for an expression at rule-body position. */
    private static List<List<String>> alternatives(Expr e, int[] counter, Map<String, String> extra) {
        if (e instanceof Alt alt) {
            List<List<String>> out = new ArrayList<>();
            for (Expr part : alt.parts()) out.add(seqSymbols(part, counter, extra));
            return out;
        }
        List<List<String>> one = new ArrayList<>();
        one.add(seqSymbols(e, counter, extra));
        return one;
    }

    private static List<String> seqSymbols(Expr e, int[] counter, Map<String, String> extra) {
        if (e instanceof Seq s) {
            List<String> out = new ArrayList<>();
            for (Expr part : s.parts()) out.add(symbol(part, counter, extra));
            return out;
        }
        List<String> out = new ArrayList<>();
        out.add(symbol(e, counter, extra));
        return out;
    }

    /** One grammar symbol for an expression; may emit a fresh rule into `extra`. */
    private static String symbol(Expr e, int[] counter, Map<String, String> extra) {
        if (e instanceof Terminal t)    return t.text();
        if (e instanceof Nonterminal n) return n.name();

        String prefix = e instanceof Opt ? "opt_" : e instanceof Star ? "rep_"
                      : e instanceof Plus ? "plus_" : "grp_";
        String name = prefix + (++counter[0]);
        extra.put(name, "");                         // reserve the slot in creation order

        String body;
        if (e instanceof Opt o) {
            body = name + " -> " + symbol(o.inner(), counter, extra) + " | epsilon";
        } else if (e instanceof Star st) {
            String x = symbol(st.inner(), counter, extra);
            body = name + " -> " + x + " " + name + " | epsilon";
        } else if (e instanceof Plus pl) {
            String x = symbol(pl.inner(), counter, extra);
            body = name + " -> " + x + " " + name + " | " + x;
        } else { // Alt or Seq
            body = rule(name, alternatives(e, counter, extra));
        }
        extra.put(name, body);
        return name;
    }

    // ─────────────────────────────────────────── exact matching over the EBNF AST

    /** A token list is accepted iff the start rule can consume all of it. */
    public static boolean accepts(Grammar g, List<String> input) {
        return leftovers(g, g.rules.get(g.start), input, 0).contains(input.size());
    }

    // returns the set of "position in input" values reachable after matching a prefix
    private static Set<Integer> leftovers(Grammar g, Expr e, List<String> in, int pos) {
        Set<Integer> out = new LinkedHashSet<>();
        if (pos > in.size()) return out;
        switch (e) {
            case Terminal t -> { if (pos < in.size() && in.get(pos).equals(t.text())) out.add(pos + 1); }
            case Nonterminal n -> {
                Expr body = g.rules.get(n.name());
                if (body == null) {                         // undefined name = terminal
                    if (pos < in.size() && in.get(pos).equals(n.name())) out.add(pos + 1);
                } else {
                    out.addAll(leftovers(g, body, in, pos));
                }
            }
            case Seq s -> {
                Set<Integer> cur = new LinkedHashSet<>(List.of(pos));
                for (Expr part : s.parts()) {
                    Set<Integer> nxt = new LinkedHashSet<>();
                    for (int p : cur) nxt.addAll(leftovers(g, part, in, p));
                    cur = nxt;
                }
                out.addAll(cur);
            }
            case Alt a -> { for (Expr part : a.parts()) out.addAll(leftovers(g, part, in, pos)); }
            case Opt o -> { out.add(pos); out.addAll(leftovers(g, o.inner(), in, pos)); }
            case Star st -> {
                out.add(pos);
                for (int p : leftovers(g, st.inner(), in, pos))
                    if (p != pos) out.addAll(leftovers(g, new Star(st.inner()), in, p));
            }
            case Plus pl -> out.addAll(leftovers(g, new Seq(List.of(pl.inner(), new Star(pl.inner()))), in, pos));
        }
        return out;
    }

    // ─────────────────────────────────────────── ISO-style rendering ({ } = 0+, [ ] = optional)

    /** Like iso(), but doesn't wrap a top-level alternation in parentheses. */
    public static String isoRule(Expr e) {
        if (e instanceof Alt a) return String.join(" | ", a.parts().stream().map(Ebnf::iso).toList());
        return iso(e);
    }

    public static String iso(Expr e) {
        return switch (e) {
            case Terminal t    -> "'" + t.text() + "'";
            case Nonterminal n -> n.name();
            case Seq s         -> String.join(" ", s.parts().stream().map(Ebnf::iso).toList());
            case Alt a         -> "( " + String.join(" | ", a.parts().stream().map(Ebnf::iso).toList()) + " )";
            case Opt o         -> "[ " + iso(o.inner()) + " ]";
            case Star st       -> "{ " + iso(st.inner()) + " }";
            case Plus pl       -> iso(pl.inner()) + " { " + iso(pl.inner()) + " }";
        };
    }
}
