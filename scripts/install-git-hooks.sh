#!/bin/bash

cat > .git/hooks/pre-commit << ---
#!/bin/bash

changes=\$(git diff --cached --name-status)
m="pre-commit error"

echo "\${changes}" | while read status file; do
	# Release checks
	if [[ "\$file" == "CHANGELOG" ]] ; then
		git diff --cached CHANGELOG | grep -q '## TODO' && \\
			echo "\$m release-check: CHANGELOG has TODOs" && exit 1
		echo "\${changes}" | ( ! grep -q -e '^M\\sCMakeLists.txt' ) && \\
			echo "\$m release-check: Version in CMakeLists.txt not modified!" && exit 1
		clv="\$(git diff --cached CHANGELOG | perl -ne 'if (s/^\+v(\d+\.\d+\.\d+)/\$1/) {print;last}')"
		cmv="\$(git diff --cached CMakeLists.txt | perl -ne 'if (s/^\+project\(libosdp VERSION (\d+\.\d+\.\d+)\)/\$1/) {print;last}')"
		if [[ \$clv != "\$cmv" ]]; then
			echo "\$m release-check: Version mismatch! (\$clv/\$cmv)" && exit 1
		fi
	fi
done

files=\$(git diff  --cached --name-only | grep -E ".*\.(cpp|cc|c\+\+|cxx|c|h|hpp)$")
if [[ ! -z "\${files}" ]]; then
	git diff -U0 --cached -- \${files} | ./scripts/clang-format-diff.py -p1
fi

---
chmod a+x .git/hooks/pre-commit && echo "Installed hook: .git/hooks/pre-commit"
