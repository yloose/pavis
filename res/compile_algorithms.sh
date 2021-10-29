#!/bin/bash

source_files=$(ls ${HOME}/.config/pavis/algorithms | grep '\.c$');

for source_file in $source_files; do

  gcc -shared -o ${HOME}/.config/pavis/algorithms/${source_file::-2}.so -fPIC ${HOME}/.config/pavis/algorithms/$source_file

done;
