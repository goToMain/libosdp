#!/usr/bin/env bash

set -e

SCRIPTS_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
ROOT_DIR="${SCRIPTS_DIR}/../"

function run_make_check() {
	echo "[-] Running make check"
	pushd ${ROOT_DIR}
	rm -rf build
	./configure.sh -f
	make check
	popd
}

function run_cmake_unit_test() {
	echo "[-] Running cmake unit-tests"
	rm -rf build
	cmake -B build .
	cmake --build build -t check-ut
}

function run_pytest() {
	echo "[-] Creating an isolated environment.."
	pushd ${SCRIPTS_DIR}
	rm -rf __pycache__/
	rm -rf .venv ${ROOT_DIR}/python/{build,dist,libosdp.egg-info,vendor}
	python3 -m venv .venv
	source ./.venv/bin/activate
	pip install --upgrade pip

	echo "[-] Installing dependencies.."
	pushd "${ROOT_DIR}/tests/pytest"
	pip install -r requirements.txt

	echo "[-] Installing libosdp.."
	pip install "${ROOT_DIR}/python"

	echo "[-] Running tests capturing all output.."
	pytest -vv --show-capture=all
	popd
}

run_make_check
run_cmake_unit_test
run_pytest
