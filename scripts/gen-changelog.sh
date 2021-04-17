#!/bin/bash

last_rel=$(git describe --tags | perl -pe 's/-.*//')

echo "$last_rel ## TODO"
echo -e "------\n"
date "+%d %B %Y"
echo -e "\nRelease subject ## TODO"
echo -e "\nEnhancements: ## TODO"
echo -e "\nFixes: ## TODO"

# Changes since last release
git log ${last_rel}..HEAD --oneline --no-merges --decorate=no | perl -pe 's/^\w+ /  - /'
echo -e "\n"
