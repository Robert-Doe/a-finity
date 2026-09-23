/*
 * ast.ts, AST node shapes, ported from module_16's ast.h.
 *
 * The C version uses a tagged union (NodeKind + a big union). TypeScript
 * gets to express that natively as a discriminated union, but every kind
 * and every field name below is taken directly from ast.h, nothing added,
 * nothing renamed.
 */

export type NodeKind =
  | 'INT_LIT'
  | 'IDENT'
  | 'BINOP'
  | 'UNOP'
  | 'ASSIGN'
  | 'CALL'
  | 'RETURN'
  | 'IF'
  | 'WHILE'
  | 'BLOCK'
  | 'EXPR_STMT'
  | 'VAR_DECL'
  | 'PRINT'
  | 'FUNC'
  | 'PROGRAM';

export interface ASTNodeBase {
  kind: NodeKind;
  line: number;
}

export interface IntLitNode extends ASTNodeBase {
  kind: 'INT_LIT';
  ival: number;
}

export interface IdentNode extends ASTNodeBase {
  kind: 'IDENT';
  name: string;
}

export interface BinOpNode extends ASTNodeBase {
  kind: 'BINOP';
  op: string; // operator lexeme, e.g. "+", "=="
  left: ASTNode;
  right: ASTNode;
}

export interface UnOpNode extends ASTNodeBase {
  kind: 'UNOP';
  op: string;
  left: ASTNode; // the operand (named "left" to mirror ast.h's binop union reuse)
}

export interface AssignNode extends ASTNodeBase {
  kind: 'ASSIGN';
  name: string;
  value: ASTNode;
}

export interface CallNode extends ASTNodeBase {
  kind: 'CALL';
  name: string;
  args: ASTNode[];
}

export interface ReturnNode extends ASTNodeBase {
  kind: 'RETURN';
  expr: ASTNode | null;
}

export interface IfNode extends ASTNodeBase {
  kind: 'IF';
  cond: ASTNode;
  thenBranch: ASTNode;
  elseBranch: ASTNode | null;
}

export interface WhileNode extends ASTNodeBase {
  kind: 'WHILE';
  cond: ASTNode;
  body: ASTNode;
}

export interface BlockNode extends ASTNodeBase {
  kind: 'BLOCK';
  stmts: ASTNode[];
}

export interface ExprStmtNode extends ASTNodeBase {
  kind: 'EXPR_STMT';
  expr: ASTNode;
}

export interface VarDeclNode extends ASTNodeBase {
  kind: 'VAR_DECL';
  name: string;
}

export interface PrintNode extends ASTNodeBase {
  kind: 'PRINT';
  expr: ASTNode;
}

export interface FuncNode extends ASTNodeBase {
  kind: 'FUNC';
  name: string;
  params: string[];
  body: ASTNode;
}

export interface ProgramNode extends ASTNodeBase {
  kind: 'PROGRAM';
  funcs: ASTNode[];
}

export type ASTNode =
  | IntLitNode
  | IdentNode
  | BinOpNode
  | UnOpNode
  | AssignNode
  | CallNode
  | ReturnNode
  | IfNode
  | WhileNode
  | BlockNode
  | ExprStmtNode
  | VarDeclNode
  | PrintNode
  | FuncNode
  | ProgramNode;
