import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

/**
 * Module 28 — the grammar workbench: EBNF in, diagnosis and a working
 * table-driven parser out. Every analysis is an earlier module's component:
 *
 *   FIRST / FOLLOW / PREDICT   Modules 20-22   (Predict.java)
 *   left recursion, Paull      Module 23       (LeftRec.java)
 *   LL(1) table                Module 25       (LL1Table.java)
 *   table-driven parsing       Module 26       (parse() below)
 *
 * What is new here is the glue a language designer actually needs: reading
 * the grammar as written, explaining each conflict in terms of the rules that
 * cause it, and running the finished table on real tokens.
 */
public final class Workbench {
    private Workbench() {}

    static String pad(String s, int w) { return s.length() >= w ? s : s + " ".repeat(w - s.length()); }
    static String plural(int n, String one, String many) { return n + " " + (n == 1 ? one : many); }

    // ─────────────────────────────────────────── diagnosis

    public record Conflict(String nt, String token, List<Grammar.Production> prods) {}

    /** One conflict = a nonterminal, a lookahead, and >= 2 productions predicting it. */
    public static List<Conflict> conflicts(Grammar g, Predict pr) {
        List<Conflict> out = new ArrayList<>();
        for (String nt : g.nonterminals) {
            Map<String, List<Grammar.Production>> byTok = new LinkedHashMap<>();
            for (Grammar.Production p : g.productionsFor(nt))
                for (String t : new TreeSet<>(pr.predict(p)))
                    byTok.computeIfAbsent(t, k -> new ArrayList<>()).add(p);
            for (var e : byTok.entrySet())
                if (e.getValue().size() > 1) out.add(new Conflict(nt, e.getKey(), e.getValue()));
        }
        return out;
    }

    /**
     * How does production p come to predict terminal t? Either a chain of
     * leftmost derivations ending in t, or "t follows" when p can vanish.
     */
    public static String explain(Grammar g, Predict pr, Grammar.Production p, String t) {
        Set<String> seen = new LinkedHashSet<>();
        seen.add(p.lhs());
        List<String> chain = firstChain(g, pr, p.rhs(), t, seen);
        if (chain != null) return p.lhs() + " -> " + String.join(" -> ", chain);
        return p.lhs() + " -> epsilon, and " + t + " is in FOLLOW(" + p.lhs() + ")";
    }

    private static List<String> firstChain(Grammar g, Predict pr, List<String> seq, String t, Set<String> seen) {
        for (String sym : seq) {
            if (sym.equals(t)) return new ArrayList<>(List.of(t));
            if (g.isNonterminal(sym) && !seen.contains(sym) && pr.firstOf(sym).contains(t)) {
                seen.add(sym);
                for (Grammar.Production q : g.productionsFor(sym)) {
                    if (!pr.firstOfSeq(q.rhs()).contains(t)) continue;
                    List<String> rest = firstChain(g, pr, q.rhs(), t, seen);
                    if (rest != null) { rest.add(0, sym); return rest; }
                }
            }
            if (!pr.firstOf(sym).contains(Predict.EPS)) return null;   // sym can't vanish: t must come from it
        }
        return null;
    }

    static Grammar fromG(LeftRec.G lg) {
        List<Grammar.Production> prods = new ArrayList<>();
        for (String nt : lg.order)
            for (List<String> rhs : lg.prods.get(nt))
                prods.add(new Grammar.Production(prods.size(), nt, new ArrayList<>(rhs)));
        return Grammar.of(prods, lg.start, new LinkedHashSet<>(lg.order));
    }

    public record Report(String text, Grammar grammar, boolean ll1) {}

    public static Report report(String name, Ebnf.Desugared d) {
        Grammar g = d.grammar();
        Predict pr = new Predict(g);
        StringBuilder sb = new StringBuilder();

        Set<String> terminals = g.terminals();
        int classes = (int) terminals.stream().filter(Ebnf::isTokenClass).count();
        line(sb, "grammar: " + name);
        line(sb, "  " + d.rules().size() + " EBNF rules -> " + g.productions.size() + " BNF productions | "
            + d.rules().size() + " rules + " + d.helpers().size() + " helpers = " + g.nonterminals.size()
            + " nonterminals | " + terminals.size() + " terminals ("
            + plural(classes, "token class", "token classes") + ")");
        if (!d.helpers().isEmpty()) {
            line(sb, "  helpers:");
            for (Ebnf.Helper h : d.helpers()) line(sb, "    " + pad(h.name(), 18) + " " + h.text());
        }
        List<String> nullable = new ArrayList<>();
        for (String nt : g.nonterminals) if (pr.firstOf(nt).contains(Predict.EPS)) nullable.add(nt);
        line(sb, "  nullable: " + (nullable.isEmpty() ? "(none)" : String.join(" ", nullable)));

        Grammar work = g;
        Predict workPr = pr;
        boolean lr = LeftRec.hasLeftRecursion(LeftRec.G.from(g));
        line(sb, "  left recursion: " + (lr ? "YES" : "none"));
        if (lr) {
            work = fromG(LeftRec.paull(g));
            workPr = new Predict(work);
            line(sb, "  removed with Paull's algorithm (Module 23):");
            for (String nt : work.nonterminals) {
                List<String> alts = new ArrayList<>();
                for (Grammar.Production q : work.productionsFor(nt))
                    alts.add(q.rhs().isEmpty() ? "epsilon" : String.join(" ", q.rhs()));
                line(sb, "    " + nt + " -> " + String.join(" | ", alts));
            }
        }

        List<Conflict> cs = conflicts(work, workPr);
        if (cs.isEmpty()) {
            LL1Table table = new LL1Table(work);
            int filled = 0;
            for (String nt : work.nonterminals)
                for (String c : table.columns) if (!table.cell(nt, c).isEmpty()) filled++;
            int cells = work.nonterminals.size() * table.columns.size();
            line(sb, "  LL(1): YES  table " + work.nonterminals.size() + " x " + table.columns.size() + ", "
                + filled + " of " + cells + " cells used");
        } else {
            line(sb, "  LL(1): NO, " + plural(cs.size(), "conflict", "conflicts"));
            for (Conflict c : cs) {
                String owner = d.origin().getOrDefault(c.nt(), c.nt());
                line(sb, "    " + c.nt() + (owner.equals(c.nt()) ? "" : " (inside rule " + owner + ")")
                    + " on " + c.token() + ":");
                for (Grammar.Production q : c.prods())
                    line(sb, "      [" + q + "]  via " + explain(work, workPr, q, c.token()));
            }
        }
        return new Report(sb.toString(), work, cs.isEmpty());
    }

