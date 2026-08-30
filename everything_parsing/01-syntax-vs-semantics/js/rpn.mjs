// Module 01 — Syntax vs. Semantics.
//
// A tiny language: reverse Polish (postfix) integer arithmetic.
//   token    = INTEGER | OPERATOR
//   INTEGER  = digit {digit}            (no sign)
//   OPERATOR = "+" | "-" | "*" | "/"
//
// Two independent checkers over the SAME token list:
//
//   checkSyntax — is the string well formed? Tracks only the DEPTH of the
//                 operand stack, never a value.
//   evaluate    — run it for real. The only failure syntax cannot pre-empt is
//                 division by zero: "12 0 /" is well formed and still meaningless.

// ─────────────────────────────────────────── lexical layer

// A token is { text, col } where col is the 1-based start column in the line.
export function tokenize(line) {
  const out = [];
  let i = 0;
  const n = line.length;
  while (i < n) {
    const c = line[i];
    if (c === " " || c === "\t") { i++; continue; }
    const start = i;
    while (i < n && line[i] !== " " && line[i] !== "\t") i++;
    out.push({ text: line.slice(start, i), col: start + 1 }); // +1: columns are 1-based
  }
  return out;
}

export function isInteger(t) {
  if (t.length === 0) return false;
  for (const c of t) if (c < "0" || c > "9") return false;
  return true;
}

export function isOperator(t) {
  return t === "+" || t === "-" || t === "*" || t === "/";
}

// ─────────────────────────────────────────── syntax layer (well-formedness only)

// Well-formed iff, simulating the operand stack's DEPTH:
//   - every lexeme is an INTEGER or an OPERATOR,
//   - every operator sees depth >= 2 when it runs,
//   - the final depth is exactly 1.
// No integer is ever parsed; no arithmetic is ever done.
export function checkSyntax(tokens) {
  if (tokens.length === 0) return { ok: false, message: "empty expression", col: 0 };

  let depth = 0;
  for (const tok of tokens) {
    const t = tok.text;
    if (isInteger(t)) {
      depth += 1;
    } else if (isOperator(t)) {
      if (depth < 2) {
        return {
          ok: false,
          message: `operator '${t}' needs 2 operands, stack has ${depth}`,
          col: tok.col,
        };
      }
      depth -= 1; // pop 2 operands, push 1 result
    } else {
      return { ok: false, message: `unknown token '${t}'`, col: tok.col };
    }
  }
  if (depth !== 1) {
    return { ok: false, message: `${depth} values left on stack, expected 1`, col: 0 };
  }
  return { ok: true, message: null, col: 0 };
}

// ─────────────────────────────────────────── semantic layer (real evaluation)

// Precondition: checkSyntax(tokens).ok === true
export function evaluate(tokens) {
  const stack = [];
  for (const tok of tokens) {
    const t = tok.text;
    if (isInteger(t)) {
      stack.push(BigInt(t));
      continue;
    }
    const b = stack.pop();
    const a = stack.pop();
    let r;
    switch (t) {
      case "+": r = a + b; break;
      case "-": r = a - b; break;
      case "*": r = a * b; break;
      case "/":
        if (b === 0n) return { ok: false, value: 0n, message: "division by zero", col: tok.col };
        r = a / b; // BigInt division truncates toward zero, matching Java long
        break;
      default: throw new Error("not an operator: " + t);
    }
    stack.push(r);
  }
  return { ok: true, value: stack.pop(), message: null, col: 0 };
}
