int add(int a, int b) {
    return a + b;
}

int main(void) {
    int x;
    int y;
    x = 10;
    y = 0;
    while (x > 0) {
        y = y + x;
        x = x - 1;
    }
    if (y > 40) {
        return y;
    } else {
        return 0;
    }
}
