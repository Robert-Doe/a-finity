import java.util.List;

/**
 *   java -cp java/out Main
 *
 * A four-rule token set over the alphabet {i, f, x, y, =}, demonstrating:
 *   - longest match wins  ("iffy" -> ID, not KW_IF + "fy")
 *   - ties -> earlier rule ("if"  -> KW_IF, not ID)
 *   - the DFA + last-accept-mark mechanism, traced
 */
public final class Main {
    static final StringBuilder sb = new StringBuilder();

    public static void main(String[] args) {
        List<Lexer.Rule> rules = List.of(
            Lexer.Rule.of("KW_IF",  "if"),
            Lexer.Rule.of("ID",     "(i|f|x|y)(i|f|x|y)*"),
            Lexer.Rule.of("ASSIGN", "=="),
            Lexer.Rule.of("EQ",     "=")
        );

        p("=== Module 15 - Maximal Munch & Token Priority ===");
        p("");
        p("token rules  (priority = declaration order):");
        for (int i = 0; i < rules.size(); i++)
            p(String.format("  (%d) %-8s = %s", i, rules.get(i).name(), rules.get(i).pattern()));
        p("");

        // ── RULE 1: longest match ──
        p("RULE 1 -- longest match wins:");
        for (String s : new String[]{"iffy", "ifx", "=="}) {
            var t = Lexer.nextToken(rules, s, 0);
            p("  \"" + s + "\"" + spaces(9 - s.length() - 2) + "-> " + t
                + "   (KW_IF/EQ would have matched a shorter prefix)");
        }
        p("");

        // ── RULE 2: tie -> earlier rule ──
        p("RULE 2 -- on a length tie, the EARLIER rule wins:");
        int ifIf = Lexer.longestAccept(rules.get(0).dfa(), "if", 0);
        int ifId = Lexer.longestAccept(rules.get(1).dfa(), "if", 0);
        p("  \"if\"   -> KW_IF matches len " + ifIf + ",  ID matches len " + ifId
            + "  -- tie  ->  " + Lexer.nextToken(rules, "if", 0) + "   (rule 0 < rule 1)");
        p("");

        // ── the mechanism, traced ──
        p("the mechanism -- run the DFA, keep the LAST-ACCEPT position, back up to it:");
        traceOne(rules.get(0), "iffy");    // KW_IF
        traceOne(rules.get(1), "iffy");    // ID
        p("  -> ID's last-accept (4) beats KW_IF's (2)  ->  the token is ID \"iffy\"");
        p("");

        // ── full tokenizations ──
        p("full tokenization:");
        for (String s : new String[]{"iffy==x=y", "if==x", "x=y"}) {
            p("  input \"" + s + "\":");
            for (var t : Lexer.tokenize(rules, s)) p("    " + t);
        }
        p("");

        // ── a lexical error ──
        try {
            Lexer.tokenize(rules, "if z");
        } catch (IllegalArgumentException e) {
            p("lexical error demo:  tokenize(\"if z\")  ->  " + e.getMessage());
        }
        p("");

        p("summary: 4 rules | longest match wins | ties -> first declared | mechanism = DFA + last-accept mark");

        System.out.print(sb);
    }

    static void traceOne(Lexer.Rule r, String s) {
        int[] hits = Lexer.matchLengths(r.dfa(), s, 0);
        StringBuilder marks = new StringBuilder();
        for (int L = 1; L <= s.length(); L++) {
            boolean hit = false;
            for (int h : hits) if (h == L) hit = true;
            marks.append(s, 0, L).append(hit ? "(ACCEPT@" + L + ") " : "(-) ");
        }
        p("  " + String.format("%-8s", r.name()) + " on \"" + s + "\":  " + marks.toString().strip()
            + "   -> last accept = " + (hits.length == 0 ? "none" : hits[hits.length - 1]));
    }

    static String spaces(int n) {
        return n <= 0 ? "" : " ".repeat(n);
    }
    static void p(String s) { sb.append(s).append('\n'); }
}
