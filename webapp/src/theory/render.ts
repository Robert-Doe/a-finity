/**
 * Renders the "Parsing Theory" tab: epsilon/nullable, FIRST, FOLLOW,
 * predictive parsing condition, and the LL(1) table, all computed LIVE
 * against the real a-finity grammar by the ported course algorithms
 * (never hardcoded numbers). Concept explanations are adapted from the
 * course's own DECISIONS.md files for modules 20, 21, 22, and 25.
 */
import { Grammar, EPS, END, productionToString } from './grammar';
import { FirstSets, setStr as firstSetStr } from './firstsets';
import { FollowSets } from './followsets';
import { Predict, setStr as predictSetStr } from './predict';
import { LL1Table } from './ll1table';
import { BNF_TEXT, EBNF_TEXT, RULE_TRACE } from './grammar-data';

function esc(s: string): string {
  return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}

let cached: {
  g: Grammar;
  fs: FirstSets;
  fol: FollowSets;
  pr: Predict;
  table: LL1Table;
} | null = null;

function analysis() {
  if (cached) return cached;
  const g = Grammar.parse(BNF_TEXT);
  const fs = new FirstSets(g);
  const fol = new FollowSets(g);
  const pr = new Predict(g);
  const table = new LL1Table(g);
  cached = { g, fs, fol, pr, table };
  return cached;
}

export function renderGrammarTab(): string {
  const rows = RULE_TRACE.map(
    (r) => `<tr>
      <td class="tok-ident">${esc(r.rule)}</td>
      <td class="tok-num">${r.parserFn ? esc(r.parserFn) : ''}</td>
      <td class="tok-op">${r.note ? esc(r.note) : ''}</td>
    </tr>`
  ).join('');

  return `
    <div class="theory-block">
      <h3>The grammar, EBNF</h3>
      <p class="theory-p">Verbatim from <code>parser.ts</code>'s own header comment (itself a direct port of <code>module_16/parser.c</code>) &mdash; completed with the <code>stmt</code> sub-productions the comment only named. <code>ε</code> is the empty string: "this can match nothing."</p>
      <pre class="grammar-block">${esc(EBNF_TEXT)}</pre>
    </div>

    <div class="theory-block">
      <h3>Which parser function implements which rule</h3>
      <p class="theory-p">A hand-written recursive-descent parser is the grammar: every nonterminal below is a real function in <code>parser.ts</code>, and every function body is its rule's right-hand side, expressed as code.</p>
      <table class="token-table trace-table">
        <thead><tr><th>Rule</th><th>Function</th><th>Note</th></tr></thead>
        <tbody>${rows}</tbody>
      </table>
    </div>

    <div class="theory-block">
      <h3>Desugared to plain BNF</h3>
      <p class="theory-p">The FIRST/FOLLOW/predictive-parsing machinery on the next tab only understands bare productions &mdash; no <code>*</code>, <code>?</code>, or grouping. Below is the EBNF above desugared by hand, following the exact rule taught in the course's BNF/EBNF/ABNF module: <code>X*</code> becomes a fresh right-recursive <code>Xrep -&gt; X Xrep | ε</code>, and <code>X?</code> becomes <code>Xopt -&gt; X | ε</code>. This is what actually gets parsed and analyzed on the next tab &mdash; <code>OR</code> stands in for the literal <code>||</code> only because the grammar-text format's own alternative separator is a bare <code>|</code>.</p>
      <pre class="grammar-block small">${esc(BNF_TEXT)}</pre>
    </div>
  `;
}

