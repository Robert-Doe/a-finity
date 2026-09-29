import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;

/**
 *   java -cp java/out Main
 *
 * 1. run a nondeterministic NFA for (a|b)*abb, showing the state SET evolve
 * 2. cross-check its language against Module 10's DFA and Module 3's regex
 * 3. take an epsilon-NFA, compute an epsilon-closure, then remove all epsilon
 *    transitions and confirm the language is unchanged
 */
public final class Main {
    static final StringBuilder sb = new StringBuilder();

    public static void main(String[] args) throws IOException {
        p("=== Module 11 - NFA and epsilon-NFA ===");
        p("");

        // ── 1. the nondeterministic NFA ──
        Nfa abb = load("ends-abb.nfa");
        p("--- fixtures/ends-abb.nfa   (nondeterministic, for (a|b)*abb) ---");
        p("states: " + String.join(" ", abb.states) + "    alphabet: " + String.join(" ", abb.alphabet));
        p("start: " + abb.start + "    accept: " + String.join(" ", abb.accept));
        p("  S has TWO transitions on 'a':  S -a-> S   and   S -a-> S1   (the 'guess')");
        p("");
        p("run \"aabb\"  (tracking the SET of possible states):");
        var tr = abb.trace(chars("aabb"));
        String[] labels = {"start", "a", "a", "b", "b"};
        for (int i = 0; i < tr.size(); i++)
            p(String.format("  after %-6s %s", labels[i], Nfa.showSet(tr.get(i))));
        p("  final set contains S3 (accepting)?  " + tr.get(tr.size() - 1).contains("S3") + "   -> ACCEPT");
        p("");

        // cross-checks
        Dfa dfa = Dfa.parse(Files.readString(Path.of("..", "10-dfa", "fixtures", "ends-abb.dfa")));
        Regex re = Regex.parse("(a|b)*abb");
        int bound = 6;
        var nfaLang = abb.language(bound).stream().filter(s -> !s.equals("epsilon")).toList();
        boolean vsDfa = new java.util.HashSet<>(nfaLang).equals(new java.util.HashSet<>(
            dfa.language(bound).stream().filter(s -> !s.equals("epsilon")).toList()));
        boolean vsRe = nfaLang.stream().allMatch(re::matches);
        p("cross-check, all up to length " + bound + ":");
        p("  NFA language == DFA language (Module 10):  " + vsDfa + "   (" + nfaLang.size() + " strings)");
        p("  every NFA-accepted string is regex-matched:  " + vsRe);
        p("");

        // ── 2. the epsilon-NFA ──
        Nfa eps = load("ab-star.nfa");
        p("--- fixtures/ab-star.nfa   (epsilon-NFA for (a|b)*) ---");
        p("has epsilon transitions?  " + eps.hasEpsilon());
        p("");
        p("epsilon-closure of {I} (the start):  " + Nfa.showSet(eps.epsilonClosure(Set.of("I"))));
        p("  -> before reading any input, the NFA is 'in' all of these at once,");
        p("     including F, so the empty string is accepted (zero repetitions).");
        p("");
        p("run \"ab\":");
        var etr = eps.trace(chars("ab"));
        String[] elabels = {"start", "a", "b"};
        for (int i = 0; i < etr.size(); i++)
            p(String.format("  after %-6s %s", elabels[i], Nfa.showSet(etr.get(i))));
        p("");

        // ── 3. epsilon elimination ──
        Nfa noEps = eps.removeEpsilon();
        p("removeEpsilon():  new NFA has epsilon transitions?  " + noEps.hasEpsilon());
        p("  new accepting states: " + String.join(" ", noEps.accept)
            + "   (every state whose epsilon-closure reached the old F)");
        int lb = 5;
        boolean sameLang = eps.language(lb).equals(noEps.language(lb));
        Regex abStar = Regex.parse("(a|b)*");
        boolean vsReStar = noEps.language(lb).stream().allMatch(w -> abStar.matches(w.equals("epsilon") ? "" : w));
        p("  L(epsilon-NFA) == L(epsilon-free NFA), up to length " + lb + ":  " + sameLang);
        p("  ...and both == regex (a|b)*:  " + vsReStar);
        p("  language: " + String.join(", ", noEps.language(lb).subList(0, Math.min(10, noEps.language(lb).size())))
            + (noEps.language(lb).size() > 10 ? ", ..." : ""));
        p("");

        p("summary: NFA(a|b)*abb == DFA == regex (" + vsDfa + ") | epsilon-closure computed"
            + " | epsilon removed, language unchanged (" + sameLang + ") -> epsilon and nondeterminism add NO power");

        System.out.print(sb);
    }

    static Nfa load(String name) throws IOException {
        return Nfa.parse(Files.readString(Path.of("fixtures", name)));
    }
    static List<String> chars(String s) {
        List<String> out = new ArrayList<>();
        for (int i = 0; i < s.length(); i++) out.add(String.valueOf(s.charAt(i)));
        return out;
    }
    static void p(String s) { sb.append(s).append('\n'); }
}
