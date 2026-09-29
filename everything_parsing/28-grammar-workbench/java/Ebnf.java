import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Module 28 — read a grammar the way language specs write it (EBNF), and
 * desugar it into the plain BNF that Modules 20-27 analyse.
 *
 *   rule    = NAME '::=' alt                  a new rule starts at "NAME ::="
 *   alt     = seq ('|' seq)*
 *   seq     = item*                           (zero items = epsilon)
 *   item    = NAME | "quoted"
 *           | '{' alt '}'                     zero or more
 *           | '[' alt ']'                     optional
 *           | '(' alt ')'                     grouping
 *
 * NAMES. A name defined by some rule is a nonterminal. An UPPER_CASE name that
 * no rule defines is a token class from the scanner (IDENT, INT_LIT). Any other
 * undefined name is an error, because it is almost always a typo.
 *
 * DESUGARING. Each bracket becomes a fresh helper nonterminal named after the
 * rule it sits in, so every BNF production can be traced back to the line of
 * EBNF that produced it:
 *
 *   { X }     ->  R_repN  ->  X R_repN | epsilon        (right-recursive: LL-friendly)
 *   [ X ]     ->  R_optN  ->  X | epsilon
 *   ( X | Y ) ->  R_grpN  ->  X | Y                     (a one-branch group is inlined)
 */
public final class Ebnf {
    private Ebnf() {}

    public static final class EbnfError extends RuntimeException {
        public final int line;
        EbnfError(int line, String msg) { super("ebnf line " + line + ": " + msg); this.line = line; }
    }

    // ── the EBNF tree ──
    sealed interface Node permits Alt, Name, Lit, Bracket {}
    record Alt(List<List<Node>> seqs) implements Node {}
    record Name(String v, int line) implements Node {}
    record Lit(String v) implements Node {}
    record Bracket(String kind, Alt body) implements Node {}   // kind: rep | opt | grp

    record Rule(String name, int line, Alt body) {}
    public record Helper(String name, String owner, String kind, String text) {}

    /** The result: a Module 2 Grammar plus what's needed to explain it. */
    public record Desugared(Grammar grammar, List<Rule> rules, List<Helper> helpers, Map<String, String> origin) {}

    // ─────────────────────────────────────────── lexing

    private record Tok(String t, String v, int line) {}

    private static final Pattern NAME = Pattern.compile("^[A-Za-z_][A-Za-z0-9_]*");

    private static List<Tok> lex(String text) {
        List<Tok> toks = new ArrayList<>();
        String[] lines = text.split("\r?\n", -1);
        for (int i = 0; i < lines.length; i++) {
            int line = i + 1;
            String raw = lines[i];
            int k = 0;
            while (k < raw.length()) {
                char c = raw.charAt(k);
                if (c == ' ' || c == '\t') { k++; continue; }
                if (c == '#') break;
                if (raw.startsWith("::=", k)) { toks.add(new Tok("::=", null, line)); k += 3; continue; }
                if ("|{}[]()".indexOf(c) >= 0) { toks.add(new Tok(String.valueOf(c), null, line)); k++; continue; }
                if (c == '"') {
                    int close = raw.indexOf('"', k + 1);
                    if (close < 0) throw new EbnfError(line, "unterminated quoted terminal");
                    if (close == k + 1) throw new EbnfError(line, "empty quoted terminal");
                    toks.add(new Tok("LIT", raw.substring(k + 1, close), line));
                    k = close + 1;
                    continue;
                }
                Matcher m = NAME.matcher(raw.substring(k));
                if (!m.find()) throw new EbnfError(line, "unexpected '" + c + "'");
                toks.add(new Tok("NAME", m.group(), line));
                k += m.group().length();
            }
        }
        toks.add(new Tok("EOF", null, lines.length));
        return toks;
    }

    // ─────────────────────────────────────────── parsing

    private static final class Parser {
        final List<Tok> toks;
        int i = 0;
        Parser(List<Tok> toks) { this.toks = toks; }

        Tok peek(int o) { return toks.get(i + o); }
        Tok peek()      { return toks.get(i); }
        Tok next()      { return toks.get(i++); }
        boolean ruleStartsHere() { return peek().t().equals("NAME") && peek(1).t().equals("::="); }

        List<Rule> rules() {
            List<Rule> out = new ArrayList<>();
            while (!peek().t().equals("EOF")) {
                if (!ruleStartsHere())
                    throw new EbnfError(peek().line(), "expected 'Name ::=', found " + show(peek()));
                Tok name = next();
                next();
                out.add(new Rule(name.v(), name.line(), alt()));
            }
            return out;
        }

        Alt alt() {
            List<List<Node>> seqs = new ArrayList<>();
            seqs.add(seq());
            while (peek().t().equals("|")) { next(); seqs.add(seq()); }
            return new Alt(seqs);
        }

        List<Node> seq() {
            List<Node> items = new ArrayList<>();
            while (true) {
                String t = peek().t();
                if (t.equals("EOF") || t.equals("|") || t.equals(")") || t.equals("]") || t.equals("}")
                    || ruleStartsHere()) return items;
                items.add(item());
            }
        }

