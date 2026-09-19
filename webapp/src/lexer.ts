/*
 * lexer.ts — TypeScript port of a-finity's real lexer.
 *
 * Ported directly from:
 *   module_16 - The Complete Compiler/token.h
 *   module_16 - The Complete Compiler/lexer.c
 *
 * This is the same hand-rolled scanner "mycc" uses: same TokenType set,
 * same keyword table, same two-character-operator disambiguation, same
 * line/col bookkeeping. Only the storage strategy changes (a JS string
 * index instead of a C pointer into a null-terminated buffer) — the
 * control flow and character classification are a 1:1 transliteration.
 */

export enum TokenType {
  // Literals & identifiers
  INT_LIT = 'INT_LIT',
  IDENT = 'IDENT',

  // Keywords
  KW_INT = 'KW_int',
  KW_RETURN = 'KW_return',
  KW_IF = 'KW_if',
  KW_ELSE = 'KW_else',
  KW_WHILE = 'KW_while',
  KW_VOID = 'KW_void',
  KW_PRINT = 'KW_print',

  // Punctuation
  LPAREN = 'LPAREN',
  RPAREN = 'RPAREN',
  LBRACE = 'LBRACE',
  RBRACE = 'RBRACE',
  SEMICOLON = 'SEMICOLON',
  COMMA = 'COMMA',

  // Arithmetic operators
  PLUS = 'PLUS',
  MINUS = 'MINUS',
  STAR = 'STAR',
  SLASH = 'SLASH',
  PERCENT = 'PERCENT',

  // Relational / logical operators
  EQ = 'EQ',
  NEQ = 'NEQ',
  LT = 'LT',
  LE = 'LE',
  GT = 'GT',
  GE = 'GE',

  // Assignment
  ASSIGN = 'ASSIGN',

  // Logical
  AND = 'AND',
  OR = 'OR',
  NOT = 'NOT',

  // Sentinels
  EOF = 'EOF',
}

/** Printable spelling for each token type — matches token_type_name() in lexer.c. */
export const TOKEN_TYPE_NAME: Record<TokenType, string> = {
  [TokenType.INT_LIT]: 'INT_LIT',
  [TokenType.IDENT]: 'IDENT',
  [TokenType.KW_INT]: 'int',
  [TokenType.KW_RETURN]: 'return',
  [TokenType.KW_IF]: 'if',
  [TokenType.KW_ELSE]: 'else',
  [TokenType.KW_WHILE]: 'while',
  [TokenType.KW_VOID]: 'void',
  [TokenType.KW_PRINT]: 'print',
  [TokenType.LPAREN]: '(',
  [TokenType.RPAREN]: ')',
  [TokenType.LBRACE]: '{',
  [TokenType.RBRACE]: '}',
  [TokenType.SEMICOLON]: ';',
  [TokenType.COMMA]: ',',
  [TokenType.PLUS]: '+',
  [TokenType.MINUS]: '-',
  [TokenType.STAR]: '*',
  [TokenType.SLASH]: '/',
  [TokenType.PERCENT]: '%',
  [TokenType.EQ]: '==',
  [TokenType.NEQ]: '!=',
  [TokenType.LT]: '<',
  [TokenType.LE]: '<=',
  [TokenType.GT]: '>',
  [TokenType.GE]: '>=',
  [TokenType.ASSIGN]: '=',
  [TokenType.AND]: '&&',
  [TokenType.OR]: '||',
  [TokenType.NOT]: '!',
  [TokenType.EOF]: 'EOF',
};

export interface Token {
  type: TokenType;
  lexeme: string;
  line: number;
  col: number;
  ival: number; // only meaningful for INT_LIT
}

interface KwEntry {
  word: string;
  type: TokenType;
}

/** Keyword table — identical rows to kw_table[] in lexer.c. */
const KW_TABLE: KwEntry[] = [
  { word: 'int', type: TokenType.KW_INT },
  { word: 'return', type: TokenType.KW_RETURN },
  { word: 'if', type: TokenType.KW_IF },
  { word: 'else', type: TokenType.KW_ELSE },
  { word: 'while', type: TokenType.KW_WHILE },
  { word: 'void', type: TokenType.KW_VOID },
  { word: 'print', type: TokenType.KW_PRINT },
];

function isDigit(c: string | undefined): boolean {
  return c !== undefined && c >= '0' && c <= '9';
}

function isAlpha(c: string | undefined): boolean {
  return c !== undefined && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'));
}

function isAlnumOrUnderscore(c: string | undefined): boolean {
  return c !== undefined && (isAlpha(c) || isDigit(c) || c === '_');
}

function isSpace(c: string | undefined): boolean {
  return c === ' ' || c === '\t' || c === '\n' || c === '\r' || c === '\f' || c === '\v';
}

/** Thrown to surface a lexer error (unexpected character) to the UI,
 * mirroring lexer.c's stderr diagnostic — except here we can't silently
 * continue with an EOF token the way the C driver does, so we collect the
 * message and let the caller decide how to display it. */
export interface LexError {
  message: string;
  line: number;
  col: number;
}

export class Lexer {
  private readonly src: string;
  private pos = 0;
  private line = 1;
  private col = 1;
  readonly errors: LexError[] = [];

  constructor(src: string) {
    this.src = src;
  }

  private peekChar(offset = 0): string | undefined {
    const i = this.pos + offset;
    return i < this.src.length ? this.src[i] : undefined;
  }

