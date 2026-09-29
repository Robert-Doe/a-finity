// Module 05 — a parse tree, and the two canonical derivations you can read off it.
//
//   leftmost   — at each step expand the leftmost not-yet-expanded nonterminal
//   rightmost  — ... the rightmost one
//
// Both use the identical multiset of productions and end at the identical tree.
// Only the intermediate sentential forms differ.

import { productionToString } from "./grammar.mjs";

export class ParseTree {
  constructor(symbol) {
    this.symbol = symbol;
    this.children = null;      // null => unexpanded; [] => expanded by an epsilon production
    this.production = null;    // the production applied to expand this node
  }

  get isExpanded() { return this.children !== null; }

  expand(p) {
    this.production = p;
    this.children = p.rhs.map(s => new ParseTree(s));
  }

  // ── build from production choices ──

  static build(g, choices, leftmost) {
    const root = new ParseTree(g.start);
    for (const c of choices) {
      const node = frontier(g, root, leftmost);
      if (!node) throw new Error(`choices left over: tree already complete at choice ${c}`);
      const p = g.productions[c];
      if (p.lhs !== node.symbol)
        throw new Error(
          `production ${c} (${productionToString(p)}) does not match frontier nonterminal '${node.symbol}'`);
      node.expand(p);
    }
    if (frontier(g, root, leftmost)) throw new Error("choices ran out before the tree was complete");
    return root;
  }

  // ── read a derivation off a finished tree ──

  static derivation(g, root, leftmost) {
    const steps = [];
    let frontierNodes = [root];
    steps.push({ form: frontierNodes.map(n => n.symbol), applied: null });

    for (;;) {
      const i = pick(g, frontierNodes, leftmost);
      if (i < 0) break;
      const node = frontierNodes[i];
      frontierNodes = [...frontierNodes.slice(0, i), ...node.children, ...frontierNodes.slice(i + 1)];
      steps.push({ form: frontierNodes.map(n => n.symbol), applied: node.production });
    }
    return steps;
  }

  // ── yield + rendering ──

  terminalYield() {
    const out = [];
    (function walk(n) {
      if (!n.isExpanded) { out.push(n.symbol); return; }
      n.children.forEach(walk);           // epsilon production: no children, adds nothing
    })(this);
    return out;
  }

  render() {
    let sb = this.symbol + "\n";
    const kids = (n, prefix) => {
      if (!n.isExpanded) return;
      if (n.children.length === 0) { sb += prefix + "+- epsilon\n"; return; }
      n.children.forEach((k, i) => {
        const last = i === n.children.length - 1;
        sb += prefix + "+- " + k.symbol + "\n";
        kids(k, prefix + (last ? "   " : "|  "));
      });
    };
    kids(this, "");
    return sb.replace(/\s+$/, "");
  }
}

export function productionMultiset(root) {
  const used = [];
  (function walk(n) {
    if (!n.isExpanded) return;
    used.push(n.production);
    n.children.forEach(walk);
  })(root);
  used.sort((a, b) => a.index - b.index);
  return "[" + used.map(compact).join(", ") + "]";
}

export function compact(p) {
  return p.lhs + "->" + (p.rhs.length === 0 ? "eps" : p.rhs.join(""));
}

// ── helpers ──

function frontier(g, root, leftmost) {
  const fr = [];
  (function collect(n) {
    if (!n.isExpanded) { if (g.isNonterminal(n.symbol)) fr.push(n); return; }
    n.children.forEach(collect);
  })(root);
  if (fr.length === 0) return null;
  return leftmost ? fr[0] : fr[fr.length - 1];
}

function pick(g, nodes, leftmost) {
  const ok = n => n.isExpanded && g.isNonterminal(n.symbol);
  if (leftmost) {
    for (let i = 0; i < nodes.length; i++) if (ok(nodes[i])) return i;
  } else {
    for (let i = nodes.length - 1; i >= 0; i--) if (ok(nodes[i])) return i;
  }
  return -1;
}