        Node item() {
            Tok tok = next();
            if (tok.t().equals("NAME")) return new Name(tok.v(), tok.line());
            if (tok.t().equals("LIT")) return new Lit(tok.v());
            String close = switch (tok.t()) { case "{" -> "}"; case "[" -> "]"; case "(" -> ")"; default -> null; };
            if (close == null) throw new EbnfError(tok.line(), "unexpected " + show(tok));
            Alt body = alt();
            Tok got = next();
            if (!got.t().equals(close))
                throw new EbnfError(got.line(), "expected '" + close + "' to match '" + tok.t()
                    + "' on line " + tok.line() + ", found " + show(got));
            String kind = tok.t().equals("{") ? "rep" : tok.t().equals("[") ? "opt" : "grp";
            return new Bracket(kind, body);
        }
    }

    private static String show(Tok tok) {
        return switch (tok.t()) {
            case "EOF" -> "end of file";
            case "NAME" -> "'" + tok.v() + "'";
            case "LIT" -> "\"" + tok.v() + "\"";
            default -> "'" + tok.t() + "'";
        };
    }

    // ─────────────────────────────────────────── rendering EBNF back to text

    static String text(Node n) {
        if (n instanceof Alt a) {
            List<String> parts = new ArrayList<>();
            for (List<Node> s : a.seqs()) {
                if (s.isEmpty()) { parts.add("epsilon"); continue; }
                List<String> xs = new ArrayList<>();
                for (Node x : s) xs.add(text(x));
                parts.add(String.join(" ", xs));
            }
            return String.join(" | ", parts);
        }
        if (n instanceof Name nm) return nm.v();
        if (n instanceof Lit l) return "\"" + l.v() + "\"";
        Bracket b = (Bracket) n;
        String open = b.kind().equals("rep") ? "{ " : b.kind().equals("opt") ? "[ " : "( ";
        String close = b.kind().equals("rep") ? " }" : b.kind().equals("opt") ? " ]" : " )";
        return open + text(b.body()) + close;
    }

    // ─────────────────────────────────────────── desugaring to BNF

    public static boolean isTokenClass(String name) { return name.matches("[A-Z][A-Z0-9_]*"); }
    static String literal(String v) { return "\"" + v + "\""; }

    public static Desugared desugar(String src) {
        List<Rule> rules = new Parser(lex(src)).rules();
        if (rules.isEmpty()) throw new EbnfError(1, "no rules");

        Map<String, Integer> defined = new LinkedHashMap<>();
        for (Rule r : rules) {
            Integer first = defined.putIfAbsent(r.name(), r.line());
            if (first != null)
                throw new EbnfError(r.line(), r.name() + " is defined twice (first on line " + first + ")");
        }

        Lowering low = new Lowering(defined);
        for (Rule r : rules) {
            low.addNt(r.name(), r.name());
            List<List<String>> alts = new ArrayList<>();
            for (List<Node> s : r.body().seqs()) alts.add(low.lowerSeq(s, r.name()));
            low.bodies.put(r.name(), alts);
        }

        // Emit rules first (file order), each followed by its helpers.
        List<String> emitOrder = new ArrayList<>();
        for (Rule r : rules) {
            emitOrder.add(r.name());
            for (Helper h : low.helpers) if (h.owner().equals(r.name())) emitOrder.add(h.name());
        }
        List<Grammar.Production> prods = new ArrayList<>();
        for (String nt : emitOrder)
            for (List<String> rhs : low.bodies.get(nt))
                prods.add(new Grammar.Production(prods.size(), nt, rhs));

        Grammar g = Grammar.of(prods, rules.get(0).name(), new LinkedHashSet<>(emitOrder));
        return new Desugared(g, rules, low.helpers, low.origin);
    }

    private static final class Lowering {
        final Map<String, Integer> defined;
        final Map<String, List<List<String>>> bodies = new LinkedHashMap<>();
        final List<Helper> helpers = new ArrayList<>();
        final Map<String, String> origin = new LinkedHashMap<>();
        final Map<String, Integer> counters = new LinkedHashMap<>();

        Lowering(Map<String, Integer> defined) { this.defined = defined; }

        void addNt(String name, String owner) { bodies.put(name, new ArrayList<>()); origin.put(name, owner); }

        String fresh(String owner, String kind) {
            int n = counters.merge(owner, 1, Integer::sum);
            return owner + "_" + kind + n;
        }

        List<String> lowerSeq(List<Node> items, String owner) {
            List<String> out = new ArrayList<>();
            for (Node it : items) {
                if (it instanceof Name nm) {
                    if (!defined.containsKey(nm.v()) && !isTokenClass(nm.v()))
                        throw new EbnfError(nm.line(), "'" + nm.v() + "' is not defined (token classes are UPPER_CASE)");
                    out.add(nm.v());
                } else if (it instanceof Lit l) {
                    out.add(literal(l.v()));
                } else {
                    Bracket b = (Bracket) it;
                    if (b.kind().equals("grp") && b.body().seqs().size() == 1) {
                        out.addAll(lowerSeq(b.body().seqs().get(0), owner));
                        continue;
                    }
                    String name = fresh(owner, b.kind());
                    addNt(name, owner);
                    helpers.add(new Helper(name, owner, b.kind(), text(b)));
                    List<List<String>> alts = new ArrayList<>();
                    for (List<Node> s : b.body().seqs()) alts.add(lowerSeq(s, owner));
                    List<List<String>> body = bodies.get(name);
                    if (b.kind().equals("rep")) {
                        for (List<String> a : alts) { List<String> x = new ArrayList<>(a); x.add(name); body.add(x); }
                        body.add(List.of());
                    } else if (b.kind().equals("opt")) {
                        body.addAll(alts);
                        body.add(List.of());
                    } else {
                        body.addAll(alts);
                    }
                    out.add(name);
                }
            }
            return out;
        }
    }
}
