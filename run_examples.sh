#!/bin/bash
for f in examples/*.rads; do
  echo "Running $f"
  ./rads "$f"
done
