import java.util.ArrayList;
import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;
import java.util.regex.Matcher;

/**
 * Module 16 — a scanner GENERATOR: token spec in, working lexer out.
 *
 *   SPEC     one directive per line ('#' in column 1 starts a comment):
 *              token    NAME  pattern     emit a NAME token
 *              discard  NAME  pattern     match it, then throw it away
 *              keywords FROM  w1 w2 ...   a FROM lexeme spelled like a
 *                                          keyword becomes that keyword
 *   BUILD    each pattern -> NFA (Thompson), all NFAs joined under one fresh
 *            start state with every accept state TAGGED by its rule, then the
 *            subset construction -> one combined DFA. A DFA state that holds
 *            several tagged accepts resolves to the EARLIEST declared rule.
 *   SCAN     longest match: drive the DFA as far as it will go, remember the
 *            last accepting (position, rule), back up to it, emit. Keywords
 *            are resolved AFTER the match by a table lookup, so `iffy` is one
 *            identifier and never `if` + `fy`.
 *   CHECK    a pattern that accepts the empty string is refused at load time:
 *            the scan loop would match zero characters forever.
 */
public final class Scanner {

    public record Rule(int index, String action, String name, String pattern, int line) {}
    public record Keywords(String from, List<String> words, int line) {}
    public record Token(String kind, String lexeme, int line, int col) {
        public String brief() { return kind.equals("EOF") ? "EOF" : kind + "(" + lexeme + ")"; }
        @Override public String toString() {
            return String.format("%-7s%-10s", line + ":" + col, kind) + (kind.equals("EOF") ? "" : lexeme);
        }
    }

    public static final class SpecError extends RuntimeException {
        public final int line;
        SpecError(int line, String msg) { super("spec line " + line + ": " + msg); this.line = line; }
    }

    public static final class ScanError extends RuntimeException {
        public final int line, col;
        public final char ch;
        public List<Token> partial = List.of();   // everything scanned before the bad character
        ScanError(int line, int col, char ch) {
            super(line + ":" + col + ": no token can start with " + showChar(ch));
            this.line = line; this.col = col; this.ch = ch;
        }
    }

    static String showChar(char ch) {
        if (ch == '\n') return "'\\n'";
        if (ch == '\t') return "'\\t'";
        if (!Pattern.isSourceChar(ch)) return String.format("U+%04X", (int) ch);
        return "'" + ch + "'";
    }

    final List<Rule> rules;
    final Keywords keywords;                 // null if the spec has none
    final Dfa dfa;
    final Map<String, Integer> stateRule;    // DFA state -> winning rule index
    final String sink;                       // the DFA state for "no NFA state left", or null

    private Scanner(List<Rule> rules, Keywords keywords, Dfa dfa, Map<String, Integer> stateRule, String sink) {
        this.rules = rules;
        this.keywords = keywords;
        this.dfa = dfa;
        this.stateRule = stateRule;
        this.sink = sink;
    }

    private static final java.util.regex.Pattern LINE =
        java.util.regex.Pattern.compile("^(\\S+)\\s+(\\S+)\\s*(.*)$");

