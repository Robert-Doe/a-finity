// node js/tableparser.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Grammar } from "./grammar.mjs";
import { TableParser } from "./tableparser.mjs";

console.log("tableparser.test.mjs");

const toks = s => s.split(" ");
const eqArr = (a, b) => a.length === b.length && a.every((x, i) => x === b[i]);

const g = Grammar.parse(`
%start E
E  -> T Ep
Ep -> + T Ep | epsilon
T  -> F Tp
Tp -> * F Tp | epsilon
F  -> ( E ) | id | num
`);
const p = new TableParser(g);

const r = p.parse(toks("id + id * id"));
that(r.ok, "id + id * id accepted");
equals(r.productions[0], "E -> T Ep", "first production is the start rule");
equals(r.productions.length, 11, "11 productions applied");

that(eqArr(p.replay(r.productions), toks("id + id * id")),
  "replaying the productions leftmost yields exactly the input");

equals(r.tree.symbol, "E", "root is E");
equals(r.tree.kids.length, 2, "E has two children (T, Ep)");

that(p.parse(toks("( id + num ) * id")).ok, "( id + num ) * id accepted");
that(p.parse(toks("num")).ok, "num accepted");

const e1 = p.parse(toks("id +"));
that(!e1.ok, "id + rejected");
that(e1.error.includes("position 2"), "error at position 2");

const e2 = p.parse(toks("id id"));
that(!e2.ok, "id id rejected");
that(e2.error.includes("Tp"), "blank cell for Tp on 'id'");

const e3 = p.parse(toks("( id + id"));
that(!e3.ok, "( id + id rejected");
that(e3.error.includes(")"), "error mentions the expected ')'");

that(e1.productions.includes("Ep -> + T Ep"), "productions before the error are kept");

summary();
