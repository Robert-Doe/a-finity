int dot(int a, int b, int c, int d) {
    return a * c + b * d;
}

int main(void) {
    int x;
    int y;
    x = dot(2, 3, 4, 5);
    y = dot(1, 1, 6, 6);
    return x + y;
}