    public static Scanner fromSpec(String text) {
        List<Rule> rules = new ArrayList<>();
        Keywords keywords = null;
        Set<String> names = new LinkedHashSet<>();
        String[] lines = text.split("\r?\n", -1);
        for (int i = 0; i < lines.length; i++) {
            int lineNo = i + 1;
            String line = lines[i].strip();
            if (line.isEmpty() || line.startsWith("#")) continue;
            Matcher m = LINE.matcher(line);
            if (!m.matches()) throw new SpecError(lineNo, "expected '<directive> <name> ...'");
            String directive = m.group(1), name = m.group(2), rest = m.group(3);
            if (directive.equals("keywords")) {
                if (keywords != null) throw new SpecError(lineNo, "only one 'keywords' line is allowed");
                List<String> words = rest.isEmpty() ? List.of() : Arrays.asList(rest.split("\\s+"));
                if (words.isEmpty()) throw new SpecError(lineNo, "'keywords' needs at least one word");
                keywords = new Keywords(name, words, lineNo);
                continue;
            }
            if (!directive.equals("token") && !directive.equals("discard"))
                throw new SpecError(lineNo, "expected 'token', 'discard' or 'keywords', found '" + directive + "'");
            if (rest.isEmpty()) throw new SpecError(lineNo, name + " has no pattern");
            if (!names.add(name)) throw new SpecError(lineNo, name + " is declared twice");
            rules.add(new Rule(rules.size(), directive, name, rest, lineNo));
        }
        if (rules.isEmpty()) throw new SpecError(0, "the spec declares no tokens");
        if (keywords != null) {
            String from = keywords.from();
            if (rules.stream().noneMatch(r -> r.name().equals(from) && r.action().equals("token")))
                throw new SpecError(keywords.line(), "keywords come from '" + from + "', which is not a token");
        }

        // Compile each pattern once; refuse any that accepts the empty string.
        List<Nfa> nfas = new ArrayList<>();
        for (Rule r : rules) {
            Regex re;
            try { re = Pattern.parse(r.pattern()); }
            catch (Pattern.PatternError e) { throw new SpecError(r.line(), r.name() + ": " + e.getMessage()); }
            Nfa n = Thompson.build(re);
            if (!Nfa.intersect(n.epsilonClosure(Set.of(n.start)), n.accept).isEmpty())
                throw new SpecError(r.line(),
                    r.name() + " accepts the empty string; a token must consume at least one character");
            nfas.add(n);
        }

        // One combined NFA: a fresh START with an epsilon edge into each rule's NFA.
        StringBuilder states = new StringBuilder("states: START");
        Set<String> alphabet = new TreeSet<>();
        Map<String, Integer> acceptRule = new LinkedHashMap<>();
        List<String> edges = new ArrayList<>();
        for (Rule r : rules) {
            Nfa n = nfas.get(r.index());
            String p = "r" + r.index() + "_";
            for (String st : n.states) states.append(" ").append(p).append(st);
            edges.add("START epsilon " + p + n.start);
            for (String acc : n.accept) acceptRule.put(p + acc, r.index());
            for (var e : n.delta.entrySet())
                for (var bySym : e.getValue().entrySet())
                    for (String to : bySym.getValue())
                        edges.add(p + e.getKey() + " " + bySym.getKey() + " " + p + to);
            alphabet.addAll(n.alphabet);
        }
        StringBuilder sb = new StringBuilder(states);
        sb.append("\nalphabet: ").append(String.join(" ", alphabet));
        sb.append("\nstart: START\naccept: ").append(String.join(" ", acceptRule.keySet())).append("\n");
        for (String e : edges) sb.append(e).append("\n");

        Nfa combined = Nfa.parse(sb.toString());
        Dfa dfa = Subset.determinize(combined);

        Map<String, Integer> stateRule = new LinkedHashMap<>();
        String sink = null;
        for (var e : Subset.legend(combined).entrySet()) {
            if (e.getValue().isEmpty()) sink = e.getKey();
            int best = Integer.MAX_VALUE;
            for (String s : e.getValue()) {
                Integer ri = acceptRule.get(s);
                if (ri != null && ri < best) best = ri;
            }
            if (best != Integer.MAX_VALUE) stateRule.put(e.getKey(), best);
        }
        return new Scanner(rules, keywords, dfa, stateRule, sink);
    }

    /** Longest match starting at pos: {end, rule}, or null if nothing matches. */
    int[] longestMatch(String src, int pos) {
        String state = dfa.start;
        int end = -1, rule = -1;
        for (int i = pos; i < src.length(); i++) {
            String sym = Pattern.symbolOf(src.charAt(i));
            if (!dfa.alphabet.contains(sym)) break;
            state = dfa.step(state, sym);
            if (state.equals(sink)) break;          // no rule can extend this match
            Integer ri = stateRule.get(state);
            if (ri != null) { end = i + 1; rule = ri; }
        }
        return end > pos ? new int[] { end, rule } : null;
    }

    /**
     * Tokens with 1-based line:col, ending in EOF. Discarded matches still move
     * the position, so every later token keeps its true location.
     */
    public List<Token> scan(String src) {
        List<Token> out = new ArrayList<>();
        int pos = 0, line = 1, col = 1;
        while (pos < src.length()) {
            int[] m = longestMatch(src, pos);
            if (m == null) {
                ScanError err = new ScanError(line, col, src.charAt(pos));
                err.partial = out;
                throw err;
            }
            Rule rule = rules.get(m[1]);
            String lexeme = src.substring(pos, m[0]);
            if (rule.action().equals("token")) out.add(new Token(kindOf(rule.name(), lexeme), lexeme, line, col));
            for (int k = 0; k < lexeme.length(); k++) {
                if (lexeme.charAt(k) == '\n') { line++; col = 1; } else col++;
            }
            pos = m[0];
        }
        out.add(new Token("EOF", "", line, col));
        return out;
    }

    String kindOf(String ruleName, String lexeme) {
        if (keywords != null && ruleName.equals(keywords.from()) && keywords.words().contains(lexeme))
            return lexeme.toUpperCase();
        return ruleName;
    }

    public int dfaStates()    { return dfa.states.size(); }
    public int alphabetSize() { return dfa.alphabet.size(); }
}
