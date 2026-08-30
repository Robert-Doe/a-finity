// Everything Parsing — the whole test framework, on purpose.
// Canonical copy: prerequisites/assert/Assert.java
// Each module keeps its own copy in java/ so `javac java/*.java` just works.
//
// No JUnit, no Gradle. A test file is a class with a `public static void main`
// that calls these and finishes with Assert.summary(). Non-zero exit on failure
// so `run.md` commands and CI can tell pass from fail.

public final class Assert {
    private static int passed = 0;
    private static int failed = 0;

    private Assert() {}

    public static void that(boolean cond, String label) {
        if (cond) {
            passed++;
        } else {
            failed++;
            System.out.println("  FAIL  " + label);
        }
    }

    public static void equals(Object actual, Object expected, String label) {
        boolean ok = (actual == null) ? expected == null : actual.equals(expected);
        if (ok) {
            passed++;
        } else {
            failed++;
            System.out.println("  FAIL  " + label);
            System.out.println("        expected: " + show(expected));
            System.out.println("        actual  : " + show(actual));
        }
    }

    private static String show(Object o) {
        if (o == null) return "null";
        String s = o.toString();
        return s.contains("\n") ? "\n----\n" + s + "\n----" : "\"" + s + "\"";
    }

    /** Print the tally and exit non-zero if anything failed. Call last. */
    public static void summary() {
        System.out.println();
        System.out.println(failed == 0
            ? "OK   " + passed + " passed"
            : "FAIL " + failed + " failed, " + passed + " passed");
        if (failed != 0) System.exit(1);
    }
}
