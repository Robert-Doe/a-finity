"""Re-theme the compiler-track tutorials to match the Everything Parsing design.

    python tools/apply-course-theme.py            # every module_NN/tutorial.html
    python tools/apply-course-theme.py FILE ...   # specific files

The compiler tutorials were written with dark, per-page palettes. The parsing
course uses one light "engineering paper" theme (everything_parsing/assets/
course.css): paper background with a faint grid, ink text, Space Grotesk
headings, Newsreader body, IBM Plex Mono code, and DARK code blocks.

Each page keeps its own layout CSS; only colour and type change:

  * page context  (cards, callouts, tables, text, borders)
        dark surfaces -> paper/card tones, light text -> ink, bright accents ->
        darker accents readable on paper, dark borders -> the rule colour
  * code context  (pre, terminals, syntax-highlight classes)
        left dark, like the parsing course's code blocks; dark backgrounds are
        unified to the parsing ink colour
  * fonts         swapped to the parsing course's three families

CSS variables stay defined as-is (code context still uses them); page-context
declarations get the variable's value substituted and converted. A marker
comment makes the script idempotent.
"""
import colorsys
import pathlib
import re
import sys

MARK = "<!-- a-finity course theme -->"
INK = (0x14, 0x20, 0x1A)

FONTS_LINK = ('<link href="https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500;600'
              '&family=Newsreader:ital,opsz,wght@0,6..72,400;0,6..72,500;0,6..72,600;1,6..72,400'
              '&family=Space+Grotesk:wght@500;700&display=swap" rel="stylesheet">')

BRIDGE = """
<style>
/* a-finity course theme: shared look with everything_parsing/assets/course.css */
html { -webkit-text-size-adjust: 100%; }
body {
  background-color: #E7EBE3;
  background-image:
    linear-gradient(#C6D0C0 .5px, transparent .5px),
    linear-gradient(90deg, #C6D0C0 .5px, transparent .5px);
  background-size: 26px 26px;
  color: #14201A;
  font-family: 'Newsreader', Georgia, 'Times New Roman', serif;
  font-size: 17.5px;
  line-height: 1.62;
}
h1, h2, h3, h4 { font-family: 'Space Grotesk', ui-sans-serif, system-ui, sans-serif; letter-spacing: -.02em; }
h1 { color: #14201A !important; font-weight: 700; line-height: 1.02; }
h2, h3 { color: #14201A !important; }
h2 { font-weight: 700; }
pre, code, kbd, samp { font-family: 'IBM Plex Mono', ui-monospace, 'SF Mono', Menlo, monospace; }
pre { background-color: #14201A; color: #E4EDE6; border-radius: 2px; }
pre code { color: #E4EDE6; background: none; border: 0; padding: 0; }
a { color: #2A3E9B; }
::selection { background: #EFEA5A; color: #14201A; }
@media (max-width: 640px) {
  body { padding-left: 16px; padding-right: 16px; }
  table { display: block; max-width: 100%; overflow-x: auto; }
  pre { max-width: 100%; overflow-x: auto; }
  :not(pre) > code { overflow-wrap: anywhere; word-break: break-word; }
  nav, [class*="nav"] { flex-wrap: wrap; }
  img, svg { max-width: 100%; height: auto; }
  [class*="grid"], [class*="row"], [class*="col"] { min-width: 0; }
  [class*="split"], [class*="grammar-row"], [class*="two-col"], [class*="before-after"] { grid-template-columns: 1fr !important; }
  header, [class*="topbar"], [class*="header"] { flex-wrap: wrap; }
}
</style>
"""

MONO = "'IBM Plex Mono', ui-monospace, 'SF Mono', Menlo, monospace"
DISPLAY = "'Space Grotesk', ui-sans-serif, system-ui, sans-serif"
BODY = "'Newsreader', Georgia, 'Times New Roman', serif"

# Only real code surfaces stay dark: <pre>, terminal panels, and the
# "window" panels. Classes used INSIDE those surfaces keep their original
# colours there via a scoped copy of their rule (see dark_copies).
CODE_CTX = re.compile(r"\bpre\b|\bkbd\b|\bsamp\b|\.(terminal|win-[\w-]+)\b")
DARK_SURFACES = ("pre", ".terminal", ".win-box")

