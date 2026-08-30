import java.util.List;
import java.util.Set;
import java.util.TreeSet;

/**
 *   java -cp java/out Main
 *
 * 1. minimize the subset-construction DFA for (a|b)*abb; show the partition,
 *    check both algorithms agree, print the minimal DFA
 * 2. show it is isomorphic to Module 10's hand-written DFA (Myhill-Nerode: the
 *    minimal DFA is unique)
 * 3. use minimal-DFA isomorphism to decide regex equivalence
 */
public final class Main {
    static final StringBuilder sb = new StringBuilder();

    public static void main(String[] args) {
        p("=== Module 14 - DFA Minimization ===");
        p("");

        Dfa subset = Subset.determinize(Thompson.build(Regex.parse("(a|b)*abb")));
        p("subset-construction DFA for /(a|b)*abb/:  " + subset.states.size() + " states");
        p("");

        var byRefine = Minimize.partitionRefinement(subset);
        var byTable  = Minimize.tableFilling(subset);
        p("partition refinement -> " + byRefine.size() + " equivalence classes:");
        for (Set<String> c : byRefine) p("  " + set(c));
        p("");
        p("table filling -> " + byTable.size() + " classes;  same partition as refinement:  "
            + samePartition(byRefine, byTable));
        p("");

        Dfa min = Minimize.minimal(subset);
        p("minimal DFA:  " + min.states.size() + " states");
        p(min.transitionTable());
        p("");

        Dfa hand = Dfa.parse("""
            states: S A AB ABB
            alphabet: a b
            start: S
            accept: ABB
            S a A
            S b S
            A a A
            A b AB
            AB a A
            AB b ABB
            ABB a A
            ABB b S
            """);
        p("Module 10's hand-written DFA:  " + hand.states.size() + " states (already minimal: "
            + (Minimize.minimal(hand).states.size() == hand.states.size()) + ")");
        p("minimal DFA is isomorphic to the hand-written DFA:  " + Minimize.isomorphic(min, hand));
        p("");
        p("Myhill-Nerode:  L(/(a|b)*abb/) has exactly " + min.states.size()
            + " right-language classes  =  " + min.states.size() + " states, and that DFA is unique.");
        p("");

        // ── regex equivalence ──
        p("--- regex equivalence (decided via minimal-DFA isomorphism) ---");
        String[][] pairs = {
            {"(a|b)*", "(a*b*)*", "both = Sigma*"},
            {"a**", "a*", ""},
            {"a(b|c)", "ab|ac", "concatenation distributes over |"},
            {"ab|ba", "(a|b)(a|b)", "RHS also matches aa, bb"},
            {"a*", "a+", "a+ excludes the empty string"},
            {"(a|b)*abb", "(a|b)*abb", ""},
        };
        for (String[] pr : pairs) {
            boolean eq = Minimize.equivalent(Regex.parse(pr[0]), Regex.parse(pr[1]));
            p(String.format("  %-14s == %-14s :  %-6s %s",
                "/" + pr[0] + "/", "/" + pr[1] + "/", eq, pr[2].isEmpty() ? "" : "(" + pr[2] + ")"));
        }
        p("");

        p("summary: subset DFA " + subset.states.size() + " -> minimal " + min.states.size()
            + " | partition-refinement and table-filling agree | minimal DFA is unique (Myhill-Nerode)"
            + " -> regex equivalence is decidable");

        System.out.print(sb);
    }

    static String set(Set<String> c) {
        return "{ " + String.join(", ", new TreeSet<>(c)) + " }";
    }
    static boolean samePartition(List<Set<String>> a, List<Set<String>> b) {
        Set<Set<String>> sa = new java.util.HashSet<>();
        for (var s : a) sa.add(new TreeSet<>(s));
        Set<Set<String>> sb = new java.util.HashSet<>();
        for (var s : b) sb.add(new TreeSet<>(s));
        return sa.equals(sb);
    }
    static void p(String s) { sb.append(s).append('\n'); }
}
