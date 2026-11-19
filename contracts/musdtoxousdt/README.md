# musdtoxousdt

AMAX 上的开放式 MUSDT / USDT 兑换池。

- MUSDT：`amax.mtoken`，6 位精度。
- USDT：`usdt.token`，6 位精度。
- 空备注：创建兑换订单，兑出的币归转入者；收到的币同时进入公共流动性。
- 精确备注 `refuel`：仅补充所转入币种的公共流动性，不创建订单，不获得兑换币、份额或退款权。
- 其他备注、币种或精度均拒绝，整笔转账回滚。
- 默认比例为 `1 MUSDT = 1 USDT`。初始管理员为 `amgenesisdao`；仅当前管理员可修改比例和管理员。
- 手续费默认为兑出总额的 **0.3%**，管理员可通过 `fee_bps` 配置（30 = 0.3%，范围 0–9999，即 0% 至 99.99%），使用兑出币种收取，每笔成交直接转到固定收款账户 `amgenesisdao`，不计入可用流动性。`claimfees` 独立动作已移除；即使之后变更管理员，手续费收款地址仍固定为 `amgenesisdao`。

## 流动性、队列及到账

可兑换额度来自 `config` 表的 `musdt_liquidity` / `usdt_liquidity`，不再单独手工设置额度。新手续费会立即转走。`musdt_fees` / `usdt_fees` 字段为保持已部署表结构兼容而保留，仅表示旧版本未转出的手续费；升级时通过 `setconfig` 将其自动转给 `amgenesisdao` 并清零，新订单不再增加这些字段。不要直接 issue/mint 给合约：没有 transfer 通知的入账不会记为流动性。

每个方向独立 FIFO。目标流动性不足时，订单保留在 `orders` 表，不报额度不足错误；队首必须整单成交，不会拆单，也不会让小订单插队。比例和手续费在订单提交时锁定，管理员后续修改不影响已有订单。

每次兑换或 `refuel` 转入最多自动成交并直接付款 10 单，包括此前排队的订单。超过本次上限的订单保留在队列中，等待下一笔兑换或 `refuel` 转入继续处理；新订单不会越过同方向的旧订单。成交时将扣除手续费后的兑换币直接转给原付款人，不需要领取动作。

队列处理仅是合约内部私有函数，不提供 `process` 独立 action，不需要 keeper。没有新转入时，即使池中流动性充足，剩余订单也不会自行执行。

`credits` 表和 `claim` 动作已移除；升级前必须确认旧表为空。

直接付款采用整笔交易原子执行：如果用户收款合约或手续费收款合约拒收代币，本批付款及触发本批处理的转入都会回滚，订单和费用保持原状；该方向的拒收队首仍会阻塞后续成交。这是直接付款与原先隔离领取方式的行为差异。

转入的兑换本金会立即加入池子，可用于另一方向成交，因此订单没有取消/退款接口。余额不足时需等待流动性补充；链上不会自行定时执行，仅由下一笔兑换或 `refuel` 转入触发。

采用整数运算：总兑出金额向下取整至 6 位，手续费向上取整至最小单位；净兑出为零的订单拒绝。默认比例下转入 `100.000000 MUSDT`，获得 `99.700000 USDT`，同时将 `0.300000 USDT` 手续费转给 `amgenesisdao`。

## 构建与本地回归

```sh
# 使用现有 Docker CDT wrapper，也可通过 CDT_CPP 指定 AMAX CDT。
CDT_CPP=/Users/ray/.local/bin/flon-cdt-tool contracts/musdtoxousdt/build.sh
CDT_CPP=/Users/ray/.local/bin/flon-cdt-tool contracts/musdtoxousdt/build.sh --test
```

产物位于 `build/musdtoxousdt/`。脚本使用 ABI 1.1，并将兼容 FLON CDT 的 ABI 前缀转为主网采用的 `amax::abi/1.1`。本地回归使用独立 Docker AMAX 1.0.4 节点和测试代币，不访问主网，不使用真实钱包，结束后删除测试容器。原生 CMake 构建也已加入目标 `musdtoxousdt`。两种构建入口均通过 `clean_abi.py` 排除读取外部代币余额所用的 `accounts` 表及 `token_account` 类型，发布的表仅有 `config`、`orders`。

## 操作示例

以下假设 `amcli` 已配置正确 RPC 和钱包。部署后需要为合约 active 权限添加自身 `amax.code`，保留原有签名权限，才能支付代币。以下仅为操作说明，构建和测试不会部署或修改主网权限。

```sh
# 只补充 USDT 流动性
amcli push action usdt.token transfer '["alice","musdtoxousdt","100.000000 USDT","refuel"]' -p alice

# 兑换 MUSDT -> USDT；目标流动性不足则排队
amcli push action amax.mtoken transfer '["alice","musdtoxousdt","10.000000 MUSDT",""]' -p alice

# 管理员设置 1 MUSDT = 2 USDT；反向使用倒数
amcli push action musdtoxousdt setconfig '["amgenesisdao","1.000000 MUSDT","2.000000 USDT",30]' -p amgenesisdao

amcli get table musdtoxousdt musdtoxousdt config
amcli get table musdtoxousdt musdtoxousdt orders
```

AMAX 不允许在转账通知中向转入者收费，因此排队订单的 RAM 由合约承担；成交后订单行删除并退回相应 RAM。部署时需为公开排队预留 RAM。已完成的订单及资金流可通过链上交易历史追踪。
