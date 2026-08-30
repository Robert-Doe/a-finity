import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

/**
 * Module 16 — a complete lexical analyzer, the way ASU CSE 340's Project 1
 * builds it.
 *
 *   INPUT   a token-spec file: one "NAME  regex" per line, in priority order,
 *           plus optional "%skip NAME" lines
 *   BUILD   compile every pattern's regex -> NFA (Thompson), UNION them under a
 *           fresh start with a tag on each accept state, then DETERMINIZE.
 *           Each combined-DFA state now knows the highest-priority token it
 *           accepts.
 *   SCAN    one left-to-right pass, maximal munch: run the combined DFA,
 *           remember the last (position, token) it accepted, back up to it,
 *           emit; skip %skip tokens; a stuck position with no accept is a
 *           lexical error with an exact column.
 *   CHECK   at load time, reject any pattern that can match the empty string
 *           ("epsilon IS NOOOOOT A TOKEN").
 */
public final class SpecLexer {

    public record Rule(int index, String name, String pattern) {}
    public record Token(String kind, String lexeme, int pos) {
        @Override public String toString() {
            return String.format("%-8s \"%s\"", kind, lexeme) + " @" + pos;
        }
    }

    final List<Rule> rules;
    final Set<String> skip;
    final Dfa dfa;
    final Map<String, Integer> stateToken;   // combined-DFA state -> winning rule index (or absent)

    private SpecLexer(List<Rule> rules, Set<String> skip, Dfa dfa, Map<String, Integer> stateToken) {
        this.rules = rules;
        this.skip = skip;
        this.dfa = dfa;
        this.stateToken = stateToken;
    }

    // ── load a spec ──

    public static SpecLexer load(String specText) {
        List<Rule> rules = new ArrayList<>();
        Set<String> skip = new LinkedHashSet<>();
        for (String raw : specText.split("\r?\n")) {
            int h = raw.indexOf('#');
            String line = (h >= 0 ? raw.substring(0, h) : raw).stripTrailing();
            if (line.strip().isEmpty()) continue;
            if (line.strip().startsWith("%skip")) {
                skip.add(line.strip().split("\\s+")[1]);
                continue;
            }
            // NAME  regex   (split on the first run of whitespace)
            String s = line.strip();
            int sp = firstSpace(s);
            if (sp < 0) throw new IllegalArgumentException("spec line has no regex: " + s);
            String name = s.substring(0, sp);
            String pattern = s.substring(sp).strip();
            rules.add(new Rule(rules.size(), name, pattern));
        }
        if (rules.isEmpty()) throw new IllegalArgumentException("spec has no token rules");

        // "epsilon IS NOOOOOT A TOKEN" — reject any pattern that matches ""
        for (Rule r : rules) {
            Nfa n = Thompson.build(Regex.parse(r.pattern()));
            if (!Nfa.intersect(n.epsilonClosure(Set.of(n.start)), n.accept).isEmpty())
                throw new IllegalArgumentException(
                    "token '" + r.name() + "' can match the empty string  (epsilon IS NOOOOOT A TOKEN)");
        }

        // build the combined, tagged NFA
        StringBuilder sb = new StringBuilder("states: START");
        Set<String> alphabet = new TreeSet<>();
        Map<String, Integer> acceptRule = new LinkedHashMap<>();
        List<String> trans = new ArrayList<>();
        for (Rule r : rules) {
            Nfa n = Thompson.build(Regex.parse(r.pattern()));
            String p = "r" + r.index() + "_";
            for (String st : n.states) sb.append(" ").append(p).append(st);
            trans.add("START epsilon " + p + n.start);
            for (String acc : n.accept) acceptRule.put(p + acc, r.index());
            for (var e : n.delta.entrySet())
                for (var bySym : e.getValue().entrySet())
                    for (String to : bySym.getValue())
                        trans.add(p + e.getKey() + " " + bySym.getKey() + " " + p + to);
            alphabet.addAll(n.alphabet);
        }
        sb.append("\nalphabet: ").append(String.join(" ", alphabet));
        sb.append("\nstart: START\naccept:");
        for (String a : acceptRule.keySet()) sb.append(" ").append(a);
        sb.append("\n");
        for (String t : trans) sb.append(t).append("\n");

        Nfa combined = Nfa.parse(sb.toString());
        Dfa dfa = Subset.determinize(combined);
        var legend = Subset.legend(combined);

        Map<String, Integer> stateToken = new LinkedHashMap<>();
        for (var e : legend.entrySet()) {
            int best = Integer.MAX_VALUE;
            for (String nfaState : e.getValue()) {
                Integer ri = acceptRule.get(nfaState);
                if (ri != null && ri < best) best = ri;
            }
            if (best != Integer.MAX_VALUE) stateToken.put(e.getKey(), best);
        }
        return new SpecLexer(rules, skip, dfa, stateToken);
    }

    // ── scan ──

    public List<Token> tokenize(String input) {
        List<Token> out = new ArrayList<>();
        int pos = 0;
        while (pos < input.length()) {
            String state = dfa.start;
            int lastPos = -1, lastRule = -1;
            Integer atStart = stateToken.get(state);
            if (atStart != null) { lastPos = pos; lastRule = atStart; }
            for (int i = pos; i < input.length(); i++) {
                String sym = String.valueOf(input.charAt(i));
                if (!dfa.alphabet.contains(sym)) break;
                state = dfa.step(state, sym);
                Integer ri = stateToken.get(state);
                if (ri != null) { lastPos = i + 1; lastRule = ri; }
            }
            if (lastPos <= pos)
                throw new LexicalError(pos, input.charAt(pos));
            String lexeme = input.substring(pos, lastPos);
            String kind = rules.get(lastRule).name();
            if (!skip.contains(kind)) out.add(new Token(kind, lexeme, pos));
            pos = lastPos;
        }
        return out;
    }

    public int combinedDfaStates() { return dfa.states.size(); }

    public static final class LexicalError extends RuntimeException {
        public final int pos; public final char ch;
        LexicalError(int pos, char ch) {
            super("lexical error at position " + pos + " (char '" + ch + "')");
            this.pos = pos; this.ch = ch;
        }
    }

    // ── helpers ──

    private static int firstSpace(String s) {
        for (int i = 0; i < s.length(); i++) if (Character.isWhitespace(s.charAt(i))) return i;
        return -1;
    }
}
