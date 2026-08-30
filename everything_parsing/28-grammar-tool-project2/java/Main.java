import java.nio.file.Files;
import java.nio.file.Path;

public final class Main {

    static final String[] FILES = {
        "fixtures/stmts.grammar",
        "fixtures/expr.grammar",
        "fixtures/json.grammar",
    };

    public static void main(String[] args) throws Exception {
        StringBuilder sb = new StringBuilder();
        sb.append("=== Module 28 - Grammar Analysis Tool (CSE 340 Project 2) ===\n\n");
        sb.append("one program, seven sections, each a component from Modules 20-27:\n");
        sb.append("symbols | nullable | FIRST | FOLLOW | left-recursion-free | left-factored | LL(1)\n\n");

        for (String file : FILES) {
            Grammar g = Grammar.parse(Files.readString(Path.of(file)));
            GrammarTool tool = new GrammarTool(g);

            sb.append("##################### ").append(file).append(" #####################\n\n");
            sb.append("input grammar:\n");
            for (Grammar.Production p : g.productions)
                sb.append("  ").append(p).append('\n');
            sb.append('\n');
            sb.append(tool.report());
            sb.append('\n');
        }

        System.out.print(sb.toString().replace("\r\n", "\n"));
    }
}
