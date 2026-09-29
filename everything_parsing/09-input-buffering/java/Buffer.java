/**
 * Module 09 — the two-buffer input scheme from the Dragon Book (§3.2).
 *
 * The problem: a scanner reads a character, decides it needs to look further,
 * reads more, then sometimes RETRACTS (a lexeme actually ended one character
 * back). The real constraint is that input arrives in blocks and you can't hold
 * the whole file.
 *
 * The scheme:
 *   - two halves of `halfSize` characters each, filled from the source on demand
 *   - a SENTINEL slot after each half. The sentinel is a value that cannot occur
 *     in the input, so the inner loop tests ONLY `c == SENTINEL` — one compare
 *     that means either "reload the other half" or (as EOF) "stop", instead of
 *     testing `forward == end` on every character.
 *   - `lexemeBegin` marks the start of the current lexeme; `forward` scans ahead.
 *   - retract is `forward--`: O(1).
 *
 * Cost: each source character is copied into a half exactly once, so N chars
 * cost ~N/halfSize buffer loads — O(1) amortized per character.
 *
 * Known limit (also the Dragon Book's): a lexeme must fit within one half, so
 * you never need to retract across the halfway boundary into an overwritten half.
 */
public final class Buffer {

    public static final int EOF       = -1;   // true end of input
    private static final int SENTINEL = -2;   // "this slot is a half boundary; reload"

    private final String source;
    private final int halfSize;
    private final int[] data;      // [ half0 (halfSize) | S | half1 (halfSize) | S ]
    private int sourcePos = 0;     // next source char not yet copied into a half
    private int forward;           // index into data
    private int lexemeBegin;
    private int consumed = 0;      // real characters advanced past

    // instrumentation for the tutorial
    int bufferLoads = 0;
    int forwardReads = 0;         // one per `data[forward]` inspection in advance()

    public Buffer(String source, int halfSize) {
        this.source = source;
        this.halfSize = halfSize;
        this.data = new int[2 * (halfSize + 1)];
        fillHalf(0);
        forward = 0;
        lexemeBegin = 0;
    }

    private int halfStart(int half)     { return half * (halfSize + 1); }
    private int sentinelIndex(int half) { return halfStart(half) + halfSize; }
    private int whichHalf(int idx)      { return idx < (halfSize + 1) ? 0 : 1; }

    private void fillHalf(int half) {
        bufferLoads++;
        int base = halfStart(half);
        int n = Math.min(halfSize, source.length() - sourcePos);
        for (int i = 0; i < n; i++) data[base + i] = source.charAt(sourcePos + i);
        sourcePos += n;
        if (n < halfSize) data[base + n] = EOF;             // source ran out inside this half
        else               data[sentinelIndex(half)] = SENTINEL; // half full: boundary
    }

    private void crossBoundary() {
        int other = 1 - whichHalf(forward);
        fillHalf(other);
        forward = halfStart(other);
    }

    /** The character at `forward` without moving. Char or EOF, never SENTINEL. */
    public int peek() {
        while (true) {
            int c = data[forward];
            if (c == SENTINEL) { crossBoundary(); continue; }
            return c;
        }
    }

    /** Return the character at `forward`, then move forward. At EOF, returns EOF and stays. */
    public int advance() {
        while (true) {
            int c = data[forward];
            forwardReads++;
            if (c == SENTINEL) { crossBoundary(); continue; }
            if (c == EOF) return EOF;
            forward++;
            consumed++;
            return c;
        }
    }

    /** Begin a new lexeme at the current position. */
    public void mark() { lexemeBegin = forward; }

    /** Push `forward` back by n characters. n must keep us within the current half. */
    public void retract(int n) {
        for (int i = 0; i < n; i++) {
            forward--;
            consumed--;
            if (forward < halfStart(whichHalf(forward)) || data[forward] == SENTINEL)
                throw new IllegalStateException("retract crossed the halfway boundary (lexeme too long)");
        }
    }

    /** The text from lexemeBegin to forward, stitched across the halfway split. */
    public String lexeme() {
        StringBuilder sb = new StringBuilder();
        int i = lexemeBegin;
        while (i != forward) {
            int c = data[i];
            if (c == SENTINEL) { i = halfStart(1 - whichHalf(i)); continue; }
            if (c == EOF) break;
            sb.append((char) c);
            i++;
        }
        return sb.toString();
    }

    public boolean atEof()   { return peek() == EOF; }
    public int consumed()    { return consumed; }
    public int currentHalf() { return whichHalf(forward); }
}