  private nextChar(): string {
    const c = this.src[this.pos++];
    if (c === '\n') {
      this.line++;
      this.col = 1;
    } else {
      this.col++;
    }
    return c;
  }

  private skipWhitespaceAndComments(): void {
    for (;;) {
      while (this.pos < this.src.length && isSpace(this.peekChar())) {
        this.nextChar();
      }

      // line comment //
      if (this.peekChar() === '/' && this.peekChar(1) === '/') {
        while (this.pos < this.src.length && this.peekChar() !== '\n') {
          this.nextChar();
        }
        continue;
      }

      // block comment /* */
      if (this.peekChar() === '/' && this.peekChar(1) === '*') {
        this.nextChar();
        this.nextChar();
        while (this.pos < this.src.length) {
          if (this.peekChar() === '*' && this.peekChar(1) === '/') {
            this.nextChar();
            this.nextChar();
            break;
          }
          this.nextChar();
        }
        continue;
      }

      break;
    }
  }

  private makeTok(type: TokenType, lexeme: string, line: number, col: number, ival = 0): Token {
    return { type, lexeme, line, col, ival };
  }

  /** Scan and return the next token — direct port of lexer_next(). */
  next(): Token {
    this.skipWhitespaceAndComments();

    if (this.pos >= this.src.length) {
      return this.makeTok(TokenType.EOF, '', this.line, this.col);
    }

    const line = this.line;
    const col = this.col;
    const start = this.pos;
    const c = this.nextChar();

    // Integer literal
    if (isDigit(c)) {
      while (this.pos < this.src.length && isDigit(this.peekChar())) this.nextChar();
      const lexeme = this.src.slice(start, this.pos);
      return this.makeTok(TokenType.INT_LIT, lexeme, line, col, parseInt(lexeme, 10));
    }

    // Identifier or keyword
    if (isAlpha(c) || c === '_') {
      while (this.pos < this.src.length && isAlnumOrUnderscore(this.peekChar())) this.nextChar();
      const lexeme = this.src.slice(start, this.pos);
      const kw = KW_TABLE.find((k) => k.word === lexeme);
      return this.makeTok(kw ? kw.type : TokenType.IDENT, lexeme, line, col);
    }

    // Two-character operators
    if (c === '=' && this.peekChar() === '=') { this.nextChar(); return this.makeTok(TokenType.EQ, '==', line, col); }
    if (c === '!' && this.peekChar() === '=') { this.nextChar(); return this.makeTok(TokenType.NEQ, '!=', line, col); }
    if (c === '<' && this.peekChar() === '=') { this.nextChar(); return this.makeTok(TokenType.LE, '<=', line, col); }
    if (c === '>' && this.peekChar() === '=') { this.nextChar(); return this.makeTok(TokenType.GE, '>=', line, col); }
    if (c === '&' && this.peekChar() === '&') { this.nextChar(); return this.makeTok(TokenType.AND, '&&', line, col); }
    if (c === '|' && this.peekChar() === '|') { this.nextChar(); return this.makeTok(TokenType.OR, '||', line, col); }

    // Single-character operators and punctuation
    switch (c) {
      case '(': return this.makeTok(TokenType.LPAREN, c, line, col);
      case ')': return this.makeTok(TokenType.RPAREN, c, line, col);
      case '{': return this.makeTok(TokenType.LBRACE, c, line, col);
      case '}': return this.makeTok(TokenType.RBRACE, c, line, col);
      case ';': return this.makeTok(TokenType.SEMICOLON, c, line, col);
      case ',': return this.makeTok(TokenType.COMMA, c, line, col);
      case '+': return this.makeTok(TokenType.PLUS, c, line, col);
      case '-': return this.makeTok(TokenType.MINUS, c, line, col);
      case '*': return this.makeTok(TokenType.STAR, c, line, col);
      case '/': return this.makeTok(TokenType.SLASH, c, line, col);
      case '%': return this.makeTok(TokenType.PERCENT, c, line, col);
      case '<': return this.makeTok(TokenType.LT, c, line, col);
      case '>': return this.makeTok(TokenType.GT, c, line, col);
      case '=': return this.makeTok(TokenType.ASSIGN, c, line, col);
      case '!': return this.makeTok(TokenType.NOT, c, line, col);
      default:
        this.errors.push({ message: `unexpected character '${c}'`, line, col });
        // Recover like the C driver: keep tokenizing from here rather than halting.
        return this.next();
    }
  }

  /** Snapshot cursor state so the parser can back out of a speculative
   * lookahead (used to disambiguate `IDENT '=' ...` from `IDENT` starting
   * an ordinary expression — see parse_assign() in parser.c, which does
   * the same thing via `Lexer saved = *p->lex;`). */
  saveState(): { pos: number; line: number; col: number } {
    return { pos: this.pos, line: this.line, col: this.col };
  }

  restoreState(s: { pos: number; line: number; col: number }): void {
    this.pos = s.pos;
    this.line = s.line;
    this.col = s.col;
  }

  /** Tokenize the entire source, stopping after EOF. Used by the UI to
   * populate the token table in one shot. */
  tokenizeAll(): Token[] {
    const out: Token[] = [];
    for (;;) {
      const t = this.next();
      out.push(t);
      if (t.type === TokenType.EOF) break;
      if (out.length > 50000) break; // safety valve against pathological input
    }
    return out;
  }
}
