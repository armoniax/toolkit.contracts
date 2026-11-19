#!/usr/bin/env python3
"""Exercise compiled WASM on an isolated AMAX node; never contacts a public RPC.
Requires Docker and build/musdtoxousdt/{musdtoxousdt,swaptoken}.{wasm,abi}.
"""
import json
import subprocess
import time
import uuid
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD = ROOT / 'build/musdtoxousdt'
NODE = 'musdt-test-' + uuid.uuid4().hex[:8]
IMAGE = 'ghcr.io/armoniax/amnod:1.0.4'
# Public, disposable development genesis key. Never use on a real chain.
PUB = 'AM6MRyAjQq8ud7hVNYcfnVPJqcVpscN5So8BhtHuGYqET5GDW5CV'
DEV_KEY = '5KQwrPbwdL6PhXujxW37FSSQZ1JiwsST4cqQzDeyXtP79zkvFD3'
SWAP = 'musdtoxousdt'
ADMIN = 'amgenesisdao'


def run(args, check=True):
    return subprocess.run(args, capture_output=True, text=True, check=check, timeout=45)


def cli(*args, error=None):
    p = run(['docker', 'exec', NODE, 'amcli', '--wallet-url',
             'unix:///tmp/test-wallet.sock', *args], check=False)
    if error is not None:
        assert p.returncode and error in p.stderr, p.stdout + p.stderr
        return
    assert p.returncode == 0, p.stdout + p.stderr
    return json.loads(p.stdout) if p.stdout.strip().startswith(('{', '[')) else p.stdout


def push(code, action, data, actor, error=None):
    if action == 'setconfig' and len(data) == 3:
        data = data + [30]
    return cli('push', 'action', code, action, json.dumps(data), '-p', actor, '-j', '-f', error=error)


def transfer(code, amount, memo='', error=None):
    return push(code, 'transfer', ['alice', SWAP, amount, memo], 'alice', error)


def config():
    return cli('get', 'table', SWAP, SWAP, 'config')['rows'][0]

def orders():
    return cli('get', 'table', SWAP, SWAP, 'orders', '--limit', '100')['rows']


def snapshot():
    return [config(), orders()] + [cli('get', 'table', code, owner, 'accounts')['rows']
                         for code in ('amax.mtoken', 'usdt.token', 'fake.token')
                         for owner in ('alice', SWAP, ADMIN)]


def rejected(code, amount, memo, error):
    before = snapshot()
    transfer(code, amount, memo, error)
    assert snapshot() == before, 'failed transfer did not roll back'


def permission(with_code):
    auth = {'threshold': 1, 'keys': [{'key': PUB, 'weight': 1}], 'accounts': [], 'waits': []}
    if with_code:
        auth['accounts'] = [{'permission': {'actor': SWAP, 'permission': 'amax.code'}, 'weight': 1}]
    cli('set', 'account', 'permission', SWAP, 'active', json.dumps(auth), 'owner', '-p', SWAP)


