import java.util.ArrayList;
import java.util.List;

/**
 * Module 15 — the two rules that turn a set of token patterns into a scanner.
 *
 *   RULE 1  Maximal munch (longest match wins):
 *           at each position, the token is the LONGEST prefix that any pattern
 *           matches. "iffy" is an identifier, not the keyword "if" followed by
 *           "fy".
 *
 *   RULE 2  Priority (ties broken by declaration order):
 *           if two patterns match the same longest length, the one declared
 *           FIRST wins. That's how "if" is a keyword and not just an identifier.
 *
 * Mechanism: for each pattern, compile regex -> minimal DFA (Modules 12-14).
 * Run the DFA from the current position, remembering the last position it was
 * in an accepting state ("the last-accept mark"). When it can go no further,
 * back up to that mark and emit a token.
 */
public final class Lexer {

    public record Rule(String name, String pattern, Dfa dfa) {
        static Rule of(String name, String pattern) {
            return new Rule(name, pattern,
                Minimize.minimal(Subset.determinize(Thompson.build(Regex.parse(pattern)))));
        }
    }

    public record Token(String kind, String text, int start) {
        @Override public String toString() { return kind + " \"" + text + "\" @" + start; }
    }

    /** The furthest exclusive end index at which `dfa` was accepting, scanning from `pos`; -1 if never. */
    static int longestAccept(Dfa dfa, String input, int pos) {
        String state = dfa.start;
        int lastAccept = dfa.accept.contains(state) ? pos : -1;   // the empty match, if the DFA accepts ""
        for (int i = pos; i < input.length(); i++) {
            String sym = String.valueOf(input.charAt(i));
            if (!dfa.alphabet.contains(sym)) break;               // character not in this pattern's alphabet
            state = dfa.step(state, sym);
            if (dfa.accept.contains(state)) lastAccept = i + 1;
        }
        return lastAccept;
    }

    /** One token at `pos` under both rules, or null if nothing matches a non-empty prefix. */
    static Token nextToken(List<Rule> rules, String input, int pos) {
        int bestLen = 0, bestRule = -1;
        for (int i = 0; i < rules.size(); i++) {
            int end = longestAccept(rules.get(i).dfa(), input, pos);
            int len = end < 0 ? 0 : end - pos;
            if (len > bestLen) {          // strictly greater -> RULE 1; on a tie the earlier rule stays -> RULE 2
                bestLen = len;
                bestRule = i;
            }
        }
        if (bestRule < 0 || bestLen == 0) return null;
        return new Token(rules.get(bestRule).name(), input.substring(pos, pos + bestLen), pos);
    }

    /** Tokenize the whole input; throws at the first position where nothing matches. */
    static List<Token> tokenize(List<Rule> rules, String input) {
        List<Token> out = new ArrayList<>();
        int pos = 0;
        while (pos < input.length()) {
            Token t = nextToken(rules, input, pos);
            if (t == null)
                throw new IllegalArgumentException(
                    "lexical error at position " + pos + " (char '" + input.charAt(pos) + "')");
            out.add(t);
            pos += t.text().length();
        }
        return out;
    }

    /** For the tutorial: the per-length match report for one pattern at one position. */
    static int[] matchLengths(Dfa dfa, String input, int pos) {
        List<Integer> hits = new ArrayList<>();
        String state = dfa.start;
        if (dfa.accept.contains(state)) hits.add(0);
        for (int i = pos; i < input.length(); i++) {
            String sym = String.valueOf(input.charAt(i));
            if (!dfa.alphabet.contains(sym)) break;
            state = dfa.step(state, sym);
            if (dfa.accept.contains(state)) hits.add(i + 1 - pos);
        }
        return hits.stream().mapToInt(Integer::intValue).toArray();
    }
}
