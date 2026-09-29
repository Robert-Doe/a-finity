# Module 12 — Run It

From `12-thompson-construction/`. Verified: OpenJDK 24, Node v22.14.0. No fixture arg.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main
```

Expected (`expected/main.out`), key parts:

```
regex /a(b|c)/   tree: (cat a (alt b c))
nodes: 5   |regex| (no parens): 4

Thompson NFA:
  states: q0 q1 q2 q3 q4 q5 q6 q7
  start: q0   accept: q7
  q0 a q1
  q1 epsilon q6
  q2 b q3
  q3 epsilon q7
  q4 c q5
  q5 epsilon q7
  q6 epsilon q2
  q6 epsilon q4
  -> 8 states

state count is always <= 2*nodes  (and <= 2*|regex|):
  regex           nodes  chars states   <=2*nodes  <=2*chars
  /a/                 1      1      2   yes        yes
  ...
  /(a|b)*abb/        10      7     14   yes        yes

full pipeline for /(a|b)*abb/:
  Thompson NFA:            14 states, has epsilon: true
  removeEpsilon():         14 states, has epsilon: false
  L(Thompson NFA) up to length 6:  15 strings
    == regex /(a|b)*abb/:          true
    == DFA(Module 10):             true
```

---

## JavaScript — identical bytes

```
node js/main.mjs
```

---

## Tests

```
java -cp java/out ThompsonTest    # OK 97 passed
node js/thompson.test.mjs         # OK 97 passed
```

---

## The six gadgets

| Regex node | New states | Transitions added |
|---|---|---|
| `∅` (Empty) | 2 | none — accepts nothing |
| `ε` (Epsilon) | 2 | `s -ε-> e` |
| `c` (Char) | 2 | `s -c-> e` |
| `Concat(l, r)` | 0 | `l.end -ε-> r.start` |
| `Union(l, r)` | 2 | `s -ε-> l.start`, `s -ε-> r.start`, `l.end -ε-> e`, `r.end -ε-> e` |
| `Star(x)` | 2 | `s -ε-> x.start`, `s -ε-> e`, `x.end -ε-> x.start`, `x.end -ε-> e` |

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| state count exceeds `2*nodes` | a gadget adds more than its budget (Concat should add **0**) | check `frag()` — Concat only adds one ε-line, no `fresh()` calls |
| the NFA rejects a string the regex matches | a gadget wired the wrong end | each fragment has exactly one `start` and one `end`; Concat wires `l.end -> r.start`, Star loops `x.end -> x.start` |
| `Nfa needs states and a start state` | the regex was `∅` or `ε` only and produced no alphabet | this build allows an empty alphabet — update your local `Nfa.parse` if you copied an older one |
| `(a*)*` loops forever | you're using a *backtracking* matcher, not the set simulator | `Nfa.accepts` tracks a state set — it never backtracks |
