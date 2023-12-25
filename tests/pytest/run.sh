#!/bin/bash

PYTEST_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
pushd ${PYTEST_DIR}

rm -rf .venv
rm -rf ../../python/{build,dist,libosdp.egg-info}
python3 -m venv .venv
source ./.venv/bin/activate

pip install pytest
pushd ../../python
python3 setup.py install
popd

pytest -vv --show-capture=all