COLOR_RE = re.compile(r"#[0-9a-fA-F]{8}\b|#[0-9a-fA-F]{6}\b|#[0-9a-fA-F]{4}\b|#[0-9a-fA-F]{3}\b|rgba?\([^)]*\)|\b(?:white|black)\b")


# ─────────────────────────────────────────── colour maths

def parse_color(tok):
    if tok == "white":
        return 255, 255, 255, None
    if tok == "black":
        return 0, 0, 0, None
    if tok.startswith("#"):
        h = tok[1:]
        if len(h) in (3, 4):
            h = "".join(c * 2 for c in h)
        alpha = f"{int(h[6:8], 16) / 255:.2f}" if len(h) == 8 else None
        return int(h[0:2], 16), int(h[2:4], 16), int(h[4:6], 16), alpha
    nums = [x.strip() for x in tok[tok.index("(") + 1:-1].replace("/", ",").split(",")]
    r, g, b = (int(float(n)) for n in nums[:3])
    a = nums[3] if len(nums) > 3 else None
    return r, g, b, a


def fmt(rgb, alpha):
    r, g, b = (max(0, min(255, round(c))) for c in rgb)
    if alpha is not None:
        return f"rgba({r}, {g}, {b}, {alpha})"
    return f"#{r:02X}{g:02X}{b:02X}"


def hls(rgb):
    return colorsys.rgb_to_hls(*(c / 255 for c in rgb))


def from_hls(h, l, s):
    return tuple(c * 255 for c in colorsys.hls_to_rgb(h, l, s))


PAPER_HUE = 0.27  # the grey-green of the paper and ink


def as_background(rgb, alpha):
    h, l, s = hls(rgb)
    translucent = alpha is not None and float(alpha) < 0.6
    if l >= 0.35 or translucent:
        # a bright fill: badge or tint -> a light wash of the same hue
        if translucent:
            return from_hls(h, 0.38, min(s, 0.6))
        return from_hls(h, 0.90, min(s, 0.55))
    if s > 0.3 and l > 0.02:
        # a tinted dark panel (callout backgrounds) -> a pale tint of that hue
        return from_hls(h, 0.94, min(s, 0.55))
    # neutral dark surface -> card, a little lighter the lighter it was
    return from_hls(PAPER_HUE, 0.955 + min(l, 0.3) * 0.08, 0.16)


def as_text(rgb, alpha):
    h, l, s = hls(rgb)
    # (faint decorative text: alpha is raised in convert_value)
    if l < 0.3:
        return rgb                       # already dark text (on a bright badge)
    if s < 0.5 and l > 0.72:
        return INK                       # main text
    if s < 0.5:
        return from_hls(h, 0.36, min(s, 0.18))   # muted text -> soft
    return from_hls(h, 0.32, min(s, 0.72))       # accent -> darker accent


def as_border(rgb, alpha):
    h, l, s = hls(rgb)
    if l < 0.38 and s < 0.6:
        return (0xC6, 0xD0, 0xC0)        # the rule colour
    return from_hls(h, 0.42, min(s, 0.65))


def code_background(rgb, alpha):
    h, l, s = hls(rgb)
    if l < 0.14 and s < 0.7 and alpha is None:
        return INK
    return rgb


def code_text(rgb, alpha):
    h, l, s = hls(rgb)
    if l < 0.55 and alpha is None:
        return from_hls(h, 0.58, s)      # dim-on-purpose text: still dim, but readable on ink
    return rgb


ROLE = {
    "background": as_background, "background-color": as_background, "background-image": as_background,
    "color": as_text, "fill": as_background, "stop-color": as_background,
    "border": as_border, "border-color": as_border, "border-top": as_border, "border-bottom": as_border,
    "border-left": as_border, "border-right": as_border, "border-top-color": as_border,
    "border-bottom-color": as_border, "border-left-color": as_border, "border-right-color": as_border,
    "outline": as_border, "outline-color": as_border, "stroke": as_border, "box-shadow": as_border,
    "text-shadow": as_text, "caret-color": as_text, "text-decoration-color": as_text,
}


