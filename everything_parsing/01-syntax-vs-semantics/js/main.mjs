// Runs both checkers over every input in a fixture file and prints a report.
//
// Fixture format: one input per line, wrapped in double quotes. Lines that are
// blank or start with '#' are ignored. "" is the empty expression.
//
//   node js/main.mjs fixtures/inputs.txt

import { readFileSync } from "node:fs";
import { tokenize, checkSyntax, evaluate } from "./rpn.mjs";

const path = process.argv[2] ?? "fixtures/inputs.txt";
const lines = readFileSync(path, "utf8").split(/\r?\n/);

console.log("=== Module 01 - Syntax vs. Semantics - RPN checker ===");
console.log();

let n = 0, syntaxPass = 0, semanticsChecked = 0, semanticsPass = 0, syntaxOkButSemanticFail = 0;

for (const raw of lines) {
  const line = raw.trim();
  if (line.length === 0 || line.startsWith("#")) continue;
  if (line.length < 2 || line[0] !== '"' || line[line.length - 1] !== '"') continue;
  const input = line.slice(1, -1);
  n++;

  console.log(`[${n}] "${input}"`);

  const tokens = tokenize(input);
  const syn = checkSyntax(tokens);

  if (syn.ok) {
    syntaxPass++;
    console.log("    syntax   : PASS");
  } else {
    console.log("    syntax   : FAIL  " + at(syn.col, syn.message));
  }

  if (!syn.ok) {
    console.log("    semantics: (skipped: syntax failed)");
  } else {
    semanticsChecked++;
    const sem = evaluate(tokens);
    if (sem.ok) {
      semanticsPass++;
      console.log(`    semantics: PASS  value = ${sem.value}`);
    } else {
      syntaxOkButSemanticFail++;
      console.log("    semantics: FAIL  " + at(sem.col, sem.message));
    }
  }
  console.log();
}

console.log(
  `summary: inputs=${n}` +
  `  syntaxPass=${syntaxPass}` +
  `  semanticsChecked=${semanticsChecked}` +
  `  semanticsPass=${semanticsPass}` +
  `  syntaxOkButSemanticFail=${syntaxOkButSemanticFail}`
);

// "col N: message" when a column is known, otherwise just the message.
function at(col, message) {
  return col > 0 ? `col ${col}: ${message}` : message;
}
