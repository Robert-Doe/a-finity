int fib(int n) {
    int a;
    int b;
    int tmp;
    a = 0;
    b = 1;
    while (n > 0) {
        tmp = a + b;
        a = b;
        b = tmp;
        n = n - 1;
    }
    return a;
}

int main(void) {
    return fib(10);
}
