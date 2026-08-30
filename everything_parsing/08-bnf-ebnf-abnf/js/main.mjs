// node js/main.mjs   — identical output bytes to the Java build.

import { readFileSync } from "node:fs";
import { Grammar } from "./grammar.mjs";
import { enumerate } from "./derivation.mjs";
import { parse, toBnfText, accepts, isoRule } from "./ebnf.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 08 - BNF, EBNF, ABNF & Syntax Diagrams ===");
p();

report("expr.ebnf", 7,
  [["NUM"], ["NUM", "+", "NUM"], ["(", "NUM", ")"], ["NUM", "*", "NUM", "+", "NUM"], ["NUM", "+", "NUM", "*", "NUM"]],
  [["+", "NUM"], ["NUM", "+"], ["(", "NUM"]]);

report("signed.ebnf", 4,
  [["0"], ["-", "1"], ["1", "2"], ["-", "0", "1", "2"]],
  [["-"], ["-", "-", "1"], []]);

cheatSheet();

p("summary: expr.ebnf -> BNF, same language | signed.ebnf -> BNF, same language"
  + " | operators ? * + ( ) all desugared into fresh nonterminals");

process.stdout.write(sb);

function report(file, maxLen, shouldAccept, shouldReject) {
  const eg = parse(readFileSync("fixtures/" + file, "utf8"));
  const bnfText = toBnfText(eg);
  const bg = Grammar.parse(bnfText);

  p("--- fixtures/" + file + " ---");
  p("EBNF, ISO-style  ( { } = zero or more,  [ ] = optional ):");
  for (const [name, body] of eg.rules) p("  " + pad(name, 8) + " = " + isoRule(body));
  p();

  p("desugared to pure BNF:");
  for (const line of bnfText.split("\n")) if (line && !line.startsWith("%")) p("  " + line);
  p();

  const bnfLang = enumerate(bg, maxLen);
  let allAgree = true;
  for (const w of bnfLang) {
    const toks = w === "epsilon" ? [] : w.split(" ");
    if (!accepts(eg, toks)) allAgree = false;
  }
  p("equivalence check (BNF enumerated to length " + maxLen + "):");
  p("  " + bnfLang.length + " strings in L(BNF); every one also accepted by the EBNF matcher: " + allAgree);

  let posOk = true, negOk = true;
  for (const s of shouldAccept) posOk = posOk && accepts(eg, s) && inLang(bnfLang, s);
  for (const s of shouldReject) negOk = negOk && !accepts(eg, s) && !inLang(bnfLang, s);
  p("  sample accepts all pass: " + posOk + "   sample rejects all pass: " + negOk);
  p();
}

function inLang(lang, toks) {
  return lang.includes(toks.length === 0 ? "epsilon" : toks.join(" "));
}

function cheatSheet() {
  p("notation cheat-sheet:");
  const rows = [
    ["concept", "BNF", "EBNF (this course)", "ISO-EBNF", "ABNF"],
    ["define", "::=  or  ->", "=", "=", "="],
    ["alternative", "|", "|", "|", "/"],
    ["optional", "(extra rule)", "x?", "[ x ]", "[x]  /  *1x"],
    ["zero or more", "(recursion)", "x*", "{ x }", "*x"],
    ["one or more", "(recursion)", "x+", "x { x }", "1*x"],
    ["grouping", "(extra rule)", "( x )", "( x )", "( x )"],
    ["terminal", "literal text", "'x'  \"x\"", "'x'  \"x\"", "\"x\" (ci),  %x41"],
  ];
  for (const r of rows)
    p("  " + pad(r[0], 14) + " " + pad(r[1], 16) + " " + pad(r[2], 20) + " " + pad(r[3], 12) + " " + r[4]);
  p();
}

function pad(s, w) { s = String(s); return s.length >= w ? s : s + " ".repeat(w - s.length); }
