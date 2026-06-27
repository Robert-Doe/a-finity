/* sample.c — Demo program for Module 09: IR Optimization
 *
 * This program is designed to show all three optimization passes at work:
 *
 *   Constant folding:
 *     a = 3 + 4   becomes   a = 7    (computed at compile time)
 *     b = 2 * 5   becomes   b = 10   (computed at compile time)
 *
 *   Copy propagation:
 *     After folding, the temporaries holding 7 and 10 feed directly into
 *     the STORE instructions, eliminating redundant copies.
 *
 *   Dead-code elimination:
 *     Not visibly triggered by this program (no unconditional jumps are
 *     generated for this straight-line code), but the pass runs harmlessly.
 *
 * NOTE: c = square(a) cannot be folded even after a is known to be 7,
 * because function calls are opaque — the optimizer does not inline
 * functions in this module.
 */

int square(int x) {
    return x * x;
}

int main(void) {
    int a;
    int b;
    int c;
    a = 3 + 4;
    b = 2 * 5;
    c = square(a);
    return c + b;
}
