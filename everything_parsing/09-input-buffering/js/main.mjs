// node js/main.mjs   — identical output bytes to the Java build.

import { Buffer } from "./buffer.mjs";
import { next } from "./scanner.mjs";

let sb = "";
const p = (s = "") => { sb += s + "\n"; };

const src = "the 12.5 quick brownish fox 12.go +9 ok";
const half = 8;

p("=== Module 09 - Input Buffering: Two-Buffer Scheme & Sentinels ===");
p();
p(`source (${src.length} chars): "${src}"`);
p("buffer half-size: " + half);
p();

const b = new Buffer(src, half);
const r = [0];
const toks = [];

p("scan trace  (token | running buffer loads):");
let t;
while ((t = next(b, r)) !== null) {
  toks.push(t);
  p("  " + pad(t.kind, 6) + " " + pad(`"${t.text}"`, 10) + ` loads=${b.bufferLoads}  half=${b.currentHalf()}`);
}
p();

p('what happened at "12.go":  read \'1\' \'2\' \'.\', peeked \'g\' (not a digit),');
p('  retract(1) put \'.\' back  ->  NUM "12", then \'.\' scanned as ERR, then WORD "go"');
p();

p('what happened at "brownish":  it begins in half 1 and ends in half 0 after a reload;');
p('  Buffer.lexeme() stitches across the halfway split -> "' + (toks.find(x => x.text === "brownish")?.text ?? "(not found)") + '"');
p();

p("totals:");
p("  characters consumed : " + b.consumed());
p("  buffer loads        : " + b.bufferLoads);
p("  loads per character : " + ((b.bufferLoads / b.consumed()).toFixed(3)) + "   (amortized O(1))");
p("  forward reads       : " + b.forwardReads
  + "   (one 'data[forward]' compare per advance -- the sentinel means we never ALSO test 'forward == end')");
p("  retracts            : " + r[0]);
p();

p(`summary: ${src.length} source chars | ${b.bufferLoads} buffer loads (~1 per ${half}-char half) | ~1 compare per char | lexeme stitched across the split | retract is O(1)`);

process.stdout.write(sb);

function pad(s, w) { s = String(s); return s.length >= w ? s : s + " ".repeat(w - s.length); }
