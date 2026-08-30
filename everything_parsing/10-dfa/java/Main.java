import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

/**
 *   java -cp java/out Main
 *
 * Simulates three hand-written DFAs, prints their transition tables and sample
 * runs, enumerates their languages, cross-checks one against the Module-3 regex
 * engine, and shows a DFA's pigeonhole repeat — the concrete face of "a DFA
 * cannot count".
 */
public final class Main {
    static final StringBuilder sb = new StringBuilder();

    public static void main(String[] args) throws IOException {
        p("=== Module 10 - DFA: Deterministic Finite Automata ===");
        p("");

        section("ends-abb.dfa", "the regex (a|b)*abb",
            new String[]{"abb", "aabb", "ab", "abba", ""});
        // cross-check against Module 3's regex engine
        Dfa d = load("ends-abb.dfa");
        Regex re = Regex.parse("(a|b)*abb");
        int bound = 6;
        var dfaLang = d.language(bound).stream().filter(s -> !s.equals("epsilon")).toList();
        var reLang  = re.toLanguage(bound).list().stream().filter(s -> !s.isEmpty()).toList();
        boolean same = new java.util.HashSet<>(dfaLang).equals(new java.util.HashSet<>(reLang));
        p("cross-check vs regex (a|b)*abb, both up to length " + bound + ":");
        p("  DFA language == regex language:  " + same + "   (" + dfaLang.size() + " strings)");
        p("");

        section("even-a.dfa", "even number of a's",
            new String[]{"", "a", "aa", "bab", "abba"});

        section("a-star-b-star.dfa", "a*b*, NOT a^n b^n",
            new String[]{"aaab", "abb", "ba", "aabb"});
        Dfa g = load("a-star-b-star.dfa");
        int[] rep = g.repeatOn("a");
        p("why a*b* is not { a^n b^n }:");
        p("  accepts \"aaab\"? " + g.accepts(chars("aaab"))
            + "    accepts \"abb\"? " + g.accepts(chars("abb"))
            + "    accepts \"ba\"? " + g.accepts(chars("ba")));
        p("  repeatOn('a'): the DFA is in the SAME state after " + plur(rep[0]) + " and after " + plur(rep[1]));
        p("  -> it keeps no count of a's. Module 7 proved NO dfa can accept { a^n b^n }.");
        p("     a*b* is the closest regular over-approximation.");
        p("");

        p("summary: 3 DFAs simulated | DFA(ends-abb) language == regex (a|b)*abb == " + same
            + " | a*b* accepts \"aaab\" (so it is not { a^n b^n })");

        System.out.print(sb);
    }

    static void section(String file, String desc, String[] tests) throws IOException {
        Dfa d = load(file);
        p("--- fixtures/" + file + "  (" + desc + ") ---");
        p("states: " + String.join(" ", d.states) + "    alphabet: " + String.join(" ", d.alphabet));
        p("start: " + d.start + "    accept: " + String.join(" ", d.accept));
        p("");
        p("transition table  ( -> start,  * accepting ):");
        p(d.transitionTable());
        p("");
        p("runs:");
        for (String t : tests) {
            var trace = d.trace(chars(t));
            String shown = t.isEmpty() ? "\"\"" : "\"" + t + "\"";
            p(String.format("  %-8s -> %-20s %s", shown, String.join(" ", trace),
                d.accepts(chars(t)) ? "ACCEPT" : "reject"));
        }
        p("");
        var lang = d.language(5);
        String shown = lang.size() <= 15 ? String.join(", ", lang)
            : String.join(", ", lang.subList(0, 15)) + ", ... (" + lang.size() + " total)";
        p("language (length <= 5): " + shown);
        p("");
    }

    static Dfa load(String name) throws IOException {
        return Dfa.parse(Files.readString(Path.of("fixtures", name)));
    }
    static List<String> chars(String s) {
        List<String> out = new ArrayList<>();
        for (int i = 0; i < s.length(); i++) out.add(String.valueOf(s.charAt(i)));
        return out;
    }
    static String plur(int n) { return n == 1 ? "1 'a'" : n + " 'a's"; }
    static void p(String s) { sb.append(s).append('\n'); }
}
