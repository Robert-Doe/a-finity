/*
 * parser.ts — TypeScript port of a-finity's real recursive-descent parser.
 *
 * Ported directly from module_16 - The Complete Compiler/parser.c.
 * Grammar (verbatim from the header comment in parser.c):
 *
 *   program    := func*
 *   func       := type IDENT '(' params ')' block
 *   params     := ε | type IDENT (',' type IDENT)*
 *   block      := '{' stmt* '}'
 *   stmt       := var_decl | return_stmt | if_stmt | while_stmt
 *               | print_stmt | expr_stmt
 *   expr_stmt  := expr ';'
 *   expr       := assign
 *   assign     := IDENT '=' assign | or_expr
 *   or_expr    := and_expr ('||' and_expr)*
 *   and_expr   := eq_expr  ('&&' eq_expr)*
 *   eq_expr    := rel_expr (('=='|'!=') rel_expr)*
 *   rel_expr   := add_expr (('<'|'<='|'>'|'>=') add_expr)*
 *   add_expr   := mul_expr (('+' | '-') mul_expr)*
 *   mul_expr   := unary   (('*'|'/'|'%') unary)*
 *   unary      := '-' unary | '!' unary | primary
 *   primary    := INT_LIT | IDENT | IDENT '(' args ')' | '(' expr ')'
 *
 * Every parse_* function below mirrors its C counterpart function-for-
 * function, including the two-token-lookahead trick parse_assign() uses
 * (save lexer + current token, try IDENT '=', restore if it doesn't match).
 */

import { Lexer, Token, TokenType, TOKEN_TYPE_NAME } from './lexer';
import type { ASTNode } from './ast';

export interface ParseError {
  message: string;
  line: number;
  col: number;
}

export class Parser {
  private lex: Lexer;
  private cur: Token;
  readonly errors: ParseError[] = [];

  constructor(lex: Lexer) {
    this.lex = lex;
    this.cur = this.lex.next();
  }

  private error(msg: string): void {
    this.errors.push({ message: msg, line: this.cur.line, col: this.cur.col });
  }

  private advance(): Token {
    const prev = this.cur;
    this.cur = this.lex.next();
    return prev;
  }

  private check(t: TokenType): boolean {
    return this.cur.type === t;
  }

  private match(t: TokenType): boolean {
    if (!this.check(t)) return false;
    this.advance();
    return true;
  }

  private expect(t: TokenType): Token {
    if (!this.check(t)) {
      this.error(`expected '${TOKEN_TYPE_NAME[t]}' but got '${this.cur.lexeme || TOKEN_TYPE_NAME[this.cur.type]}'`);
    }
    return this.advance();
  }

  // ----------------------------------------------------------------- expressions

  private parsePrimary(): ASTNode {
    const line = this.cur.line;

    if (this.check(TokenType.INT_LIT)) {
      const t = this.advance();
      return { kind: 'INT_LIT', line, ival: t.ival };
    }

    if (this.check(TokenType.IDENT)) {
      const t = this.advance();
      // function call?
      if (this.check(TokenType.LPAREN)) {
        this.advance(); // eat (
        const args: ASTNode[] = [];
        if (!this.check(TokenType.RPAREN)) {
          do {
            args.push(this.parseExpr());
          } while (this.match(TokenType.COMMA));
        }
        this.expect(TokenType.RPAREN);
        return { kind: 'CALL', line, name: t.lexeme, args };
      }
      // plain identifier
      return { kind: 'IDENT', line, name: t.lexeme };
    }

    if (this.match(TokenType.LPAREN)) {
      const inner = this.parseExpr();
      this.expect(TokenType.RPAREN);
      return inner;
    }

    this.error('expected expression');
    // dummy node so parsing can continue, matching parser.c's recovery
    return { kind: 'INT_LIT', line, ival: 0 };
  }

  private parseUnary(): ASTNode {
    const line = this.cur.line;
    if (this.check(TokenType.MINUS) || this.check(TokenType.NOT)) {
      const op = this.advance();
      return { kind: 'UNOP', line, op: op.lexeme, left: this.parseUnary() };
    }
    return this.parsePrimary();
  }

  private parseMul(): ASTNode {
    let left = this.parseUnary();
    while (this.check(TokenType.STAR) || this.check(TokenType.SLASH) || this.check(TokenType.PERCENT)) {
      const op = this.advance();
      left = { kind: 'BINOP', line: left.line, op: op.lexeme, left, right: this.parseUnary() };
    }
    return left;
  }

  private parseAdd(): ASTNode {
    let left = this.parseMul();
    while (this.check(TokenType.PLUS) || this.check(TokenType.MINUS)) {
      const op = this.advance();
      left = { kind: 'BINOP', line: left.line, op: op.lexeme, left, right: this.parseMul() };
    }
    return left;
  }

  private parseRel(): ASTNode {
    let left = this.parseAdd();
    while (
      this.check(TokenType.LT) || this.check(TokenType.LE) ||
      this.check(TokenType.GT) || this.check(TokenType.GE)
    ) {
      const op = this.advance();
      left = { kind: 'BINOP', line: left.line, op: op.lexeme, left, right: this.parseAdd() };
    }
    return left;
  }

