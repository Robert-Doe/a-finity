int add(int a, int b) {
    return a + b;
}

int main(void) {
    int i;
    int sum;
    i = 1;
    sum = 0;
    while (i < 6) {
        sum = add(sum, i);
        i = i + 1;
    }
    return sum;
}