def convert_value(prop, value, variables, code, svg_text=False):
    prop = prop.strip().lower()
    if prop.startswith("--"):
        return value
    if prop == "font-family":
        v = value.lower()
        if any(k in v for k in ("mono", "code", "consolas", "courier", "menlo")):
            return MONO
        if "playfair" in v:
            return DISPLAY
        return BODY
    role = as_text if svg_text and prop == "fill" else ROLE.get(prop)
    if role is None:
        return value
    if code:
        if role is as_text:
            return COLOR_RE.sub(lambda m: fmt(code_text(*_split(m.group(0))), parse_color(m.group(0))[3]), value)
        if role is not as_background:
            return value
        return COLOR_RE.sub(lambda m: fmt(code_background(*_split(m.group(0))), parse_color(m.group(0))[3]), value)
    value = substitute_vars(value, variables)

    def one(m):
        rgb, alpha = _split(m.group(0))
        if role is as_text and alpha is not None and float(alpha) < 0.55:
            alpha = "0.55"
        return fmt(role(rgb, alpha), alpha)
    return COLOR_RE.sub(one, value)


def _split(tok):
    r, g, b, a = parse_color(tok)
    return (r, g, b), a


def substitute_vars(value, variables, depth=0):
    def repl(m):
        name, fallback = m.group(1), m.group(2)
        v = variables.get(name)
        if v is None:
            return fallback.strip() if fallback else m.group(0)
        return substitute_vars(v, variables, depth + 1) if depth < 5 else v
    return re.sub(r"var\(\s*(--[\w-]+)\s*(?:,\s*([^)]*))?\)", repl, value)


# ─────────────────────────────────────────── CSS walking

def convert_declarations(body, variables, code):
    out = []
    for decl in split_decls(body):
        if ":" not in decl:
            out.append(decl)
            continue
        prop, val = decl.split(":", 1)
        out.append(f"{prop}:{convert_value(prop, val, variables, code)}")
    return ";".join(out)


def split_decls(body):
    parts, depth, cur = [], 0, ""
    for ch in body:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == ";" and depth == 0:
            parts.append(cur)
            cur = ""
        else:
            cur += ch
    parts.append(cur)
    return parts


def convert_css(css, variables):
    out, i = [], 0
    while i < len(css):
        brace = css.find("{", i)
        if brace < 0:
            out.append(css[i:])
            break
        selector = css[i:brace]
        # find the matching close brace
        depth, j = 1, brace + 1
        while j < len(css) and depth:
            if css[j] == "{":
                depth += 1
            elif css[j] == "}":
                depth -= 1
            j += 1
        inner = css[brace + 1:j - 1]
        sel = re.sub(r"/\*.*?\*/", "", selector, flags=re.S).strip()
        if sel.startswith("@media") or sel.startswith("@supports"):
            out.append(selector + "{" + convert_css(inner, variables) + "}")
        elif sel.startswith("@"):
            out.append(selector + "{" + inner + "}")
        elif sel.startswith(":root"):
            out.append(selector + "{" + inner + "}")
        else:
            code = bool(CODE_CTX.search(sel))
            out.append(selector + "{" + convert_declarations(inner, variables, code) + "}")
        i = j
    return "".join(out)


def root_variables(css):
    variables = {}
    for block in re.findall(r":root\s*\{([^}]*)\}", css):
        for name, val in re.findall(r"(--[\w-]+)\s*:\s*([^;]+)", block):
            variables[name] = val.strip()
    return variables


# ─────────────────────────────────────────── HTML

def convert_inline(html, variables):
    # style="..." attributes; skip elements whose class marks a code surface
    pre_spans = [(a.start(), b.end()) for a, b in
                 zip(re.finditer(r"<pre\b", html), re.finditer(r"</pre>", html))]

    def in_pre(pos):
        return any(a <= pos < b for a, b in pre_spans)

    def repl(m):
        tag, before, style, after = m.group(1), m.group(2), m.group(3), m.group(4)
        cls = re.search(r'class="([^"]*)"', before + after)
        code = in_pre(m.start()) or bool(cls and CODE_CTX.search("." + " .".join(cls.group(1).split())))
        return f"<{tag}{before}style=\"{convert_declarations(style, variables, code)}\"{after}>"
    html = re.sub(r"<(\w+)([^>]*?)style=\"([^\"]*)\"([^>]*)>", repl, html)

    # SVG presentation attributes
    def svg_attr(m):
        tag, attrs = m.group(1), m.group(2)
        def one(a):
            name, val = a.group(1), a.group(2)
            if COLOR_RE.search(val) or "var(" in val:
                val = convert_value(name, val, variables, False, svg_text=tag in ("text", "tspan"))
            return f'{name}="{val}"'
        attrs = re.sub(r'\bfont-family="([^"]*)"',
                       lambda f: f'font-family="{convert_value("font-family", f.group(1), variables, False)}"', attrs)
        return "<" + tag + re.sub(r'\b(fill|stroke|stop-color)="([^"]*)"', one, attrs) + ">"
    html = re.sub(r"<(rect|circle|ellipse|path|line|polyline|polygon|text|tspan|stop|g)(\s[^>]*)>", svg_attr, html)
    return html


