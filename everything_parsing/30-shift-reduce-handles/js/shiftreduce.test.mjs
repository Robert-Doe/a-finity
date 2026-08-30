// node js/shiftreduce.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Grammar } from "./grammar.mjs";
import { ShiftReduce, compact } from "./shiftreduce.mjs";

console.log("shiftreduce.test.mjs");

const expr = () => Grammar.parse(`
%start E
E -> E + T | T
T -> T * F | F
F -> ( E ) | id
`);
const toks = s => s.split(" ");
const eqArr = (a, b) => a.length === b.length && a.every((x, i) => x === b[i]);

// helper mirroring java rightmostDerivation
function rightmost(g, reductions) {
  let form = [g.start];
  for (const p of [...reductions].reverse()) {
    let i = -1;
    for (let k = form.length - 1; k >= 0; k--) if (form[k] === p.lhs) { i = k; break; }
    form = [...form.slice(0, i), ...p.rhs, ...form.slice(i + 1)];
  }
  return form;
}

const g = expr();
const sr = new ShiftReduce(g);

const r = sr.parse(toks("id + id * id"));
that(r.ok, "id + id * id parses bottom-up");
that(!r.conflict, "unambiguous grammar: no shift-reduce conflict");
equals(r.steps[r.steps.length - 1].action, "ACCEPT", "ends in ACCEPT");

that(eqArr(rightmost(g, r.reductions), toks("id + id * id")),
  "reversing the reductions yields a rightmost derivation ending at the input");
equals(compact(r.reductions[r.reductions.length - 1]), "E -> E + T",
  "the final reduction corresponds to the top of the rightmost derivation");

const red = r.reductions.map(compact);
that(red.indexOf("T -> T * F") < red.indexOf("E -> E + T"),
  "T -> T * F is reduced before E -> E + T");

const r2 = sr.parse(toks("( id + id ) * id"));
that(r2.ok, "( id + id ) * id parses");
that(eqArr(rightmost(g, r2.reductions), toks("( id + id ) * id")), "and reverses correctly");
that(r2.steps.some(s => s.action.includes("F -> ( E )")), "( E ) is reduced as a handle");

const r3 = sr.parse(toks("id"));
equals(r3.reductions.length, 3, "id needs 3 reductions");

const amb = Grammar.parse(`
%start E
E -> E + E | id
`);
const ra = new ShiftReduce(amb).parse(toks("id + id + id"));
that(ra.ok, "the ambiguous grammar still finds A parse");
that(ra.conflict, "and reports a shift-reduce conflict");
that(ra.conflictNote.includes("E + E"), "the conflict state stack is E + E");
that(ra.conflictNote.includes("shift") && ra.conflictNote.includes("reduce"),
  "both shift and reduce lead to accept");

summary();
