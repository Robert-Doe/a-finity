/**
 * Draws the real AST (the same tree main.ts's text view renders, from the
 * real parser.ts) as an actual node-and-edge diagram, using d3-hierarchy
 * for tree layout. Emits a plain SVG string — no DOM binding, no d3-selection
 * — so it drops straight into the existing template-string rendering style.
 */
import { hierarchy, tree as d3tree, type HierarchyPointNode } from 'd3-hierarchy';
import type { ASTNode } from '../ast';

interface TreeDatum {
  label: string;
  detail: string;
  children: TreeDatum[];
}

function toTreeDatum(node: ASTNode): TreeDatum {
  const field = (name: string, child: ASTNode | null): TreeDatum[] => (child ? [{ ...toTreeDatum(child), detail: `${name}: ${toTreeDatum(child).detail}` }] : []);

  switch (node.kind) {
    case 'PROGRAM':
      return { label: 'PROGRAM', detail: '', children: node.funcs.map(toTreeDatum) };
    case 'FUNC':
      return { label: 'FUNC', detail: `${node.name}(${node.params.join(', ')})`, children: [toTreeDatum(node.body)] };
    case 'BLOCK':
      return { label: 'BLOCK', detail: `${node.stmts.length} stmt(s)`, children: node.stmts.map(toTreeDatum) };
    case 'VAR_DECL':
      return { label: 'VAR_DECL', detail: node.name, children: [] };
    case 'RETURN':
      return { label: 'RETURN', detail: '', children: node.expr ? [toTreeDatum(node.expr)] : [] };
    case 'PRINT':
      return { label: 'PRINT', detail: '', children: [toTreeDatum(node.expr)] };
    case 'IF': {
      const children = [
        { ...toTreeDatum(node.cond), label: `cond: ${toTreeDatum(node.cond).label}` },
        { ...toTreeDatum(node.thenBranch), label: `then: ${toTreeDatum(node.thenBranch).label}` },
      ];
      if (node.elseBranch) children.push({ ...toTreeDatum(node.elseBranch), label: `else: ${toTreeDatum(node.elseBranch).label}` });
      return { label: 'IF', detail: '', children };
    }
    case 'WHILE':
      return {
        label: 'WHILE',
        detail: '',
        children: [
          { ...toTreeDatum(node.cond), label: `cond: ${toTreeDatum(node.cond).label}` },
          { ...toTreeDatum(node.body), label: `body: ${toTreeDatum(node.body).label}` },
        ],
      };
    case 'EXPR_STMT':
      return { label: 'EXPR_STMT', detail: '', children: [toTreeDatum(node.expr)] };
    case 'ASSIGN':
      return { label: 'ASSIGN', detail: `${node.name} =`, children: [toTreeDatum(node.value)] };
    case 'BINOP':
      return { label: 'BINOP', detail: node.op, children: [toTreeDatum(node.left), toTreeDatum(node.right)] };
    case 'UNOP':
      return { label: 'UNOP', detail: node.op, children: [toTreeDatum(node.left)] };
    case 'CALL':
      return { label: 'CALL', detail: node.name, children: node.args.map(toTreeDatum) };
    case 'IDENT':
      return { label: 'IDENT', detail: node.name, children: [] };
    case 'INT_LIT':
      return { label: 'INT_LIT', detail: String(node.ival), children: [] };
    default:
      return { label: '?', detail: '', children: [] };
  }
}

const NODE_W = 110;
const NODE_H = 46;

export function renderAstTreeSvg(ast: ASTNode): string {
  const root = hierarchy(toTreeDatum(ast), (d) => d.children);
  const layout = d3tree<TreeDatum>().nodeSize([NODE_W, NODE_H * 1.8]);
  const laidOut = layout(root);

  let minX = Infinity,
    maxX = -Infinity,
    maxY = -Infinity;
  laidOut.each((n) => {
    minX = Math.min(minX, n.x);
    maxX = Math.max(maxX, n.x);
    maxY = Math.max(maxY, n.y);
  });
  if (!isFinite(minX)) {
    minX = 0;
    maxX = 0;
    maxY = 0;
  }

  const pad = 60;
  const width = maxX - minX + pad * 2;
  const height = maxY + pad * 2;
  const ox = pad - minX;
  const oy = pad;

  const links = laidOut
    .links()
    .map((l: { source: HierarchyPointNode<TreeDatum>; target: HierarchyPointNode<TreeDatum> }) => {
      const sx = l.source.x + ox,
        sy = l.source.y + oy + NODE_H / 2;
      const tx = l.target.x + ox,
        ty = l.target.y + oy - NODE_H / 2;
      const my = (sy + ty) / 2;
      return `<path class="tree-link" d="M${sx},${sy} C${sx},${my} ${tx},${my} ${tx},${ty}"/>`;
    })
    .join('');

  const nodes = laidOut
    .descendants()
    .map((n: HierarchyPointNode<TreeDatum>) => {
      const x = n.x + ox,
        y = n.y + oy;
      const isLeaf = !n.children || n.children.length === 0;
      const label = esc(n.data.label);
      const detail = n.data.detail ? esc(n.data.detail) : '';
      return `<g class="tree-node ${isLeaf ? 'leaf' : ''}" transform="translate(${x - NODE_W / 2},${y - NODE_H / 2})">
        <rect width="${NODE_W}" height="${NODE_H}" rx="8"/>
        <text x="${NODE_W / 2}" y="${detail ? 18 : NODE_H / 2 + 4}" text-anchor="middle" class="tree-label">${label}</text>
        ${detail ? `<text x="${NODE_W / 2}" y="34" text-anchor="middle" class="tree-detail">${detail}</text>` : ''}
      </g>`;
    })
    .join('');

  return `<svg viewBox="0 0 ${width} ${height}" width="${width}" height="${height}" xmlns="http://www.w3.org/2000/svg">
    <g>${links}${nodes}</g>
  </svg>`;
}

function esc(s: string): string {
  return s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').length > 18
    ? s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').slice(0, 17) + '…'
    : s.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
}
