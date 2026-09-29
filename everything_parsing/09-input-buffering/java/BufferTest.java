/** java -cp java/out BufferTest */
public final class BufferTest {
    public static void main(String[] args) {
        System.out.println("BufferTest");

        // ── basic advance / EOF ──
        Buffer b = new Buffer("abc", 4);
        Assert.equals((char) b.advance(), 'a', "advance 1");
        Assert.equals((char) b.advance(), 'b', "advance 2");
        Assert.equals((char) b.advance(), 'c', "advance 3");
        Assert.equals(b.advance(), Buffer.EOF, "advance past end returns EOF");
        Assert.equals(b.advance(), Buffer.EOF, "and stays at EOF");
        Assert.that(b.atEof(), "atEof after exhaustion");

        // ── peek does not move ──
        Buffer p = new Buffer("xy", 4);
        Assert.equals((char) p.peek(), 'x', "peek");
        Assert.equals((char) p.peek(), 'x', "peek again, same");
        Assert.equals((char) p.advance(), 'x', "advance after peek");

        // ── mark + lexeme ──
        Buffer m = new Buffer("hello world", 16);
        m.mark();
        for (int i = 0; i < 5; i++) m.advance();
        Assert.equals(m.lexeme(), "hello", "lexeme from mark to forward");

        // ── retract is O(1) and reversible ──
        Buffer rt = new Buffer("12x", 8);
        rt.mark();
        rt.advance(); rt.advance(); rt.advance();      // consumed "12x"
        Assert.equals(rt.consumed(), 3, "consumed 3");
        rt.retract(1);
        Assert.equals(rt.consumed(), 2, "retract(1) -> consumed 2");
        Assert.equals(rt.lexeme(), "12", "lexeme after retract is 12");
        Assert.equals((char) rt.advance(), 'x', "advance re-reads the retracted char");

        // ── the sentinel: crossing a half boundary transparently ──
        Buffer big = new Buffer("abcdefghijABCDEFGHIJ", 4);   // 20 chars, halfSize 4
        StringBuilder got = new StringBuilder();
        int c;
        while ((c = big.advance()) != Buffer.EOF) got.append((char) c);
        Assert.equals(got.toString(), "abcdefghijABCDEFGHIJ", "reads the whole input across many reloads");
        Assert.equals(big.consumed(), 20, "consumed all 20");
        // 20 chars / 4 per half = 5 loads, +1 for the trailing exact-multiple EOF fill
        Assert.equals(bufferLoads(big), 6, "20 chars, halfSize 4 => 6 loads (exact multiple: +1 for EOF fill)");

        // ── amortized: loads grow like N / halfSize, not like N ──
        Buffer a1 = readAll(new Buffer("x".repeat(100), 10));
        Buffer a2 = readAll(new Buffer("x".repeat(1000), 10));
        Assert.that(bufferLoads(a2) < bufferLoads(a1) * 12,
            "10x the input is ~10x the loads, not 10x*something-bigger  (" + bufferLoads(a1) + " vs " + bufferLoads(a2) + ")");
        Assert.equals(bufferLoads(a1), 11, "100 chars / 10 => 10 loads + 1 EOF fill");
        Assert.equals(bufferLoads(a2), 101, "1000 chars / 10 => 100 loads + 1 EOF fill");

        // ── lexeme stitched across the halfway split ──
        Buffer sp = new Buffer("aa bbbb cc", 4);   // "bbbb" starts in half 0, ends in half 1
        int[] r = {0};
        var toks = new java.util.ArrayList<String>();
        Scanner.Tok tk;
        while ((tk = Scanner.next(sp, r)) != null) toks.add(tk.text());
        Assert.equals(toks.toString(), "[aa, bbbb, cc]", "the lexeme was stitched across two halves");

        // ── a lexeme longer than a half cannot be retracted across the boundary ──
        Buffer overflow = new Buffer("abcdefghij", 4);
        overflow.mark();
        for (int i = 0; i < 6; i++) overflow.advance();     // forward now well into a later half
        Assert.that(throwsOn(() -> overflow.retract(5)),
            "retract that would cross the halfway boundary throws (Dragon Book's lexeme-size limit)");

        Assert.summary();
    }

    static int bufferLoads(Buffer b) { return b.bufferLoads; }
    static Buffer readAll(Buffer b) { while (b.advance() != Buffer.EOF) {} return b; }
    static boolean throwsOn(Runnable r) {
        try { r.run(); return false; } catch (RuntimeException e) { return true; }
    }
}
