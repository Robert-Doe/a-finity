int multiply(int a, int b) {
    return a * b;
}

int main(void) {
    int x;
    int result;
    x = 6;
    result = multiply(x, 7);
    if (result > 40) {
        return result;
    } else {
        return 0;
    }
}
