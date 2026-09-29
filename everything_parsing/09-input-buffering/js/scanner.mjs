// A tiny scanner built on Buffer, to exercise peek / advance / mark / retract.
//   WORD [a-z]+   NUM [0-9]+   FLOAT [0-9]+ '.' [0-9]+   PLUS '+'   (spaces skipped)
// The FLOAT rule is where retract is needed: after "12." peek one more char;
// if it isn't a digit, the '.' belongs to the next token and forward retracts one.

import { EOF } from "./buffer.mjs";

const isDigit = (c) => c >= 48 && c <= 57;
const isLower = (c) => c >= 97 && c <= 122;

export function next(b, retracts) {
  while (b.peek() === 32) b.advance();
  b.mark();
  const c = b.peek();
  if (c === EOF) return null;

  if (isLower(c)) {
    do { b.advance(); } while (isLower(b.peek()));
    return { kind: "WORD", text: b.lexeme() };
  }
  if (isDigit(c)) {
    do { b.advance(); } while (isDigit(b.peek()));
    if (b.peek() === 46) { // '.'
      b.advance();
      if (isDigit(b.peek())) {
        do { b.advance(); } while (isDigit(b.peek()));
        return { kind: "FLOAT", text: b.lexeme() };
      }
      b.retract(1);
      retracts[0]++;
      return { kind: "NUM", text: b.lexeme() };
    }
    return { kind: "NUM", text: b.lexeme() };
  }
  if (c === 43) { b.advance(); return { kind: "PLUS", text: b.lexeme() }; } // '+'

  b.advance();
  return { kind: "ERR", text: b.lexeme() };
}

export function scanAll(b, retracts) {
  const out = [];
  let t;
  while ((t = next(b, retracts)) !== null) out.push(t);
  return out;
}
