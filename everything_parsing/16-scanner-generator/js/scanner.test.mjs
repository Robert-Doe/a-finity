// node js/scanner.test.mjs

import { readFileSync } from "node:fs";
import { that, equals, summary } from "./assert.mjs";
import { Scanner, ScanError, SpecError } from "./scanner.mjs";
import { parsePattern, PatternError } from "./pattern.mjs";
import { matches } from "./regex.mjs";

console.log("scanner.test.mjs");

const sc = Scanner.fromSpec(readFileSync("fixtures/ajoda.tokens", "utf8"));
const kinds = s => sc.scan(s).map(t => t.kind).join(" ");

// ── the pattern layer desugars to plain regex nodes ──
that(matches(parsePattern("[a-c]+"), "cab"), "[a-c]+ matches cab");
that(!matches(parsePattern("[^a]"), "a"), "[^a] refuses a");
that(matches(parsePattern('"->"'), "->"), "quoted literal needs no escaping");
equals(Scanner.fromSpec(String.raw`token P \(\s`).scan("( ").map(t => t.kind).join(" "),
  "P EOF", "escapes: a paren then a space");
that(throwsPattern("[a-"), "unclosed class is a pattern error");
that(throwsPattern("a b"), "bare space is a pattern error");

// ── longest match ──
equals(kinds("<= >= == != -> && ||"), "LE GE EQ NE ARROW AND OR EOF", "two-char operators win");
equals(kinds("3.14"), "FLOAT_LIT EOF", "3.14 is one float, not 3 . 14");

// ── keywords by lookup, never by prefix ──
equals(kinds("if iffy"), "IF IDENT EOF", "if is a keyword, iffy is not");
equals(kinds("while1 truest"), "IDENT IDENT EOF", "keyword prefixes stay identifiers");

// ── positions survive discarded text ──
const t = sc.scan("// note\n  let");
equals(t[0].kind + " " + t[0].line + ":" + t[0].col, "LET 2:3", "comment + indent skipped, position kept");

// ── errors carry line:col ──
try { sc.scan("x\n  @"); that(false, "should throw"); }
catch (e) { that(e instanceof ScanError && e.line === 2 && e.col === 3, "'@' reported at 2:3"); }

// ── the spec is checked at load ──
that(refused("token A [0-9]*"), "a star-only pattern is refused");
that(refused("token A ()"), "() is refused");
that(refused("token A a\ntoken A b"), "duplicate names are refused");
that(refused("tokn A a"), "unknown directive is refused");
that(!refused("token A a\ndiscard B b"), "a clean spec loads");

// ── empty input is just EOF ──
equals(kinds(""), "EOF", "empty input -> EOF");

summary();

function refused(spec) {
  try { Scanner.fromSpec(spec); return false; } catch (e) { return e instanceof SpecError; }
}
function throwsPattern(src) {
  try { parsePattern(src); return false; } catch (e) { return e instanceof PatternError; }
}
