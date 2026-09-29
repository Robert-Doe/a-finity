// node js/recursivedescent.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { lex } from "./lexer.mjs";
import { parse, render, terminalYield, SyntaxError_ } from "./recursivedescent.mjs";
import { replay, sentence } from "./derivation.mjs";

console.log("recursivedescent.test.mjs");

// ── the tiny lexer ──
equals(lex("1 + 2").length, 4, "1 + 2 -> num + num EOF");
equals(lex("(a)")[1].kind, "id", "middle token of (a) is id");
equals(lex("12")[0].text, "12", "digits run together into one num");

// ── the claim: productions fired ARE a leftmost derivation ──
const r = parse("1 + 2 * 3");
equals(r.rules[0], "E -> T E'", "first production is the start rule");
equals(r.rules.length, 11, "1 + 2 * 3 fires 11 productions");
equals(sentence(r.rules), "num + num * num",
  "replaying the fired productions as a leftmost derivation yields the token kinds");
equals(replay(r.rules).length, r.rules.length + 1,
  "a derivation has one more sentential form than it has steps");

// ── replay throws if the production list is not leftmost ──
let threw = false;
try { replay(["E -> T E'", "E' -> epsilon"]); } catch { threw = true; }
that(threw, "replay rejects a production that does not expand the leftmost nonterminal");

// ── the tree ──
equals(r.tree.symbol, "E", "root is E");
equals(r.tree.kids.length, 2, "E has exactly two children: T and E'");
equals(terminalYield(r.tree).join(" "), "num + num * num", "the tree's terminal yield is the input");

// ── precedence: * binds tighter than + ──
that(parse("1 + 2 * 3").rules.includes("T' -> * F T'"), "the * is consumed by a T' (a factor-level rule)");

// ── associativity: right-recursive E' means a+b+c leans right ──
const assoc = parse("a + b + c");
equals(assoc.rules.filter(x => x === "E' -> + T E'").length, 2,
  "a + b + c uses E' -> + T E' twice (nested to the right)");

// ── parens override precedence ──
that(parse("(1 + 2) * 3").rules.includes("F -> ( E )"), "parenthesised sub-expression uses F -> ( E )");

// ── syntax errors, with position ──
const errPos = s => { try { parse(s); return -1; } catch (e) { return e.pos; } };
const errMsg = s => { try { parse(s); return ""; } catch (e) { return e.message; } };
equals(errPos("1 +"), 3, "'1 +' : error at position 3 (end of input, expecting a factor)");
that(errMsg("1 +").includes("factor"), "'1 +' : message mentions a missing factor");
equals(errPos("(1 + 2"), 6, "'(1 + 2' : error at position 6, expecting ')'");
that(errMsg("(1 + 2").includes(")"), "'(1 + 2' : message mentions ')'");
equals(errPos("1 2"), 2, "'1 2' : trailing token at position 2 (expected EOF)");
equals(errPos(""), 0, "empty input : error at position 0");

// ── a well-formed single factor ──
equals(JSON.stringify(parse("a").rules),
  JSON.stringify(["E -> T E'", "T -> F T'", "F -> id", "T' -> epsilon", "E' -> epsilon"]),
  "'a' : five productions, both primes go to epsilon");

summary();
