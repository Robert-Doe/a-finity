import java.nio.file.Files;
import java.nio.file.Path;

public final class Main {

    static final String[] FILES = {
        "fixtures/abc.grammar",
        "fixtures/danglingelse.grammar",
        "fixtures/expr.grammar",
    };

    public static void main(String[] args) throws Exception {
        StringBuilder sb = new StringBuilder();
        sb.append("=== Module 22 - FOLLOW Sets (least fixed point, given FIRST) ===\n\n");
        sb.append("rule 1:  $ in FOLLOW(start)\n");
        sb.append("rule 2:  B -> a A b            =>  FIRST(b)\\{epsilon}  in FOLLOW(A)\n");
        sb.append("rule 3:  B -> a A b, b nullable =>  FOLLOW(B)          in FOLLOW(A)\n");
        sb.append("         (b nullable includes b empty, i.e. A is the last symbol)\n\n");

        for (String file : FILES) {
            Grammar g = Grammar.parse(Files.readString(Path.of(file)));
            FollowSets fs = new FollowSets(g);

            sb.append("########## ").append(file).append("\n\n");
            for (Grammar.Production p : g.productions)
                sb.append("  (").append(p.index()).append(") ").append(p).append('\n');
            sb.append('\n');

            sb.append("FIRST (from Module 21):\n");
            for (String nt : g.nonterminals)
                sb.append(String.format("      FIRST(%-2s) = %s%n", nt, FirstSets.setStr(fs.first.firstOf(nt))));
            sb.append("  nullable: ").append(FirstSets.setStr(fs.first.nullable)).append('\n');
            sb.append('\n');

            sb.append("FOLLOW iteration:\n");
            for (int i = 0; i < fs.rounds.size(); i++) {
                boolean last = i == fs.rounds.size() - 1;
                sb.append("  round ").append(i).append(last ? "  (stable -- one more pass adds nothing):\n" : ":\n");
                var round = fs.rounds.get(i);
                for (String nt : g.nonterminals)
                    sb.append(String.format("      FOLLOW(%-2s) = %s%n", nt, FollowSets.setStr(round.get(nt))));
            }
            sb.append("  one more pass changes nothing? ").append(fs.isStable()).append('\n');
            sb.append('\n');

            // cross-check vs Module 20's independent FOLLOW
            Predict pr = new Predict(g);
            boolean agree = true;
            for (String nt : g.nonterminals)
                if (!new java.util.TreeSet<>(fs.followOf(nt)).equals(new java.util.TreeSet<>(pr.followOf(nt))))
                    agree = false;
            sb.append("cross-check vs Module 20 Predict.followOf: ").append(agree ? "MATCH" : "MISMATCH").append('\n');
            sb.append('\n');
        }

        sb.append("why $ is not optional:\n");
        sb.append("  without $, FOLLOW(start) starts empty. A nullable start rule (or any\n");
        sb.append("  nullable rule that can sit at end of input) would then have an empty\n");
        sb.append("  PREDICT contribution from FOLLOW, and the parser could not tell that\n");
        sb.append("  'take the epsilon production and finish' is legal when the input runs out.\n");
        sb.append("  $ is a real terminal the driver appends to the token stream (Module 26).\n");

        System.out.print(sb.toString().replace("\r\n", "\n"));
    }
}
