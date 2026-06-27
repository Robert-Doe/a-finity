/*
 * sample.c — Module 11 demonstration program
 *
 * Shows if/else and while control flow working together.
 *
 * Expected result:
 *   abs_val(-5) = 5
 *   sum_to(10)  = 1+2+3+4+5+6+7+8+9 = 45  (i goes 1..9, condition i<10)
 *   return a + b = 5 + 45 = 50
 *   exit code: 50
 */

int abs_val(int x) {
    if (x < 0) {
        return 0 - x;
    } else {
        return x;
    }
}

int sum_to(int n) {
    int i;
    int s;
    i = 1;
    s = 0;
    while (i < n) {
        s = s + i;
        i = i + 1;
    }
    return s;
}

int main(void) {
    int a;
    int b;
    a = abs_val(0 - 5);
    b = sum_to(10);
    return a + b;
}
