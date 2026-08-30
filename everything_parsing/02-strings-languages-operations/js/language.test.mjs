// node js/language.test.mjs

import { that, equals, summary } from "./assert.mjs";
import { Language } from "./language.mjs";

console.log("language.test.mjs");

// ── the empty string vs the empty language ──
equals(Language.EMPTY.size, 0, "{} has no strings");
equals(Language.EPSILON.size, 1, "{ epsilon } has exactly one string");
that(Language.EPSILON.contains(""), "{ epsilon } contains the empty string");
that(!Language.EMPTY.contains(""), "{} does not contain the empty string");

// ── canonical order ──
equals(Language.of("bb", "a", "ab", "").render(),
  "{ epsilon, a, ab, bb }", "render is (length, lexicographic)");

// ── union ──
equals(Language.of("a", "b").union(Language.of("b", "c")).render(),
  "{ a, b, c }", "union deduplicates");

// ── concat ──
equals(Language.of("a", "b").concat(Language.of("x", "y")).render(),
  "{ ax, ay, bx, by }", "concat is the glued cartesian product");
equals(Language.EMPTY.concat(Language.of("a")).render(), "{ }",
  "empty language annihilates under concat");
equals(Language.EPSILON.concat(Language.of("a", "b")).render(), "{ a, b }",
  "{ epsilon } is the identity under concat");

// ── power ──
equals(Language.of("a", "b").power(0).render(), "{ epsilon }", "L^0 = { epsilon }");
equals(Language.of("a", "b").power(1).render(), "{ a, b }", "L^1 = L");
equals(Language.of("a", "b").power(2).render(), "{ aa, ab, ba, bb }", "L^2");
equals(Language.of("ab").power(3).render(), "{ ababab }", "{ab}^3");

// ── bounded Kleene star ──
equals(Language.of("a").star(4).render(),
  "{ epsilon, a, aa, aaa, aaaa }", "{a}* up to length 4");
equals(Language.EMPTY.star(5).render(), "{ epsilon }", "empty language star = { epsilon }");
equals(Language.of("", "a").star(3).render(),
  "{ epsilon, a, aa, aaa }", "star terminates when L contains epsilon");

// ── Sigma* ──
equals(Language.sigmaStar(["a", "b"], 2).render(),
  "{ epsilon, a, b, aa, ab, ba, bb }", "Sigma* over {a,b} up to length 2");
equals(Language.sigmaStar(["a", "b"], 3).size, 15, "|Sigma*<=3| = 1+2+4+8");

// ── a regular expression, rebuilt from operations ──
const re = Language.of("a").union(Language.of("b"))
  .concat(Language.of("a").star(3))
  .intersect(Language.sigmaStar(["a", "b"], 4));
equals(re.render(), "{ a, b, aa, ba, aaa, baa, aaaa, baaa }", "(a|b)a* up to length 4");
that(re.contains("b") && re.contains("ba") && re.contains("baaa"),
  "(a|b)a* accepts b, ba, baaa");
that(!re.contains("ab") && !re.contains(""),
  "(a|b)a* rejects ab and the empty string");

// ── intersect / minus ──
equals(Language.of("a", "b", "c").intersect(Language.of("b", "c", "d")).render(),
  "{ b, c }", "intersection");
equals(Language.of("a", "b", "c").minus(Language.of("b")).render(),
  "{ a, c }", "difference");

summary();
