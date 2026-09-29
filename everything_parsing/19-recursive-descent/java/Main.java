import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

public final class Main {

    public static void main(String[] args) throws Exception {
        StringBuilder sb = new StringBuilder();

        sb.append("=== Module 19 - Recursive-Descent Parsing ===\n\n");
        sb.append("grammar (no left recursion; Module 23 derives this from  E -> E + T):\n");
        sb.append("  E  -> T E'\n");
        sb.append("  E' -> + T E'  |  epsilon\n");
        sb.append("  T  -> F T'\n");
        sb.append("  T' -> * F T'  |  epsilon\n");
        sb.append("  F  -> ( E )   |  num  |  id\n");
        sb.append("  one method per nonterminal; the productions they fire ARE a leftmost derivation\n\n");

        for (String line : Files.readAllLines(Path.of("fixtures/exprs.txt"))) {
            if (line.isBlank() || line.startsWith("#")) continue;
            sb.append("--- ").append(line).append(" ---\n");

            List<Lexer.Token> toks = Lexer.lex(line);
            sb.append("  tokens: ");
            for (Lexer.Token t : toks) sb.append(' ').append(t);
            sb.append('\n');

            try {
                RecursiveDescent.Result r = RecursiveDescent.parse(line);

                sb.append("  productions fired, in order (top of the list = first call):\n");
                for (String rule : r.rules()) sb.append("      ").append(rule).append('\n');

                List<Derivation.Step> steps = Derivation.replay(r.rules());
                sb.append("  same list, replayed as a leftmost derivation:\n");
                sb.append("      E\n");
                for (int i = 1; i < steps.size(); i++) {
                    Derivation.Step s = steps.get(i);
                    sb.append(String.format("      =>  %-24s (%s)%n", s.form(), s.rule()));
                }

                String sentence = steps.get(steps.size() - 1).form();
                String kinds = joinKinds(toks);
                sb.append("  derived sentence: ").append(sentence).append('\n');
                sb.append("  token kinds     : ").append(kinds).append('\n');
                sb.append("  match? ").append(sentence.equals(kinds)).append('\n');

                sb.append("  concrete syntax tree:\n");
                for (String tl : RecursiveDescent.render(r.tree()).split("\n"))
                    sb.append("    ").append(tl).append('\n');
                sb.append("  yield: ").append(String.join(" ", RecursiveDescent.terminalYield(r.tree()))).append('\n');

            } catch (RecursiveDescent.SyntaxError e) {
                sb.append("  SYNTAX ERROR at position ").append(e.pos).append(": ").append(e.getMessage()).append('\n');
            }
            sb.append('\n');
        }

        sb.append("why not parse the Module 6 grammar  E -> E + T  directly?\n");
        sb.append("  parseE() would call parseE() as its first action, cursor unmoved:\n");
        sb.append("  infinite recursion, stack overflow, zero tokens consumed. Recursive\n");
        sb.append("  descent REQUIRES a non-left-recursive grammar (Module 23). The right-\n");
        sb.append("  recursive E' here makes  a + b + c  lean RIGHT in the tree; left\n");
        sb.append("  associativity is put back when lowering to an AST (Module 39), or by\n");
        sb.append("  writing E' as a loop:\n");
        sb.append("      Node e = parseT();\n");
        sb.append("      while (at(\"+\")) { expect(\"+\"); e = plus(e, parseT()); }   // left-leaning\n");

        System.out.print(sb.toString().replace("\r\n", "\n"));
    }

    private static String joinKinds(List<Lexer.Token> toks) {
        StringBuilder k = new StringBuilder();
        for (Lexer.Token t : toks) {
            if (t.kind().equals("EOF")) continue;
            if (k.length() > 0) k.append(' ');
            k.append(t.kind());
        }
        return k.toString();
    }
}
