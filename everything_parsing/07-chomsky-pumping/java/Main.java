import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

/**
 *   java -cp java/out Main
 *
 * 1. classify four grammars (regular vs context-free)
 * 2. show a CFG generates { a^n b^n }
 * 3. run the regular pumping lemma as an adversary — it has no pumping length
 * 4. run the context-free pumping lemma against { a^n b^n c^n } — same result
 * 5. print the hierarchy
 */
public final class Main {
    static final StringBuilder sb = new StringBuilder();

    public static void main(String[] args) throws IOException {
        p("=== Module 07 - The Chomsky Hierarchy & Pumping Lemmas ===");
        p("");

        // 1. classification
        p("grammar classification:");
        for (String f : new String[]{"a-star-b", "b-a-star", "anbn", "expr"}) {
            Grammar g = load(f + ".grammar");
            GrammarClass.Kind k = GrammarClass.classify(g);
            String tag = GrammarClass.isRegular(g) ? "regular" : "not regular";
            p(String.format("  %-26s %-32s %-14s (%s)",
                "fixtures/" + f + ".grammar", clip(firstRule(g), 32), k, tag));
        }
        p("");

        // 2. a CFG for { a^n b^n }
        Grammar anbn = load("anbn.grammar");
        var lang = Derivation.enumerate(anbn, 8).stream()
            .map(w -> w.equals("epsilon") ? "epsilon" : w.replace(" ", "")).toList();
        p("a context-free grammar DOES generate { a^n b^n }:");
        p("  fixtures/anbn.grammar  enumerated to length 8:  " + String.join(", ", lang));
        p("");

        // 3. regular pumping lemma against { a^n b^n }
        p("no regular grammar can -- the regular pumping lemma has no valid pumping length:");
        boolean everyPFails = true;
        for (int pp = 1; pp <= 6; pp++) {
            var w = Pumping.regularRefutation(pp);
            everyPFails &= w.allEscaped();
            p(String.format("  p=%d: s=%-10s all %d/%d splits escape;  x=%s y=%s z=%s, pump^%d -> \"%s\"  (%s)",
                pp, w.s(), w.decompositionsTried(), w.decompositionsTried(),
                q(w.x()), q(w.y()), q(w.z()), w.k(), w.pumped(),
                Pumping.inAnBn(w.pumped()) ? "IN L?!" : "not in a^n b^n"));
        }
        p("  for EVERY p, a^p b^p is in L but no split survives pumping  ->  { a^n b^n } is NOT regular");
        p("");

        // 4. context-free pumping lemma against { a^n b^n c^n }
        p("one level up -- the context-free pumping lemma has no valid pumping length for { a^n b^n c^n }:");
        for (int pp = 1; pp <= 4; pp++) {
            var w = Pumping.cflRefutation(pp);
            everyPFails &= w.allEscaped();
            p(String.format("  p=%d: s=%-12s all %d 5-splits escape;  v=%s w=%s x=%s, pump^%d -> \"%s\"  (%s)",
                pp, w.s(), w.decompositionsTried(),
                q(w.v()), q(w.w()), q(w.x()), w.k(), w.pumped(),
                Pumping.inAnBnCn(w.pumped()) ? "IN L?!" : "not in a^n b^n c^n"));
        }
        p("  ->  { a^n b^n c^n } is context-sensitive but NOT context-free");
        p("");

        // 5. the hierarchy
        p("the hierarchy (each type STRICTLY contains the one below):");
        p("  Type 3  regular            a*, (a|b)*abb         finite automaton / regex");
        p("  Type 2  context-free       a^n b^n, balanced()   pushdown automaton");
        p("  Type 1  context-sensitive  a^n b^n c^n           linear-bounded automaton");
        p("  Type 0  recursively enum.  { <M,w> : M halts }   Turing machine");
        p("");

        p("summary: 2 regular grammars + 2 context-free classified"
            + " | { a^n b^n } not regular (pumping p=1..6)"
            + " | { a^n b^n c^n } not context-free (pumping p=1..4)"
            + " | allAdversaryRunsSucceeded=" + everyPFails);

        System.out.print(sb);
    }

    static String firstRule(Grammar g) {
        // "S -> a S | b"  style, first lhs and its alternatives
        String lhs = g.productions.get(0).lhs();
        StringBuilder r = new StringBuilder(lhs).append(" -> ");
        boolean first = true;
        for (Grammar.Production pr : g.productions) {
            if (!pr.lhs().equals(lhs)) break;
            if (!first) r.append(" | ");
            r.append(pr.rhs().isEmpty() ? "epsilon" : String.join(" ", pr.rhs()));
            first = false;
        }
        return r.toString();
    }

    static Grammar load(String name) throws IOException {
        return Grammar.parse(Files.readString(Path.of("fixtures", name)));
    }
    static void p(String s) { sb.append(s).append('\n'); }
    static String q(String s) { return s.isEmpty() ? "\"\"" : "\"" + s + "\""; }
    static String clip(String s, int n) { return s.length() <= n ? s : s.substring(0, n - 3) + "..."; }
}
