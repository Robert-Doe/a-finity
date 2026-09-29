import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

/**
 * Module 02 — a context-free grammar, stored as data.
 *
 * A grammar is (N, Sigma, P, S):
 *   N     = nonterminals  — every symbol that appears on a left-hand side
 *   Sigma = terminals     — every other symbol that appears in a right-hand side
 *   P     = productions    — ordered; each is  lhs -> [rhs symbols]  (empty = epsilon)
 *   S     = start symbol   — the first lhs, unless a "%start X" line overrides it
 *
 * File format (see fixtures/*.grammar):
 *   # comment to end of line
 *   %start S
 *   S -> a S b | epsilon
 *   symbols are whitespace-separated; 'epsilon' or an empty alternative is the empty string
 */
public final class Grammar {

    /** One production. An empty rhs list means epsilon (S -> ). */
    record Production(int index, String lhs, List<String> rhs) {
        boolean isEpsilon() { return rhs.isEmpty(); }
        @Override public String toString() {
            return lhs + " -> " + (rhs.isEmpty() ? "epsilon" : String.join(" ", rhs));
        }
    }

    final List<Production> productions;
    final String start;
    final Set<String> nonterminals;   // insertion order = order of first appearance as lhs

    private Grammar(List<Production> productions, String start, Set<String> nonterminals) {
        this.productions = productions;
        this.start = start;
        this.nonterminals = nonterminals;
    }

    boolean isNonterminal(String sym) { return nonterminals.contains(sym); }

    /** Terminals: rhs symbols that are never a lhs (and not the epsilon keyword). */
    Set<String> terminals() {
        Set<String> t = new LinkedHashSet<>();
        for (Production p : productions)
            for (String s : p.rhs())
                if (!nonterminals.contains(s)) t.add(s);
        return t;
    }

    List<Production> productionsFor(String nt) {
        List<Production> out = new ArrayList<>();
        for (Production p : productions) if (p.lhs().equals(nt)) out.add(p);
        return out;
    }

    // ─────────────────────────────────────────── the file loader

    static Grammar parse(String text) {
        List<String[]> rawRules = new ArrayList<>(); // {lhs, rhsAlternativesJoinedByBar}
        String explicitStart = null;

        for (String line : text.split("\r?\n")) {
            int hash = line.indexOf('#');
            if (hash >= 0) line = line.substring(0, hash);
            line = line.strip();
            if (line.isEmpty()) continue;

            if (line.startsWith("%start")) {
                String[] parts = line.split("\\s+");
                if (parts.length != 2)
                    throw new IllegalArgumentException("bad %start line: " + line);
                explicitStart = parts[1];
                continue;
            }

            int arrow = line.indexOf("->");
            if (arrow < 0)
                throw new IllegalArgumentException("rule has no '->': " + line);
            if (line.indexOf("->", arrow + 2) >= 0)
                throw new IllegalArgumentException("rule has more than one '->': " + line);

            String lhs = line.substring(0, arrow).strip();
            if (lhs.isEmpty() || lhs.contains(" "))
                throw new IllegalArgumentException("left-hand side must be one symbol: " + line);
            String rhs = line.substring(arrow + 2).strip();
            rawRules.add(new String[]{lhs, rhs});
        }
        if (rawRules.isEmpty())
            throw new IllegalArgumentException("grammar has no rules");

        Set<String> nts = new LinkedHashSet<>();
        for (String[] r : rawRules) nts.add(r[0]);

        List<Production> prods = new ArrayList<>();
        for (String[] r : rawRules) {
            String lhs = r[0];
            for (String alt : r[1].split("\\|")) {
                alt = alt.strip();
                List<String> syms = new ArrayList<>();
                if (!alt.isEmpty() && !alt.equals("epsilon")) {
                    for (String s : alt.split("\\s+")) {
                        if (s.equals("epsilon"))
                            throw new IllegalArgumentException(
                                "'epsilon' cannot be mixed with other symbols: " + r[1]);
                        syms.add(s);
                    }
                }
                prods.add(new Production(prods.size(), lhs, syms));
            }
        }

        String start = explicitStart != null ? explicitStart : rawRules.get(0)[0];
        if (!nts.contains(start))
            throw new IllegalArgumentException("start symbol '" + start + "' is not a nonterminal");

        return new Grammar(prods, start, nts);
    }
}
