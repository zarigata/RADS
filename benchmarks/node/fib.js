// fib.js — naive recursive fibonacci
function fib(n) {
  if (n < 2) return n;
  return fib(n - 1) + fib(n - 2);
}
const r = fib(28);
console.log(`fib(28) = ${r}`);
if (r !== 317811) throw new Error("wrong result");
