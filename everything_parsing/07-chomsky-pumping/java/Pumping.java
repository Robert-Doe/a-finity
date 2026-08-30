/**
 * Module 07 — the pumping lemmas, run as an adversary.
 *
 * PUMPING LEMMA (regular): if L is regular, there is a length p such that every
 * s in L with |s| >= p splits as s = x y z with |y| >= 1, |xy| <= p, and
 * x y^k z is in L for ALL k >= 0.
 *
 * To prove { a^n b^n } is NOT regular we play the adversary: for ANY claimed p,
 * we hand back the string s = a^p b^p and show that EVERY legal (x, y, z) split
 * has some k with x y^k z not in L. The lemma cannot be satisfied for any p, so
 * L is not regular.  Same game, five parts, for the context-free lemma and
 * { a^n b^n c^n }.
 */
public final class Pumping {
    private Pumping() {}

    // ── the two target languages, as membership predicates ──

    static boolean inAnBn(String s) {
        int i = 0;
        while (i < s.length() && s.charAt(i) == 'a') i++;
        int a = i;
        while (i < s.length() && s.charAt(i) == 'b') i++;
        return i == s.length() && a == s.length() - a;
    }

    static boolean inAnBnCn(String s) {
        int i = 0;
        while (i < s.length() && s.charAt(i) == 'a') i++;
        int a = i;
        while (i < s.length() && s.charAt(i) == 'b') i++;
        int b = i - a;
        while (i < s.length() && s.charAt(i) == 'c') i++;
        int c = i - a - b;
        return i == s.length() && a == b && b == c;
    }

    // ── regular refutation for { a^n b^n } ──

    record RegularWitness(String s, String x, String y, String z, int k, String pumped,
                          int decompositionsTried, boolean allEscaped) {}

    static RegularWitness regularRefutation(int p) {
        String s = "a".repeat(p) + "b".repeat(p);
        int tried = 0;
        boolean allEscaped = true;
        String repX = "", repY = "", repZ = "", repPumped = "";
        int repK = 0;

        // every split s = x y z with |x y| = b <= p and |y| = b - a >= 1
        for (int b = 1; b <= p; b++) {
            for (int a = 0; a < b; a++) {
                tried++;
                String x = s.substring(0, a), y = s.substring(a, b), z = s.substring(b);
                int escapeK = firstEscape(x, y, z, Pumping::inAnBn);
                if (escapeK < 0) { allEscaped = false; }
                else if (repPumped.isEmpty()) {           // remember the first (a=0,b=1) as the representative
                    repX = x; repY = y; repZ = z; repK = escapeK;
                    repPumped = x + y.repeat(escapeK) + z;
                }
            }
        }
        return new RegularWitness(s, repX, repY, repZ, repK, repPumped, tried, allEscaped);
    }

    // ── context-free refutation for { a^n b^n c^n } ──

    record CflWitness(String s, String u, String v, String w, String x, String y,
                      int k, String pumped, int decompositionsTried, boolean allEscaped) {}

    static CflWitness cflRefutation(int p) {
        String s = "a".repeat(p) + "b".repeat(p) + "c".repeat(p);
        int n = s.length();
        int tried = 0;
        boolean allEscaped = true;
        String rU = "", rV = "", rW = "", rX = "", rY = "", rP = "";
        int rK = 0;

        // s = u v w x y, with v w x contiguous, |v w x| <= p, |v x| >= 1
        for (int i = 0; i <= n; i++) {                    // start of v
            for (int j = i; j <= Math.min(n, i + p); j++) {        // start of w
                for (int l = j; l <= Math.min(n, i + p); l++) {    // start of x
                    for (int m = l; m <= Math.min(n, i + p); m++) {// end of x
                        int vLen = j - i, xLen = m - l;
                        if (vLen + xLen < 1) continue;    // |v x| >= 1
                        if (m - i > p) continue;          // |v w x| <= p
                        tried++;
                        String u = s.substring(0, i), v = s.substring(i, j),
                               w = s.substring(j, l), x = s.substring(l, m), y = s.substring(m);
                        int esc = firstEscape5(u, v, w, x, y, Pumping::inAnBnCn);
                        if (esc < 0) allEscaped = false;
                        else if (rP.isEmpty()) {
                            rU = u; rV = v; rW = w; rX = x; rY = y; rK = esc;
                            rP = u + v.repeat(esc) + w + x.repeat(esc) + y;
                        }
                    }
                }
            }
        }
        return new CflWitness(s, rU, rV, rW, rX, rY, rK, rP, tried, allEscaped);
    }

    // ── helpers: smallest k in {0, 2, 3} with the pumped string outside L ──

    private interface InL { boolean test(String s); }

    private static int firstEscape(String x, String y, String z, InL inL) {
        for (int k : new int[]{2, 0, 3}) {
            if (!inL.test(x + y.repeat(k) + z)) return k;
        }
        return -1;
    }
    private static int firstEscape5(String u, String v, String w, String x, String y, InL inL) {
        for (int k : new int[]{2, 0, 3}) {
            if (!inL.test(u + v.repeat(k) + w + x.repeat(k) + y)) return k;
        }
        return -1;
    }
}
