// Module 31 — LR(0) items and the canonical collection (mirror of java/Lr0.java).
//
// item: production + dot.  CLOSURE: dot before nonterminal B -> add B -> . g.
// GOTO(I,X): shift the dot past X, then CLOSURE. Item sets = DFA states; the DFA
// recognizes viable prefixes (Module 30). Complete item -> reduce; outgoing
// terminal edge -> shift; both -> LR(0) conflict.

export class Lr0 {
  constructor(g) {
    this.startAug = g.start + "'";
    this.prods = [{ index: 0, lhs: this.startAug, rhs: [g.start] }];
    for (const p of g.productions) this.prods.push({ index: this.prods.length, lhs: p.lhs, rhs: [...p.rhs] });
    this.nonterminals = new Set([this.startAug, ...g.nonterminals]);
    this.states = [];      // array of Set<"prod.dot">  (we keep the item objects too)
    this.itemLists = [];   // array of item-object arrays, in insertion order
    this.trans = new Map(); // stateIdx -> Map<symbol, stateIdx>
    this._build();
  }

  isNT(s) { return this.nonterminals.has(s); }
  afterDot(it) { const r = this.prods[it.prod].rhs; return it.dot < r.length ? r[it.dot] : null; }

  closure(kernel) {
    const items = [...kernel];
    const seen = new Set(items.map(k));
    let changed = true;
    while (changed) {
      changed = false;
      for (const it of [...items]) {
        const B = this.afterDot(it);
        if (B != null && this.isNT(B)) {
          for (const p of this.prods) {
            if (p.lhs === B) {
              const ni = { prod: p.index, dot: 0 };
              if (!seen.has(k(ni))) { seen.add(k(ni)); items.push(ni); changed = true; }
            }
          }
        }
      }
    }
    return items;
  }

  gotoSet(items, X) {
    const kernel = [];
    for (const it of items)
      if (this.afterDot(it) === X) kernel.push({ prod: it.prod, dot: it.dot + 1 });
    return kernel.length === 0 ? [] : this.closure(kernel);
  }

  _build() {
    const start = this.closure([{ prod: 0, dot: 0 }]);
    this.itemLists.push(start);
    this.states.push(new Set(start.map(k)));
    const index = new Map([[key(start), 0]]);

    const work = [0];
    while (work.length > 0) {
      const si = work.shift();
      const I = this.itemLists[si];

      const symbols = [];
      const sseen = new Set();
      for (const it of I) {
        const s = this.afterDot(it);
        if (s != null && !sseen.has(s)) { sseen.add(s); symbols.push(s); }
      }

      for (const X of symbols) {
        const J = this.gotoSet(I, X);
        if (J.length === 0) continue;
        let target = index.get(key(J));
        if (target === undefined) {
          target = this.itemLists.length;
          this.itemLists.push(J);
          this.states.push(new Set(J.map(k)));
          index.set(key(J), target);
          work.push(target);
        }
        if (!this.trans.has(si)) this.trans.set(si, new Map());
        this.trans.get(si).set(X, target);
      }
    }
  }

  conflicts() {
    const out = [];
    for (let s = 0; s < this.itemLists.length; s++) {
      const complete = [];
      for (const it of this.itemLists[s])
        if (this.afterDot(it) === null && it.prod !== 0) complete.push(it.prod);
      const edges = this.trans.get(s) ?? new Map();
      const shiftSyms = [...edges.keys()].filter(x => !this.isNT(x));

      if (complete.length >= 2)
        out.push({ state: s, kind: "reduce/reduce", detail: "productions [" + complete.join(", ") + "] both reduce here" });
      if (complete.length > 0 && shiftSyms.length > 0)
        out.push({
          state: s, kind: "shift/reduce",
          detail: "reduce by " + this.itemStr({ prod: complete[0], dot: this.prods[complete[0]].rhs.length })
            + " vs shift [" + shiftSyms.join(", ") + "]",
        });
    }
    return out;
  }

  isLR0() { return this.conflicts().length === 0; }

  itemStr(it) {
    const r = this.prods[it.prod].rhs;
    let s = this.prods[it.prod].lhs + " ->";
    for (let i = 0; i <= r.length; i++) {
      if (i === it.dot) s += " .";
      if (i < r.length) s += " " + r[i];
    }
    if (r.length === 0 && it.dot === 0) s += " .";
    return s;
  }

  render() {
    let sb = "augmented grammar:\n";
    for (const p of this.prods)
      sb += "  (" + p.index + ") " + p.lhs + " -> " + (p.rhs.length === 0 ? "epsilon" : p.rhs.join(" ")) + "\n";
    sb += "\n";
    for (let s = 0; s < this.itemLists.length; s++) {
      sb += "I" + s + ":\n";
      for (const it of this.itemLists[s]) sb += "   " + this.itemStr(it) + "\n";
      const edges = this.trans.get(s);
      if (edges && edges.size > 0) {
        const sorted = [...edges.entries()].sort((a, b) => a[0] < b[0] ? -1 : a[0] > b[0] ? 1 : 0);
        sb += "   ---" + sorted.map(([x, t]) => "  " + x + " -> I" + t).join("") + "\n";
      }
    }
    return sb;
  }
}

const k = it => it.prod + "." + it.dot;
function key(items) { return items.map(k).sort().join(","); }
