// node js/recovery.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { SpecLexer, LexicalError } from "./speclexer.mjs";
import { Strategy, tokenize as recover } from "./recovery.mjs";

console.log("recovery.test.mjs");

const lex = SpecLexer.load([
  "KW_LET   let",
  "KW_IN    in",
  "ID       (e|i|l|n|t|x|y)(e|i|l|n|t|x|y)*",
  "NUM      (0|1|2|3|4|5|6|7|8|9)(0|1|2|3|4|5|6|7|8|9)*",
  "ASSIGN   =",
  "WS       .",
  "%skip WS",
].join("\n"));

const kinds = r => r.tokens.map(t => t.kind).join(" ");

// ── PANIC_ONE ──
const a = recover(lex, "x@y#x", Strategy.PANIC_ONE);
equals(kinds(a), "ID ERROR ID ERROR ID", "x@y#x");
equals(a.errors.length, 2, "two errors");
equals(a.errors[0].from, 1, "first at position 1");
equals(a.errors[1].from, 3, "second at position 3");

const b = recover(lex, "x@@@#y", Strategy.PANIC_ONE);
equals(b.errors.length, 4, "@@@# -> 4 separate errors");

// ── PANIC_TO_SYNC ──
const c = recover(lex, "x@@@#y", Strategy.PANIC_TO_SYNC);
equals(kinds(c), "ID ERROR ID", "x@@@#y -> ID ERROR ID");
equals(c.errors.length, 1, "one error for the run");
equals(c.errors[0].from, 1, "from position 1");
equals(c.errors[0].to, 5, "to position 5");
equals(c.tokens[1].lexeme, "@@@#", "ERROR token text");

// ── all garbage ──
equals(kinds(recover(lex, "@@@", Strategy.PANIC_TO_SYNC)), "ERROR", "@@@ -> one ERROR");
equals(recover(lex, "@@@", Strategy.PANIC_ONE).errors.length, 3, "@@@ -> 3 errors under PANIC_ONE");

// ── error mid-stream ──
const f = recover(lex, "let.x=@=9", Strategy.PANIC_ONE);
equals(kinds(f), "KW_LET ID ASSIGN ERROR ASSIGN NUM", "continues past the '@'");
equals(f.errors.length, 1, "one error");

// ── one scan finds all errors ──
const g = recover(lex, "@x@y@", Strategy.PANIC_ONE);
equals(g.errors.length, 3, "one recovering scan reports all 3");
that(throwsLexical(() => lex.tokenize("@x@y@")), "Module 16's tokenize still stops at the first");

// ── clean input ──
const h = recover(lex, "let.x=9", Strategy.PANIC_TO_SYNC);
equals(h.errors.length, 0, "clean input -> no errors");
that(!h.tokens.some(t => t.kind === "ERROR"), "no ERROR tokens");

summary();

function throwsLexical(fn) { try { fn(); return false; } catch (e) { return e instanceof LexicalError; } }
