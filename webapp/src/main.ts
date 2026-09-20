import './style.css';
import { Lexer, Token, TokenType } from './lexer';
import { Parser } from './parser';
import type { ASTNode } from './ast';
import { renderAstTreeSvg } from './theory/tree';
import { renderGrammarTab, renderParseTheoryTab, renderDocsTab } from './theory/render';

const KEYWORD_TYPES = new Set<TokenType>([
  TokenType.KW_INT, TokenType.KW_RETURN, TokenType.KW_IF, TokenType.KW_ELSE,
  TokenType.KW_WHILE, TokenType.KW_VOID, TokenType.KW_PRINT,
]);

const OP_TYPES = new Set<TokenType>([
  TokenType.PLUS, TokenType.MINUS, TokenType.STAR, TokenType.SLASH, TokenType.PERCENT,
  TokenType.EQ, TokenType.NEQ, TokenType.LT, TokenType.LE, TokenType.GT, TokenType.GE,
  TokenType.ASSIGN, TokenType.AND, TokenType.OR, TokenType.NOT,
  TokenType.LPAREN, TokenType.RPAREN, TokenType.LBRACE, TokenType.RBRACE,
  TokenType.SEMICOLON, TokenType.COMMA,
]);

const EXAMPLES: { label: string; code: string }[] = [
  {
    label: 'factorial',
    code: `int factorial(int n) {
    if (n <= 1) {
        return 1;
    }
    return n * factorial(n - 1);
}

int main() {
    print(factorial(6));
    return 0;
}
`,
  },
  {
    label: 'fibonacci + while',
    code: `int fib(int n) {
    int a;
    int b;
    int i;
    a = 0;
    b = 1;
    i = 0;
    while (i < n) {
        int t;
        t = a + b;
        a = b;
        b = t;
        i = i + 1;
    }
    return a;
}

int main() {
    print(fib(10));
    return 0;
}
`,
  },
  {
    label: 'operators + booleans',
    code: `int classify(int x) {
    if (x == 0 || x < 0 && x != -1) {
        print(0);
    } else {
        print(1);
    }
    return x % 2;
}
`,
  },
];

function escapeHtml(s: string): string {
  return s
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;');
}

function tokenClass(t: Token): string {
  if (t.type === TokenType.EOF) return 'tok-eof';
  if (KEYWORD_TYPES.has(t.type)) return 'tok-kw';
  if (t.type === TokenType.INT_LIT) return 'tok-num';
  if (t.type === TokenType.IDENT) return 'tok-ident';
  if (OP_TYPES.has(t.type)) return 'tok-op';
  return '';
}

function renderTokenTable(tokens: Token[]): string {
  const rows = tokens
    .map(
      (t, i) => `<tr>
        <td class="tok-num">${i}</td>
        <td class="${tokenClass(t)}">${escapeHtml(t.type)}</td>
        <td class="${tokenClass(t)}">${escapeHtml(t.lexeme || '')}</td>
        <td class="tok-op">${t.line}:${t.col}</td>
      </tr>`
    )
    .join('');
  return `<table class="token-table">
    <thead><tr><th>#</th><th>type</th><th>lexeme</th><th>pos</th></tr></thead>
    <tbody>${rows}</tbody>
  </table>`;
}

/** Render the AST as an indented, color-coded tree — a lightweight
 * nested-box view without needing a graphics library. */
