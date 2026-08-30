// node js/buffer.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Buffer, EOF } from "./buffer.mjs";
import { next } from "./scanner.mjs";

console.log("buffer.test.mjs");
const ch = (n) => String.fromCharCode(n);

// ── basic advance / EOF ──
let b = new Buffer("abc", 4);
equals(ch(b.advance()), "a", "advance 1");
equals(ch(b.advance()), "b", "advance 2");
equals(ch(b.advance()), "c", "advance 3");
equals(b.advance(), EOF, "advance past end returns EOF");
equals(b.advance(), EOF, "and stays at EOF");
that(b.atEof(), "atEof after exhaustion");

// ── peek does not move ──
const p = new Buffer("xy", 4);
equals(ch(p.peek()), "x", "peek");
equals(ch(p.peek()), "x", "peek again, same");
equals(ch(p.advance()), "x", "advance after peek");

// ── mark + lexeme ──
const m = new Buffer("hello world", 16);
m.mark();
for (let i = 0; i < 5; i++) m.advance();
equals(m.lexeme(), "hello", "lexeme from mark to forward");

// ── retract ──
const rt = new Buffer("12x", 8);
rt.mark();
rt.advance(); rt.advance(); rt.advance();
equals(rt.consumed(), 3, "consumed 3");
rt.retract(1);
equals(rt.consumed(), 2, "retract(1) -> consumed 2");
equals(rt.lexeme(), "12", "lexeme after retract is 12");
equals(ch(rt.advance()), "x", "advance re-reads the retracted char");

// ── the sentinel: crossing half boundaries transparently ──
const big = new Buffer("abcdefghijABCDEFGHIJ", 4);
let got = "", c;
while ((c = big.advance()) !== EOF) got += ch(c);
equals(got, "abcdefghijABCDEFGHIJ", "reads the whole input across many reloads");
equals(big.consumed(), 20, "consumed all 20");
equals(big.bufferLoads, 6, "20 chars, halfSize 4 => 6 loads (exact multiple: +1 for EOF fill)");

// ── amortized: loads ~ N / halfSize ──
const a1 = readAll(new Buffer("x".repeat(100), 10));
const a2 = readAll(new Buffer("x".repeat(1000), 10));
that(a2.bufferLoads < a1.bufferLoads * 12, `10x input ~ 10x loads (${a1.bufferLoads} vs ${a2.bufferLoads})`);
equals(a1.bufferLoads, 11, "100 chars / 10 => 10 loads + 1 EOF fill");
equals(a2.bufferLoads, 101, "1000 chars / 10 => 100 loads + 1 EOF fill");

// ── lexeme stitched across the halfway split ──
const sp = new Buffer("aa bbbb cc", 4);
const r = [0];
const toks = [];
let tk;
while ((tk = next(sp, r)) !== null) toks.push(tk.text);
equals(JSON.stringify(toks), '["aa","bbbb","cc"]', "the lexeme was stitched across two halves");

// ── retract past the boundary throws ──
const overflow = new Buffer("abcdefghij", 4);
overflow.mark();
for (let i = 0; i < 6; i++) overflow.advance();
that(throwsOn(() => overflow.retract(5)), "retract across the halfway boundary throws");

summary();

function readAll(b) { while (b.advance() !== EOF) {} return b; }
function throwsOn(fn) { try { fn(); return false; } catch { return true; } }
