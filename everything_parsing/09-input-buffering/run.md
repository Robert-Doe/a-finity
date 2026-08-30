# Module 09 — Run It

From `09-input-buffering/`. Verified: OpenJDK 24, Node v22.14.0.
No fixture argument — `Main` scans one hard-coded source string.

---

## Java

```
javac -d java/out java/*.java
java -cp java/out Main
```

Expected (`expected/main.out`):

```
=== Module 09 - Input Buffering: Two-Buffer Scheme & Sentinels ===

source (39 chars): "the 12.5 quick brownish fox 12.go +9 ok"
buffer half-size: 8

scan trace  (token | running buffer loads):
  WORD   "the"      loads=1  half=0
  FLOAT  "12.5"     loads=2  half=1
  WORD   "quick"    loads=2  half=1
  WORD   "brownish" loads=3  half=0
  WORD   "fox"      loads=4  half=1
  NUM    "12"       loads=4  half=1
  ERR    "."        loads=4  half=1
  WORD   "go"       loads=5  half=0
  PLUS   "+"        loads=5  half=0
  NUM    "9"        loads=5  half=0
  WORD   "ok"       loads=5  half=0

...

totals:
  characters consumed : 39
  buffer loads        : 5
  loads per character : 0.128   (amortized O(1))
  forward reads       : 40   (one 'data[forward]' compare per advance ...)
  retracts            : 1
```

---

## JavaScript — identical bytes

```
node js/main.mjs
```

---

## Tests

```
java -cp java/out BufferTest
node js/buffer.test.mjs
```

Both end `OK   22 passed`.

---

## Getting a blank or broken result?

| Symptom | Cause | Fix |
|---|---|---|
| `retract crossed the halfway boundary (lexeme too long)` | a lexeme longer than one half | that's the Dragon Book's constraint — raise `halfSize` above your longest lexeme |
| `lexeme()` returns `""` or a truncated string | the lexeme's start half was overwritten by a reload before `lexeme()` was called | same cause: lexeme must fit in one half |
| `buffer loads` is one higher than `ceil(len / halfSize)` | the source length is an exact multiple of `halfSize`, so a final fill of 0 chars places the EOF sentinel | expected — the `+1` in the Dragon Book formula |
| `advance()` never returns EOF | the source contains the raw code unit used for a sentinel (only possible if you changed `SENTINEL`/`EOF` to a real character) | keep them as `-1` / `-2`, outside the char range |
