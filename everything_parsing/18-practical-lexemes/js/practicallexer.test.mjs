// node js/practicallexer.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { lex, tokStr } from "./practicallexer.mjs";

console.log("practicallexer.test.mjs");

const kinds = s => lex(s).tokens.map(t => t.kind).join(" ");
const one = s => { const t = lex(s).tokens; return t.length === 0 ? "(none)" : tokStr(t[0]); };
const text = s => lex(s).tokens[0].text;

// ── reserved-word trick ──
equals(kinds("let x in y"), "KW_LET ID KW_IN ID", "keywords looked up");
equals(kinds("lettuce letters"), "ID ID", "words starting with a keyword are ID");
equals(kinds("if1 then2"), "ID ID", "if1 is an identifier");

// ── numbers ──
equals(one("0"), 'INT "0"', "0");
equals(one("42"), 'INT "42"', "42");
equals(one("3.14"), 'FLOAT "3.14"', "3.14");
equals(one("1e10"), 'FLOAT "1e10"', "1e10");
equals(one("2.5E-3"), 'FLOAT "2.5E-3"', "2.5E-3");
equals(one("0xDEAD"), 'HEX "0xDEAD"', "0xDEAD");
equals(kinds("1e"), "INT ID", "1e retracts: INT then ID");

const dotcase = lex("5.x");
equals(dotcase.tokens.map(t => t.kind).join(" "), "INT ID", "5.x -> INT then ID");
equals(dotcase.errors.length, 1, "the stray '.' is one error");

// ── strings + escapes ──
equals(text('"hi"'), "hi", "plain string");
equals(text('"a\\nb"'), "a\nb", "\\n becomes a newline");
equals(text('"tab\\there"'), "tab\there", "\\t");
equals(text('"quote \\" done"'), 'quote " done', "\\\" embedded quote");
equals(text('"back \\\\ slash"'), "back \\ slash", "\\\\ one backslash");
equals(lex('"\\u0041\\u0042"').tokens[0].text, "AB", "\\u0041\\u0042 -> AB");

const bad = lex('"oops');
equals(bad.errors.length, 1, "unterminated string -> 1 error");
that(bad.errors[0].includes("position 0"), "names the opening position");
equals(bad.tokens[0].kind, "STRING?", "yields STRING? token");
that(lex('"a\\qb"').errors.some(e => e.includes("\\q")), "bad escape reported");

// ── comments ──
equals(kinds("a // ignore this\nb"), "ID ID", "// to end of line");
equals(kinds("a /* c */ b"), "ID ID", "/* */ block comment");
equals(kinds("a /* /* nested */ still */ b"), "ID ID", "NESTED comment fully consumed");
equals(kinds("a /* /* not closed */ b"), "ID", "one close for two opens -> runs to EOF");
that(lex("x /* /*").errors.some(e => e.includes("unterminated block comment")), "unterminated nested comment reported");

// ── the "not regular" point ──
const deep = lex("/* a /* b */ c */ done");
equals(deep.tokens.map(t => t.kind).join(" "), "ID", "nested comment skipped; only 'done' remains");
equals(deep.tokens[0].text, "done", "and it's 'done'");

// ── a small program ──
equals(kinds("let pi = 3.14 * r"), "KW_LET ID OP FLOAT OP ID", "a small program");

summary();
