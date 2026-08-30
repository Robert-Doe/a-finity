// node js/pumping.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Grammar } from "./grammar.mjs";
import { classify, isRegular, Kind } from "./grammarclass.mjs";
import { regularRefutation, cflRefutation, inAnBn, inAnBnCn } from "./pumping.mjs";

console.log("pumping.test.mjs");

// ── grammar classification ──
equals(classify(Grammar.parse("S -> a S | b")), Kind.RIGHT_LINEAR, "S -> a S | b is right-linear");
equals(classify(Grammar.parse("S -> S a | b")), Kind.LEFT_LINEAR, "S -> S a | b is left-linear");
equals(classify(Grammar.parse("S -> a S b | epsilon")), Kind.CONTEXT_FREE,
  "a^n b^n grammar is context-free (nonterminal in the middle)");
equals(classify(Grammar.parse("E -> E + E | id")), Kind.CONTEXT_FREE,
  "E -> E + E | id is context-free (two nonterminals)");
that(isRegular(Grammar.parse("S -> a S | b S | b")), "A -> wB / A -> w only is regular");
that(!isRegular(Grammar.parse("S -> a S b | epsilon")), "a^n b^n grammar is not regular");

// ── the language predicates ──
that(inAnBn("") && inAnBn("ab") && inAnBn("aabb"), "a^n b^n members");
that(!inAnBn("aab") && !inAnBn("ba") && !inAnBn("abab"), "a^n b^n non-members");
that(inAnBnCn("") && inAnBnCn("abc") && inAnBnCn("aabbcc"), "a^n b^n c^n members");
that(!inAnBnCn("aabbc") && !inAnBnCn("abcabc"), "a^n b^n c^n non-members");

// ── regular pumping lemma: NO valid pumping length for a^n b^n ──
for (let p = 1; p <= 8; p++) {
  const w = regularRefutation(p);
  that(w.allEscaped, `p=${p}: every x y z split of a^p b^p escapes under pumping`);
  that(inAnBn(w.s), "the chosen string a^p b^p is in the language");
  that(!inAnBn(w.pumped), "the pumped representative is not in a^n b^n");
  equals(w.decompositionsTried, (p * (p + 1)) / 2, "checked all p(p+1)/2 valid splits");
}

// ── context-free pumping lemma: NO valid pumping length for a^n b^n c^n ──
for (let p = 1; p <= 4; p++) {
  const w = cflRefutation(p);
  that(w.allEscaped, `p=${p}: every 5-split of a^p b^p c^p escapes under pumping`);
  that(inAnBnCn(w.s), "the chosen string a^p b^p c^p is in the language");
  that(!inAnBnCn(w.pumped), "the pumped representative is not in a^n b^n c^n");
}

summary();
