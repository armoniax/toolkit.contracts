#!/usr/bin/env python3
"""Remove external token table metadata emitted by CDT's multi_index discovery."""
import json
import re
import sys
from pathlib import Path

path = Path(sys.argv[1])
abi = json.loads(path.read_text())
abi['tables'] = [table for table in abi['tables'] if table['name'] != 'accounts']
abi['structs'] = [row for row in abi['structs'] if row['name'] != 'token_account']
# Do not publish dangling references if future code starts exposing this layout.
assert not re.search(r'\btoken_account\b', json.dumps(abi)), 'token_account is still referenced by ABI types'
path.write_text(json.dumps(abi, indent=2) + '\n')
