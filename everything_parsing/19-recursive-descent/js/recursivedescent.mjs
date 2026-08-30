// Module 19 — Predictive recursive-descent parser (mirror of java/RecursiveDescent.java).
//
//   E  -> T E'
//   E' -> + T E'  |  epsilon
//   T  -> F T'
//   T' -> * F T'  |  epsilon
//   F  -> ( E )   |  num  |  id
//
// One function per nonterminal. Each expansion pushes its production onto
// `rules` the instant it commits, so `rules` — read top to bottom — is a
// leftmost derivation. `derivation.mjs` replays it to prove that.

import { lex } from "./lexer.mjs";

export class SyntaxError_ extends Error {
  constructor(msg, pos) { super(msg); this.pos = pos; }
}

function node(symbol) { return { symbol, kids: [] }; }

export function parse(src) {
  const toks = lex(src);
  let p = 0;
  const rules = [];

  const peek = () => toks[p];
  const at = kind => peek().kind === kind;
  const describe = t => (t.kind === "EOF" ? "end of input" : `'${t.text}'`);

  function expect(kind) {
    if (!at(kind)) {
      const t = peek();
      throw new SyntaxError_(`expected ${kind} but saw ${describe(t)}`, t.pos);
    }
    return toks[p++];
  }

  function term(t) {
    const label = (t.kind === "num" || t.kind === "id") ? `${t.kind} (${t.text})` : t.kind;
    return node(label);
  }

  function parseE() {
    rules.push("E -> T E'");
    const n = node("E");
    n.kids.push(parseT());
    n.kids.push(parseEprime());
    return n;
  }

  function parseEprime() {
    const n = node("E'");
    if (at("+")) {
      rules.push("E' -> + T E'");
      n.kids.push(term(expect("+")));
      n.kids.push(parseT());
      n.kids.push(parseEprime());
    } else {
      rules.push("E' -> epsilon");
      n.kids.push(node("epsilon"));
    }
    return n;
  }

  function parseT() {
    rules.push("T -> F T'");
    const n = node("T");
    n.kids.push(parseF());
    n.kids.push(parseTprime());
    return n;
  }

  function parseTprime() {
    const n = node("T'");
    if (at("*")) {
      rules.push("T' -> * F T'");
      n.kids.push(term(expect("*")));
      n.kids.push(parseF());
      n.kids.push(parseTprime());
    } else {
      rules.push("T' -> epsilon");
      n.kids.push(node("epsilon"));
    }
    return n;
  }

  function parseF() {
    const n = node("F");
    if (at("(")) {
      rules.push("F -> ( E )");
      n.kids.push(term(expect("(")));
      n.kids.push(parseE());
      n.kids.push(term(expect(")")));
    } else if (at("num")) {
      rules.push("F -> num");
      n.kids.push(term(expect("num")));
    } else if (at("id")) {
      rules.push("F -> id");
      n.kids.push(term(expect("id")));
    } else {
      const t = peek();
      throw new SyntaxError_(
        `expected '(', num, or id to start a factor but saw ${describe(t)}`, t.pos);
    }
    return n;
  }

  const tree = parseE();
  expect("EOF");
  return { tree, rules };
}

// ── ASCII tree render (matches Module 5) ──

export function render(root) {
  let sb = root.symbol + "\n";
  const walk = (n, prefix) => {
    n.kids.forEach((k, i) => {
      const last = i === n.kids.length - 1;
      sb += prefix + "+- " + k.symbol + "\n";
      walk(k, prefix + (last ? "   " : "|  "));
    });
  };
  walk(root, "");
  return sb.replace(/\s+$/, "");
}

export function terminalYield(root) {
  const out = [];
  const walk = n => {
    if (n.kids.length === 0) {
      if (n.symbol !== "epsilon") {
        const sp = n.symbol.indexOf(" ");
        out.push(sp < 0 ? n.symbol : n.symbol.slice(0, sp));
      }
      return;
    }
    n.kids.forEach(walk);
  };
  walk(root);
  return out;
}
