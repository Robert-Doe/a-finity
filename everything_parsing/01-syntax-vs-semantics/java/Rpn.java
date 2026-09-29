import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Deque;
import java.util.List;

/**
 * Module 01 — Syntax vs. Semantics.
 *
 * A tiny language: reverse Polish (postfix) integer arithmetic.
 *   token  = INTEGER | OPERATOR
 *   INTEGER  = digit {digit}          (no sign; negatives arise from '-' the operator)
 *   OPERATOR = "+" | "-" | "*" | "/"
 *
 * Two independent checkers over the SAME token list:
 *
 *   checkSyntax  — is the string well formed? Tracks only the *depth* of the
 *                  operand stack, never a value. Answers "could this be
 *                  evaluated at all?"
 *
 *   evaluate     — run it for real on an integer stack. The only thing that can
 *                  go wrong that syntax did not already catch is division by
 *                  zero: "12 0 /" is perfectly well formed and still meaningless.
 *
 * That gap between the two answers is the entire point of the module.
 */
public final class Rpn {
    private Rpn() {}

    // ─────────────────────────────────────────── lexical layer

    /** A raw lexeme plus its 1-based start column in the source line. */
    record Token(String text, int col) {}

    static List<Token> tokenize(String line) {
        List<Token> out = new ArrayList<>();
        int i = 0, n = line.length();
        while (i < n) {
            char c = line.charAt(i);
            if (c == ' ' || c == '\t') { i++; continue; }
            int start = i;
            while (i < n && line.charAt(i) != ' ' && line.charAt(i) != '\t') i++;
            out.add(new Token(line.substring(start, i), start + 1)); // +1: columns are 1-based
        }
        return out;
    }

    static boolean isInteger(String t) {
        if (t.isEmpty()) return false;
        for (int k = 0; k < t.length(); k++) {
            char c = t.charAt(k);
            if (c < '0' || c > '9') return false;
        }
        return true;
    }

    static boolean isOperator(String t) {
        return t.equals("+") || t.equals("-") || t.equals("*") || t.equals("/");
    }

    // ─────────────────────────────────────────── syntax layer (well-formedness only)

    record SyntaxResult(boolean ok, String message, int col) {
        static SyntaxResult pass()                 { return new SyntaxResult(true, null, 0); }
        static SyntaxResult fail(String m, int col) { return new SyntaxResult(false, m, col); }
    }

    /**
     * Well-formed iff, simulating the operand stack's DEPTH:
     *   - every lexeme is an INTEGER or an OPERATOR,
     *   - every operator sees depth >= 2 when it runs,
     *   - the final depth is exactly 1.
     * No integer is ever parsed; no arithmetic is ever done.
     */
    static SyntaxResult checkSyntax(List<Token> tokens) {
        if (tokens.isEmpty()) return SyntaxResult.fail("empty expression", 0);

        int depth = 0;
        for (Token tok : tokens) {
            String t = tok.text();
            if (isInteger(t)) {
                depth += 1;
            } else if (isOperator(t)) {
                if (depth < 2) {
                    return SyntaxResult.fail(
                        "operator '" + t + "' needs 2 operands, stack has " + depth, tok.col());
                }
                depth -= 1; // pop 2 operands, push 1 result
            } else {
                return SyntaxResult.fail("unknown token '" + t + "'", tok.col());
            }
        }
        if (depth != 1) {
            return SyntaxResult.fail(depth + " values left on stack, expected 1", 0);
        }
        return SyntaxResult.pass();
    }

    // ─────────────────────────────────────────── semantic layer (real evaluation)

    record SemanticResult(boolean ok, long value, String message, int col) {
        static SemanticResult of(long v)            { return new SemanticResult(true, v, null, 0); }
        static SemanticResult err(String m, int col) { return new SemanticResult(false, 0, m, col); }
    }

    /** Precondition: checkSyntax(tokens).ok() is true. */
    static SemanticResult evaluate(List<Token> tokens) {
        Deque<Long> stack = new ArrayDeque<>();
        for (Token tok : tokens) {
            String t = tok.text();
            if (isInteger(t)) {
                stack.push(Long.parseLong(t));
                continue;
            }
            long b = stack.pop();
            long a = stack.pop();
            long r;
            switch (t) {
                case "+": r = a + b; break;
                case "-": r = a - b; break;
                case "*": r = a * b; break;
                case "/":
                    if (b == 0) return SemanticResult.err("division by zero", tok.col());
                    r = a / b; // Java long division truncates toward zero
                    break;
                default: throw new IllegalStateException("not an operator: " + t);
            }
            stack.push(r);
        }
        return SemanticResult.of(stack.pop());
    }
}
