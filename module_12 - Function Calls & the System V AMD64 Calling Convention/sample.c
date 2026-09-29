/* sample.c — Module 12 sample program
 *
 * Demonstrates function calls with multiple arguments using
 * the System V AMD64 calling convention.
 *
 * clamp(x, lo, hi) returns x clamped to the range [lo, hi].
 *
 * Expected exit code: clamp(-3,0,10) + clamp(15,0,10) + clamp(5,0,10)
 *                   = 0 + 10 + 5 = 15
 */

int clamp(int x, int lo, int hi) {
    if (x < lo) {
        return lo;
    }
    if (x > hi) {
        return hi;
    }
    return x;
}

int main(void) {
    int a;
    int b;
    int c;
    a = clamp(0 - 3, 0, 10);
    b = clamp(15, 0, 10);
    c = clamp(5, 0, 10);
    return a + b + c;
}
