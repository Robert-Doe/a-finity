import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.stream.Collectors;

/** java -cp java/out RecoveryTest */
public final class RecoveryTest {
    public static void main(String[] args) throws IOException {
        System.out.println("RecoveryTest");
        Scanner sc = Scanner.fromSpec(Files.readString(Path.of("fixtures", "ajoda.tokens")));
        var ONE = Recovery.Strategy.SKIP_ONE;
        var RUN = Recovery.Strategy.SKIP_TO_RESTART;

        // ── SKIP_ONE: one ERROR per bad character ──
        var a = Recovery.scanRecovering(sc, "x$y?z", ONE);
        Assert.equals(kinds(a), "IDENT ERROR IDENT ERROR IDENT EOF", "x$y?z");
        Assert.equals(a.errors().stream().map(e -> String.valueOf(e.col())).collect(Collectors.joining(",")),
            "2,4", "errors at columns 2 and 4");

        var b = Recovery.scanRecovering(sc, "a @@@# b", ONE);
        Assert.equals(b.errors().size(), 4, "@@@# -> 4 separate errors");

        // ── SKIP_TO_RESTART: one ERROR per run ──
        var c = Recovery.scanRecovering(sc, "a @@@# b", RUN);
        Assert.equals(kinds(c), "IDENT ERROR IDENT EOF", "a @@@# b -> IDENT ERROR IDENT");
        Assert.equals(c.errors().size(), 1, "one error for the run");
        Assert.equals(c.tokens().get(1).lexeme(), "@@@#", "the ERROR token holds the whole run");
        Assert.equals(c.errors().get(0).col(), 3, "the run starts at column 3");

        // ── all garbage ──
        Assert.equals(kinds(Recovery.scanRecovering(sc, "@@@", RUN)), "ERROR EOF", "@@@ -> one ERROR");
        Assert.equals(Recovery.scanRecovering(sc, "@@@", ONE).errors().size(), 3, "@@@ -> 3 errors under SKIP_ONE");

        // ── scanning continues past an error ──
        var f = Recovery.scanRecovering(sc, "let n: i64 = 5 @ 2;", ONE);
        Assert.equals(kinds(f), "LET IDENT COLON I64 ASSIGN INT_LIT ERROR INT_LIT SEMI EOF", "continues past '@'");

        // ── line numbers across a file ──
        var g = Recovery.scanRecovering(sc, "a\n  $\nb ?", ONE);
        Assert.equals(g.errors().stream().map(e -> e.line() + ":" + e.col()).collect(Collectors.joining(" ")),
            "2:3 3:3", "errors keep line:col");
        Assert.that(throwsScan(() -> sc.scan("$x$")), "Module 16's scan still stops at the first");

        // ── discarded text is never an error ──
        Assert.equals(Recovery.scanRecovering(sc, "// @#$?\nx", ONE).errors().size(), 0, "comment contents are fine");

        // ── clean input ──
        var h = Recovery.scanRecovering(sc, "let x: i64 = 9;", RUN);
        Assert.equals(h.errors().size(), 0, "clean input -> no errors");
        Assert.that(h.tokens().stream().noneMatch(t -> t.kind().equals("ERROR")), "no ERROR tokens");

        Assert.summary();
    }

    static String kinds(Recovery.Result r) {
        return r.tokens().stream().map(Scanner.Token::kind).collect(Collectors.joining(" "));
    }
    static boolean throwsScan(Runnable run) {
        try { run.run(); return false; } catch (Scanner.ScanError e) { return true; }
    }
}
