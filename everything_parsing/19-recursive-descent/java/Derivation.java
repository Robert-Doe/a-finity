import java.util.ArrayList;
import java.util.List;
import java.util.Set;

/**
 * Replays a list of productions as a LEFTMOST derivation.
 *
 * Start at the sentential form [E]. For each production in the list, find the
 * leftmost nonterminal in the current form and replace it with the production's
 * right-hand side. If the list really is the firing order of a recursive-descent
 * parse, then (a) every production's left-hand side matches the leftmost
 * nonterminal at that step — never anything else — and (b) the final form is
 * exactly the input's token kinds. That is the proof that "the call stack is the
 * leftmost derivation".
 */
public final class Derivation {

    private static final Set<String> NONTERMINALS = Set.of("E", "E'", "T", "T'", "F");

    public record Step(String form, String rule) {}

    public static List<Step> replay(List<String> rules) {
        List<String> form = new ArrayList<>(List.of("E"));
        List<Step> steps = new ArrayList<>();
        steps.add(new Step(join(form), null));

        for (String rule : rules) {
            int i = leftmostNonterminal(form);
            if (i < 0)
                throw new IllegalStateException("form has no nonterminal but rule remains: " + rule);

            String[] halves = rule.split(" -> ", 2);
            String lhs = halves[0].trim();
            if (!form.get(i).equals(lhs))
                throw new IllegalStateException(
                    "rule '" + rule + "' expands " + lhs + " but the leftmost nonterminal is " + form.get(i)
                    + " — the derivation is not leftmost");

            List<String> rhs = new ArrayList<>();
            for (String sym : halves[1].trim().split("\\s+"))
                if (!sym.equals("epsilon")) rhs.add(sym);

            form.remove(i);
            form.addAll(i, rhs);
            steps.add(new Step(join(form), rule));
        }
        return steps;
    }

    /** Just the last sentential form — the sentence the derivation produces. */
    public static String sentence(List<String> rules) {
        List<Step> s = replay(rules);
        return s.get(s.size() - 1).form();
    }

    private static int leftmostNonterminal(List<String> form) {
        for (int i = 0; i < form.size(); i++)
            if (NONTERMINALS.contains(form.get(i))) return i;
        return -1;
    }

    private static String join(List<String> form) {
        return form.isEmpty() ? "epsilon" : String.join(" ", form);
    }
}