COLOR_PROPS = ("color", "background", "background-color", "border-color", "fill")


def classes_on_dark_surfaces(html):
    """Classes of elements that sit inside <pre>, .terminal or .win-* panels."""
    found = set()
    depth = 0
    for m in re.finditer(r"<(/?)(\w+)([^>]*)>", html):
        closing, tag, attrs = m.group(1), m.group(2).lower(), m.group(3)
        if tag == "pre":
            depth += -1 if closing else 1
            continue
        if depth > 0 and not closing:
            c = re.search(r'class="([^"]*)"', attrs)
            if c:
                found.update(c.group(1).split())
    # panels: everything between a terminal/win-box opening and its closing div
    for m in re.finditer(r'<div[^>]*class="[^"]*\b(terminal|win-box)\b[^"]*"[^>]*>', html):
        seg = html[m.end():m.end() + 20000]
        depth, end = 1, len(seg)
        for d in re.finditer(r"<(/?)div\b", seg):
            depth += -1 if d.group(1) else 1
            if depth == 0:
                end = d.start()
                break
        for c in re.findall(r'class="([^"]*)"', seg[:end]):
            found.update(c.split())
    return found


def dark_copies(css_blocks, classes, variables):
    """`pre .cls { original colours }` for every class used on a dark surface."""
    out = []
    for css in css_blocks:
        for sel, body in re.findall(r"([^{}@]+)\{([^{}]*)\}", css):
            sel = re.sub(r"/\*.*?\*/", "", sel, flags=re.S).strip()
            if not sel or CODE_CTX.search(sel) or sel.startswith(":root"):
                continue
            for part in (x.strip() for x in sel.split(",")):
                last = part.split()[-1] if part.split() else ""
                cls = re.findall(r"\.([\w-]+)", last)
                if not cls or not any(c in classes for c in cls):
                    continue
                decls = []
                for d in split_decls(body):
                    if ":" not in d:
                        continue
                    prop, val = d.split(":", 1)
                    if prop.strip().lower() in COLOR_PROPS:
                        decls.append(f"{prop.strip()}:{convert_value(prop, substitute_vars(val, variables), variables, True)}")
                if decls:
                    scoped = ", ".join(f"{surface} {last}" for surface in DARK_SURFACES)
                    out.append(f"{scoped} {{ {'; '.join(decls)} }}")
    return "\n".join(out)


def theme(path):
    html = path.read_text(encoding="utf-8")
    if MARK in html:
        return False
    styles = re.findall(r"<style>(.*?)</style>", html, flags=re.S)
    variables = {}
    for s in styles:
        variables.update(root_variables(s))
    copies = dark_copies(styles, classes_on_dark_surfaces(html), variables)
    html = re.sub(r"<style>(.*?)</style>", lambda m: "<style>" + convert_css(m.group(1), variables) + "</style>",
                  html, flags=re.S)
    body_start = html.find("</head>")
    head, body = html[:body_start], html[body_start:]
    body = convert_inline(body, variables)

    head = re.sub(r'<link[^>]*fonts\.googleapis\.com/css2[^>]*>\s*', "", head)
    if 'fonts.googleapis.com"' not in head:
        head += ('<link rel="preconnect" href="https://fonts.googleapis.com">\n'
                 '<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>\n')
    head += FONTS_LINK + "\n" + BRIDGE
    if copies:
        head += "<style>\n/* original colours for highlighting inside code blocks */\n" + copies + "\n</style>\n"
    head += MARK + "\n"
    path.write_text(head + body, encoding="utf-8", newline="")
    return True


if __name__ == "__main__":
    root = pathlib.Path(__file__).resolve().parent.parent
    files = [pathlib.Path(a) for a in sys.argv[1:]] or sorted(root.glob("module_*/tutorial.html"))
    for f in files:
        print(("themed  " if theme(f) else "skipped ") + str(f.relative_to(root) if f.is_absolute() else f))
