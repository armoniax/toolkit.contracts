#include "musdtoxousdt.hpp"
#include "swap_math.hpp"
#include <limits>
#include <tuple>

using namespace eosio;

asset musdtoxousdt::balance(name token_contract, symbol token_symbol) const {
    token_accounts accounts(token_contract, get_self().value);
    const auto itr = accounts.find(token_symbol.code().raw());
    if (itr == accounts.end()) return asset(0, token_symbol);
    check(itr->balance.symbol == token_symbol, "token balance precision mismatch");
    return itr->balance;
}

musdtoxousdt::config_row musdtoxousdt::config() const {
    config_table table(get_self(), get_self().value);
    auto state = table.get_or_default();
    if (!state.admin.value) state.admin = fee_receiver;
    return state;
}

void musdtoxousdt::send(name to, asset quantity, const std::string& memo) const {
    const auto bank = quantity.symbol == musdt_symbol ? musdt_contract : usdt_contract;
    action(permission_level{get_self(), name("active")}, bank, name("transfer"),
           std::make_tuple(get_self(), to, quantity, memo)).send();
}

void musdtoxousdt::setconfig(name admin, asset musdt_rate, asset usdt_rate, uint16_t fee_bps) {
    auto state = config();
    require_auth(state.admin);
    check(is_account(admin), "admin account does not exist");
    check(is_account(fee_receiver), "fee receiver account does not exist");
    check(musdt_rate.is_valid() && musdt_rate.symbol == musdt_symbol && musdt_rate.amount > 0,
          "expected positive 6-decimal MUSDT rate");
    check(usdt_rate.is_valid() && usdt_rate.symbol == usdt_symbol && usdt_rate.amount > 0,
          "expected positive 6-decimal USDT rate");
    check(fee_bps < 10000, "fee_bps must be 0..9999");
    state.fee_bps = fee_bps;
    state.admin = admin;
    state.musdt_rate = musdt_rate;
    state.usdt_rate = usdt_rate;
    config_table(get_self(), get_self().value).set(state, get_self());
    pay_legacy_fees();
}

void musdtoxousdt::pay_legacy_fees() {
    auto state = config();
    const auto musdt = state.musdt_fees;
    const auto usdt = state.usdt_fees;
    if (musdt.amount == 0 && usdt.amount == 0) return;
    state.musdt_fees.amount = 0;
    state.usdt_fees.amount = 0;
    config_table(get_self(), get_self().value).set(state, get_self());
    if (musdt.amount > 0) send(fee_receiver, musdt, "swap fees");
    if (usdt.amount > 0) send(fee_receiver, usdt, "swap fees");
}

void musdtoxousdt::ontransfer(name from, name to, asset quantity, std::string memo) {
    if (from == get_self() || to != get_self()) return;
    const auto bank = get_first_receiver();
    check(bank == musdt_contract || bank == usdt_contract, "unsupported token contract");
    require_auth(from);
    const bool is_musdt = bank == musdt_contract;
    check(quantity.is_valid() && quantity.symbol == (is_musdt ? musdt_symbol : usdt_symbol),
          "unsupported token symbol or precision");
    check(quantity.amount > 0, "quantity must be positive");
    check(memo.empty() || memo == "refuel", "memo must be empty or refuel");

    auto state = config();
    auto& liquidity = is_musdt ? state.musdt_liquidity : state.usdt_liquidity;
    liquidity += quantity;
    if (memo.empty()) {
        const auto numerator = is_musdt ? state.usdt_rate.amount : state.musdt_rate.amount;
        const auto denominator = is_musdt ? state.musdt_rate.amount : state.usdt_rate.amount;
        const auto gross = swap_math::gross(quantity.amount, numerator, denominator);
        check(gross > 0 && gross <= asset::max_amount, "output amount out of range");
        const asset output(static_cast<int64_t>(gross), is_musdt ? usdt_symbol : musdt_symbol);
        const asset fee(swap_math::fee(output.amount, state.fee_bps), output.symbol);
        check(output.amount > fee.amount, "amount too small after fee");
        check(state.next_id < std::numeric_limits<uint64_t>::max(), "order id exhausted");
        orders_table orders(get_self(), get_self().value);
        orders.emplace(get_self(), [&](auto& row) {
            row.id = state.next_id++;
            row.owner = from;
            row.input = quantity;
            row.gross_output = output;
            row.fee = fee;
            row.created_at = time_point_sec(current_time_point());
        });
    }
    config_table(get_self(), get_self().value).set(state, get_self());
    process_queue();
}

void musdtoxousdt::process_queue() {
    // Bound CPU use; remaining orders wait for the next swap or refuel.
    constexpr uint16_t max_orders = 10;
    auto state = config();
    orders_table orders(get_self(), get_self().value);
    auto index = orders.get_index<"bydirection"_n>();
    uint16_t processed = 0;
    bool progress = true;
    while (processed < max_orders && progress) {
        progress = false;
        for (uint8_t direction = 0; direction < 2 && processed < max_orders; ++direction) {
            const auto itr = index.lower_bound(static_cast<uint128_t>(direction) << 64);
            if (itr == index.end() || (itr->input.symbol == musdt_symbol ? 0 : 1) != direction) continue;
            const auto gross = itr->gross_output;
            auto& liquidity = direction == 0 ? state.usdt_liquidity : state.musdt_liquidity;
            if (liquidity < gross) continue; // Never skip the head of this direction.
            const auto fee = itr->fee;
            const auto payout = gross - fee;
            const auto owner = itr->owner;
            check(balance(direction == 0 ? usdt_contract : musdt_contract, gross.symbol) >= payout,
                  "insufficient payout balance");
            liquidity -= gross;
            index.erase(itr);
            // Reserve every payout before inline transfers execute. Failure of
            // any transfer rolls back orders, liquidity, fees and the deposit.
            config_table(get_self(), get_self().value).set(state, get_self());
            send(owner, payout, "swap payout");
            if (fee.amount > 0) send(fee_receiver, fee, "swap fees");
            ++processed;
            progress = true;
        }
    }
}
