// node js/lexer.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { makeRule, longestAccept, nextToken, tokenize, tokenStr } from "./lexer.mjs";

console.log("lexer.test.mjs");

const rules = [
  makeRule("KW_IF", "if"),
  makeRule("ID", "(i|f|x|y)(i|f|x|y)*"),
  makeRule("ASSIGN", "=="),
  makeRule("EQ", "="),
];

const tok = (rs, s) => { const t = nextToken(rs, s, 0); return t === null ? "null" : tokenStr(t); };
const toks = (rs, s) => "[" + tokenize(rs, s).map(tokenStr).join(", ") + "]";

// ── RULE 1 ──
equals(tok(rules, "iffy"), 'ID "iffy" @0', "iffy -> ID (4 > 2)");
equals(tok(rules, "ifx"), 'ID "ifx" @0', "ifx -> ID");
equals(tok(rules, "=="), 'ASSIGN "==" @0', "== -> ASSIGN (2 > 1)");
equals(tok(rules, "ifff"), 'ID "ifff" @0', "ifff -> ID");

// ── RULE 2 ──
equals(tok(rules, "if"), 'KW_IF "if" @0', "if -> KW_IF on the tie");
equals(tok(rules, "="), 'EQ "=" @0', "= -> EQ");

const swapped = [makeRule("ID", "(i|f|x|y)(i|f|x|y)*"), makeRule("KW_IF", "if")];
equals(tok(swapped, "if"), 'ID "if" @0', "with ID first, 'if' lexes as ID");

// ── last-accept mark ──
equals(longestAccept(rules[0].dfa, "iffy", 0), 2, "KW_IF last-accepts at 2");
equals(longestAccept(rules[1].dfa, "iffy", 0), 4, "ID last-accepts at 4");
equals(longestAccept(rules[2].dfa, "iffy", 0), -1, "ASSIGN never matches a prefix of iffy");
equals(longestAccept(rules[1].dfa, "x=y", 0), 1, "ID accepts only 'x' before '='");

// ── full tokenization ──
equals(toks(rules, "iffy==x=y"),
  '[ID "iffy" @0, ASSIGN "==" @4, ID "x" @6, EQ "=" @7, ID "y" @8]', "iffy==x=y");
equals(toks(rules, "if==x"), '[KW_IF "if" @0, ASSIGN "==" @2, ID "x" @4]', "if==x");
equals(toks(rules, "ifif"), '[ID "ifif" @0]', "ifif is one identifier");
equals(toks(rules, "==="), '[ASSIGN "==" @0, EQ "=" @2]', "=== -> ASSIGN then EQ");

// ── lexical error ──
that(throwsOn(() => tokenize(rules, "if z")), "space -> lexical error");

// ── empty input ──
equals(toks(rules, ""), "[]", "empty input -> no tokens");

summary();

function throwsOn(fn) { try { fn(); return false; } catch { return true; } }
