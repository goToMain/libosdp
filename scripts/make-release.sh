#!/usr/bin/env bash

usage() {
	cat >&2<<----
	LibOSDP release helper 

	OPTIONS:
	  --patch		Release version bump type: patch (default)
	  --major		Release version bump type: major
	  --minor		Release version bump type: minor
	  -h, --help		Print this help
	---
}

function setup_py_inc_version() {
	dir=$1
	inc=$2
	perl -pi -se '
	if (/^project_version = "(\d+)\.(\d+)\.(\d+)"$/) {
		$maj=$1; $min=$2; $pat=$3;
		if ($major) { $maj+=1; $min=0; $pat=0; }
		if ($minor) { $min+=1; $pat=0; }
		$pat+=1 if $patch;
		$_="project_version = \"$maj.$min.$pat\"\n"
	}' -- -$inc $dir/setup.py
}

function cmake_inc_version() {
	dir=$1
	inc=$2
	perl -pi -se '
	if (/^project\((\w+) VERSION (\d+)\.(\d+)\.(\d+)\)$/) {
		$maj=$2; $min=$3; $pat=$4;
		if ($major) { $maj+=1; $min=0; $pat=0; }
		if ($minor) { $min+=1; $pat=0; }
		$pat+=1 if $patch;
		$_="project($1 VERSION $maj.$min.$pat)\n"
	}' -- -$inc $dir/CMakeLists.txt
}

function platformio_inc_version() {
	inc=$1
	perl -pi -se '
	if (/^#define PROJECT_VERSION (\s+) "(\d+)\.(\d+)\.(\d+)"$/) {
		$maj=$2; $min=$3; $pat=$4;
		if ($major) { $maj+=1; $min=0; $pat=0; }
		if ($minor) { $min+=1; $pat=0; }
		$pat+=1 if $patch;
		$_="#define PROJECT_VERSION $1 \"$maj.$min.$pat\"\n"
	}' -- -$inc platformio/osdp_config.h

	perl -pi -se '
	if (/^  "version": "(\d+)\.(\d+)\.(\d+)",$/) {
		$maj=$1; $min=$2; $pat=$3;
		if ($major) { $maj+=1; $min=0; $pat=0; }
		if ($minor) { $min+=1; $pat=0; }
		$pat+=1 if $patch;
		$_="  \"version\": \"$maj.$min.$pat\",\n"
	}' -- -$inc library.json
}

function generate_change_log() {
	last_rel=$(git tag --list 'v*' --sort=v:refname | tail -1)
	version=$(perl -ne 'print $1 if (/ VERSION (\d+.\d+.\d)\)$/)' CMakeLists.txt)

	cat <<-EOF
	v${version} ## TODO
	------

	$(date "+%d %B %Y")
	
	Release subject ## TODO"

	Enhancements: ## TODO"

	Fixes: ## TODO"
	EOF

	# Changes since last release
	git log ${last_rel}..HEAD --oneline --no-merges --decorate=no | perl -pe 's/^\w+ /  - /'
}

function prepare_libosdp_release() {
	cmake_inc_version "." $1
	setup_py_inc_version "python" $1
	platformio_inc_version $1
	generate_change_log > /tmp/rel.txt
	printf '%s\n\n\n%s\n' "$(cat /tmp/rel.txt)" "$(cat CHANGELOG)" > CHANGELOG
}

function do_libosdp_release() {
	if [[ "$(git diff --stat)" == "" ]]; then
		prepare_libosdp_release $1
		exit 0
	fi
	git diff --cached --name-status | while read status file; do
		if [[ "$file" != "CHANGELOG" ]] && \
		   [[ "$file" != "CMakeLists.txt" ]] && \
		   [[ "$file" != "python/setup.py" ]] && \
		   [[ "$file" != "library.json" ]] && \
		   [[ "$file" != "platformio/osdp_config.h" ]]
		then
			echo "ERROR:"
			echo "  Only CHANGELOG CMakeLists.txt and few other files must be modified"
			echo "  to make a release commit. To prepare a new release, run this"
			echo "  script on a clean git tree."
			exit 1
		fi
	done
	version=$(perl -ne 'print $1 if (/ VERSION (\d+\.\d+\.\d+)\)$/)' CMakeLists.txt)
	if grep -q -E "^v$version ## TODO$" CHANGELOG; then
		echo "CHANGELOG needs to be updated manually"
		exit 1
	fi
	git add CHANGELOG CMakeLists.txt python/setup.py library.json platformio/osdp_config.h &&
	git commit -s -m "Release v$version" &&
	git tag "v$version" -s -a -m "Release v$version"
}

INC="patch"
COMPONENT="libosdp"
while [ $# -gt 0 ]; do
	case $1 in
	--patch)		INC="patch";;
	--major)		INC="major";;
	--minor)		INC="minor";;
	-h|--help)             usage; exit 0;;
	*) echo -e "Unknown option $1\n"; usage; exit 1;;
	esac
	shift
done

do_libosdp_release $INC