function renderAst(node: ASTNode, depth = 0): string {
  const pad = '  '.repeat(depth);
  const line = (label: string, extra = ''): string =>
    `${pad}<span class="ast-node"><span class="ast-kind">${label}</span>${extra}</span>\n`;

  switch (node.kind) {
    case 'PROGRAM':
      return node.funcs.map((f) => renderAst(f, depth)).join('');
    case 'FUNC': {
      let out = line('FUNC', ` <span class="ast-val">${escapeHtml(node.name)}</span>(<span class="ast-field">${node.params.join(', ')}</span>)`);
      out += renderAst(node.body, depth + 1);
      return out;
    }
    case 'BLOCK': {
      let out = line('BLOCK');
      for (const s of node.stmts) out += renderAst(s, depth + 1);
      return out;
    }
    case 'VAR_DECL':
      return line('VAR_DECL', ` <span class="ast-val">${escapeHtml(node.name)}</span>`);
    case 'RETURN': {
      let out = line('RETURN');
      if (node.expr) out += renderAst(node.expr, depth + 1);
      return out;
    }
    case 'PRINT': {
      let out = line('PRINT');
      out += renderAst(node.expr, depth + 1);
      return out;
    }
    case 'IF': {
      let out = line('IF');
      out += `${pad}  <span class="ast-field">cond:</span>\n${renderAst(node.cond, depth + 2)}`;
      out += `${pad}  <span class="ast-field">then:</span>\n${renderAst(node.thenBranch, depth + 2)}`;
      if (node.elseBranch) {
        out += `${pad}  <span class="ast-field">else:</span>\n${renderAst(node.elseBranch, depth + 2)}`;
      }
      return out;
    }
    case 'WHILE': {
      let out = line('WHILE');
      out += `${pad}  <span class="ast-field">cond:</span>\n${renderAst(node.cond, depth + 2)}`;
      out += `${pad}  <span class="ast-field">body:</span>\n${renderAst(node.body, depth + 2)}`;
      return out;
    }
    case 'EXPR_STMT': {
      let out = line('EXPR_STMT');
      out += renderAst(node.expr, depth + 1);
      return out;
    }
    case 'ASSIGN': {
      let out = line('ASSIGN', ` <span class="ast-val">${escapeHtml(node.name)}</span> =`);
      out += renderAst(node.value, depth + 1);
      return out;
    }
    case 'BINOP': {
      let out = line('BINOP', ` <span class="ast-val">${escapeHtml(node.op)}</span>`);
      out += renderAst(node.left, depth + 1);
      out += renderAst(node.right, depth + 1);
      return out;
    }
    case 'UNOP': {
      let out = line('UNOP', ` <span class="ast-val">${escapeHtml(node.op)}</span>`);
      out += renderAst(node.left, depth + 1);
      return out;
    }
    case 'CALL': {
      let out = line('CALL', ` <span class="ast-val">${escapeHtml(node.name)}</span>(`);
      for (const a of node.args) out += renderAst(a, depth + 1);
      return out;
    }
    case 'IDENT':
      return line('IDENT', ` <span class="ast-val">${escapeHtml(node.name)}</span>`);
    case 'INT_LIT':
      return line('INT_LIT', ` <span class="ast-val">${node.ival}</span>`);
    default:
      return line('?');
  }
}

// ---------------------------------------------------------------- app shell

const app = document.getElementById('app')!;

app.innerHTML = `
  <header class="topbar">
    <div class="topbar__brand">a-finity</div>
    <nav class="topbar__links">
      <a href="https://github.com/Robert-Doe/a-finity" target="_blank" rel="noopener">GitHub</a>
      <a href="https://robertdoe.com">&larr; robertdoe.com</a>
    </nav>
  </header>

  <section class="hero">
    <h1>Compiler Playground</h1>
    <p>A real port of a-finity's hand-written lexer and recursive-descent parser, running client-side. Type toy-C source below and watch the actual token stream and AST it produces.</p>
  </section>

  <main class="main">
    <div class="playground">
      <div class="card">
        <div class="card__header"><span class="card__title">Source</span></div>
        <div class="card__body">
          <div class="examples" id="examples"></div>
          <textarea class="source" id="source" spellcheck="false"></textarea>
          <div class="run-row">
            <button class="btn" id="run">Run</button>
            <span class="status" id="status"></span>
          </div>
        </div>
      </div>

      <div class="card">
        <div class="tabs" id="tabs">
          <button class="tab active" data-tab="tokens">Tokens</button>
          <button class="tab" data-tab="ast">AST</button>
          <button class="tab" data-tab="tree">Parse Tree</button>
          <button class="tab" data-tab="grammar">Grammar</button>
          <button class="tab" data-tab="theory">Parsing Theory</button>
          <button class="tab" data-tab="docs">Docs</button>
        </div>
        <div class="card__body scroll-panel" id="panel"></div>
      </div>
    </div>
  </main>

  <footer class="footer">
    a-finity is a from-scratch C compiler, built module by module.
    <a href="https://github.com/Robert-Doe/a-finity" target="_blank" rel="noopener">Read the source &rarr;</a>
  </footer>
`;

