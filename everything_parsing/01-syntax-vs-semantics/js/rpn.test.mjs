// node js/rpn.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { tokenize, isInteger, isOperator, checkSyntax, evaluate } from "./rpn.mjs";

console.log("rpn.test.mjs");

// ── tokenize: columns are 1-based, whitespace is skipped ──
const t = tokenize("  3   40 +");
equals(t.length, 3, "three tokens");
equals(t[0].text, "3", "first lexeme");
equals(t[0].col, 3, "first lexeme starts at col 3");
equals(t[1].text, "40", "second lexeme");
equals(t[1].col, 7, "second lexeme starts at col 7");
equals(t[2].col, 10, "operator starts at col 10");

// ── classification ──
that(isInteger("0") && isInteger("1234"), "digits are integers");
that(!isInteger("") && !isInteger("-5") && !isInteger("1a"),
  "empty, signed, and mixed are not integers");
that(isOperator("+") && isOperator("/"), "+ and / are operators");
that(!isOperator("++") && !isOperator("%"), "++ and % are not operators");

// ── syntax: well-formed cases ──
that(syntaxOk("3 4 +"), "3 4 + is well formed");
that(syntaxOk("10 2 - 3 *"), "10 2 - 3 * is well formed");
that(syntaxOk("12 0 /"), "12 0 / is well formed (syntax cannot see the zero)");

// ── syntax: the four failure shapes ──
equals(syntaxMsg(""), "empty expression", "empty input");
equals(syntaxMsg("3 x +"), "unknown token 'x'", "unknown lexeme");
equals(syntaxMsg("3 4 + +"),
  "operator '+' needs 2 operands, stack has 1", "operand underflow");
equals(syntaxMsg("3 4"),
  "2 values left on stack, expected 1", "leftover operands");

// ── the column of each syntax error ──
equals(syntaxCol("3 x +"), 3, "unknown token column");
equals(syntaxCol("3 4 + +"), 7, "underflow points at the offending operator");
equals(syntaxCol("3 4"), 0, "leftover-operands error has no single column");

// ── semantics: value and the one real error ──
equals(evalStr("3 4 +"), 7n, "3 + 4");
equals(evalStr("10 2 - 3 *"), 24n, "(10 - 2) * 3");
equals(evalStr("7 2 /"), 3n, "7 / 2 truncates toward zero");
equals(evalStr("0 5 - 2 /"), -2n, "(-5) / 2 truncates toward zero, not down");

const div0 = evaluate(tokenize("12 0 /"));
that(!div0.ok, "12 0 / fails semantically");
equals(div0.message, "division by zero", "the semantic error message");
equals(div0.col, 6, "the '/' is at column 6");

// ── the headline claim: syntax PASS and semantics FAIL on the same string ──
const toks = tokenize("12 0 /");
that(checkSyntax(toks).ok && !evaluate(toks).ok,
  "12 0 / : syntactically valid, semantically invalid");

summary();

function syntaxOk(s)  { return checkSyntax(tokenize(s)).ok; }
function syntaxMsg(s) { return checkSyntax(tokenize(s)).message; }
function syntaxCol(s) { return checkSyntax(tokenize(s)).col; }
function evalStr(s)   { return evaluate(tokenize(s)).value; }
