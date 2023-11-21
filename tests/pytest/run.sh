#!/bin/bash

rm -rf .venv
python3 -m venv .venv
source test_venv/bin/activate

pip install pytest
pushd ../../python
python3 setup.py install
popd

pytest -vv --show-capture=no