export function renderParseTheoryTab(): string {
  const { g, fs, fol, pr, table } = analysis();

  // --- Nullable ---
  const nullableRounds = fs.nullableRounds
    .map((round, i) => `<div class="fixpoint-row"><span class="fp-round">round ${i}</span><span class="fp-set">${round.size ? esc([...round].sort().join(', ')) : '∅'}</span></div>`)
    .join('');

  // --- FIRST ---
  const firstRows = [...g.nonterminals]
    .map((nt) => `<tr><td class="tok-ident">${esc(nt)}</td><td class="tok-str">${esc(firstSetStr(fs.firstOf(nt)))}</td></tr>`)
    .join('');

  // --- FOLLOW ---
  const followRows = [...g.nonterminals]
    .map((nt) => `<tr><td class="tok-ident">${esc(nt)}</td><td class="tok-str">${esc(firstSetStr(fol.followOf(nt)))}</td></tr>`)
    .join('');

  // --- Predict conflicts ---
  const conflicts = pr.conflicts();
  const conflictBlock =
    conflicts.length === 0
      ? `<div class="hint">No conflicts &mdash; every nonterminal's PREDICT sets are pairwise disjoint. This grammar is LL(1).</div>`
      : conflicts
          .map(
            (c) => `<div class="conflict-card">
        <div class="conflict-head"><span class="tok-eof">CONFLICT</span> nonterminal <code>${esc(c.nonterminal)}</code> on lookahead <code>${esc(c.token)}</code></div>
        <div class="conflict-body">
          <div><code>${esc(c.a)}</code></div>
          <div><code>${esc(c.b)}</code></div>
        </div>
      </div>`
          )
          .join('');

  // --- LL(1) table (full) ---
  const cols = table.columns.slice().sort();
  const tableHead = `<tr><th>NT \\ lookahead</th>${cols.map((c) => `<th>${esc(c)}</th>`).join('')}</tr>`;
  const tableRows = [...g.nonterminals]
    .map((nt) => {
      const cells = cols
        .map((c) => {
          const ps = table.cell(nt, c);
          const txt = ps.length === 0 ? '·' : ps.length === 1 ? String(ps[0].index) : '!';
          const cls = ps.length > 1 ? 'll1-conflict' : ps.length === 1 ? 'll1-filled' : 'll1-empty';
          return `<td class="${cls}" title="${ps.map(productionToString).map(esc).join(' | ')}">${txt}</td>`;
        })
        .join('');
      return `<tr><td class="tok-ident">${esc(nt)}</td>${cells}</tr>`;
    })
    .join('');

  const indexed = g.productions.map((p) => `<div class="prod-line"><span class="tok-num">(${p.index})</span> ${esc(productionToString(p))}</div>`).join('');

  return `
    <div class="theory-block">
      <h3>ε (epsilon) and NULLABLE</h3>
      <p class="theory-p"><code>ε</code> is the empty string &mdash; a production <code>A -&gt; ε</code> means "A can match zero tokens." NULLABLE is the smallest set of nonterminals that can vanish entirely: A is nullable if <code>A -&gt; ε</code> directly, or if every symbol in some <code>A -&gt; X1 X2 .. Xk</code> is itself nullable. That's a <strong>least fixed point</strong>: start with the empty set, add anything justified by the current set, repeat until a full pass adds nothing. Below is every round on the real grammar &mdash; watch nonterminals join as their dependencies become nullable first.</p>
      <div class="fixpoint-list">${nullableRounds}</div>
      <div class="hint">Final NULLABLE set: ${esc(firstSetStr(fs.nullable))}</div>
    </div>

    <div class="theory-block">
      <h3>FIRST sets</h3>
      <p class="theory-p">FIRST(X) is every terminal that can legally start a string derived from X (plus <code>ε</code> if X is nullable). Same fixed-point shape as NULLABLE, but over sets of terminals instead of a yes/no flag. A predictive parser uses FIRST to decide, on seeing the next token, which production to expand.</p>
      <table class="token-table set-table"><thead><tr><th>Symbol</th><th>FIRST</th></tr></thead><tbody>${firstRows}</tbody></table>
    </div>

    <div class="theory-block">
      <h3>FOLLOW sets</h3>
      <p class="theory-p">FOLLOW(A) is every terminal that can appear immediately after A in some valid derivation, plus <code>$</code> (end of input) if A can be the very last thing. Exactly three rules build it: (1) <code>$ &isin; FOLLOW(start)</code>; (2) for <code>B -&gt; &alpha; A &beta;</code>, everything in <code>FIRST(&beta;)</code> except <code>ε</code> is in <code>FOLLOW(A)</code>; (3) if that <code>&beta;</code> is nullable (or empty), <code>FOLLOW(B) &sube; FOLLOW(A)</code> too &mdash; the rule everyone forgets, and the one that makes FOLLOW circular enough to need its own fixed point.</p>
      <table class="token-table set-table"><thead><tr><th>Nonterminal</th><th>FOLLOW</th></tr></thead><tbody>${followRows}</tbody></table>
    </div>

    <div class="theory-block">
      <h3>The predictive parsing condition</h3>
      <p class="theory-p">A predictive (LL(1)) parser picks <code>A -&gt; &alpha;</code> the instant it sees a lookahead token in <code>PREDICT(A -&gt; &alpha;)</code>: that's <code>FIRST(&alpha;)</code> when &alpha; isn't nullable, or <code>(FIRST(&alpha;) \\ {ε}) &cup; FOLLOW(A)</code> when it is &mdash; because choosing &alpha; might consume nothing, so whatever can follow A also has to predict this alternative. A grammar is LL(1) exactly when every nonterminal's PREDICT sets, across its own alternatives, are pairwise disjoint. Checking FIRST alone would call the dangling-else grammar LL(1), which it is not &mdash; that's exactly why the nullable case pulls in FOLLOW.</p>
      <p class="theory-p">Running this check on the real a-finity grammar below finds <strong>${conflicts.length}</strong> conflict${conflicts.length === 1 ? '' : 's'} &mdash; and they're the same two spots the real hand-written parser has to work around instead of predicting purely by lookahead:</p>
      <div class="conflict-list">${conflictBlock}</div>
      <div class="theory-block-note">
        <p><strong>elseTail (dangling else):</strong> given <code>if (c1) if (c2) s1 else s2</code>, the grammar alone can't tell whether that <code>else</code> belongs to the inner or outer <code>if</code> &mdash; both <code>elseTail -&gt; else stmt</code> and <code>elseTail -&gt; ε</code> predict on <code>else</code>. <code>parseStmt()</code> resolves it the standard way, matching real C: greedily bind every <code>else</code> to the nearest unmatched <code>if</code>, by just checking for <code>KW_ELSE</code> immediately rather than deciding via lookahead sets.</p>
        <p><strong>assign (IDENT):</strong> both <code>assign -&gt; IDENT = assign</code> and <code>assign -&gt; or_expr</code> can start with <code>IDENT</code> (an assignment target, or just a variable read). One token of lookahead can't disambiguate <code>x = 1</code> from <code>x + 1</code>. <code>parseAssign()</code> doesn't try to predict &mdash; it saves the lexer state, consumes the identifier, peeks one more token for <code>=</code>, and restores the checkpoint if it guessed wrong. That's genuine backtracking, deliberately reached for at the one point in this grammar where prediction can't work.</p>
      </div>
    </div>

    <div class="theory-block">
      <h3>The LL(1) parsing table</h3>
      <p class="theory-p"><code>M[A][t]</code> is the production to apply when expanding nonterminal A with lookahead t &mdash; built by placing production <code>p</code> into <code>M[p.lhs][t]</code> for every <code>t</code> in <code>PREDICT(p)</code>. A cell holding two productions (marked <code>!</code> below) is a conflict cell. This is the literal table a table-driven parser would push onto a stack and consult; a-finity's own parser reaches the same decisions via if/else chains and recursive calls instead of an explicit table &mdash; same predictions, different mechanism (${g.productions.length} productions, ${g.nonterminals.size} nonterminals, ${cols.length} terminal columns).</p>
      <div class="scroll-panel table-scroll">
        <table class="token-table ll1-table"><thead>${tableHead}</thead><tbody>${tableRows}</tbody></table>
      </div>
      <details class="prod-index">
        <summary>Production index (referenced by number above)</summary>
        <div class="prod-index-body">${indexed}</div>
      </details>
    </div>
  `;
}

