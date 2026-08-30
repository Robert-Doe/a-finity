# Module 10 — Run It

From `10-dfa/`. Verified: OpenJDK 24, Node v22.14.0. No fixture argument.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main
```

Expected (`expected/main.out`), key parts:

```
--- fixtures/ends-abb.dfa  (the regex (a|b)*abb) ---
states: S A AB ABB    alphabet: a b
start: S    accept: ABB

transition table  ( -> start,  * accepting ):
  state      a          b
  -> S        A          S
     A        A          AB
     AB       A          ABB
    *ABB      A          S

runs:
  "abb"    -> S A AB ABB           ACCEPT
  "aabb"   -> S A A AB ABB         ACCEPT
  "ab"     -> S A AB               reject
  "abba"   -> S A AB ABB A         reject
  ""       -> S                    reject

language (length <= 5): abb, aabb, babb, aaabb, ababb, baabb, bbabb

cross-check vs regex (a|b)*abb, both up to length 6:
  DFA language == regex language:  true   (15 strings)
```

Then `even-a.dfa` and `a-star-b-star.dfa`, ending with the pigeonhole demo:

```
why a*b* is not { a^n b^n }:
  accepts "aaab"? true    accepts "abb"? true    accepts "ba"? false
  repeatOn('a'): the DFA is in the SAME state after 0 'a's and after 1 'a'
```

---

## JavaScript — identical bytes

```
node js/main.mjs
```

---

## Tests

```
java -cp java/out DfaTest    # OK 61 passed
node js/dfa.test.mjs         # OK 60 passed
```

---

## DFA fixture format

```
states: S A B
alphabet: a b
start: S
accept: A B
S a A          # fromState symbol toState, one per line
```

`# ...` comments. Any `(state, symbol)` pair with no line routes to an implicit
`<dead>` trap state, so `delta` is always total.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `DFA needs states, alphabet, and a start state` | a header line is missing or misspelled | need `states:`, `alphabet:`, `start:` (accept optional) |
| `transition to unknown state 'X'` | a transition's target isn't in `states:` | add it, or fix the typo |
| every string rejected | the start state isn't in `accept:` and no path reaches an accepting state | check `accept:` and the transitions |
| `DFA language == regex language` is `false` | the DFA and the regex describe different languages, **or** the enumeration bound is too small to see a difference | widen `bound`; if still false, the DFA is wrong |
| a run ends in `<dead>` unexpectedly | a `(state, symbol)` you thought you defined is missing | check the transition lines for that state |
