// array_churn.js — array push/pop/map churn
let arr = [];
for (let i = 0; i < 20000; i++) {
  arr.push(i % 97);
}
let acc = 0;
for (let i = 0; i < 5000; i++) {
  acc += arr.pop();
}
const mapped = arr.map((x) => x * 2);
console.log(`acc = ${acc}, last mapped = ${mapped.pop()}`);
