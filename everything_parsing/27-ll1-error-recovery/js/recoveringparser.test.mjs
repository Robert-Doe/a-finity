// node js/recoveringparser.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Grammar } from "./grammar.mjs";
import { RecoveringParser } from "./recoveringparser.mjs";

console.log("recoveringparser.test.mjs");

const toks = s => s.split(" ");

const g = Grammar.parse(`
%start E
E  -> T Ep
Ep -> + T Ep | epsilon
T  -> F Tp
Tp -> * F Tp | epsilon
F  -> ( E ) | id | num
`);
const p = new RecoveringParser(g);

const ok = p.parse(toks("id + id * id"));
that(ok.clean(), "id + id * id: accepted, no errors");

const r1 = p.parse(toks("id + * id"));
that(r1.accepted, "id + * id recovers to end of input");
equals(r1.errors.length, 1, "one error");
that(r1.errors[0].message.includes("discarding '*'"), "the stray '*' is discarded");
equals(r1.errors[0].pos, 2, "error reported at position 2");

const r2 = p.parse(toks("( id + )"));
that(r2.accepted, "( id + ) recovers");
that(r2.errors[0].message.includes("skipping T"), "T is skipped");

const r3 = p.parse(toks("( id + id"));
that(r3.accepted, "( id + id recovers");
that(r3.errors[0].message.includes("inserted missing ')'"), "the ')' is inserted");

const r4 = p.parse(toks("+ id"));
that(r4.accepted, "+ id recovers");
equals(r4.errors[0].pos, 0, "error at position 0");

const r5 = p.parse(toks("id id + id"));
that(r5.accepted, "id id + id recovers");
equals(r5.errors.length, 1, "one error");

const r6 = p.parse(toks("* * * *"));
that(r6 != null, "pathological input still returns");
that(r6.errors.length > 0, "and reports errors");

that(p.syncSet("T").has(")"), "FOLLOW(T) contains )");
that(p.syncSet("T").has("+"), "FOLLOW(T) contains +");

summary();
