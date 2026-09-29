// node js/regex.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { parse, tree, toLanguage, matches } from "./regex.mjs";

console.log("regex.test.mjs");

// ── parsing: the six node kinds ──
equals(tree(parse("a")), "a", "a single Char");
equals(tree(parse("ab")), "(cat a b)", "juxtaposition is Concat");
equals(tree(parse("a|b")), "(alt a b)", "| is Union");
equals(tree(parse("a*")), "(star a)", "* is Star");

// ── precedence: * > concat > | ──
equals(tree(parse("ab*")), "(cat a (star b))", "* binds tighter than concat");
equals(tree(parse("a|bc")), "(alt a (cat b c))", "concat binds tighter than |");
equals(tree(parse("a|bc*")), "(alt a (cat b (star c)))", "a | (b(c*))");
equals(tree(parse("(ab)*")), "(star (cat a b))", "parens override: (ab)*");
equals(tree(parse("a|b|c")), "(alt (alt a b) c)", "| is left-associative");

// ── sugar desugars at parse time ──
equals(tree(parse("a+")), "(cat a (star a))", "a+ = a a*");
equals(tree(parse("a?")), "(alt a eps)", "a? = a | epsilon");

// ── empty concatenation is epsilon ──
equals(tree(parse("()")), "eps", "() denotes epsilon");
equals(tree(parse("(|a)")), "(alt eps a)", "(|a) = epsilon | a");

// ── malformed regexes are rejected ──
that(rejects("("), "unbalanced (");
that(rejects("a)"), "unbalanced )");
that(rejects("*a"), "* with nothing to repeat");
that(rejects("a**") === false, "a** is legal (star of a star)");

// ── the meaning as a bounded Language (reuses Module 2) ──
equals(toLanguage(parse("(a|b)a*"), 4).render(),
  "{ a, b, aa, ba, aaa, baa, aaaa, baaa }", "(a|b)a* up to length 4");
equals(toLanguage(parse("a*"), 3).render(),
  "{ epsilon, a, aa, aaa }", "a* up to length 3");

// ── exact matching (not bounded) ──
const e = parse("(a|b)*abb");
that(e && matches(e, "abb"), "(a|b)*abb matches abb");
that(matches(e, "aabb"), "matches aabb");
that(matches(e, "babb"), "matches babb");
that(matches(e, "abababb"), "matches abababb");
that(!matches(e, "ab"), "does not match ab");
that(!matches(e, "abba"), "does not match abba");
that(!matches(e, ""), "does not match the empty string");

const opt = parse("a?b?c?");
that(matches(opt, ""), "a?b?c? matches the empty string");
that(matches(opt, "abc"), "matches abc");
that(matches(opt, "ac"), "matches ac");
that(!matches(opt, "acb"), "does not match acb (order fixed)");

const st = parse("(ab)*");
that(matches(st, "") && matches(st, "ab") && matches(st, "abab"), "(ab)* matches even repetitions");
that(!matches(st, "aba") && !matches(st, "a"), "(ab)* rejects partials");

// matching terminates even when a starred subexpression can match epsilon
that(matches(parse("(a*)*"), "aaa"), "(a*)* still terminates and matches");
that(matches(parse("(a*)*"), ""), "(a*)* matches empty");

summary();

function rejects(src) { try { parse(src); return false; } catch { return true; } }