const sourceEl = document.getElementById('source') as HTMLTextAreaElement;
const statusEl = document.getElementById('status')!;
const panelEl = document.getElementById('panel')!;
const examplesEl = document.getElementById('examples')!;
const tabsEl = document.getElementById('tabs')!;
const playgroundEl = document.querySelector('.playground')!;

type Tab = 'tokens' | 'ast' | 'tree' | 'grammar' | 'theory' | 'docs';
let activeTab: Tab = 'tokens';

// Grammar/Parsing Theory/Docs are properties of the fixed a-finity
// grammar, not of whatever the user typed in Source — so they don't need
// Source visible alongside them, and giving them the full width instead
// of half a 50/50 split is what the LL(1) table and grammar text actually
// need to read without horizontal scrolling on an ordinary desktop.
const REFERENCE_TABS = new Set<Tab>(['grammar', 'theory', 'docs']);

function updateLayoutMode(): void {
  playgroundEl.classList.toggle('playground--wide', REFERENCE_TABS.has(activeTab));
}
let lastTokens: Token[] = [];
let lastAst: ASTNode | null = null;
let lastErrors: string[] = [];

for (const ex of EXAMPLES) {
  const btn = document.createElement('button');
  btn.className = 'chip';
  btn.textContent = ex.label;
  btn.addEventListener('click', () => {
    sourceEl.value = ex.code;
    run();
  });
  examplesEl.appendChild(btn);
}

tabsEl.addEventListener('click', (e) => {
  const target = e.target as HTMLElement;
  const tab = target.dataset.tab as Tab | undefined;
  if (!tab) return;
  activeTab = tab;
  for (const el of tabsEl.querySelectorAll('.tab')) {
    el.classList.toggle('active', (el as HTMLElement).dataset.tab === tab);
  }
  updateLayoutMode();
  renderPanel();
});

function renderPanel(): void {
  // The theory tabs are static (don't depend on the parsed program) and
  // stay visible even when the source has parse errors — you can read the
  // grammar theory without a working example loaded.
  if (activeTab === 'grammar') {
    panelEl.innerHTML = renderGrammarTab();
    return;
  }
  if (activeTab === 'theory') {
    panelEl.innerHTML = renderParseTheoryTab();
    return;
  }
  if (activeTab === 'docs') {
    panelEl.innerHTML = renderDocsTab();
    return;
  }

  if (lastErrors.length > 0) {
    panelEl.innerHTML = `<ul class="error-list">${lastErrors.map((e) => `<li>${escapeHtml(e)}</li>`).join('')}</ul>`;
    return;
  }
  if (activeTab === 'tokens') {
    panelEl.innerHTML = renderTokenTable(lastTokens);
  } else if (activeTab === 'ast') {
    panelEl.innerHTML = `<div class="ast-tree">${lastAst ? renderAst(lastAst) : ''}</div>`;
  } else {
    panelEl.innerHTML = `<div class="tree-view">${lastAst ? renderAstTreeSvg(lastAst) : ''}</div>`;
  }
}

function run(): void {
  const src = sourceEl.value;
  lastErrors = [];

  const lexer = new Lexer(src);
  const tokens = lexer.tokenizeAll();
  lastTokens = tokens;

  if (lexer.errors.length > 0) {
    lastErrors.push(...lexer.errors.map((e) => `lexer: ${e.line}:${e.col}: ${e.message}`));
  }

  // Re-run a fresh lexer for the parser (parser owns its own cursor).
  const parser = new Parser(new Lexer(src));
  const ast = parser.parseProgram();
  lastAst = ast;

  if (parser.errors.length > 0) {
    lastErrors.push(...parser.errors.map((e) => `parser: ${e.line}:${e.col}: ${e.message}`));
  }

  if (lastErrors.length > 0) {
    statusEl.textContent = `${lastErrors.length} error(s)`;
    statusEl.className = 'status err';
  } else {
    statusEl.textContent = `${tokens.length - 1} tokens, ${ast.kind === 'PROGRAM' ? ast.funcs.length : 0} function(s)`;
    statusEl.className = 'status ok';
  }

  renderPanel();
}

document.getElementById('run')!.addEventListener('click', run);
sourceEl.addEventListener('keydown', (e) => {
  if ((e.ctrlKey || e.metaKey) && e.key === 'Enter') run();
});

// Boot with the first example already loaded.
sourceEl.value = EXAMPLES[0].code;
run();
