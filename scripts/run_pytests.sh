#!/usr/bin/env bash
#
# LibOSDP PyTest Runner - Enhanced script for running Python tests
#
# USAGE EXAMPLES:
#
# Show help:
#   ./scripts/run_pytests.sh --help
#
# Run all tests (full setup):
#   ./scripts/run_pytests.sh
#
# Run specific test file:
#   ./scripts/run_pytests.sh test_commands.py
#
# Run specific test function:
#   ./scripts/run_pytests.sh test_commands.py::test_command_mfg_with_reply
#
# Run tests matching pattern:
#   ./scripts/run_pytests.sh -k "mfg"
#
# Skip setup and run specific test (faster for development):
#   ./scripts/run_pytests.sh -s test_events.py::test_event_mfg_reply
#
# Fast fail with quiet output:
#   ./scripts/run_pytests.sh -q -f
#
# Custom pytest arguments:
#   ./scripts/run_pytests.sh --tb=short -x
#

set -e

SCRIPTS_DIR="$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
ROOT_DIR="${SCRIPTS_DIR}/../"

# Help function
show_help() {
    cat << EOF
LibOSDP PyTest Runner

USAGE:
    $0 [OPTIONS] [PYTEST_ARGS]

DESCRIPTION:
    Sets up an isolated Python environment and runs LibOSDP Python tests.

OPTIONS:
    -h, --help              Show this help message
    -s, --skip-setup        Skip environment setup (use existing .venv)
    -q, --quiet             Run tests with minimal output
    -v, --verbose           Run tests with extra verbose output (default)
    -f, --fast-fail         Stop on first test failure

EXAMPLES:
    # Run all tests
    $0

    # Run specific test file
    $0 test_commands.py

    # Run specific test function
    $0 test_commands.py::test_command_mfg_with_reply

    # Run tests matching pattern
    $0 -k "mfg"

    # Run with custom pytest args
    $0 --tb=short -x

    # Skip setup and run specific test
    $0 -s test_events.py::test_event_mfg_reply

EOF
}

# Parse arguments
SKIP_SETUP=false
PYTEST_ARGS=()
VERBOSE_LEVEL="-vv"

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -s|--skip-setup)
            SKIP_SETUP=true
            shift
            ;;
        -q|--quiet)
            VERBOSE_LEVEL="-v"
            shift
            ;;
        -v|--verbose)
            VERBOSE_LEVEL="-vv"
            shift
            ;;
        -f|--fast-fail)
            PYTEST_ARGS+=("-x")
            shift
            ;;
        *)
            # All other arguments are passed to pytest
            PYTEST_ARGS+=("$1")
            shift
            ;;
    esac
done

pushd ${ROOT_DIR}/tests/pytest/

if [ "$SKIP_SETUP" = false ]; then
    echo "[-] Creating an isolated environment.."
    rm -rf __pycache__/
    rm -rf .venv ${ROOT_DIR}/python/{build,dist,libosdp.egg-info,vendor}
    python3 -m venv .venv
    source ./.venv/bin/activate
    pip install --upgrade pip

    echo "[-] Installing dependencies.."
    pip install -r requirements.txt

    echo "[-] Installing libosdp.."
    pip install "${ROOT_DIR}/python"
else
    echo "[-] Using existing environment.."
    if [ ! -d .venv ]; then
        echo "ERROR: .venv directory not found. Run without -s/--skip-setup first."
        exit 1
    fi
    source ./.venv/bin/activate
fi

echo "[-] Running tests capturing all output.."
pytest ${VERBOSE_LEVEL} --show-capture=all "${PYTEST_ARGS[@]}"
