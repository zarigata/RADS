// string_build.js — string concatenation in a loop
let s = "";
for (let i = 0; i < 5000; i++) {
  s = s + "rads" + i + ";";
}
console.log(`done, last=${s}`);
