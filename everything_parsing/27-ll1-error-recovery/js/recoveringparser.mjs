// Module 27 — LL(1) error recovery: panic mode + phrase-level
// (mirror of java/RecoveringParser.java).
//
//   terminal mismatch on top -> PHRASE-LEVEL: insert the expected token, keep input.
//   M[A][t] blank/conflicted -> PANIC MODE:
//       t in FOLLOW(A) (or $) -> pop A (skip the nonterminal)
//       otherwise             -> discard t, retry
// every branch pops the stack or advances the input, so it terminates.

import { Predict, setStr, END } from "./predict.mjs";
import { LL1Table } from "./ll1table.mjs";

export class RecoveringParser {
  constructor(g) {
    this.g = g;
    this.tbl = new LL1Table(g);
    this.pr = new Predict(g);
  }

  syncSet(nt) { return this.pr.followOf(nt); }

  parse(tokens) {
    const g = this.g, tbl = this.tbl, pr = this.pr;
    const input = [...tokens, END];
    const errors = [];
    const trace = [];

    const stack = [END, g.start];

    let ip = 0, step = 0, guard = 1000;
    let accepted = false;

    while (stack.length > 0 && guard-- > 0) {
      const top = stack[stack.length - 1];
      const look = input[ip];

      if (top === END) {
        if (look === END) {
          trace.push(row(step++, top, look, "ACCEPT"));
          accepted = true;
        } else {
          errors.push({ pos: ip, message: `extra input: discarding '${look}'` });
          trace.push(row(step++, top, look, "discard (extra input)"));
          ip++;
          continue;
        }
        break;
      }

      if (!g.isNonterminal(top)) {
        if (top === look) {
          trace.push(row(step++, top, look, "match"));
          stack.pop(); ip++;
        } else {
          errors.push({ pos: ip, message: `inserted missing '${top}' before '${look}'` });
          trace.push(row(step++, top, look, `insert '${top}' (phrase-level)`));
          stack.pop();
        }
        continue;
      }

      const ps = tbl.cell(top, look);
      if (ps.length === 1) {
        const p = ps[0];
        trace.push(row(step++, top, look, "expand " + compact(p)));
        stack.pop();
        for (let i = p.rhs.length - 1; i >= 0; i--) stack.push(p.rhs[i]);
        continue;
      }

      const sync = pr.followOf(top);
      if (look === END || sync.has(look)) {
        errors.push({ pos: ip, message: `no rule for ${top} on '${look}'; skipping ${top} (lookahead in FOLLOW)` });
        trace.push(row(step++, top, look, `pop ${top} (sync on FOLLOW)`));
        stack.pop();
      } else {
        errors.push({ pos: ip, message: `no rule for ${top} on '${look}'; discarding '${look}'` });
        trace.push(row(step++, top, look, `discard '${look}'`));
        ip++;
      }
    }
    return {
      accepted, errors, trace,
      clean() { return accepted && errors.length === 0; },
    };
  }
}

export function compact(p) {
  return p.lhs + " -> " + (p.rhs.length === 0 ? "epsilon" : p.rhs.join(" "));
}
export { setStr };

function row(step, top, look, action) {
  return "  " + String(step).padStart(2) + "  top " + padEnd(top, 4) + "  look " + padEnd(look, 4) + "  " + action;
}
function padEnd(s, w) { s = String(s); while (s.length < w) s += " "; return s; }
