import java.util.ArrayList;
import java.util.List;

/**
 * Module 19 — Predictive recursive-descent parser.
 *
 * Grammar (already free of left recursion — Module 23 shows the transform that
 * produces this shape from the natural  E -> E + T  form):
 *
 *   E  -> T E'
 *   E' -> + T E'  |  epsilon
 *   T  -> F T'
 *   T' -> * F T'  |  epsilon
 *   F  -> ( E )   |  num  |  id
 *
 * THE CLAIM: one method per nonterminal, and the sequence of productions the
 * methods fire — read top to bottom — is exactly a LEFTMOST derivation. Each
 * expansion method appends its production to `rules` the instant it commits.
 * `Derivation.replay(rules)` turns that list back into sentential forms and must
 * land on the token string.
 *
 * "Predictive" = the current token alone decides which alternative to take, with
 * no backtracking. Here the choice is by inspection (`E'` expands to `+ T E'`
 * iff the next token is `+`); Module 20 makes the condition formal (disjoint
 * FIRST sets) and Module 21 computes it.
 */
public final class RecursiveDescent {

    /** Concrete syntax tree node: a grammar symbol, plus children (none for terminals / epsilon). */
    public static final class Node {
        public final String symbol;
        public final List<Node> kids = new ArrayList<>();
        public Node(String symbol) { this.symbol = symbol; }
        Node add(Node k) { kids.add(k); return this; }
    }

    public static final class SyntaxError extends RuntimeException {
        public final int pos;
        SyntaxError(String msg, int pos) { super(msg); this.pos = pos; }
    }

    public record Result(Node tree, List<String> rules) {}

    private final List<Lexer.Token> toks;
    private int p = 0;
    private final List<String> rules = new ArrayList<>();

    private RecursiveDescent(List<Lexer.Token> toks) { this.toks = toks; }

    public static Result parse(String src) {
        RecursiveDescent rd = new RecursiveDescent(Lexer.lex(src));
        Node root = rd.parseE();
        rd.expect("EOF");                 // nothing may trail a complete expression
        return new Result(root, rd.rules);
    }

    // ─────────────────────────────────────────── the cursor

    private Lexer.Token peek() { return toks.get(p); }
    private boolean at(String kind) { return peek().kind().equals(kind); }

    private Lexer.Token expect(String kind) {
        if (!at(kind)) {
            Lexer.Token t = peek();
            throw new SyntaxError("expected " + kind + " but saw " + describe(t), t.pos());
        }
        return toks.get(p++);
    }

    private static String describe(Lexer.Token t) {
        return t.kind().equals("EOF") ? "end of input" : "'" + t.text() + "'";
    }

    // ─────────────────────────────────────────── one method per nonterminal

    // E -> T E'
    private Node parseE() {
        rules.add("E -> T E'");
        Node n = new Node("E");
        n.add(parseT());
        n.add(parseEprime());
        return n;
    }

    // E' -> + T E'  |  epsilon
    private Node parseEprime() {
        Node n = new Node("E'");
        if (at("+")) {
            rules.add("E' -> + T E'");
            n.add(term(expect("+")));
            n.add(parseT());
            n.add(parseEprime());
        } else {
            rules.add("E' -> epsilon");
            n.add(new Node("epsilon"));
        }
        return n;
    }

    // T -> F T'
    private Node parseT() {
        rules.add("T -> F T'");
        Node n = new Node("T");
        n.add(parseF());
        n.add(parseTprime());
        return n;
    }

    // T' -> * F T'  |  epsilon
    private Node parseTprime() {
        Node n = new Node("T'");
        if (at("*")) {
            rules.add("T' -> * F T'");
            n.add(term(expect("*")));
            n.add(parseF());
            n.add(parseTprime());
        } else {
            rules.add("T' -> epsilon");
            n.add(new Node("epsilon"));
        }
        return n;
    }

    // F -> ( E )  |  num  |  id
    private Node parseF() {
        Node n = new Node("F");
        if (at("(")) {
            rules.add("F -> ( E )");
            n.add(term(expect("(")));
            n.add(parseE());
            n.add(term(expect(")")));
        } else if (at("num")) {
            rules.add("F -> num");
            n.add(term(expect("num")));
        } else if (at("id")) {
            rules.add("F -> id");
            n.add(term(expect("id")));
        } else {
            Lexer.Token t = peek();
            throw new SyntaxError(
                "expected '(', num, or id to start a factor but saw " + describe(t), t.pos());
        }
        return n;
    }

    private static Node term(Lexer.Token t) {
        String label = switch (t.kind()) {
            case "num", "id" -> t.kind() + " (" + t.text() + ")";
            default -> t.kind();
        };
        return new Node(label);
    }

    // ─────────────────────────────────────────── ASCII tree render (matches Module 5)

    public static String render(Node root) {
        StringBuilder sb = new StringBuilder(root.symbol).append('\n');
        renderKids(root, "", sb);
        return sb.toString().stripTrailing();
    }
    private static void renderKids(Node n, String prefix, StringBuilder sb) {
        for (int i = 0; i < n.kids.size(); i++) {
            Node k = n.kids.get(i);
            boolean last = i == n.kids.size() - 1;
            sb.append(prefix).append("+- ").append(k.symbol).append('\n');
            renderKids(k, prefix + (last ? "   " : "|  "), sb);
        }
    }

    /** The terminal frontier, left to right — the string this tree parses. */
    public static List<String> terminalYield(Node root) {
        List<String> out = new ArrayList<>();
        yieldInto(root, out);
        return out;
    }
    private static void yieldInto(Node n, List<String> out) {
        if (n.kids.isEmpty()) {
            if (!n.symbol.equals("epsilon")) out.add(frontierSymbol(n.symbol));
            return;
        }
        for (Node k : n.kids) yieldInto(k, out);
    }
    private static String frontierSymbol(String label) {
        // "num (12)" -> "num", "id (a)" -> "id", "+" -> "+"
        int sp = label.indexOf(' ');
        return sp < 0 ? label : label.substring(0, sp);
    }
}
