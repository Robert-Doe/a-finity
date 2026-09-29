import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;

/**
 *   java -cp java/out Main
 *
 * 1. determinize the Thompson NFA for (a|b)*abb; show a few DFA states as the
 *    SETS of NFA states they stand for; cross-check the language
 * 2. show the exponential blow-up: NFA "k-th from the end is a" has k+1 states,
 *    its DFA has 2^k
 * 3. the complete pipeline: regex -> NFA -> DFA, language preserved at each step
 */
public final class Main {
    static final StringBuilder sb = new StringBuilder();

    public static void main(String[] args) {
        p("=== Module 13 - Subset Construction: NFA -> DFA ===");
        p("");

        // ── 1. determinize the Thompson NFA ──
        Regex re = Regex.parse("(a|b)*abb");
        Nfa nfa = Thompson.build(re);
        Dfa dfa = Subset.determinize(nfa);
        Map<String, Set<String>> leg = Subset.legend(nfa);

        p("regex /(a|b)*abb/  ->  Thompson NFA (" + nfa.states.size() + " states, epsilon: "
            + nfa.hasEpsilon() + ")  ->  subset-construction DFA (" + dfa.states.size() + " states)");
        p("");
        p("each DFA state is the SET of NFA states a run could be in:");
        int shown = 0;
        for (var e : leg.entrySet()) {
            p("  " + pad(e.getKey(), 4) + " = " + set(e.getValue())
                + (dfa.accept.contains(e.getKey()) ? "   (accepting)" : ""));
            if (++shown == 8) { p("  ... (" + leg.size() + " DFA states total)"); break; }
        }
        p("");

        int bound = 7;
        var nfaLang = nfa.language(bound);
        boolean same = nfaLang.equals(dfa.language(bound));
        boolean vsRe = nfaLang.stream().allMatch(w -> re.matches(w.equals("epsilon") ? "" : w));
        p("L(NFA) up to length " + bound + " == L(DFA):  " + same + "   (" + nfaLang.size() + " strings)");
        p("  ...and == regex /(a|b)*abb/:  " + vsRe);
        p("");

        // ── 2. the exponential blow-up ──
        p("exponential blow-up:  NFA \"the k-th symbol from the end is 'a'\" (over {a,b})");
        p(String.format("  %3s  %-12s  %-12s  %s", "k", "NFA states", "DFA states", "2^k"));
        for (int k = 1; k <= 5; k++) {
            Nfa n = Subset.kthFromEndNfa(k);
            Dfa d = Subset.determinize(n);
            int pow = 1 << k;
            p(String.format("  %3d  %-12d  %-12d  %d", k, n.states.size(), d.states.size(), pow)
                + (d.states.size() == pow ? "   (matches 2^k)" : ""));
        }
        Nfa n3 = Subset.kthFromEndNfa(3);
        Dfa d3 = Subset.determinize(n3);
        p("  spot-check k=3:  \"abaa\" (3rd from end = 'b')  DFA: "
            + (d3.acceptsString("abaa") ? "ACCEPT" : "reject")
            + "   \"baab\" (3rd from end = 'a')  DFA: "
            + (d3.acceptsString("baab") ? "ACCEPT" : "reject"));
        p("");

        // ── 3. the complete pipeline ──
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
        boolean vsHand = dfa.language(bound).equals(hand.language(bound));
        p("complete pipeline:  regex  ->  NFA (" + nfa.states.size() + ")  ->  DFA (" + dfa.states.size()
            + ")   -- and the DFA's language == the hand-written DFA from Module 10:  " + vsHand);
        p("");

        p("summary: subset construction: DFA state = set of NFA states | L preserved (" + same + ")"
            + " | worst case 2^k states (k-th-from-end), verified for k=1..5");

        System.out.print(sb);
    }

    static String set(Set<String> s) {
        return "{" + String.join(",", new TreeSet<>(s)) + "}";
    }
    static String pad(String s, int w) {
        StringBuilder b = new StringBuilder(s);
        while (b.length() < w) b.append(' ');
        return b.toString();
    }
    static void p(String s) { sb.append(s).append('\n'); }
}
