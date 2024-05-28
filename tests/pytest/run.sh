#!/bin/bash

PYTEST_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
pushd ${PYTEST_DIR}

echo "[-] Creating an isolated environment.."
rm -rf __pycache__/
rm -rf .venv ../../python/{build,dist,libosdp.egg-info,vendor}
python3 -m venv .venv
source ./.venv/bin/activate
pip install --upgrade pip

echo "[-] Installing dependencies.."
pip install -r requirements.txt

echo "[-] Installing libosdp.."
pip install ../../python

if [[ "$1" == "-n" ]]; then
	echo "To run tests do:"
	echo
	echo "cd ${PYTEST_DIR}"
	echo "source .venv/bin/activate"
	echo "pytest -vv --show-capture=all [test_<name>.py]"
	exit
fi

echo "[-] Running tests capturing all output.."
pytest -vv --show-capture=all
