// node js/ll1table.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Grammar } from "./grammar.mjs";
import { LL1Table } from "./ll1table.mjs";

console.log("ll1table.test.mjs");

const load = g => new LL1Table(Grammar.parse(g));

const e = load(`
%start E
E  -> T Ep
Ep -> + T Ep | epsilon
T  -> F Tp
Tp -> * F Tp | epsilon
F  -> ( E ) | id | num
`);

that(e.isLL1(), "the expression grammar's table has no conflicts");
equals(e.conflicts().length, 0, "zero conflicted cells");

equals(e.cell("E", "id")[0].index, 0, "M[E][id] = production 0");
equals(e.cell("E", "(")[0].index, 0, "M[E][(] = 0");
equals(e.cell("Ep", "+")[0].index, 1, "M[Ep][+] = 1");
equals(e.cell("Ep", ")")[0].index, 2, "M[Ep][)] = 2 (from FOLLOW)");
equals(e.cell("Ep", "$")[0].index, 2, "M[Ep][$] = 2");
that(e.cell("E", "+").length === 0, "M[E][+] is blank");
equals(e.cell("Tp", "*")[0].index, 4, "M[Tp][*] = 4");

that(e.columns.includes("$"), "$ is a column");
that(!e.columns.includes("E"), "nonterminals are not columns");

const de = load(`
%start S
S  -> other | if b then S Sp
Sp -> else S | epsilon
`);
that(!de.isLL1(), "left-factored dangling else is still not LL(1)");
equals(de.conflicts().length, 1, "exactly one conflicted cell");
const c = de.conflicts()[0];
equals(c.nt, "Sp", "conflict is in row Sp");
equals(c.token, "else", "conflict is in column else");
equals(c.ps.length, 2, "two productions compete");

const st = load(`
%start P
P -> L
L -> S L | epsilon
S -> id assign E semi
E -> id | num
`);
that(st.isLL1(), "statement grammar is LL(1)");
equals(st.cell("L", "$")[0].index, 2, "M[L][$] = L -> epsilon");
equals(st.cell("L", "id")[0].index, 1, "M[L][id] = L -> S L");

summary();
