#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
mkdir -p build/musdtoxousdt
trap 'rm -f -- musdtoxousdt.gen.cpp swap_token.gen.cpp' EXIT
# The installed wrapper runs CDT in Docker; CDT_CPP can select an AMAX CDT instead.
compiler="${CDT_CPP:-flon-cdt-tool}"
"$compiler" -abigen --abi-version=1.1 -contract musdtoxousdt \
  -I contracts/musdtoxousdt/include \
  -o build/musdtoxousdt/musdtoxousdt.wasm contracts/musdtoxousdt/src/musdtoxousdt.cpp
# AMAX uses a different ABI version prefix from the compatible FLON compiler.
python3 - <<'PY'
import json
from pathlib import Path
p = Path('build/musdtoxousdt/musdtoxousdt.abi')
a = json.loads(p.read_text())
a['version'] = 'amax::abi/1.1'
p.write_text(json.dumps(a, indent=2) + '\n')
PY
python3 contracts/musdtoxousdt/clean_abi.py build/musdtoxousdt/musdtoxousdt.abi
if [[ "${1:-}" == "--test" ]]; then
  "$compiler" -abigen --abi-version=1.1 -contract swaptoken \
    -o build/musdtoxousdt/swaptoken.wasm tests/fixtures/swap_token.cpp
  python3 - <<'PY'
import json
from pathlib import Path
p = Path('build/musdtoxousdt/swaptoken.abi')
a = json.loads(p.read_text())
a['version'] = 'amax::abi/1.1'
p.write_text(json.dumps(a, indent=2) + '\n')
PY
  docker run --rm --platform linux/amd64 --entrypoint sh \
    -v "$PWD:$PWD" -w "$PWD" fullon/flon-cdt:0.9.0 -c \
    'g++ -std=c++17 -Wall -Wextra -Werror -I contracts/musdtoxousdt/include tests/musdtoxousdt_math.cpp -o build/musdtoxousdt/math_test && build/musdtoxousdt/math_test'
  python3 tests/musdtoxousdt_local.py
fi
