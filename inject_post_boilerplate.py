import os

RAW_DIR = "pre-posts"
POSTS_DIR = "posts"
HEADER_FILE = "header.html"
FOOTER_FILE = "footer.html"

os.makedirs(POSTS_DIR, exist_ok=True)

with open(HEADER_FILE, encoding="utf-8") as f:
    header_html = f.read()
with open(FOOTER_FILE, encoding="utf-8") as f:
    footer_html = f.read()

for fname in os.listdir(RAW_DIR):
    if not fname.endswith(".html"):
        continue

    input_path = os.path.join(RAW_DIR, fname)
    with open(input_path, encoding="utf-8") as f:
        post_content = f.read()

    final_html = f"{header_html}\n{post_content}\n{footer_html}"

    output_path = os.path.join(POSTS_DIR, fname)
    with open(output_path, "w", encoding="utf-8") as f:
        f.write(final_html)

    print(f"Processed {fname} -> {output_path}")