  private parseEq(): ASTNode {
    let left = this.parseRel();
    while (this.check(TokenType.EQ) || this.check(TokenType.NEQ)) {
      const op = this.advance();
      left = { kind: 'BINOP', line: left.line, op: op.lexeme, left, right: this.parseRel() };
    }
    return left;
  }

  private parseAnd(): ASTNode {
    let left = this.parseEq();
    while (this.check(TokenType.AND)) {
      const op = this.advance();
      left = { kind: 'BINOP', line: left.line, op: op.lexeme, left, right: this.parseEq() };
    }
    return left;
  }

  private parseOr(): ASTNode {
    let left = this.parseAnd();
    while (this.check(TokenType.OR)) {
      const op = this.advance();
      left = { kind: 'BINOP', line: left.line, op: op.lexeme, left, right: this.parseAnd() };
    }
    return left;
  }

  private parseAssign(): ASTNode {
    // Peek: if IDENT followed by '=', it's assignment.
    if (this.check(TokenType.IDENT)) {
      const savedLex = this.lex.saveState();
      const savedCur = this.cur;

      const ident = this.advance();
      if (this.check(TokenType.ASSIGN)) {
        this.advance(); // eat =
        return { kind: 'ASSIGN', line: ident.line, name: ident.lexeme, value: this.parseAssign() };
      }
      // Not assignment — restore and parse as or_expr.
      this.lex.restoreState(savedLex);
      this.cur = savedCur;
    }
    return this.parseOr();
  }

  private parseExpr(): ASTNode {
    return this.parseAssign();
  }

  // ----------------------------------------------------------------- statements

  private parseStmt(): ASTNode {
    const line = this.cur.line;

    // var declaration: int name;
    if (this.check(TokenType.KW_INT)) {
      this.advance();
      const name = this.expect(TokenType.IDENT);
      this.expect(TokenType.SEMICOLON);
      return { kind: 'VAR_DECL', line, name: name.lexeme };
    }

    // return
    if (this.match(TokenType.KW_RETURN)) {
      if (this.check(TokenType.SEMICOLON)) {
        this.advance();
        return { kind: 'RETURN', line, expr: null };
      }
      const expr = this.parseExpr();
      this.expect(TokenType.SEMICOLON);
      return { kind: 'RETURN', line, expr };
    }

    // if
    if (this.match(TokenType.KW_IF)) {
      this.expect(TokenType.LPAREN);
      const cond = this.parseExpr();
      this.expect(TokenType.RPAREN);
      const thenBranch = this.parseStmt();
      let elseBranch: ASTNode | null = null;
      if (this.match(TokenType.KW_ELSE)) {
        elseBranch = this.parseStmt();
      }
      return { kind: 'IF', line, cond, thenBranch, elseBranch };
    }

    // while
    if (this.match(TokenType.KW_WHILE)) {
      this.expect(TokenType.LPAREN);
      const cond = this.parseExpr();
      this.expect(TokenType.RPAREN);
      const body = this.parseStmt();
      return { kind: 'WHILE', line, cond, body };
    }

    // print(expr);
    if (this.match(TokenType.KW_PRINT)) {
      this.expect(TokenType.LPAREN);
      const val = this.parseExpr();
      this.expect(TokenType.RPAREN);
      this.expect(TokenType.SEMICOLON);
      return { kind: 'PRINT', line, expr: val };
    }

    // block
    if (this.check(TokenType.LBRACE)) {
      return this.parseBlock();
    }

    // expression statement
    const expr = this.parseExpr();
    this.expect(TokenType.SEMICOLON);
    return { kind: 'EXPR_STMT', line, expr };
  }

  private parseBlock(): ASTNode {
    const line = this.cur.line;
    this.expect(TokenType.LBRACE);
    const stmts: ASTNode[] = [];
    let guard = 0;
    while (!this.check(TokenType.RBRACE) && !this.check(TokenType.EOF)) {
      stmts.push(this.parseStmt());
      if (++guard > 100000) break; // safety valve
    }
    this.expect(TokenType.RBRACE);
    return { kind: 'BLOCK', line, stmts };
  }

  // ----------------------------------------------------------------- functions / program

  private isType(): boolean {
    return this.check(TokenType.KW_INT) || this.check(TokenType.KW_VOID);
  }

  private parseFunc(): ASTNode | null {
    const line = this.cur.line;
    if (!this.isType()) {
      this.error('expected type');
      return null;
    }
    this.advance();

    const name = this.expect(TokenType.IDENT);
    this.expect(TokenType.LPAREN);

    const params: string[] = [];
    if (!this.check(TokenType.RPAREN)) {
      do {
        if (!this.isType()) {
          this.error('expected parameter type');
          break;
        }
        this.advance(); // eat type
        const pname = this.expect(TokenType.IDENT);
        params.push(pname.lexeme);
      } while (this.match(TokenType.COMMA));
    }
    this.expect(TokenType.RPAREN);

    const body = this.parseBlock();

    return { kind: 'FUNC', line, name: name.lexeme, params, body };
  }

  parseProgram(): ASTNode {
    const funcs: ASTNode[] = [];
    while (!this.check(TokenType.EOF)) {
      const fn = this.parseFunc();
      if (!fn) break;
      funcs.push(fn);
    }
    return { kind: 'PROGRAM', line: 1, funcs };
  }
}
