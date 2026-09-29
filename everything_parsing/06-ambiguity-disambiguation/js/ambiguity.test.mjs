// node js/ambiguity.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Grammar } from "./grammar.mjs";
import { enumerate } from "./derivation.mjs";
import { allTrees, isAmbiguousFor, smallestAmbiguousString } from "./ambiguity.mjs";

console.log("ambiguity.test.mjs");

const amb = Grammar.parse("E -> E + E | E * E | ( E ) | id");
const unamb = Grammar.parse("E -> E + T | T\nT -> T * F | F\nF -> ( E ) | id\n");

const mul = ["id", "+", "id", "*", "id"];
const add = ["id", "+", "id", "+", "id"];

// ── the ambiguous grammar ──
equals(allTrees(amb, ["id"]).length, 1, "id has one tree");
equals(allTrees(amb, ["id", "+", "id"]).length, 1, "id + id has one tree");
equals(allTrees(amb, mul).length, 2, "id + id * id has TWO trees");
equals(allTrees(amb, add).length, 2, "id + id + id has TWO trees");
that(isAmbiguousFor(amb, mul), "grammar is ambiguous for id + id * id");

const t = allTrees(amb, mul);
const v0 = evalExpr(t[0]), v1 = evalExpr(t[1]);
that((v0 === 6 && v1 === 8) || (v0 === 8 && v1 === 6), "the two trees evaluate to 6 and 8");
that(v0 !== v1, "ambiguity here means two different values");

const a = allTrees(amb, add);
equals(evalExpr(a[0]), 6, "left-assoc value");
equals(evalExpr(a[1]), 6, "right-assoc value (addition is associative)");
that(grouping(a[0]) !== grouping(a[1]), "but the groupings differ");

// ── the disambiguated grammar ──
equals(allTrees(unamb, mul).length, 1, "disambiguated: id + id * id has ONE tree");
equals(allTrees(unamb, add).length, 1, "disambiguated: id + id + id has ONE tree");
equals(grouping(allTrees(unamb, mul)[0]), "(id + (id * id))", "disambiguated groups * before +");
equals(grouping(allTrees(unamb, add)[0]), "((id + id) + id)", "disambiguated makes + left-associative");

// ── same language ──
equals(enumerate(amb, 7).join("|"), enumerate(unamb, 7).join("|"),
  "ambiguous and disambiguated grammars generate the same language (length <= 7)");

// ── smallest ambiguous string ──
equals(smallestAmbiguousString(amb, 7).join(" "), "id * id * id", "smallest ambiguous string");
that(smallestAmbiguousString(unamb, 9) === null, "disambiguated grammar: none up to length 9");

// ── dangling else ──
const dang = Grammar.parse("S -> if x then S | if x then S else S | a | b");
const ie = ["if", "x", "then", "if", "x", "then", "a", "else", "b"];
equals(allTrees(dang, ie).length, 2, "dangling else: two trees");

summary();

function evalExpr(n) {
  if (!n.isExpanded) return n.symbol === "id" ? 2 : 0;
  const k = n.children;
  if (k.length === 1) return evalExpr(k[0]);
  if (k.length === 3) {
    if (k[0].symbol === "(") return evalExpr(k[1]);
    const x = evalExpr(k[0]), y = evalExpr(k[2]);
    if (k[1].symbol === "+") return x + y;
    if (k[1].symbol === "*") return x * y;
  }
  return 0;
}
function grouping(n) {
  if (!n.isExpanded) return n.symbol;
  const k = n.children;
  if (k.length === 1) return grouping(k[0]);
  if (k.length === 3) {
    if (k[0].symbol === "(") return "(" + grouping(k[1]) + ")";
    return "(" + grouping(k[0]) + " " + k[1].symbol + " " + grouping(k[2]) + ")";
  }
  return "?";
}
