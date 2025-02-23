#!/usr/bin/env bash

set -e

SCRIPTS_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
ROOT_DIR="${SCRIPTS_DIR}/../"

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
pytest -vv --show-capture=all $@
