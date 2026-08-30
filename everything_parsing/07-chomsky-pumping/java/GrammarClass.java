import java.util.List;

/**
 * Module 07 — where does a grammar sit in the Chomsky hierarchy?
 *
 * We only need to tell the two lowest types apart:
 *
 *   REGULAR (Type 3)      every production is  A -> w B   or  A -> w
 *   (right-linear)        with w a string of terminals and B a single nonterminal.
 *                         Left-linear (A -> B w | w) is also regular.
 *
 *   CONTEXT_FREE (Type 2) every production has a single nonterminal on the left
 *                         (all our grammars), but some right-hand side has a
 *                         nonterminal that isn't at the very end/start, or has
 *                         two nonterminals.
 *
 * The point of the classification: a REGULAR grammar is subject to the pumping
 * lemma for regular languages, and `Pumping` shows that lemma has no valid
 * pumping length for { a^n b^n } — so no regular grammar generates it.
 */
public final class GrammarClass {
    private GrammarClass() {}

    public enum Kind { RIGHT_LINEAR, LEFT_LINEAR, CONTEXT_FREE }

    static Kind classify(Grammar g) {
        boolean allRight = true, allLeft = true;
        for (Grammar.Production p : g.productions) {
            List<String> rhs = p.rhs();
            int nts = 0, firstNt = -1, lastNt = -1;
            for (int i = 0; i < rhs.size(); i++) {
                if (g.isNonterminal(rhs.get(i))) {
                    nts++;
                    if (firstNt < 0) firstNt = i;
                    lastNt = i;
                }
            }
            if (nts == 0) continue;                       // A -> w : fine for both
            if (nts > 1) { allRight = false; allLeft = false; continue; }
            if (lastNt != rhs.size() - 1) allRight = false;  // nonterminal not at the end
            if (firstNt != 0) allLeft = false;               // nonterminal not at the start
        }
        if (allRight) return Kind.RIGHT_LINEAR;
        if (allLeft)  return Kind.LEFT_LINEAR;
        return Kind.CONTEXT_FREE;
    }

    static boolean isRegular(Grammar g) {
        Kind k = classify(g);
        return k == Kind.RIGHT_LINEAR || k == Kind.LEFT_LINEAR;
    }
}
