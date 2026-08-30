# Module 11 — Run It

From `11-nfa-epsilon/`. Verified: OpenJDK 24, Node v22.14.0. No fixture argument.
Reads one fixture from `../10-dfa/` for the cross-check.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main
```

Expected (`expected/main.out`), key parts:

```
run "aabb"  (tracking the SET of possible states):
  after start  {S}
  after a      {S,S1}
  after a      {S,S1}
  after b      {S,S2}
  after b      {S,S3}
  final set contains S3 (accepting)?  true   -> ACCEPT

cross-check, all up to length 6:
  NFA language == DFA language (Module 10):  true   (15 strings)
  every NFA-accepted string is regex-matched:  true

epsilon-closure of {I} (the start):  {A1,B1,F,I,U}

run "ab":
  after start  {A1,B1,F,I,U}
  after a      {A1,A2,B1,F,U,W}
  after b      {A1,B1,B2,F,U,W}

removeEpsilon():  new NFA has epsilon transitions?  false
  L(epsilon-NFA) == L(epsilon-free NFA), up to length 5:  true
  ...and both == regex (a|b)*:  true
```

---

## JavaScript — identical bytes

```
node js/main.mjs
```

---

## Tests

```
java -cp java/out NfaTest    # OK 112 passed
node js/nfa.test.mjs         # OK 112 passed
```

---

## NFA fixture format

Same as the DFA format, plus:
- `epsilon` is a legal symbol on a transition line
- a state may have **several** lines for the same symbol (that's the nondeterminism)
- `delta` is **not** completed to total — "no transition" is a legal dead end

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| a string you expect to be accepted is rejected | some path dies before the end — check every branch, not just the "obvious" one | trace it: `nfa.trace(chars(w))` shows the state set at each step |
| `epsilon-closure` missing a state | an `epsilon` transition line is missing or misspelled (must be exactly `epsilon`) | check the fixture |
| `removeEpsilon()` changes the language | the new accepting-states rule or the `delta'(q,a) = closure(move(closure(q), a))` formula is wrong | both are in `Nfa.removeEpsilon`; the closure must wrap the whole move |
| the ε-NFA for `(a|b)*` only accepts `a*` or `b*` | the loop-back `epsilon` goes to the wrong branch — it must return to the **shared** branch point, not to one arm | see `ab-star.nfa`: `W epsilon U`, and `U` branches to both `A1` and `B1` |
