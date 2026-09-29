// Module 26 — Table-Driven Predictive Parsing (mirror of java/TableParser.java).
//
// Explicit stack + the LL(1) table + a loop. No recursion, no per-grammar code.
// The stack holds [symbol, node]; it represents "the rest of the sentential
// form, left end on top" — recursive descent's call stack, made into data.
// The production sequence is a leftmost derivation.

import { Predict, END } from "./predict.mjs";
import { LL1Table } from "./ll1table.mjs";

function node(symbol) { return { symbol, kids: [], lexeme: null }; }

export class TableParser {
  constructor(g) { this.g = g; this.tbl = new LL1Table(g); }

  parse(tokens) {
    const g = this.g, tbl = this.tbl;
    const input = [...tokens, END];
    const productions = [];
    const trace = [];

    const root = node(g.start);
    const stack = [[END, null], [g.start, root]];

    let ip = 0, step = 0;
    while (stack.length > 0) {
      const [top, nd] = stack[stack.length - 1];
      const look = input[ip];

      if (top === END) {
        if (look === END) {
          trace.push(row(step++, "$", "$", "ACCEPT"));
          return { ok: true, tree: root, productions, trace, error: null };
        }
        trace.push(row(step++, top, look, "error: input left over"));
        return { ok: false, tree: root, productions, trace,
                 error: `unexpected token '${look}' at position ${ip}` };
      }

      if (!g.isNonterminal(top)) {
        if (top === look) {
          nd.lexeme = look;
          trace.push(row(step++, top, look, "match"));
          stack.pop(); ip++;
        } else {
          trace.push(row(step++, top, look, "error: expected " + top));
          return { ok: false, tree: root, productions, trace,
                   error: `expected '${top}' but saw '${look}' at position ${ip}` };
        }
        continue;
      }

      const ps = tbl.cell(top, look);
      if (ps.length !== 1) {
        const why = ps.length === 0 ? "blank cell" : "CONFLICT";
        trace.push(row(step++, top, look, `error: M[${top}][${look}] ${why}`));
        return { ok: false, tree: root, productions, trace,
                 error: `no production for ${top} on '${look}' at position ${ip}` };
      }
      const p = ps[0];
      productions.push(compact(p));
      trace.push(row(step++, top, look, "expand " + compact(p)));

      stack.pop();
      const kids = [];
      for (const s of p.rhs) { const k = node(s); nd.kids.push(k); kids.push(k); }
      if (p.rhs.length === 0) nd.kids.push(node("epsilon"));
      for (let i = p.rhs.length - 1; i >= 0; i--) stack.push([p.rhs[i], kids[i]]);
    }
    return { ok: false, tree: root, productions, trace, error: "stack emptied unexpectedly" };
  }

  replay(productions) {
    const g = this.g;
    let form = [g.start];
    for (const prod of productions) {
      let i = -1;
      for (let k = 0; k < form.length; k++) if (g.isNonterminal(form[k])) { i = k; break; }
      const [, rhsText] = prod.split(" -> ");
      const rhs = rhsText === "epsilon" ? [] : rhsText.split(" ");
      form = [...form.slice(0, i), ...rhs, ...form.slice(i + 1)];
    }
    return form;
  }
}

export function compact(p) {
  return p.lhs + " -> " + (p.rhs.length === 0 ? "epsilon" : p.rhs.join(" "));
}

function row(step, top, look, action) {
  return "  " + String(step).padStart(2) + "  top " + padEnd(top, 4) + "  look " + padEnd(look, 4) + "  " + action;
}
function padEnd(s, w) { s = String(s); while (s.length < w) s += " "; return s; }

export function render(root) {
  let sb = label(root) + "\n";
  const walk = (n, prefix) => {
    n.kids.forEach((k, i) => {
      const last = i === n.kids.length - 1;
      sb += prefix + "+- " + label(k) + "\n";
      walk(k, prefix + (last ? "   " : "|  "));
    });
  };
  walk(root, "");
  return sb.replace(/\s+$/, "");
}
function label(n) {
  return n.lexeme != null && n.lexeme !== n.symbol ? n.symbol + " (" + n.lexeme + ")" : n.symbol;
}
