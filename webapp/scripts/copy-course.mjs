// Runs after `vite build`: publishes the course tutorials under dist/course/.
//
//   dist/course/index.html                         generated course index
//   dist/course/<module_NN - ...>/tutorial.html    compiler track
//   dist/course/everything_parsing/...             parsing course (pages + assets)
//
// Links from a tutorial to anything that isn't published here (DECISIONS.md,
// run.md, source files) are rewritten to the file on GitHub, so no link 404s.

import { readFileSync, writeFileSync, mkdirSync, readdirSync, statSync, existsSync, copyFileSync } from "node:fs";
import { join, dirname, relative, posix } from "node:path";
import { fileURLToPath } from "node:url";

const REPO = join(dirname(fileURLToPath(import.meta.url)), "..", "..");
const OUT = join(REPO, "webapp", "dist", "course");
const GITHUB = "https://github.com/Robert-Doe/a-finity";

const toPosix = (p) => p.split("\\").join("/");

function walk(dir) {
  const out = [];
  for (const name of readdirSync(dir)) {
    if (name === "node_modules" || name === "out" || name.startsWith(".")) continue;
    const full = join(dir, name);
    if (statSync(full).isDirectory()) out.push(...walk(full));
    else out.push(full);
  }
  return out;
}

// ── what gets published ──
const parsingDir = join(REPO, "everything_parsing");
if (!existsSync(parsingDir)) throw new Error("copy-course: everything_parsing/ not found; is the build running from the repo?");

const compilerModules = readdirSync(REPO).filter(d => /^module_\d\d/.test(d) && existsSync(join(REPO, d, "tutorial.html"))).sort();
if (compilerModules.length === 0) throw new Error("copy-course: no module_NN/tutorial.html found");

const published = new Set();                       // repo-relative posix paths
for (const m of compilerModules) published.add(`${m}/tutorial.html`);
for (const f of walk(parsingDir)) {
  const rel = toPosix(relative(REPO, f));
  if (rel.endsWith(".html") || rel.startsWith("everything_parsing/assets/")) published.add(rel);
}

// ── copy, rewriting links to unpublished files ──
function rewrite(html, relPath) {
  return html.replace(/(href|src)="([^"]*)"/g, (whole, attr, value) => {
    if (/^(https?:|mailto:|data:|#|\/\/)/.test(value) || value === "") return whole;
    const [pathPart, hash = ""] = value.split("#");
    if (pathPart === "") return whole;
    let target;
    try { target = posix.normalize(posix.join(posix.dirname(relPath), pathPart.split("/").map(decodeURIComponent).join("/"))); }
    catch { return whole; }
    if (target.startsWith("..")) return whole;
    const isDir = pathPart.endsWith("/");
    const key = isDir ? posix.join(target, "index.html") : target;
    if (published.has(key)) return whole;
    const kind = isDir || !existsSync(join(REPO, target)) || statSync(join(REPO, target)).isDirectory() ? "tree" : "blob";
    const url = `${GITHUB}/${kind}/main/${target.split("/").map(encodeURIComponent).join("/")}${hash ? "#" + hash : ""}`;
    return `${attr}="${url}"`;
  });
}

let pages = 0;
for (const rel of published) {
  const src = join(REPO, rel);
  const dst = join(OUT, rel);
  mkdirSync(dirname(dst), { recursive: true });
  if (rel.endsWith(".html")) {
    writeFileSync(dst, rewrite(readFileSync(src, "utf8"), rel));
    pages++;
  } else {
    copyFileSync(src, dst);
  }
}

// ── the course index ──
const esc = (s) => s.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;");
const titleOf = (rel) => {
  const m = /<title>([^<]*)<\/title>/i.exec(readFileSync(join(REPO, rel), "utf8"));
  return (m ? m[1] : rel).replace(/\s*\|\s*Everything Parsing\s*$/, "").trim();
};
const href = (rel) => rel.split("/").map(encodeURIComponent).join("/");

const compilerItems = compilerModules.map(m => {
  const num = m.slice(7, 9);
  const name = m.replace(/^module_\d\d\s*-\s*/, "");
  return `<li><a href="${href(m + "/tutorial.html")}"><span class="n">${num}</span>${esc(name)}</a></li>`;
}).join("\n");

const parsingModules = readdirSync(parsingDir)
  .filter(d => /^\d\d-/.test(d) && existsSync(join(parsingDir, d, "tutorial.html")))
  .sort();
const parsingItems = parsingModules.map(d => {
  const rel = `everything_parsing/${d}/tutorial.html`;
  const t = titleOf(rel).replace(/^Module \d+\s*[—-]\s*/, "");
  const cap = /Capstone/.test(readFileSync(join(REPO, rel), "utf8").slice(0, 2000)) ? ' <span class="tag">capstone</span>' : "";
  return `<li><a href="${href(rel)}"><span class="n">${d.slice(0, 2)}</span>${t}${cap}</a></li>`;
}).join("\n");

