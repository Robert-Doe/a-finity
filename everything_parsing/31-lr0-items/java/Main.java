import java.nio.file.Files;
import java.nio.file.Path;

public final class Main {

    public static void main(String[] args) throws Exception {
        StringBuilder sb = new StringBuilder();
        sb.append("=== Module 31 - LR(0) Items & the Canonical Collection ===\n\n");
        sb.append("item      : a production with a dot -- E -> E . plus T\n");
        sb.append("CLOSURE(I): dot before a nonterminal B -> pull in B -> . g for every B rule\n");
        sb.append("GOTO(I, X): move the dot past X in every item that has it there, then CLOSURE\n");
        sb.append("the item sets are DFA states; the DFA recognizes viable prefixes (Module 30).\n\n");

        for (String file : new String[]{"fixtures/parens.grammar", "fixtures/expr.grammar"}) {
            Grammar g = Grammar.parse(Files.readString(Path.of(file)));
            Lr0 lr = new Lr0(g);

            sb.append("##################### ").append(file).append(" #####################\n\n");
            sb.append(lr.render());
            sb.append('\n');
            sb.append(lr.states.size()).append(" states.\n");

            if (lr.isLR0()) {
                sb.append("LR(0) = YES  -- no state has (shift + complete item) or two complete items.\n");
            } else {
                sb.append("LR(0) = NO   -- ").append(lr.conflicts().size()).append(" conflict(s):\n");
                for (Lr0.Conflict c : lr.conflicts())
                    sb.append("   I").append(c.state()).append("  ").append(c.kind())
                      .append(": ").append(c.detail()).append('\n');
                sb.append("   -> SLR(1) (Module 32) uses FOLLOW to decide when to reduce.\n");
            }
            sb.append('\n');
        }

        System.out.print(sb.toString().replace("\r\n", "\n"));
    }
}
