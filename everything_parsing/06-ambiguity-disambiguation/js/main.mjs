// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { Grammar, productionToString } from "./grammar.mjs";
import { enumerate } from "./derivation.mjs";
import { allTrees, smallestAmbiguousString } from "./ambiguity.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 06 - Ambiguity & Disambiguation ===");
p();

const amb   = load("expr-ambiguous.grammar");
const unamb = load("expr-unambiguous.grammar");
const dang  = load("dangling-else.grammar");

const mul = ["id", "+", "id", "*", "id"];
const add = ["id", "+", "id", "+", "id"];

// 1. ambiguous
p("--- fixtures/expr-ambiguous.grammar ---");
rules(amb);
exprTrees(amb, mul, "id + id * id");
exprTrees(amb, add, "id + id + id");
p("smallest ambiguous string (length <= 7): " + smallestAmbiguousString(amb, 7).join(" "));
p();

// 2. disambiguated
p("--- fixtures/expr-unambiguous.grammar ---");
rules(unamb);
exprTrees(unamb, mul, "id + id * id");
exprTrees(unamb, add, "id + id + id");
p();

// 3. same language?
p("--- language equivalence (ambiguous vs disambiguated) ---");
const la = enumerate(amb, 7);
const lu = enumerate(unamb, 7);
p("L(ambiguous)   up to length 7: " + la.length + " strings");
p("L(unambiguous) up to length 7: " + lu.length + " strings");
p("same language? " + sameList(la, lu));
p();

// 4. dangling else
p("--- fixtures/dangling-else.grammar ---");
rules(dang);
const ie = ["if", "x", "then", "if", "x", "then", "a", "else", "b"];
const trees = allTrees(dang, ie);
p(`string "${ie.join(" ")}" has ${trees.length} parse trees:`);
trees.forEach((t, k) => {
  const elseAtTop = t.isExpanded && t.children.length === 6;
  p();
  p("  TREE " + String.fromCharCode(65 + k) + " - else binds to the "
    + (elseAtTop ? "OUTER" : "INNER") + " if");
  p(indent(t.render()));
});
p();

p("summary: ambiguous: 2 trees for id+id*id (values 8 vs 6), 2 for id+id+id"
  + " | disambiguated: 1 tree each, same language=" + sameList(la, lu)
  + " | dangling-else: " + trees.length + " trees");

process.stdout.write(sb);

// ── per-grammar reporting ──

function exprTrees(g, target, label) {
  const t = allTrees(g, target);
  p(`string "${label}" has ${t.length} parse tree${t.length === 1 ? ":" : "s:"}`);
  t.forEach((tree, k) => {
    p();
    p("  TREE " + String.fromCharCode(65 + k) + "   value(id=2) = " + evalExpr(tree)
      + "   grouping: " + grouping(tree));
    p(indent(tree.render()));
  });
  p();
}

function rules(g) {
  for (const pr of g.productions) p(`  (${pr.index}) ${productionToString(pr)}`);
  p();
}

function evalExpr(n) {
  if (!n.isExpanded) return n.symbol === "id" ? 2 : 0;
  const k = n.children;
  if (k.length === 1) return evalExpr(k[0]);
  if (k.length === 3) {
    if (k[0].symbol === "(") return evalExpr(k[1]);
    const a = evalExpr(k[0]), b = evalExpr(k[2]);
    if (k[1].symbol === "+") return a + b;
    if (k[1].symbol === "*") return a * b;
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

// ── plumbing ──

function load(name) { return Grammar.parse(readFileSync("fixtures/" + name, "utf8")); }
function sameList(a, b) { return a.length === b.length && a.every((x, i) => x === b[i]); }
function indent(block) {
  return block.split("\n").map(l => "    " + l).join("\n").replace(/\s+$/, "");
}
