// file_io.js — write/read/append cycle
const fs = require("fs");
const path = "benchmarks/tmp_io_node.txt";
for (let i = 0; i < 200; i++) {
  fs.writeFileSync(path, `line of benchmark data ${i}\n`);
  fs.readFileSync(path, "utf8");
  fs.appendFileSync(path, `appended ${i}\n`);
  fs.readFileSync(path, "utf8");
}
console.log("file_io done ok=true");
