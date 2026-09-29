// Everything Parsing — the whole test framework, on purpose.
// Canonical copy: prerequisites/assert/assert.mjs
// Each module keeps its own copy in js/ so imports stay relative and simple.
//
// No test runner. A *.test.mjs file imports these, calls them, and ends with
// summary(). process.exit(1) on failure so run.md commands and CI can tell.

let passed = 0;
let failed = 0;

export function that(cond, label) {
  if (cond) { passed++; return; }
  failed++;
  console.log("  FAIL  " + label);
}

export function equals(actual, expected, label) {
  const a = norm(actual);
  const e = norm(expected);
  if (a === e) { passed++; return; }
  failed++;
  console.log("  FAIL  " + label);
  console.log("        expected: " + show(e));
  console.log("        actual  : " + show(a));
}

function norm(v) {
  if (typeof v === "string") return v;
  if (typeof v === "bigint") return v.toString() + "n";
  return JSON.stringify(v);
}

function show(s) {
  return String(s).includes("\n") ? "\n----\n" + s + "\n----" : '"' + s + '"';
}

export function summary() {
  console.log();
  console.log(failed === 0
    ? "OK   " + passed + " passed"
    : "FAIL " + failed + " failed, " + passed + " passed");
  if (failed !== 0) process.exit(1);
}