const index = `<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>a-finity courses</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link href="https://fonts.googleapis.com/css2?family=Space+Grotesk:wght@500;700&family=Inter:wght@400;600&family=JetBrains+Mono:wght@500&display=swap" rel="stylesheet">
<style>
  :root { --bg:#0b0b0d; --card:#18181b; --text:#eeece6; --dim:#9a9992; --accent:#d4a017; --soft:rgba(212,160,23,.14); --border:#2a2a2e; }
  * { box-sizing: border-box; }
  body { margin:0; background:var(--bg); color:var(--text); font:16px/1.6 Inter, system-ui, sans-serif; }
  .wrap { max-width: 960px; margin: 0 auto; padding: 32px 16px 64px; }
  .top { display:flex; justify-content:space-between; align-items:center; gap:16px; flex-wrap:wrap; }
  .top a { color: var(--dim); text-decoration:none; font-size:14px; }
  .top a:hover { color: var(--accent); }
  h1 { font-family:'Space Grotesk', system-ui, sans-serif; font-size: clamp(32px, 6vw, 48px); margin: 32px 0 8px; }
  .lede { color: var(--dim); max-width: 60ch; margin: 0 0 32px; }
  section { background:var(--card); border:1px solid var(--border); border-radius:12px; padding:24px; margin-bottom:24px; }
  h2 { font-family:'Space Grotesk', system-ui, sans-serif; margin:0 0 4px; font-size:24px; }
  .sub { color: var(--dim); margin: 0 0 16px; font-size: 15px; }
  .start { display:inline-block; margin: 0 0 16px; color: var(--bg); background: var(--accent); padding: 8px 14px; border-radius: 8px; text-decoration:none; font-weight:600; font-size:14px; }
  ol { list-style:none; margin:0; padding:0; display:grid; grid-template-columns: repeat(auto-fill, minmax(260px, 1fr)); gap: 4px 16px; }
  li a { display:flex; gap:10px; align-items:baseline; padding:6px 8px; border-radius:6px; color:var(--text); text-decoration:none; font-size:15px; }
  li a:hover { background: var(--soft); }
  .n { font-family:'JetBrains Mono', monospace; color: var(--accent); font-size:13px; min-width: 2ch; }
  .tag { font-size:11px; color:var(--accent); border:1px solid var(--accent); border-radius:999px; padding:0 6px; margin-left:4px; }
  .note { color: var(--dim); font-size: 14px; margin: 12px 0 0; }
  .note a, .sub a { color: var(--accent); }
</style>
</head>
<body>
<div class="wrap">
  <div class="top">
    <a href="../">&larr; Compiler Playground</a>
    <a href="${GITHUB}" target="_blank" rel="noopener">GitHub</a>
  </div>
  <h1>Courses</h1>
  <p class="lede">Two tracks, built module by module with the code, a decisions log and a tutorial for every step. Start at module 01 of either track; each tutorial links to the next.</p>

  <section id="compiler">
    <h2>Build a C Compiler</h2>
    <p class="sub">From reading a source file to emitting a runnable x86-64 ELF object, in C. ${compilerModules.length} modules.</p>
    <a class="start" href="${href(compilerModules[0] + "/tutorial.html")}">Start with module 01</a>
    <ol>
${compilerItems}
    </ol>
  </section>

  <section id="parsing">
    <h2>Everything Parsing</h2>
    <p class="sub">Formal languages, automata, and top-down and bottom-up parsing, with every concept implemented in Java and JavaScript side by side. ${parsingModules.length} modules published so far.</p>
    <a class="start" href="everything_parsing/prerequisites/index.html">Start with the prerequisites</a>
    <ol>
${parsingItems}
    </ol>
    <p class="note">Full plan: <a href="${GITHUB}/blob/main/everything_parsing/ROADMAP.md" target="_blank" rel="noopener">ROADMAP.md</a> &middot; <a href="${GITHUB}/blob/main/everything_parsing/GLOSSARY.md" target="_blank" rel="noopener">GLOSSARY.md</a></p>
  </section>

  <section id="interpreters">
    <h2>Interpreters <span class="tag">coming next</span></h2>
    <p class="sub">A tree-walking interpreter, a bytecode compiler and stack VM, closures, garbage collection and performance, in Java and JavaScript. <a href="${GITHUB}/blob/main/interpreters/ROADMAP.md" target="_blank" rel="noopener">Read the plan</a>.</p>
  </section>
</div>
</body>
</html>
`;
mkdirSync(OUT, { recursive: true });
writeFileSync(join(OUT, "index.html"), index);
console.log(`copy-course: ${pages} pages published under dist/course/ (${compilerModules.length} compiler modules, ${parsingModules.length} parsing modules)`);