try:
    for name in ('musdtoxousdt', 'swaptoken'):
        for ext in ('wasm', 'abi'):
            assert (BUILD / f'{name}.{ext}').is_file(), f'Build {name}.{ext} first'
    run(['docker', 'run', '-d', '--name', NODE, '--platform', 'linux/amd64',
         '--entrypoint', 'amnod', IMAGE, '--data-dir', '/tmp/chain', '--config-dir', '/tmp/conf',
         '--enable-stale-production', '--producer-name', 'amax', '--plugin', 'eosio::chain_api_plugin',
         '--plugin', 'eosio::producer_plugin', '--max-transaction-time', '1000',
         '--abi-serializer-max-time-ms', '2000', '--http-server-address', '127.0.0.1:8888'])
    for _ in range(80):
        p = run(['docker', 'exec', NODE, 'amcli', 'get', 'info'], check=False)
        if p.returncode == 0:
            break
        time.sleep(0.5)
    else:
        raise RuntimeError('local node did not start')
    run(['docker', 'exec', NODE, 'sh', '-c',
         'mkdir -p /tmp/wallet; amkey --wallet-dir /tmp/wallet '
         '--unix-socket-path /tmp/test-wallet.sock --http-server-address 127.0.0.1:8900 '
         '>/tmp/wallet.log 2>&1 &'])
    time.sleep(1)
    cli('wallet', 'create', '-n', 'test', '--to-console')
    cli('wallet', 'import', '-n', 'test', '--private-key', DEV_KEY)
    for account in (SWAP, 'amax.mtoken', 'usdt.token', 'fake.token', 'alice', 'rejector', ADMIN):
        cli('create', 'account', 'amax', account, PUB, PUB)
    for account, artifact in ((SWAP, 'musdtoxousdt'), ('amax.mtoken', 'swaptoken'),
                              ('usdt.token', 'swaptoken'), ('fake.token', 'swaptoken'), ('rejector', 'swaptoken')):
        for ext in ('wasm', 'abi'):
            run(['docker', 'cp', str(BUILD / f'{artifact}.{ext}'), f'{NODE}:/tmp/{artifact}.{ext}'])
        cli('set', 'contract', account, '/tmp', f'{artifact}.wasm', f'{artifact}.abi', '-p', account)
    permission(True)
    for code, symbol in (('amax.mtoken', 'MUSDT'), ('usdt.token', 'USDT'), ('fake.token', 'MUSDT')):
        push(code, 'mint', ['alice', '1000.000000 ' + symbol], code)
    abi = cli('get', 'abi', SWAP)
    assert {t['name'] for t in abi['tables']} == {'config', 'orders'}
    assert {a['name'] for a in abi['actions']} == {'setconfig'}
    # Removed actions must fail WASM dispatch, even with raw action data.
    for removed_action in ('claim', 'process', 'claimfees'):
        cli('push', 'transaction', json.dumps({'actions': [{
            'account': SWAP, 'name': removed_action,
            'authorization': [{'actor': 'alice', 'permission': 'active'}],
            'data': ''}]}), '-j', '-f', error='8000000000000000000')
    print('Local AMAX node and contracts ready', flush=True)
    # Empty memo queues instead of failing. Incoming swap tokens become liquidity.
    transfer('amax.mtoken', '100.000000 MUSDT')
    assert len(orders()) == 1 and orders()[0]['owner'] == 'alice'
    assert config()['musdt_liquidity'] == '100.000000 MUSDT'
    assert config()['usdt_fees'] == '0.000000 USDT'
    # USDT swap can settle independently, and its deposit releases the MUSDT order.
    transfer('usdt.token', '100.000000 USDT')
    assert orders() == []
    assert config()['musdt_liquidity'] == '0.000000 MUSDT'
    assert config()['usdt_liquidity'] == '0.000000 USDT'
    assert config()['musdt_fees'] == '0.000000 MUSDT'
    assert cli('get', 'table', 'amax.mtoken', ADMIN, 'accounts')['rows'] == [{'balance': '0.300000 MUSDT'}]
    assert config()['usdt_fees'] == '0.000000 USDT'
    assert cli('get', 'table', 'usdt.token', ADMIN, 'accounts')['rows'] == [{'balance': '0.300000 USDT'}]
    for code in ('amax.mtoken', 'usdt.token'):
        assert cli('get', 'table', code, 'alice', 'accounts')['rows'][0]['balance'].startswith('999.700000 ')
    fee_before_refuel = cli('get', 'table', 'usdt.token', ADMIN, 'accounts')['rows']
    transfer('usdt.token', '20.000000 USDT', 'refuel')
    assert cli('get', 'table', 'usdt.token', ADMIN, 'accounts')['rows'] == fee_before_refuel
    assert orders() == [] and config()['usdt_liquidity'] == '20.000000 USDT'
    transfer('amax.mtoken', '30.000000 MUSDT')
    transfer('amax.mtoken', '5.000000 MUSDT')
    queued = orders()
    assert [row['input'] for row in queued] == ['30.000000 MUSDT', '5.000000 MUSDT']
    # Small tail cannot jump a blocked large head. Exactly enough releases only head.
    usdt_before = cli('get', 'table', 'usdt.token', 'alice', 'accounts')['rows'][0]['balance']
    transfer('usdt.token', '10.000000 USDT', 'refuel')
    usdt_after = cli('get', 'table', 'usdt.token', 'alice', 'accounts')['rows'][0]['balance']
    from decimal import Decimal
    assert Decimal(usdt_after.split()[0]) - Decimal(usdt_before.split()[0]) == Decimal('19.910000')
    assert [row['input'] for row in orders()] == ['5.000000 MUSDT']
    transfer('usdt.token', '5.000000 USDT', 'refuel')
    assert orders() == [] and config()['usdt_liquidity'] == '0.000000 USDT'
    print('PASS: open swaps, fee accounting, refuel, independent FIFO queues, no head skipping', flush=True)
    rejected('amax.mtoken', '1.000000 MUSDT', 'Refuel', 'memo must be empty or refuel')
    rejected('fake.token', '1.000000 MUSDT', 'refuel', 'unsupported token contract')
    push('amax.mtoken', 'mint', ['alice', '1.000000 USDT'], 'amax.mtoken')
    rejected('amax.mtoken', '1.000000 USDT', 'refuel', 'unsupported token symbol or precision')
    rejected('amax.mtoken', '0.000001 MUSDT', '', 'amount too small after fee')
    before = snapshot()
    push(SWAP, 'setconfig', [ADMIN, '1.000000 MUSDT', '2.000000 USDT'], 'alice', error='missing authority')
    push(SWAP, 'setconfig', [ADMIN, '0.000000 MUSDT', '1.000000 USDT'], ADMIN, error='expected positive')
    push(SWAP, 'setconfig', [ADMIN, '1.00000 MUSDT', '1.000000 USDT'], ADMIN, error='expected positive')
    assert snapshot() == before
    # Existing order holds its original quote after administrator changes the rate.
    transfer('amax.mtoken', '2.000000 MUSDT')
    push(SWAP, 'setconfig', [ADMIN, '1.000000 MUSDT', '2.000000 USDT'], ADMIN)
    transfer('amax.mtoken', '1.000000 MUSDT')
    assert [row['gross_output'] for row in orders()] == ['2.000000 USDT', '2.000000 USDT']
    transfer('usdt.token', '4.000000 USDT', 'refuel')
    assert orders() == []
    # Reverse exchange uses reciprocal rate.
    transfer('usdt.token', '2.000000 USDT')
    assert orders() == []
    # A failed inline payout must restore incoming tokens, order IDs, pools and fees.
    permission(False)
    rejected('amax.mtoken', '0.500000 MUSDT', '', 'does not have signatures')
    permission(True)
    transfer('amax.mtoken', '0.500000 MUSDT')
    assert config()['admin'] == ADMIN
    print('PASS: input validation, admin authorization, locked quotes, reciprocal rates, atomic payout rollback, automatic fees', flush=True)
    # A batch is bounded; ONLY the next swap/refuel can continue the queue.
    push(SWAP, 'setconfig', [ADMIN, '1.000000 MUSDT', '1.000000 USDT'], ADMIN)
    transfer('amax.mtoken', '1.000000 MUSDT')
    for _ in range(12):
        transfer('amax.mtoken', '1.000000 MUSDT')
    assert len(orders()) == 12
    transfer('usdt.token', '12.000000 USDT', 'refuel')
    assert len(orders()) == 2
    remaining_ids = [row['id'] for row in orders()]
    transfer('amax.mtoken', '1.000000 MUSDT')
    assert len(orders()) == 1 and orders()[0]['id'] > max(remaining_ids)
    transfer('usdt.token', '1.000000 USDT', 'refuel')
    assert orders() == []
    # Transfer administration; old administrator loses access.
    push(SWAP, 'setconfig', ['alice', '1.000000 MUSDT', '1.000000 USDT'], ADMIN)
    push(SWAP, 'setconfig', [ADMIN, '1.000000 MUSDT', '1.000000 USDT'], ADMIN, error='missing authority')
    push(SWAP, 'setconfig', [ADMIN, '1.000000 MUSDT', '1.000000 USDT'], 'alice')
    # Configurable fees: zero and 1%, locked fee for queued orders.
    transfer('amax.mtoken', '10.000000 MUSDT')
    assert orders()[0]['fee'] == '0.030000 USDT'
    push(SWAP, 'setconfig', [ADMIN, '1.000000 MUSDT', '1.000000 USDT', 100], ADMIN)
    transfer('amax.mtoken', '10.000000 MUSDT')
    assert [row['fee'] for row in orders()] == ['0.030000 USDT', '0.100000 USDT']
    transfer('usdt.token', '20.000000 USDT', 'refuel')
    push(SWAP, 'setconfig', [ADMIN, '1.000000 MUSDT', '1.000000 USDT', 10000], ADMIN, error='fee_bps must be 0..9999')
    push(SWAP, 'setconfig', [ADMIN, '1.000000 MUSDT', '1.000000 USDT', 0], ADMIN)
    transfer('usdt.token', '0.000001 USDT')
    # A rejecting fee destination must roll back both payouts and fee accounting.
    cli('set', 'contract', ADMIN, '/tmp', 'swaptoken.wasm', 'swaptoken.abi', '-p', ADMIN)
    push(SWAP, 'setconfig', [ADMIN, '1.000000 MUSDT', '1.000000 USDT', 30], ADMIN)
    transfer('usdt.token', '1.000000 USDT', 'refuel')
    rejected('amax.mtoken', '1.000000 MUSDT', '', 'recipient refuses payout')
    push(ADMIN, 'setreject', [False], ADMIN)
    transfer('amax.mtoken', '1.000000 MUSDT')
    push(SWAP, 'setconfig', [ADMIN, '1.000000 MUSDT', '1.000000 USDT', 0], ADMIN)
    # Direct batch payments are atomic, including earlier successful transfers.
    auth = {'threshold': 1, 'keys': [{'key': PUB, 'weight': 1}],
            'accounts': [{'permission': {'actor': 'rejector', 'permission': 'amax.code'}, 'weight': 1}], 'waits': []}
    cli('set', 'account', 'permission', 'rejector', 'active', json.dumps(auth), 'owner', '-p', 'rejector')
    push('amax.mtoken', 'mint', ['rejector', '2.000000 MUSDT'], 'amax.mtoken')
    transfer('amax.mtoken', '1.000000 MUSDT')
    push('rejector', 'exchange', ['amax.mtoken', SWAP, '2.000000 MUSDT'], 'rejector')
    before = snapshot()
    transfer('usdt.token', '3.000000 USDT', 'refuel', error='recipient refuses payout')
    assert snapshot() == before
    assert len(orders()) == 2
    push('rejector', 'setreject', [False], 'rejector')
    transfer('usdt.token', '3.000000 USDT', 'refuel')
    assert orders() == []
    assert cli('get', 'table', 'usdt.token', 'rejector', 'accounts')['rows'] == [{'balance': '2.000000 USDT'}]
    from decimal import Decimal
    state = config()
    for code, sym, field in [('amax.mtoken', 'MUSDT', 'musdt'), ('usdt.token', 'USDT', 'usdt')]:
        actual = cli('get', 'table', code, SWAP, 'accounts')['rows']
        amount = lambda value: Decimal(value.split()[0])
        actual_balance = next(amount(row['balance']) for row in actual if row['balance'].endswith(' ' + sym))
        accounted = amount(state[field + '_liquidity']) + amount(state[field + '_fees'])
        assert actual_balance == accounted, (sym, actual_balance, accounted)
    print('PASS: configurable/locked fees, zero fee dust, automatic queued payouts, atomic batch rollback and retry', flush=True)
    print('PASS: bounded batches, swap/refuel-only FIFO continuation, admin transfer', flush=True)
finally:
    run(['docker', 'rm', '-f', NODE], check=False)
