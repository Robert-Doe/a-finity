import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Set;

public final class Main {

    public static void main(String[] args) throws Exception {
        StringBuilder sb = new StringBuilder();
        sb.append("=== Module 29 - LL(k), LL(*), and Backtracking RD ===\n\n");

        // ── part 1: FIRST_k and the minimal k ──
        sb.append("### PART 1 - one token isn't always enough\n\n");
        for (String file : new String[]{"fixtures/expr.grammar", "fixtures/label.grammar", "fixtures/equiv.grammar"}) {
            Grammar g = Grammar.parse(Files.readString(Path.of(file)));
            LeftRec.G lg = LeftRec.G.from(g);
            FirstK fk = new FirstK(lg);

            sb.append("########## ").append(file).append('\n');
            for (Grammar.Production p : g.productions) sb.append("  ").append(p).append('\n');

            for (int k = 1; k <= 2; k++) {
                sb.append("  FIRST_").append(k).append(" per production:\n");
                Map<String, Set<List<String>>> t = fk.firstKTable(k);
                for (Grammar.Production p : g.productions)
                    sb.append(String.format("    %-22s %s%n",
                        p.lhs() + " -> " + rhsText(p),
                        FirstK.show(fk.firstKOfSeq(p.rhs(), k, t))));
            }
            int m = fk.minLL(4, g);
            sb.append("  minimal k: ").append(m < 0 ? "none in 1..4 (grammar is ambiguous / not LL(k))"
                                                    : Integer.toString(m)).append('\n');
            sb.append('\n');
        }

        // ── part 2: backtracking cost ──
        sb.append("### PART 2 - backtracking recursive descent: the cost of no lookahead\n\n");

        Grammar equiv = Grammar.parse(Files.readString(Path.of("fixtures/equiv.grammar")));
        LeftRec.G equivG = LeftRec.G.from(equiv);
        sb.append("equiv.grammar  (A -> x B | x C | y ;  B -> A ;  C -> A)  on  x^n y :\n");
        sb.append(String.format("    %-6s %-14s %-16s%n", "n", "parse trees", "production tries"));
        for (int n = 1; n <= 7; n++) {
            List<String> in = new ArrayList<>();
            for (int i = 0; i < n; i++) in.add("x");
            in.add("y");
            Backtrack.Result r = new Backtrack(equivG).parse(in);
            sb.append(String.format("    %-6d %-14s %-16s%n", n,
                r.capped() ? "(capped)" : Long.toString(r.parseCount()),
                r.capped() ? ">cap" : Long.toString(r.entries())));
        }
        sb.append("    parse trees double each step (2^n) and so does the work -- no finite k helps.\n");
        sb.append("    packrat parsing (Appendix X2) memoizes (nonterminal, position) -> linear.\n\n");

        Grammar exprG = Grammar.parse(Files.readString(Path.of("fixtures/expr.grammar")));
        LeftRec.G exprLG = LeftRec.G.from(exprG);
        sb.append("expr.grammar  (LL(1))  on  id (+ id)^n :\n");
        sb.append(String.format("    %-6s %-14s %-16s%n", "tokens", "parse trees", "production tries"));
        for (int n = 0; n <= 6; n++) {
            List<String> in = new ArrayList<>();
            in.add("id");
            for (int i = 0; i < n; i++) { in.add("+"); in.add("id"); }
            Backtrack.Result r = new Backtrack(exprLG).parse(in);
            sb.append(String.format("    %-6d %-14s %-16s%n", in.size(),
                Long.toString(r.parseCount()), Long.toString(r.entries())));
        }
        sb.append("    exactly one parse tree; work grows LINEARLY -- disjoint FIRST sets mean each\n");
        sb.append("    nonterminal's wrong alternatives fail on the first token.\n\n");

        sb.append("### LL(*) in one line\n");
        sb.append("ANTLR's adaptive LL(*): instead of a fixed k, build a small DFA over the\n");
        sb.append("lookahead that reads as many tokens as needed to pick an alternative --\n");
        sb.append("regular (not bounded) lookahead. Falls back to backtracking only when the\n");
        sb.append("DFA can't decide. For label.grammar the DFA reads id then peeks one more:\n");
        sb.append("  'colon' -> alternative 1 (label) ;  otherwise -> alternative 2 (expr).\n");

        System.out.print(sb.toString().replace("\r\n", "\n"));
    }

    static String rhsText(Grammar.Production p) {
        return p.rhs().isEmpty() ? "epsilon" : String.join(" ", p.rhs());
    }
}
