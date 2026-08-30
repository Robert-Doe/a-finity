// Module 09 — the two-buffer input scheme from the Dragon Book (§3.2).
//
//   - two halves of `halfSize` characters, filled from the source on demand
//   - a SENTINEL slot after each half: the inner loop tests ONLY `c === SENTINEL`,
//     one compare that means "reload the other half" or (as EOF) "stop"
//   - lexemeBegin marks the lexeme start; forward scans ahead
//   - retract is forward--  : O(1)
//
// Cost: each source char is copied into a half exactly once → ~N/halfSize loads.
// Limit: a lexeme must fit in one half (so retract never crosses the boundary).

export const EOF = -1;
const SENTINEL = -2;

export class Buffer {
  constructor(source, halfSize) {
    this.source = source;
    this.halfSize = halfSize;
    this.data = new Int32Array(2 * (halfSize + 1));
    this.sourcePos = 0;
    this.consumedCount = 0;
    this.bufferLoads = 0;
    this.forwardReads = 0;
    this.fillHalf(0);
    this.forward = 0;
    this.lexemeBegin = 0;
  }

  halfStart(half)     { return half * (this.halfSize + 1); }
  sentinelIndex(half) { return this.halfStart(half) + this.halfSize; }
  whichHalf(idx)      { return idx < this.halfSize + 1 ? 0 : 1; }

  fillHalf(half) {
    this.bufferLoads++;
    const base = this.halfStart(half);
    const n = Math.min(this.halfSize, this.source.length - this.sourcePos);
    for (let i = 0; i < n; i++) this.data[base + i] = this.source.charCodeAt(this.sourcePos + i);
    this.sourcePos += n;
    if (n < this.halfSize) this.data[base + n] = EOF;
    else this.data[this.sentinelIndex(half)] = SENTINEL;
  }

  crossBoundary() {
    const other = 1 - this.whichHalf(this.forward);
    this.fillHalf(other);
    this.forward = this.halfStart(other);
  }

  peek() {
    for (;;) {
      const c = this.data[this.forward];
      if (c === SENTINEL) { this.crossBoundary(); continue; }
      return c;
    }
  }

  advance() {
    for (;;) {
      const c = this.data[this.forward];
      this.forwardReads++;
      if (c === SENTINEL) { this.crossBoundary(); continue; }
      if (c === EOF) return EOF;
      this.forward++;
      this.consumedCount++;
      return c;
    }
  }

  mark() { this.lexemeBegin = this.forward; }

  retract(n) {
    for (let i = 0; i < n; i++) {
      this.forward--;
      this.consumedCount--;
      if (this.forward < this.halfStart(this.whichHalf(this.forward)) || this.data[this.forward] === SENTINEL)
        throw new Error("retract crossed the halfway boundary (lexeme too long)");
    }
  }

  lexeme() {
    let s = "";
    let i = this.lexemeBegin;
    while (i !== this.forward) {
      const c = this.data[i];
      if (c === SENTINEL) { i = this.halfStart(1 - this.whichHalf(i)); continue; }
      if (c === EOF) break;
      s += String.fromCharCode(c);
      i++;
    }
    return s;
  }

  atEof()      { return this.peek() === EOF; }
  consumed()   { return this.consumedCount; }
  currentHalf() { return this.whichHalf(this.forward); }
}
