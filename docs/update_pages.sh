#!/bin/bash

# Add blur-3, blur-4, and mouse spotlight to all HTML pages

for file in documentation.html examples.html compare.html playground.html; do
    # Add blur-3 and blur-4 after blur-2
    sed -i 's|</div>\n    </header>|</div>\n    <div class="bg-blur blur-3"></div>\n    <div class="bg-blur blur-4"></div>\n\n    </header>|' "$file"

    # Add mouse spotlight script before </body>
    sed -i 's|</body>|\n    <!-- Mouse-following spotlight -->\n    <script>\n        document.addEventListener('\''\\'\\'DOMContentLoaded'\\'\\'''', () => {\n            const spotlight = document.createElement('\''\\'\\'div'\\'\\'''');\n            spotlight.className = '\''\\'\\'mouse-spotlight'\\'\\'''\\'';\n            document.body.appendChild(spotlight);\n\n            document.addEventListener('\''\\'\\'mousemove'\\'\\'''', (e) => {\n                const x = e.clientX;\n                const y = e.clientY;\n                spotlight.style.left = `\${x}px`;\n                spotlight.style.top = `\${y}px`;\n            });\n        });\n    </script>\n</body>|' "$file"

    echo "Updated: $file"
done

echo "All HTML pages updated with enhanced glass effects and mouse spotlight!"
