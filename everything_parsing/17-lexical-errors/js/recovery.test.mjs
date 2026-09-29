// node js/recovery.test.mjs

import { readFileSync } from "node:fs";
import { that, equals, summary } from "./assert.mjs";
import { Scanner, ScanError } from "./scanner.mjs";
import { Strategy, scanRecovering } from "./recovery.mjs";

console.log("recovery.test.mjs");

const sc = Scanner.fromSpec(readFileSync("fixtures/ajoda.tokens", "utf8"));
const kinds = r => r.tokens.map(t => t.kind).join(" ");

// ── SKIP_ONE: one ERROR per bad character ──
const a = scanRecovering(sc, "x$y?z", Strategy.SKIP_ONE);
equals(kinds(a), "IDENT ERROR IDENT ERROR IDENT EOF", "x$y?z");
equals(a.errors.map(e => e.col).join(","), "2,4", "errors at columns 2 and 4");

const b = scanRecovering(sc, "a @@@# b", Strategy.SKIP_ONE);
equals(b.errors.length, 4, "@@@# -> 4 separate errors");

// ── SKIP_TO_RESTART: one ERROR per run ──
const c = scanRecovering(sc, "a @@@# b", Strategy.SKIP_TO_RESTART);
equals(kinds(c), "IDENT ERROR IDENT EOF", "a @@@# b -> IDENT ERROR IDENT");
equals(c.errors.length, 1, "one error for the run");
equals(c.tokens[1].lexeme, "@@@#", "the ERROR token holds the whole run");
equals(c.errors[0].col, 3, "the run starts at column 3");

// ── all garbage ──
equals(kinds(scanRecovering(sc, "@@@", Strategy.SKIP_TO_RESTART)), "ERROR EOF", "@@@ -> one ERROR");
equals(scanRecovering(sc, "@@@", Strategy.SKIP_ONE).errors.length, 3, "@@@ -> 3 errors under SKIP_ONE");

// ── scanning continues past an error ──
const f = scanRecovering(sc, "let n: i64 = 5 @ 2;", Strategy.SKIP_ONE);
equals(kinds(f), "LET IDENT COLON I64 ASSIGN INT_LIT ERROR INT_LIT SEMI EOF", "continues past '@'");

// ── line numbers across a file ──
const g = scanRecovering(sc, "a\n  $\nb ?", Strategy.SKIP_ONE);
equals(g.errors.map(e => e.line + ":" + e.col).join(" "), "2:3 3:3", "errors keep line:col");
that(throwsScan(() => sc.scan("$x$")), "Module 16's scan still stops at the first");

// ── discarded text is never an error ──
equals(scanRecovering(sc, "// @#$?\nx", Strategy.SKIP_ONE).errors.length, 0, "comment contents are fine");

// ── clean input ──
const h = scanRecovering(sc, "let x: i64 = 9;", Strategy.SKIP_TO_RESTART);
equals(h.errors.length, 0, "clean input -> no errors");
that(!h.tokens.some(t => t.kind === "ERROR"), "no ERROR tokens");

summary();

function throwsScan(fn) { try { fn(); return false; } catch (e) { return e instanceof ScanError; } }
