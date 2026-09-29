import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.stream.Collectors;

/** java -cp java/out ScannerTest */
public final class ScannerTest {
    static Scanner sc;

    public static void main(String[] args) throws IOException {
        System.out.println("ScannerTest");
        sc = Scanner.fromSpec(Files.readString(Path.of("fixtures", "ajoda.tokens")));

        // ── the pattern layer desugars to plain regex nodes ──
        Assert.that(Pattern.parse("[a-c]+").matches("cab"), "[a-c]+ matches cab");
        Assert.that(!Pattern.parse("[^a]").matches("a"), "[^a] refuses a");
        Assert.that(Pattern.parse("\"->\"").matches("->"), "quoted literal needs no escaping");
        Assert.equals(kinds(Scanner.fromSpec("token P \\(\\s"), "( "), "P EOF", "escapes: a paren then a space");
        Assert.that(throwsPattern("[a-"), "unclosed class is a pattern error");
        Assert.that(throwsPattern("a b"), "bare space is a pattern error");

        // ── longest match ──
        Assert.equals(kinds(sc, "<= >= == != -> && ||"), "LE GE EQ NE ARROW AND OR EOF", "two-char operators win");
        Assert.equals(kinds(sc, "3.14"), "FLOAT_LIT EOF", "3.14 is one float, not 3 . 14");

        // ── keywords by lookup, never by prefix ──
        Assert.equals(kinds(sc, "if iffy"), "IF IDENT EOF", "if is a keyword, iffy is not");
        Assert.equals(kinds(sc, "while1 truest"), "IDENT IDENT EOF", "keyword prefixes stay identifiers");

        // ── positions survive discarded text ──
        var t = sc.scan("// note\n  let").get(0);
        Assert.equals(t.kind() + " " + t.line() + ":" + t.col(), "LET 2:3", "comment + indent skipped, position kept");

        // ── errors carry line:col ──
        try { sc.scan("x\n  @"); Assert.that(false, "should throw"); }
        catch (Scanner.ScanError e) { Assert.that(e.line == 2 && e.col == 3, "'@' reported at 2:3"); }

        // ── the spec is checked at load ──
        Assert.that(refused("token A [0-9]*"), "a star-only pattern is refused");
        Assert.that(refused("token A ()"), "() is refused");
        Assert.that(refused("token A a\ntoken A b"), "duplicate names are refused");
        Assert.that(refused("tokn A a"), "unknown directive is refused");
        Assert.that(!refused("token A a\ndiscard B b"), "a clean spec loads");

        // ── empty input is just EOF ──
        Assert.equals(kinds(sc, ""), "EOF", "empty input -> EOF");

        Assert.summary();
    }

    static String kinds(Scanner s, String src) {
        return s.scan(src).stream().map(Scanner.Token::kind).collect(Collectors.joining(" "));
    }
    static boolean refused(String spec) {
        try { Scanner.fromSpec(spec); return false; } catch (Scanner.SpecError e) { return true; }
    }
    static boolean throwsPattern(String src) {
        try { Pattern.parse(src); return false; } catch (Pattern.PatternError e) { return true; }
    }
}
