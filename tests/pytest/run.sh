#!/bin/bash

PYTEST_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
pushd ${PYTEST_DIR}

echo "[-] Creating an isolated environment.."
rm -rf __pycache__/
rm -rf .venv ../../python/{build,dist,libosdp.egg-info,vendor}
python3 -m venv .venv
source ./.venv/bin/activate

echo "[-] Installing dependencies.."
pip install pytest

echo "[-] Installing libosdp.."
pushd ../../python
python3 setup.py install
popd

echo "[-] Running tests capturing all output.."
pytest -vv --show-capture=all
