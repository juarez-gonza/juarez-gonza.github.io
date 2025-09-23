import os
from datetime import datetime
import re

PRE_POSTS_DIR = "pre-posts"
POSTS_DIR = "posts"
OUTPUT_FILE = "index.html"

posts = []

for fname in os.listdir(PRE_POSTS_DIR):
    if not fname.endswith(".html"):
        continue

    path = os.path.join(PRE_POSTS_DIR, fname)
    with open(path, encoding="utf-8") as f:
        lines = f.readlines()

    # Skip empty lines
    meaningful = [l.strip() for l in lines if l.strip()]

    if len(meaningful) < 2:
        raise ValueError(f"{fname} does not have enough lines for title/date")

    # First line: <h1 id="title">Post Title</h1>
    title_match = re.match(r'<h1\s+id="title">(.*)</h1>', meaningful[0])
    if not title_match:
        raise ValueError(f"{fname}: First line must be <h1 id='title'>Title</h1>")
    title = title_match.group(1).strip()

    # Second line: <p class="date">YYYY-MM-DD</p>
    date_match = re.match(r'<p\s+class="date">([\d-]+)</p>', meaningful[1])
    if not date_match:
        raise ValueError(f"{fname}: Second line must be <p class='date'>YYYY-MM-DD</p>")
    date_str = date_match.group(1)
    try:
        date = datetime.strptime(date_str, "%Y-%m-%d")
    except ValueError:
        raise ValueError(f"{fname}: Date not in YYYY-MM-DD format")

    posts.append({
        "title": title,
        "date": date,
        "date_str": date_str,
        "url": f"posts/{fname}"
    })

# Sort by date descending
posts.sort(key=lambda p: p["date"], reverse=True)

# Generate index.html
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

