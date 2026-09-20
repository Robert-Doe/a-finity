/**
 * The real a-finity grammar, in two forms, plus traceability back to the
 * actual recursive-descent parser (webapp/src/parser.ts, itself a port of
 * module_16's parser.c).
 *
 * EBNF_TEXT is the canonical, human-facing grammar — verbatim from
 * parser.ts's own header comment, completed with the statement
 * sub-productions it only named ("stmt := var_decl | ...").
 *
 * BNF_TEXT is EBNF_TEXT desugared into plain BNF (no *, ?, or grouping) by
 * hand, following the exact desugaring rules taught in
 * everything_parsing/08-bnf-ebnf-abnf: `X*` becomes a fresh right-recursive
 * `Xrep -> X Xrep | epsilon`, `X?` becomes `Xopt -> X | epsilon`. Plain BNF
 * is what Grammar.parse (ported from Module 21) can consume — the
 * nullable/FIRST/FOLLOW/predict machinery only operates on bare productions,
 * never on sugar.
 *
 * One deliberate substitution: `||` is written as the terminal `OR`, not the
 * literal characters. Grammar.parse's own alternative-separator is a bare
 * `|`, so a literal `||` inside a rule's right-hand side gets shredded by
 * `rhsWithBars.split("|")` into spurious empty alternatives — confirmed by
 * running this exact grammar through it (see scripts/validate-grammar.mjs
 * history) before this fix, `&&` is safe since it contains no `|`.
 */

export const EBNF_TEXT = `program     := func*
func        := type IDENT '(' params ')' block
type        := 'int' | 'void'
params      := ε | type IDENT (',' type IDENT)*
block       := '{' stmt* '}'
stmt        := var_decl | return_stmt | if_stmt | while_stmt
             | print_stmt | block | expr_stmt
var_decl    := 'int' IDENT ';'
return_stmt := 'return' expr? ';'
if_stmt     := 'if' '(' expr ')' stmt ('else' stmt)?
while_stmt  := 'while' '(' expr ')' stmt
print_stmt  := 'print' '(' expr ')' ';'
expr_stmt   := expr ';'
expr        := assign
assign      := IDENT '=' assign | or_expr
or_expr     := and_expr ('||' and_expr)*
and_expr    := eq_expr ('&&' eq_expr)*
eq_expr     := rel_expr (('=='|'!=') rel_expr)*
rel_expr    := add_expr (('<'|'<='|'>'|'>=') add_expr)*
add_expr    := mul_expr (('+'|'-') mul_expr)*
mul_expr    := unary (('*'|'/'|'%') unary)*
unary       := ('-'|'!') unary | primary
primary     := INT_LIT | IDENT ('(' args ')')? | '(' expr ')'
args        := ε | expr (',' expr)*`;

export const BNF_TEXT = `program -> funcList
funcList -> func funcList | epsilon
func -> type IDENT ( params ) block
type -> int | void
params -> epsilon | type IDENT paramTail
paramTail -> , type IDENT paramTail | epsilon
block -> { stmtList }
stmtList -> stmt stmtList | epsilon
stmt -> var_decl | return_stmt | if_stmt | while_stmt | print_stmt | block | expr_stmt
var_decl -> int IDENT ;
return_stmt -> return retTail
retTail -> ; | expr ;
if_stmt -> if ( expr ) stmt elseTail
elseTail -> else stmt | epsilon
while_stmt -> while ( expr ) stmt
print_stmt -> print ( expr ) ;
expr_stmt -> expr ;
expr -> assign
assign -> IDENT = assign | or_expr
or_expr -> and_expr orTail
orTail -> OR and_expr orTail | epsilon
and_expr -> eq_expr andTail
andTail -> && eq_expr andTail | epsilon
eq_expr -> rel_expr eqTail
eqTail -> == rel_expr eqTail | != rel_expr eqTail | epsilon
rel_expr -> add_expr relTail
relTail -> < add_expr relTail | <= add_expr relTail | > add_expr relTail | >= add_expr relTail | epsilon
add_expr -> mul_expr addTail
addTail -> + mul_expr addTail | - mul_expr addTail | epsilon
mul_expr -> unary mulTail
mulTail -> * unary mulTail | / unary mulTail | % unary mulTail | epsilon
unary -> - unary | ! unary | primary
primary -> INT_LIT | IDENT callTail | ( expr )
callTail -> ( args ) | epsilon
args -> epsilon | expr argTail
argTail -> , expr argTail | epsilon`;

export interface RuleTrace {
  rule: string;
  parserFn: string | null;
  note?: string;
}

/** EBNF-level rule -> the exact parser.ts function that implements it. */
export const RULE_TRACE: RuleTrace[] = [
  { rule: "program", parserFn: "parseProgram()" },
  { rule: "func", parserFn: "parseFunc()" },
  { rule: "params", parserFn: "parseFunc()", note: "the comma-separated loop inside parseFunc" },
  { rule: "block", parserFn: "parseBlock()" },
  { rule: "stmt", parserFn: "parseStmt()" },
  { rule: "var_decl / return_stmt / if_stmt / while_stmt / print_stmt", parserFn: "parseStmt()", note: "one if-branch per alternative, chosen by cur.type — this is FIRST-set dispatch in disguise" },
  { rule: "expr_stmt", parserFn: "parseStmt()", note: "the fall-through branch" },
  { rule: "expr", parserFn: "parseExpr()" },
  { rule: "assign", parserFn: "parseAssign()", note: "NOT predictive — saves/restores lexer state instead of choosing by lookahead alone (see the conflict below)" },
  { rule: "or_expr", parserFn: "parseOr()" },
  { rule: "and_expr", parserFn: "parseAnd()" },
  { rule: "eq_expr", parserFn: "parseEq()" },
  { rule: "rel_expr", parserFn: "parseRel()" },
  { rule: "add_expr", parserFn: "parseAdd()" },
  { rule: "mul_expr", parserFn: "parseMul()" },
  { rule: "unary", parserFn: "parseUnary()" },
  { rule: "primary", parserFn: "parsePrimary()" },
  { rule: "args", parserFn: "parsePrimary()", note: "the comma-separated loop inside the call-parsing branch" },
];
