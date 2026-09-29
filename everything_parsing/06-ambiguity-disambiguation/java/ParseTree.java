import java.util.ArrayList;
import java.util.List;

/**
 * Module 05 — a parse tree, and the two canonical derivations you can read off it.
 *
 * A parse tree records WHICH productions built a string and HOW they nest — but
 * not the ORDER they were applied in. Two derivations of one tree:
 *
 *   leftmost   — at each step expand the leftmost not-yet-expanded nonterminal
 *   rightmost  — ... the rightmost one
 *
 * Both use the identical multiset of productions and end at the identical tree.
 * Only the intermediate sentential forms differ. That is the module's claim.
 */
public final class ParseTree {

    final String symbol;
    List<ParseTree> children;        // null => unexpanded; [] => expanded by an epsilon production
    Grammar.Production production;   // the production applied to expand this node; null if unexpanded

    ParseTree(String symbol) { this.symbol = symbol; }

    boolean isExpanded() { return children != null; }

    void expand(Grammar.Production p) {
        this.production = p;
        this.children = new ArrayList<>();
        for (String s : p.rhs()) children.add(new ParseTree(s));
    }

    // ─────────────────────────────────────────── build from production choices

    /**
     * Apply `choices` (production indices), each to the leftmost (or rightmost)
     * unexpanded nonterminal on the frontier. Returns the finished root.
     */
    static ParseTree build(Grammar g, int[] choices, boolean leftmost) {
        ParseTree root = new ParseTree(g.start);
        for (int c : choices) {
            ParseTree node = frontier(g, root, leftmost);
            if (node == null)
                throw new IllegalArgumentException("choices left over: tree already complete at choice " + c);
            Grammar.Production p = g.productions.get(c);
            if (!p.lhs().equals(node.symbol))
                throw new IllegalArgumentException(
                    "production " + c + " (" + p + ") does not match frontier nonterminal '" + node.symbol + "'");
            node.expand(p);
        }
        if (frontier(g, root, leftmost) != null)
            throw new IllegalArgumentException("choices ran out before the tree was complete");
        return root;
    }

    /** The leftmost (or rightmost) unexpanded nonterminal, or null if the tree is done. */
    private static ParseTree frontier(Grammar g, ParseTree root, boolean leftmost) {
        List<ParseTree> fr = new ArrayList<>();
        collect(g, root, fr);
        if (fr.isEmpty()) return null;
        return leftmost ? fr.get(0) : fr.get(fr.size() - 1);
    }
    private static void collect(Grammar g, ParseTree n, List<ParseTree> out) {
        if (!n.isExpanded()) {
            if (g.isNonterminal(n.symbol)) out.add(n);
            return;
        }
        for (ParseTree k : n.children) collect(g, k, out);
    }

    // ─────────────────────────────────────────── read a derivation off a finished tree

    record Step(List<String> form, Grammar.Production applied) {}

    static List<Step> derivation(Grammar g, ParseTree root, boolean leftmost) {
        List<Step> steps = new ArrayList<>();
        List<ParseTree> frontierNodes = new ArrayList<>(List.of(root));
        steps.add(new Step(symbols(frontierNodes), null));

        while (true) {
            int i = pick(g, frontierNodes, leftmost);
            if (i < 0) break;
            ParseTree node = frontierNodes.remove(i);
            frontierNodes.addAll(i, node.children);
            steps.add(new Step(symbols(frontierNodes), node.production));
        }
        return steps;
    }

    private static int pick(Grammar g, List<ParseTree> nodes, boolean leftmost) {
        if (leftmost) {
            for (int i = 0; i < nodes.size(); i++)
                if (isExpandedNonterminal(g, nodes.get(i))) return i;
        } else {
            for (int i = nodes.size() - 1; i >= 0; i--)
                if (isExpandedNonterminal(g, nodes.get(i))) return i;
        }
        return -1;
    }
    private static boolean isExpandedNonterminal(Grammar g, ParseTree n) {
        return n.isExpanded() && g.isNonterminal(n.symbol);
    }

    private static List<String> symbols(List<ParseTree> nodes) {
        List<String> s = new ArrayList<>();
        for (ParseTree n : nodes) s.add(n.symbol);
        return s;
    }

    // ─────────────────────────────────────────── yield + rendering

    /** The terminal frontier, left to right — the string the tree parses. */
    List<String> terminalYield() {
        List<String> out = new ArrayList<>();
        yieldInto(this, out);
        return out;
    }
    private static void yieldInto(ParseTree n, List<String> out) {
        if (!n.isExpanded()) { out.add(n.symbol); return; }
        for (ParseTree k : n.children) yieldInto(k, out);   // epsilon production: no children, adds nothing
    }

    /** Multiset of productions used, sorted by production index, as text. */
    static String productionMultiset(ParseTree root) {
        List<Grammar.Production> used = new ArrayList<>();
        collectProductions(root, used);
        used.sort((x, y) -> Integer.compare(x.index(), y.index()));
        List<String> parts = new ArrayList<>();
        for (Grammar.Production p : used) parts.add(compact(p));
        return "[" + String.join(", ", parts) + "]";
    }
    private static void collectProductions(ParseTree n, List<Grammar.Production> out) {
        if (!n.isExpanded()) return;
        out.add(n.production);
        for (ParseTree k : n.children) collectProductions(k, out);
    }
    private static String compact(Grammar.Production p) {
        return p.lhs() + "->" + (p.rhs().isEmpty() ? "eps" : String.join("", p.rhs()));
    }

    /** ASCII tree, `+-` branches, `|  ` / `   ` continuations. */
    String render() {
        StringBuilder sb = new StringBuilder(symbol).append('\n');
        renderKids(this, "", sb);
        return sb.toString().stripTrailing();
    }
    private static void renderKids(ParseTree n, String prefix, StringBuilder sb) {
        if (!n.isExpanded()) return;
        if (n.children.isEmpty()) { sb.append(prefix).append("+- epsilon\n"); return; }
        for (int i = 0; i < n.children.size(); i++) {
            ParseTree k = n.children.get(i);
            boolean last = i == n.children.size() - 1;
            sb.append(prefix).append("+- ").append(k.symbol).append('\n');
            renderKids(k, prefix + (last ? "   " : "|  "), sb);
        }
    }
}
