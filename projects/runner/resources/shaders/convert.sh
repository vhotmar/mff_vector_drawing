#!/bin/bash

for file in *.frag
do
  glslc "$file" -o "${file}.spv"
done


for file in *.vert
do
  glslc "$file" -o "${file}.spv"
done