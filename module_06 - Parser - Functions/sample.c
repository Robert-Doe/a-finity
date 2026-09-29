/* sample.c — test program for Module 06 function parser */
int add(int a, int b) {
    return a + b;
}

int max(int a, int b) {
    if (a > b) {
        return a;
    }
    return b;
}

int main(void) {
    int x;
    int y;
    x = add(3, 4);
    y = max(x, 10);
    return y;
}
