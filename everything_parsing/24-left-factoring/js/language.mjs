// Bounded string enumeration for a leftrec `G` (mirror of java/Language.java).
// BFS over sentential forms, expanding the leftmost nonterminal; prune forms
// whose terminal count already exceeds maxLen. Collect all-terminal forms.

export function upTo(g, maxLen) {
  const out = new Set();
  const visited = new Set();
  const queue = [[g.start]];
  let head = 0;
  let budget = 400_000;

  while (head < queue.length && budget-- > 0) {
    const form = queue[head++];

    let lm = -1;
    for (let i = 0; i < form.length; i++) if (g.isNT(form[i])) { lm = i; break; }

    if (lm < 0) {
      if (form.length <= maxLen) out.add(form.join(" "));
      continue;
    }
    const terminals = form.filter(s => !g.isNT(s)).length;
    if (terminals > maxLen) continue;

    for (const rhs of g.prods.get(form[lm])) {
      const next = [...form.slice(0, lm), ...rhs, ...form.slice(lm + 1)];
      const key = next.join("");
      if (next.length <= 4 * maxLen + 4 && !visited.has(key)) {
        visited.add(key);
        queue.push(next);
      }
    }
  }
  return out;
}
