// Module 07 — where does a grammar sit in the Chomsky hierarchy?
//
//   REGULAR (Type 3)      every production is  A -> w B  or  A -> w   (right-linear)
//                         or  A -> B w | w                            (left-linear)
//   CONTEXT_FREE (Type 2) single nonterminal on the left, but not linear

export const Kind = { RIGHT_LINEAR: "RIGHT_LINEAR", LEFT_LINEAR: "LEFT_LINEAR", CONTEXT_FREE: "CONTEXT_FREE" };

export function classify(g) {
  let allRight = true, allLeft = true;
  for (const p of g.productions) {
    const rhs = p.rhs;
    let nts = 0, firstNt = -1, lastNt = -1;
    rhs.forEach((s, i) => {
      if (g.isNonterminal(s)) { nts++; if (firstNt < 0) firstNt = i; lastNt = i; }
    });
    if (nts === 0) continue;
    if (nts > 1) { allRight = false; allLeft = false; continue; }
    if (lastNt !== rhs.length - 1) allRight = false;
    if (firstNt !== 0) allLeft = false;
  }
  if (allRight) return Kind.RIGHT_LINEAR;
  if (allLeft) return Kind.LEFT_LINEAR;
  return Kind.CONTEXT_FREE;
}

export function isRegular(g) {
  const k = classify(g);
  return k === Kind.RIGHT_LINEAR || k === Kind.LEFT_LINEAR;
}
