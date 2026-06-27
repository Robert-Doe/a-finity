/* sample.c — test input for the Module 02 lexer demo */
int add(int a, int b) {
    return a + b;
}

int main(void) {
    int result;
    result = add(3, 4);
    if (result == 7) {
        return 0;
    }
    return 1;
}
