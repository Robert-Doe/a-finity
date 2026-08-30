// node js/main.mjs fixtures/regexes.txt
// Same output bytes as the Java build.

import { readFileSync } from "node:fs";
import { parse, tree, toLanguage, matches } from "./regex.mjs";

const path = process.argv[2] ?? "fixtures/regexes.txt";
const lines = readFileSync(path, "utf8").split(/\r?\n/);

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

p("=== Module 03 - Regular Expressions: Formal Definition ===");
p();

let n = 0;
for (const raw of lines) {
  const line = raw.trim();
  if (line === "" || line.startsWith("#")) continue;
  const bar = line.indexOf(" | ");
  const src = (bar < 0 ? line : line.slice(0, bar)).trim();
  const tests = bar < 0 ? [] : line.slice(bar + 3).trim().split(/\s+/);
  n++;

  const re = parse(src);
  p(`[${n}] /${src}/`);
  p("    tree : " + tree(re));

  const lang = toLanguage(re, 6);
  p("    L (up to length 6): " + clip(lang, 12));

  for (const t of tests) {
    const w = t === "~" ? "" : t;
    const shown = w === "" ? '""' : `"${w}"`;
    p("    " + pad(shown, 10) + (matches(re, w) ? "match" : "no"));
  }
  p();
}

p("precedence check (tightest: * , then concatenation, then |):");
for (const s of ["a|bc*", "ab*", "(ab)*", "a|b|c", "ab|cd"]) {
  p("    /" + pad(s, 8) + "  ->  " + tree(parse(s)));
}
p();

p(`summary: regexes=${n}  nodeKinds=6 (Empty, Epsilon, Char, Union, Concat, Star)  sugar: + ? desugared at parse time`);

process.stdout.write(sb);

function clip(l, max) {
  const all = l.list();
  if (all.length <= max) return l.render();
  const head = all.slice(0, max).map(w => (w === "" ? "epsilon" : w));
  return "{ " + head.join(", ") + `, ... (${all.length} total) }`;
}
function pad(s, w) { return s.length >= w ? s : s + " ".repeat(w - s.length); }
