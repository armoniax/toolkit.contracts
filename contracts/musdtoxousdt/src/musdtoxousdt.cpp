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
    if (!state.admin.value) state.admin = get_self();
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
}

void musdtoxousdt::claimfees(name to, asset quantity) {
    auto state = config();
    require_auth(state.admin);
    check(is_account(to) && to != get_self(), "invalid fee recipient");
    check(quantity.is_valid() && quantity.amount > 0, "quantity must be positive");
    check(quantity.symbol == musdt_symbol || quantity.symbol == usdt_symbol,
          "unsupported fee symbol or precision");
    auto& fees = quantity.symbol == musdt_symbol ? state.musdt_fees : state.usdt_fees;
    check(quantity <= fees, "insufficient collected fees");
    fees -= quantity;
    config_table(get_self(), get_self().value).set(state, get_self());
    send(to, quantity, "swap fees");
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
    process(10);
    // Deliver any legacy credit too; new orders pay directly in process().
    if (memo.empty()) claim(from);
}

void musdtoxousdt::process(uint16_t max_orders) {
    check(max_orders > 0 && max_orders <= 20, "max_orders must be 1..20");
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
            auto& fees = direction == 0 ? state.usdt_fees : state.musdt_fees;
            const auto payout = gross - itr->fee;
            const auto owner = itr->owner;
            check(balance(direction == 0 ? usdt_contract : musdt_contract, gross.symbol) >= payout,
                  "insufficient payout balance");
            liquidity -= gross;
            fees += itr->fee;
            index.erase(itr);
            // Reserve every payout before inline transfers execute. Failure of
            // any transfer rolls back orders, liquidity, fees and the deposit.
            config_table(get_self(), get_self().value).set(state, get_self());
            send(owner, payout, "swap payout");
            ++processed;
            progress = true;
        }
    }
}

void musdtoxousdt::claim(name owner) {
    // Backward compatibility for credits created before direct payouts.
    // New orders never write this table. Delivery always goes to the owner.
    credits_table credits(get_self(), get_self().value);
    const auto itr = credits.find(owner.value);
    if (itr == credits.end()) return;
    const auto musdt = itr->musdt;
    const auto usdt = itr->usdt;
    credits.modify(itr, same_payer, [&](auto& row) {
        row.musdt.amount = 0;
        row.usdt.amount = 0;
    });
    if (musdt.amount > 0) send(owner, musdt, "swap payout");
    if (usdt.amount > 0) send(owner, usdt, "swap payout");
}
