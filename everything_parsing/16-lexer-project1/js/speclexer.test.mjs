// node js/speclexer.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { SpecLexer, LexicalError } from "./speclexer.mjs";

console.log("speclexer.test.mjs");

const spec = [
  "KW_LET   let",
  "KW_IN    in",
  "ID       (e|i|l|n|t|x|y)(e|i|l|n|t|x|y)*",
  "NUM      (0|1|2|3|4|5|6|7|8|9)(0|1|2|3|4|5|6|7|8|9)*",
  "LE       <=",
  "LT       <",
  "ASSIGN   =",
  "WS       .",
  "%skip WS",
].join("\n");
const lex = SpecLexer.load(spec);

const kinds = s => lex.tokenize(s).map(t => t.kind).join(" ");
const lexemes = s => lex.tokenize(s).map(t => t.lexeme).join(" ");

// ── maximal munch + priority ──
equals(kinds("let.x=12.in.x"), "KW_LET ID ASSIGN NUM KW_IN ID", "keywords win, num/id munch");
equals(kinds("let.let.in"), "KW_LET KW_LET KW_IN", "each 'let' is the keyword");
equals(kinds("x<=99.in.y"), "ID LE NUM KW_IN ID", "<= is one token");
equals(kinds("in<x"), "KW_IN LT ID", "< alone is LT");

// ── identifier containing a keyword ──
equals(kinds("inlet"), "ID", "'inlet' is one identifier");
equals(lexemes("inlet"), "inlet", "lexeme is the whole thing");

// ── WS skipped, positions still count it ──
const toks = lex.tokenize("x.in");
equals(toks.length, 2, "x.in -> 2 tokens");
equals(toks[1].pos, 2, "'in' at position 2 (the '.' occupied position 1)");

// ── lexical error with position ──
try {
  lex.tokenize("x=q");
  that(false, "should have thrown");
} catch (e) {
  that(e instanceof LexicalError && e.pos === 2, "'q' -> error at position 2");
}

// ── "epsilon IS NOOOOOT A TOKEN" ──
that(rejects("A ab\nB (a|b)*"), "pattern matching '' is rejected");
that(rejects("A a?"), "a? -> rejected");
that(rejects("A ()"), "() -> rejected");
that(!rejects("A a\nB ab"), "non-nullable patterns are fine");

// ── empty input ──
equals(lex.tokenize("").length, 0, "empty input -> no tokens");

// ── one combined DFA ──
that(lex.combinedDfaStates > 0 && lex.combinedDfaStates < 200,
  `8 patterns -> one DFA of ${lex.combinedDfaStates} states`);

summary();

function rejects(spec) { try { SpecLexer.load(spec); return false; } catch { return true; } }
