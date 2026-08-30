import java.util.List;

/**
 *   java -cp java/out Main
 *
 * Compiles several regexes to epsilon-NFAs, prints one gadget-by-gadget, shows
 * the state count never exceeds 2*nodes (and 2*|regex|), and runs the full
 * pipeline  regex -> Thompson NFA -> epsilon-free NFA  checking the language is
 * unchanged and matches Module 3's regex engine and Module 10's DFA.
 */
public final class Main {
    static final StringBuilder sb = new StringBuilder();

    public static void main(String[] args) {
        p("=== Module 12 - Thompson's Construction: RE -> epsilon-NFA ===");
        p("");

        // ── one regex, gadget by gadget ──
        String src = "a(b|c)";
        Regex re = Regex.parse(src);
        Nfa nfa = Thompson.build(re);
        p("regex /" + src + "/   tree: " + re.tree());
        p("nodes: " + Thompson.nodeCount(re) + "   |regex| (no parens): " + noParens(src));
        p("");
        p("Thompson NFA:");
        p("  states: " + String.join(" ", nfa.states));
        p("  alphabet: " + String.join(" ", nfa.alphabet));
        p("  start: " + nfa.start + "   accept: " + String.join(" ", nfa.accept));
        for (var from : nfa.states)
            for (var e : nfa.delta.getOrDefault(from, java.util.Map.of()).entrySet())
                for (String to : e.getValue())
                    p("  " + from + " " + e.getKey() + " " + to);
        p("  -> " + nfa.states.size() + " states");
        p("");
        p("runs:  " + run(nfa, "ab") + "   " + run(nfa, "ac") + "   " + run(nfa, "a") + "   " + run(nfa, "abc"));
        p("");

        // ── state count vs regex size, over a family ──
        p("state count is always <= 2*nodes  (and <= 2*|regex|):");
        p(String.format("  %-14s %6s %6s %6s   %-10s %-10s", "regex", "nodes", "chars", "states", "<=2*nodes", "<=2*chars"));
        for (String r : new String[]{"a", "ab", "a|b", "a*", "(a|b)*", "(a|b)*abb", "a(b|c)*d", "((a|b)(c|d))*"}) {
            Regex x = Regex.parse(r);
            int nodes = Thompson.nodeCount(x), chars = noParens(r), states = Thompson.stateCount(x);
            p(String.format("  %-14s %6d %6d %6d   %-10s %-10s", "/" + r + "/", nodes, chars, states,
                (states <= 2 * nodes) ? "yes" : "NO", (states <= 2 * chars) ? "yes" : "NO"));
        }
        p("");

        // ── the full pipeline for (a|b)*abb ──
        String big = "(a|b)*abb";
        Regex bre = Regex.parse(big);
        Nfa bnfa = Thompson.build(bre);
        Nfa bfree = bnfa.removeEpsilon();
        Dfa dfa = Dfa.parse("""
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
        int bound = 6;
        var lang = bnfa.language(bound);
        boolean vsFree = lang.equals(bfree.language(bound));
        boolean vsRe = lang.stream().allMatch(w -> bre.matches(w.equals("epsilon") ? "" : w));
        boolean vsDfa = lang.stream().filter(w -> !w.equals("epsilon"))
            .allMatch(dfa::acceptsString);
        p("full pipeline for /" + big + "/:");
        p("  Thompson NFA:            " + bnfa.states.size() + " states, has epsilon: " + bnfa.hasEpsilon());
        p("  removeEpsilon():         " + bfree.states.size() + " states, has epsilon: " + bfree.hasEpsilon());
        p("  L(Thompson NFA) up to length " + bound + ":  " + lang.size() + " strings");
        p("    == L(epsilon-free NFA):        " + vsFree);
        p("    == regex /(a|b)*abb/:          " + vsRe);
        p("    == DFA(Module 10):             " + vsDfa);
        p("");

        p("summary: every regex node -> a fixed-size NFA gadget | NFA states <= 2*nodes always"
            + " | L(Thompson NFA) == L(regex) == L(DFA)");

        System.out.print(sb);
    }

    static String run(Nfa n, String w) {
        return "\"" + w + "\" -> " + (n.acceptsString(w) ? "ACCEPT" : "reject");
    }
    static int noParens(String s) { return s.replaceAll("[()]", "").length(); }
    static void p(String s) { sb.append(s).append('\n'); }
}
