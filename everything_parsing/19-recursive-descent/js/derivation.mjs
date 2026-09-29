// Replays a production list as a LEFTMOST derivation (mirror of java/Derivation.java).
// Start at [E]; for each production, replace the leftmost nonterminal with its
// RHS. If the list is a recursive-descent firing order, every LHS matches the
// leftmost nonterminal and the final form is the input's token kinds.

const NONTERMINALS = new Set(["E", "E'", "T", "T'", "F"]);

export function replay(rules) {
  let form = ["E"];
  const steps = [{ form: join(form), rule: null }];

  for (const rule of rules) {
    const i = leftmostNonterminal(form);
    if (i < 0) throw new Error(`form has no nonterminal but rule remains: ${rule}`);

    const [lhs, rhsText] = rule.split(" -> ");
    if (form[i] !== lhs.trim())
      throw new Error(
        `rule '${rule}' expands ${lhs.trim()} but the leftmost nonterminal is ${form[i]} ` +
        `— the derivation is not leftmost`);

    const rhs = rhsText.trim().split(/\s+/).filter(s => s !== "epsilon");
    form = [...form.slice(0, i), ...rhs, ...form.slice(i + 1)];
    steps.push({ form: join(form), rule });
  }
  return steps;
}

export function sentence(rules) {
  const s = replay(rules);
  return s[s.length - 1].form;
}

function leftmostNonterminal(form) {
  for (let i = 0; i < form.length; i++) if (NONTERMINALS.has(form[i])) return i;
  return -1;
}

function join(form) {
  return form.length === 0 ? "epsilon" : form.join(" ");
}
