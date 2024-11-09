#!/usr/bin/env bash

SCRIPTS_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
ROOT_DIR="${SCRIPTS_DIR}/../"

pushd "${ROOT_DIR}/doc"
rm -rf .venv __pycache__
python3 -m venv .venv
source ./.venv/bin/activate
pip install -r requirements.txt

cmake -B build ..
cmake --build build -t html_docs
mv build/doc/sphinx/ .
rm -rf build
mv sphinx build
