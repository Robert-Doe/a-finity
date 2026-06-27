int factorial(int n) {
    int result;
    result = 1;
    while (n > 1) {
        result = result * n;
        n = n - 1;
    }
    return result;
}

int main(void) {
    return factorial(5);
}
