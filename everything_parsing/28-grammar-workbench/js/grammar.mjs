// Module 02 — a context-free grammar, stored as data.
//
// A grammar is (N, Sigma, P, S):
//   N     = nonterminals  — every symbol that appears on a left-hand side
//   Sigma = terminals     — every other symbol that appears in a right-hand side
//   P     = productions    — ordered; each is { index, lhs, rhs: [symbols] } (rhs [] = epsilon)
//   S     = start symbol   — the first lhs, unless "%start X" overrides it

export class Grammar {
  constructor(productions, start, nonterminals) {
    this.productions = productions;        // [{ index, lhs, rhs }]
    this.start = start;
    this.nonterminals = nonterminals;      // Set, insertion order = first appearance as lhs
  }

  isNonterminal(sym) { return this.nonterminals.has(sym); }

  // Terminals: rhs symbols that are never a lhs.
  terminals() {
    const t = new Set();
    for (const p of this.productions)
      for (const s of p.rhs)
        if (!this.nonterminals.has(s)) t.add(s);
    return t;
  }

  productionsFor(nt) { return this.productions.filter(p => p.lhs === nt); }

  static parse(text) {
    const rawRules = [];      // [lhs, rhsWithBars]
    let explicitStart = null;

    for (let line of text.split(/\r?\n/)) {
      const hash = line.indexOf("#");
      if (hash >= 0) line = line.slice(0, hash);
      line = line.trim();
      if (line === "") continue;

      if (line.startsWith("%start")) {
        const parts = line.split(/\s+/);
        if (parts.length !== 2) throw new Error("bad %start line: " + line);
        explicitStart = parts[1];
        continue;
      }

      const arrow = line.indexOf("->");
      if (arrow < 0) throw new Error("rule has no '->': " + line);
      if (line.indexOf("->", arrow + 2) >= 0) throw new Error("rule has more than one '->': " + line);

      const lhs = line.slice(0, arrow).trim();
      if (lhs === "" || lhs.includes(" ")) throw new Error("left-hand side must be one symbol: " + line);
      rawRules.push([lhs, line.slice(arrow + 2).trim()]);
    }
    if (rawRules.length === 0) throw new Error("grammar has no rules");

    const nts = new Set(rawRules.map(r => r[0]));

    const prods = [];
    for (const [lhs, rhsWithBars] of rawRules) {
      for (let alt of rhsWithBars.split("|")) {
        alt = alt.trim();
        let syms = [];
        if (alt !== "" && alt !== "epsilon") {
          syms = alt.split(/\s+/);
          if (syms.includes("epsilon"))
            throw new Error("'epsilon' cannot be mixed with other symbols: " + rhsWithBars);
        }
        prods.push({ index: prods.length, lhs, rhs: syms });
      }
    }

    const start = explicitStart ?? rawRules[0][0];
    if (!nts.has(start)) throw new Error(`start symbol '${start}' is not a nonterminal`);

    return new Grammar(prods, start, nts);
  }
}

export function productionToString(p) {
  return p.lhs + " -> " + (p.rhs.length === 0 ? "epsilon" : p.rhs.join(" "));
}

export function isEpsilon(p) { return p.rhs.length === 0; }
