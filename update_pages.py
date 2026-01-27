#!/usr/bin/env python3

import sys
import os


def add_blur_and_spotlight(filepath):
    """Add blur-3, blur-4, and mouse spotlight script to HTML file"""
    with open(filepath, "r") as f:
        content = f.read()

    # Add blur-3 and blur-4 after blur-2
    content = content.replace(
        '<div class="bg-blur blur-2"></div>',
        '<div class="bg-blur blur-2"></div>\n    <div class="bg-blur blur-3"></div>\n    <div class="bg-blur blur-4"></div>',
    )

    # Add mouse spotlight script before </body>
    spotlight_script = """
    <!-- Mouse-following spotlight -->
    <script>
        document.addEventListener('DOMContentLoaded', () => {
            const spotlight = document.createElement('div');
            spotlight.className = 'mouse-spotlight';
            document.body.appendChild(spotlight);

            document.addEventListener('mousemove', (e) => {
                const x = e.clientX;
                const y = e.clientY;
                spotlight.style.left = `${x}px`;
                spotlight.style.top = `${y}px`;
            });
        });
    </script>
"""

    content = content.replace("</body>", f"{spotlight_script}\n</body>")

    with open(filepath, "w") as f:
        f.write(content)
    print(f"✓ Updated: {filepath}")


if __name__ == "__main__":
    files = ["docs-intro.html", "examples.html", "compare.html", "playground.html"]
    for file in files:
        add_blur_and_spotlight(file)
    print("\n✨ All pages updated with enhanced glass effects and mouse spotlight!")
