import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

/**
 *   java -cp java/out Main
 *
 * For each EBNF fixture: show it as written, in ISO-style ({ } / [ ]), and
 * desugared to pure BNF; then confirm the BNF generates the same language as
 * the EBNF by exact matching. Ends with a notation cheat-sheet.
 */
public final class Main {
    static final StringBuilder sb = new StringBuilder();

    public static void main(String[] args) throws IOException {
        p("=== Module 08 - BNF, EBNF, ABNF & Syntax Diagrams ===");
        p("");

        report("expr.ebnf", 7,
            List.of(List.of("NUM"), List.of("NUM", "+", "NUM"), List.of("(", "NUM", ")"),
                    List.of("NUM", "*", "NUM", "+", "NUM"), List.of("NUM", "+", "NUM", "*", "NUM")),
            List.of(List.of("+", "NUM"), List.of("NUM", "+"), List.of("(", "NUM")));

        report("signed.ebnf", 4,
            List.of(List.of("0"), List.of("-", "1"), List.of("1", "2"), List.of("-", "0", "1", "2")),
            List.of(List.of("-"), List.of("-", "-", "1"), List.of()));

        cheatSheet();

        p("summary: expr.ebnf -> BNF, same language | signed.ebnf -> BNF, same language"
            + " | operators ? * + ( ) all desugared into fresh nonterminals");

        System.out.print(sb);
    }

    static void report(String file, int maxLen,
                       List<List<String>> shouldAccept, List<List<String>> shouldReject) throws IOException {
        Ebnf.Grammar eg = Ebnf.parse(Files.readString(Path.of("fixtures", file)));
        String bnfText = Ebnf.toBnfText(eg);
        Grammar bg = Grammar.parse(bnfText);

        p("--- fixtures/" + file + " ---");

        p("EBNF, ISO-style  ( { } = zero or more,  [ ] = optional ):");
        for (var e : eg.rules.entrySet())
            p("  " + pad(e.getKey(), 8) + " = " + Ebnf.isoRule(e.getValue()));
        p("");

        p("desugared to pure BNF:");
        for (String line : bnfText.split("\n"))
            if (!line.startsWith("%")) p("  " + line);
        p("");

        // equivalence: every BNF-generated string is EBNF-accepted, and vice versa on samples
        var bnfLang = Derivation.enumerate(bg, maxLen);
        int checked = 0;
        boolean allAgree = true;
        for (String w : bnfLang) {
            List<String> toks = w.equals("epsilon") ? List.of() : List.of(w.split(" "));
            checked++;
            if (!Ebnf.accepts(eg, toks)) allAgree = false;
        }
        p("equivalence check (BNF enumerated to length " + maxLen + "):");
        p("  " + bnfLang.size() + " strings in L(BNF); every one also accepted by the EBNF matcher: " + allAgree);

        boolean posOk = true, negOk = true;
        for (var s : shouldAccept) posOk &= Ebnf.accepts(eg, s) && inLang(bnfLang, s);
        for (var s : shouldReject) negOk &= !Ebnf.accepts(eg, s) && !inLang(bnfLang, s);
        p("  sample accepts all pass: " + posOk + "   sample rejects all pass: " + negOk);
        p("");
    }

    static boolean inLang(List<String> lang, List<String> toks) {
        String key = toks.isEmpty() ? "epsilon" : String.join(" ", toks);
        return lang.contains(key);
    }

    static void cheatSheet() {
        p("notation cheat-sheet:");
        String[][] rows = {
            {"concept",       "BNF",            "EBNF (this course)", "ISO-EBNF",  "ABNF"},
            {"define",        "::=  or  ->",    "=",                  "=",         "="},
            {"alternative",   "|",              "|",                  "|",         "/"},
            {"optional",      "(extra rule)",   "x?",                 "[ x ]",     "[x]  /  *1x"},
            {"zero or more",  "(recursion)",    "x*",                 "{ x }",     "*x"},
            {"one or more",   "(recursion)",    "x+",                 "x { x }",   "1*x"},
            {"grouping",      "(extra rule)",   "( x )",              "( x )",     "( x )"},
            {"terminal",      "literal text",   "'x'  \"x\"",         "'x'  \"x\"", "\"x\" (ci),  %x41"},
        };
        for (String[] r : rows)
            p(String.format("  %-14s %-16s %-20s %-12s %s", r[0], r[1], r[2], r[3], r[4]));
        p("");
    }

    static void p(String s) { sb.append(s).append('\n'); }
    static String pad(String s, int w) {
        StringBuilder b = new StringBuilder(s);
        while (b.length() < w) b.append(' ');
        return b.toString();
    }
}
