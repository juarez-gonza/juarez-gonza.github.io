import os
import re
from datetime import datetime

POSTS_DIR = "posts"
OUTPUT_FILE = "index.html"

# Regex to grab <title> and <meta name="date" content="YYYY-MM-DD">
TITLE_RE = re.compile(r"<title>(.*?)</title>", re.IGNORECASE | re.DOTALL)
DATE_RE = re.compile(r'<meta\s+name="date"\s+content="([\d-]+)"', re.IGNORECASE)

posts = []

for fname in os.listdir(POSTS_DIR):
    if not fname.endswith(".html"):
        continue
    path = os.path.join(POSTS_DIR, fname)
    with open(path, encoding="utf-8") as f:
        text = f.read()

    title_match = TITLE_RE.search(text)
    date_match = DATE_RE.search(text)

    title = title_match.group(1).strip() if title_match else fname
    date_str = date_match.group(1) if date_match else "1970-01-01"
    date = datetime.strptime(date_str, "%Y-%m-%d")

    posts.append({
        "title": title,
        "date": date,
        "date_str": date_str,
        "url": f"{POSTS_DIR}/{fname}"
    })

# Sort by date, newest first
posts.sort(key=lambda p: p["date"], reverse=True)

# Generate HTML
html = """<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>My Blog</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="css/style.css">
</head>
<body>
  <header>
    <h1>My Blog</h1>
    <nav><a href="about.html">About</a></nav>
  </header>

  <main>
    <h2>Posts</h2>
    <ul>
"""
for post in posts:
    html += f'      <li><a href="{post["url"]}">{post["title"]}</a> <small>({post["date_str"]})</small></li>\n'

html += """    </ul>
  </main>

  <footer><p>© 2025 My Blog</p></footer>
</body>
</html>
"""

with open(OUTPUT_FILE, "w", encoding="utf-8") as f:
    f.write(html)

print(f"Generated {OUTPUT_FILE} with {len(posts)} posts.")

