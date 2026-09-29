# a-finity webapp — Compiler Playground

An interactive, in-browser demo of a-finity's real compiler front end. It is
not a mockup: `src/lexer.ts` and `src/parser.ts` are direct TypeScript ports
of the hand-written lexer (`module_16 - The Complete Compiler/lexer.c`) and
recursive-descent parser (`module_16 - The Complete Compiler/parser.c`),
transliterated function-for-function — same `TokenType` set, same keyword
table, same operator disambiguation, same grammar, same two-token-lookahead
trick for `IDENT '=' ...` assignment vs. expression statements.

Type source in the toy-C dialect the course's compiler actually accepts
(functions, `int`/`void`, `if`/`else`, `while`, `print(...)`, the usual C
expression operators) and the app runs the real lexer and parser client-side,
showing:

- **Tokens** — the full token stream as a table (index, type, lexeme, line:col)
- **AST** — the parser's actual output, rendered as an indented, color-coded tree

Three quick-load example snippets are included (factorial, fibonacci with a
`while` loop, and a boolean/operator-heavy example) via the chip buttons above
the source textarea.

## Local development

```sh
npm install
npm run dev
```

## Production build

```sh
npm run build
```

Output is written to `webapp/dist/`.

## Deploying

Deploy `webapp/` as a static site (Vercel/Netlify/Cloudflare Pages): set the
project root directory to `webapp`, build command to `npm run build`, and
output directory to `dist`.
