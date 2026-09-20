/**
 * Ported from everything_parsing/21-nullable-first/js/grammar.mjs (the
 * course's own Grammar class) — same text format, same parsing rules, typed
 * for the webapp. A grammar is (N, Sigma, P, S): nonterminals, terminals,
 * an ordered list of productions, and a start symbol.
 */
export interface Production {
  index: number;
  lhs: string;
  rhs: string[]; // [] means epsilon
}

export const EPS = "epsilon";
export const END = "$";

export class Grammar {
  constructor(
    public productions: Production[],
    public start: string,
    public nonterminals: Set<string>
  ) {}

  isNonterminal(sym: string): boolean {
    return this.nonterminals.has(sym);
  }

  terminals(): Set<string> {
    const t = new Set<string>();
    for (const p of this.productions) for (const s of p.rhs) if (!this.nonterminals.has(s)) t.add(s);
    return t;
  }

  productionsFor(nt: string): Production[] {
    return this.productions.filter((p) => p.lhs === nt);
  }

  static parse(text: string): Grammar {
    const rawRules: [string, string][] = [];
    let explicitStart: string | null = null;

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

    const nts = new Set(rawRules.map((r) => r[0]));

    const prods: Production[] = [];
    for (const [lhs, rhsWithBars] of rawRules) {
      for (let alt of rhsWithBars.split("|")) {
        alt = alt.trim();
        let syms: string[] = [];
        if (alt !== "" && alt !== "epsilon") {
          syms = alt.split(/\s+/);
          if (syms.includes("epsilon")) throw new Error("'epsilon' cannot be mixed with other symbols: " + rhsWithBars);
        }
        prods.push({ index: prods.length, lhs, rhs: syms });
      }
    }

    const start = explicitStart ?? rawRules[0][0];
    if (!nts.has(start)) throw new Error(`start symbol '${start}' is not a nonterminal`);

    return new Grammar(prods, start, nts);
  }
}

export function productionToString(p: Production): string {
  return p.lhs + " -> " + (p.rhs.length === 0 ? "epsilon" : p.rhs.join(" "));
}

export function isEpsilon(p: Production): boolean {
  return p.rhs.length === 0;
}
