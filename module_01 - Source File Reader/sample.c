/* sample.c — minimal C snippet used as test input for the Module 01 demo.
 * Feed this to ./reader to confirm the reader works before moving on. */

int add(int a, int b) {
    return a + b;   /* trivial addition so the preview is recognisable */
}

int main(void) {
    int result = add(3, 4);
    return result;
}