export function isLL1(): boolean {
  return analysis().pr.isLL1();
}

interface DocLink {
  title: string;
  description: string;
  url: string;
}

const DOCS: DocLink[] = [
  {
    title: 'LL parser (Wikipedia)',
    description: 'The formal definition of LL(1) parsing, FIRST/FOLLOW sets, and the predictive parsing table this whole tab implements.',
    url: 'https://en.wikipedia.org/wiki/LL_parser',
  },
  {
    title: 'Recursive descent parser (Wikipedia)',
    description: 'What a-finity’s parser.ts actually is: one function per nonterminal, each a direct transcription of that rule’s right-hand side.',
    url: 'https://en.wikipedia.org/wiki/Recursive_descent_parser',
  },
  {
    title: 'Dangling else (Wikipedia)',
    description: 'The classic LL(1) conflict this grammar’s elseTail rule reproduces exactly, and how real languages resolve it by convention, not grammar rewriting.',
    url: 'https://en.wikipedia.org/wiki/Dangling_else',
  },
  {
    title: 'Extended Backus–Naur form (Wikipedia)',
    description: 'EBNF notation and the ISO/IEC 14977 standard, the *, ?, and ( ) sugar the Grammar tab desugars away.',
    url: 'https://en.wikipedia.org/wiki/Extended_Backus%E2%80%93Naur_form',
  },
  {
    title: 'Compilers: Principles, Techniques, and Tools (Wikipedia)',
    description: 'The "Dragon Book", the standard reference for everything on this tab (FIRST/FOLLOW construction, LL(1) tables, predictive parsing).',
    url: 'https://en.wikipedia.org/wiki/Compilers:_Principles,_Techniques,_and_Tools',
  },
  {
    title: 'ANTLR',
    description: 'A real, production-grade parser generator that builds exactly this kind of table (and more) from an EBNF-style grammar file automatically.',
    url: 'https://www.antlr.org/',
  },
];

export function renderDocsTab(): string {
  return DOCS.map(
    (d) => `<div class="doc-card">
      <span class="doc-card-icon">\u{1F4C4}</span>
      <div class="doc-card-body">
        <strong>${esc(d.title)}</strong>, ${esc(d.description)}<br>
        <a href="${d.url}" target="_blank" rel="noopener noreferrer">${d.url}</a>
      </div>
    </div>`
  ).join('');
}
