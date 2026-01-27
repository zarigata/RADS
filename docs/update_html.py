#!/usr/bin/env python3
import sys

files = [
    "docs/async.html",
    "docs-control.html",
    "docs-functions.html",
    "docs-media.html",
    "docs-networking.html",
    "docs-stdlib.html",
    "docs-structs.html",
    "docs-syntax.html",
    "docs-types.html",
    "docs/documentation.html",
    "docs/examples.html",
    "docs/guide.html",
    "docs/index.html",
    "docs/index.v1.html",
    "docs/playground.html",
    "docs/releases.html",
]

for filepath in files:
    with open(filepath, "r") as f:
        content = f.read()

        # Add blur-3 after blur-2
        content = content.replace(
            '<div class="bg-blur blur-2"></div>',
            '<div class="bg-blur blur-2"></div>\n    <div class="bg-blur blur-3"></div>',
        )

        # Add blur-4 after blur-3
        content = content.replace(
            '<div class="bg-blur blur-3"></div>',
            '<div class="bg-blur blur-3"></div>\n    <div class="bg-blur blur-4"></div>',
        )

        # Add mouse spotlight before </body>
        if "<!-- Mouse-following spotlight -->" not in content:
            spotlight_script = '<!-- Mouse-following spotlight --><script>document.addEventListener("DOMContentLoaded", () => {const spotlight = document.createElement("div");spotlight.className = "mouse-spotlight";document.body.appendChild(spotlight);document.addEventListener("mousemove", (e) => {const x = e.clientX;const y = e.clientY;spotlight.style.left = x + "px";spotlight.style.top = y + "px";});});</script>'
            content = content.replace("</body>", spotlight_script + "</body>")

        with open(filepath, "w") as f:
            f.write(content)
        print(f"Updated: {filepath}")

print("All pages updated!")
