import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

/**
 *   java -cp java/out Main
 *
 * Fixed script over three fixtures:
 *   1. expr-ambiguous   — "id + id * id" and "id + id + id" each have 2 trees
 *   2. expr-unambiguous — same language, 1 tree each
 *   3. dangling-else    — "if x then if x then a else b" has 2 trees
 * plus a language-equivalence check between grammars 1 and 2.
 */
public final class Main {
    static final StringBuilder sb = new StringBuilder();

    public static void main(String[] args) throws IOException {
        p("=== Module 06 - Ambiguity & Disambiguation ===");
        p("");

        Grammar amb   = load("expr-ambiguous.grammar");
        Grammar unamb = load("expr-unambiguous.grammar");
        Grammar dang  = load("dangling-else.grammar");

        List<String> mul = List.of("id", "+", "id", "*", "id");
        List<String> add = List.of("id", "+", "id", "+", "id");

        // 1. the ambiguous grammar
        p("--- fixtures/expr-ambiguous.grammar ---");
        rules(amb);
        exprTrees(amb, mul, "id + id * id", true);
        exprTrees(amb, add, "id + id + id", true);
        List<String> smallest = Ambiguity.smallestAmbiguousString(amb, 7);
        p("smallest ambiguous string (length <= 7): " + String.join(" ", smallest));
        p("");

        // 2. the disambiguated grammar
        p("--- fixtures/expr-unambiguous.grammar ---");
        rules(unamb);
        exprTrees(unamb, mul, "id + id * id", false);
        exprTrees(unamb, add, "id + id + id", false);
        p("");

        // 3. same language?
        p("--- language equivalence (ambiguous vs disambiguated) ---");
        var la = Derivation.enumerate(amb, 7);
        var lu = Derivation.enumerate(unamb, 7);
        p("L(ambiguous)   up to length 7: " + la.size() + " strings");
        p("L(unambiguous) up to length 7: " + lu.size() + " strings");
        p("same language? " + la.equals(lu));
        p("");

        // 4. dangling else
        p("--- fixtures/dangling-else.grammar ---");
        rules(dang);
        List<String> ie = List.of("if", "x", "then", "if", "x", "then", "a", "else", "b");
        var trees = Ambiguity.allTrees(dang, ie);
        p("string \"" + String.join(" ", ie) + "\" has " + trees.size() + " parse trees:");
        for (int k = 0; k < trees.size(); k++) {
            ParseTree t = trees.get(k);
            boolean elseAtTop = topProductionHasElse(t);
            p("");
            p("  TREE " + (char) ('A' + k) + " - else binds to the "
                + (elseAtTop ? "OUTER" : "INNER") + " if");
            p(indent(t.render()));
        }
        p("");

        p("summary: ambiguous: 2 trees for id+id*id (values 8 vs 6), 2 for id+id+id"
            + " | disambiguated: 1 tree each, same language=" + la.equals(lu)
            + " | dangling-else: " + trees.size() + " trees");

        System.out.print(sb);
    }

    // ── per-grammar reporting ──

    static void exprTrees(Grammar g, List<String> target, String label, boolean expectAmbiguous) {
        var trees = Ambiguity.allTrees(g, target);
        p("string \"" + label + "\" has " + trees.size()
            + (trees.size() == 1 ? " parse tree:" : " parse trees:"));
        for (int k = 0; k < trees.size(); k++) {
            ParseTree t = trees.get(k);
            p("");
            p("  TREE " + (char) ('A' + k) + "   value(id=2) = " + evalExpr(t)
                + "   grouping: " + grouping(t));
            p(indent(t.render()));
        }
        p("");
    }

    static void rules(Grammar g) {
        for (Grammar.Production pr : g.productions)
            p("  (" + pr.index() + ") " + pr);
        p("");
    }

    // ── a tiny evaluator for the expression trees (id = 2) ──

    static long evalExpr(ParseTree n) {
        if (!n.isExpanded()) return n.symbol.equals("id") ? 2 : 0;
        var k = n.children;
        if (k.size() == 1) return evalExpr(k.get(0));
        if (k.size() == 3) {
            if (k.get(0).symbol.equals("(")) return evalExpr(k.get(1));
            long a = evalExpr(k.get(0)), b = evalExpr(k.get(2));
            return switch (k.get(1).symbol) {
                case "+" -> a + b;
                case "*" -> a * b;
                default -> 0;
            };
        }
        return 0;
    }

    /** A parenthesised infix rendering, so the grouping is obvious. */
    static String grouping(ParseTree n) {
        if (!n.isExpanded()) return n.symbol;
        var k = n.children;
        if (k.size() == 1) return grouping(k.get(0));
        if (k.size() == 3) {
            if (k.get(0).symbol.equals("(")) return "(" + grouping(k.get(1)) + ")";
            return "(" + grouping(k.get(0)) + " " + k.get(1).symbol + " " + grouping(k.get(2)) + ")";
        }
        return "?";
    }

    static boolean topProductionHasElse(ParseTree root) {
        return root.isExpanded() && root.children.size() == 6; // if x then S else S
    }

    // ── plumbing ──

    static Grammar load(String name) throws IOException {
        return Grammar.parse(Files.readString(Path.of("fixtures", name)));
    }
    static void p(String s) { sb.append(s).append('\n'); }
    static String indent(String block) {
        StringBuilder b = new StringBuilder();
        for (String line : block.split("\n")) b.append("    ").append(line).append('\n');
        return b.toString().stripTrailing();
    }
}