    private static void line(StringBuilder sb, String s) { sb.append(s).append('\n'); }

    // ─────────────────────────────────────────── running the table

    public record Token(String kind, String lexeme, int line, int col) {}

    /** The scanner kinds that appear in the grammar by name. */
    static final List<String> TOKEN_CLASSES = List.of("IDENT", "INT_LIT", "FLOAT_LIT");

    /**
     * Scanner token -> grammar terminal. Token classes keep their kind; everything
     * else (keywords, operators, punctuation) is the quoted lexeme.
     */
    static String terminalOf(Token tok) {
        if (tok.kind().equals("EOF")) return Predict.END;
        if (TOKEN_CLASSES.contains(tok.kind())) return tok.kind();
        return "\"" + tok.lexeme() + "\"";
    }

    public static List<Token> readTokens(String text) {
        List<Token> out = new ArrayList<>();
        for (String raw : text.split("\r?\n")) {
            if (raw.isBlank() || raw.startsWith("#")) continue;
            String[] parts = raw.strip().split("\\s+");
            String[] at = parts[0].split(":");
            out.add(new Token(parts[1], parts.length > 2 ? parts[2] : "",
                Integer.parseInt(at[0]), Integer.parseInt(at[1])));
        }
        return out;
    }

    public record Step(String top, String look, String action) {}
    public record ParseResult(boolean ok, int steps, int expansions, int matches, int maxDepth,
                              List<Step> trace, String error) {}

    /** Table-driven LL(1) parse. */
    public static ParseResult parse(Grammar g, LL1Table table, Map<String, String> origin,
                                    List<Token> tokens, int traceLimit) {
        List<String> stack = new ArrayList<>(List.of(Predict.END, g.start));
        int i = 0, expansions = 0, matches = 0, maxDepth = stack.size(), steps = 0;
        List<Step> trace = new ArrayList<>();

        while (!stack.isEmpty()) {
            steps++;
            String top = stack.remove(stack.size() - 1);
            Token tok = tokens.get(i);
            String look = terminalOf(tok);
            if (top.equals(Predict.END) && look.equals(Predict.END)) {
                if (trace.size() < traceLimit) trace.add(new Step(top, look, "accept"));
                break;
            }
            if (!g.isNonterminal(top)) {
                if (!top.equals(look))
                    return new ParseResult(false, steps, expansions, matches, maxDepth, trace,
                        where(tok) + "expected " + top + " but found " + showTok(tok));
                if (trace.size() < traceLimit) trace.add(new Step(top, look, "match"));
                matches++;
                i++;
                continue;
            }
            List<Grammar.Production> cell = table.cell(top, look);
            if (cell.isEmpty()) {
                List<String> expected = new ArrayList<>();
                for (String c : table.columns) if (!table.cell(top, c).isEmpty()) expected.add(c);
                return new ParseResult(false, steps, expansions, matches, maxDepth, trace,
                    where(tok) + "in " + origin.getOrDefault(top, top) + ", expected one of "
                    + String.join(" ", expected) + " but found " + showTok(tok));
            }
            Grammar.Production prod = cell.get(0);
            if (trace.size() < traceLimit) trace.add(new Step(top, look, prod.toString()));
            expansions++;
            for (int k = prod.rhs().size() - 1; k >= 0; k--) stack.add(prod.rhs().get(k));
            maxDepth = Math.max(maxDepth, stack.size());
        }
        return new ParseResult(true, steps, expansions, matches, maxDepth, trace, null);
    }

    private static String where(Token tok) { return tok.line() + ":" + tok.col() + ": "; }
    private static String showTok(Token tok) {
        return tok.kind().equals("EOF") ? "end of input" : "\"" + tok.lexeme() + "\"";
    }

    public static List<String> traceLines(List<Step> trace) {
        List<String> out = new ArrayList<>();
        out.add("    " + pad("top of stack", 22) + pad("lookahead", 12) + "action");
        for (Step s : trace) out.add("    " + pad(s.top(), 22) + pad(s.look(), 12) + s.action());
        return out;
    }
}
