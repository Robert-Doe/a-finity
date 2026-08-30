import java.util.List;

/**
 *   java -cp java/out Main
 *
 * Scans one source string through a two-buffer reader with an 8-character half,
 * printing each token and the running buffer-load count, then the amortized cost
 * and a note on what the sentinel is doing.
 */
public final class Main {
    static final StringBuilder sb = new StringBuilder();

    public static void main(String[] args) {
        String src = "the 12.5 quick brownish fox 12.go +9 ok";
        int half = 8;

        p("=== Module 09 - Input Buffering: Two-Buffer Scheme & Sentinels ===");
        p("");
        p("source (" + src.length() + " chars): \"" + src + "\"");
        p("buffer half-size: " + half);
        p("");

        Buffer b2 = new Buffer(src, half);
        int[] r2 = {0};
        List<Scanner.Tok> toks = new java.util.ArrayList<>();

        p("scan trace  (token | running buffer loads):");
        Scanner.Tok t;
        while ((t = Scanner.next(b2, r2)) != null) {
            toks.add(t);
            p(String.format("  %-6s %-10s loads=%d  half=%d",
                t.kind(), "\"" + t.text() + "\"", b2Loads(b2), b2.currentHalf()));
        }
        p("");

        p("what happened at \"12.go\":  read '1' '2' '.', peeked 'g' (not a digit),");
        p("  retract(1) put '.' back  ->  NUM \"12\", then '.' scanned as ERR, then WORD \"go\"");
        p("");

        p("what happened at \"brownish\":  it begins in half 1 and ends in half 0 after a reload;");
        p("  Buffer.lexeme() stitches across the halfway split -> \"" + findLexeme(toks, "brownish") + "\"");
        p("");

        p("totals:");
        p("  characters consumed : " + b2.consumed());
        p("  buffer loads        : " + b2Loads(b2));
        p("  loads per character : " + fmt((double) b2Loads(b2) / b2.consumed()) + "   (amortized O(1))");
        p("  forward reads       : " + b2Forward(b2)
            + "   (one 'data[forward]' compare per advance -- the sentinel means we never ALSO test 'forward == end')");
        p("  retracts            : " + r2[0]);
        p("");

        p("summary: " + src.length() + " source chars | " + b2Loads(b2) + " buffer loads (~1 per "
            + half + "-char half) | ~1 compare per char | lexeme stitched across the split | retract is O(1)");

        System.out.print(sb);
    }

    // reflection-free accessors (Buffer's counters are package-private)
    static int b2Loads(Buffer b)   { return b.bufferLoads; }
    static int b2Forward(Buffer b) { return b.forwardReads; }

    static String findLexeme(List<Scanner.Tok> toks, String want) {
        for (var t : toks) if (t.text().equals(want)) return t.text();
        return "(not found)";
    }

    static String fmt(double d) {
        return String.format("%.3f", d);
    }
    static void p(String s) { sb.append(s).append('\n'); }
}
