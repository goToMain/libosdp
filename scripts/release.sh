#!/bin/bash

tag=$1

if [[ -z "$tag" ]] || [[ ! -z "$(echo $tag | perl -pe 's/v\d+\.\d+\.\d+//')" ]]; then
	echo "Invalid release tag $tag; usage: $0 vX.Y.Z"
	exit 1
fi

git diff --cached --name-status | while read status file; do
	if [[ "$file" != "CHANGELOG" ]] && [[ "$file" != "CMakeLists.txt" ]]; then
		echo "Invalid modification $file. Only CHANGELOG and CMakeLists.txt must be modified"
		false
	fi
done || exit 1

git commit -s -m "Release $tag" && git tag "$tag" -a -m "Release $tag"
