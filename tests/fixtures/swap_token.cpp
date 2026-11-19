// Local-chain test fixture only. Not a production token contract.
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/singleton.hpp>
#include <tuple>
using namespace eosio;
class [[eosio::contract("swaptoken")]] swaptoken : public contract {
public:
    using contract::contract;
    struct [[eosio::table]] account {
        asset balance;
        uint64_t primary_key() const { return balance.symbol.code().raw(); }
    };
    using accounts = multi_index<"accounts"_n, account>;
    [[eosio::action]] void mint(name to, asset quantity) {
        require_auth(get_self());
        check(quantity.is_valid() && quantity.amount > 0, "invalid quantity");
        add(to, quantity);
    }
    [[eosio::action]] void transfer(name from, name to, asset quantity, std::string memo) {
        require_auth(from);
        check(from != to && is_account(to), "invalid recipient");
        check(quantity.is_valid() && quantity.amount > 0, "invalid quantity");
        accounts rows(get_self(), from.value);
        auto itr = rows.require_find(quantity.symbol.code().raw(), "no balance");
        check(itr->balance >= quantity, "overdrawn balance");
        rows.modify(itr, same_payer, [&](auto& row) { row.balance -= quantity; });
        add(to, quantity);
        require_recipient(from);
        require_recipient(to);
    }
    [[eosio::action]] void exchange(name bank, name pool, asset quantity) {
        require_auth(get_self());
        action(permission_level{get_self(), "active"_n}, bank, "transfer"_n,
               std::make_tuple(get_self(), pool, quantity, std::string(""))).send();
    }
    struct [[eosio::table("rejectcfg")]] reject_config {
        bool reject = true;
    };
    using reject_table = singleton<"rejectcfg"_n, reject_config>;
    [[eosio::action]] void setreject(bool reject) {
        require_auth(get_self());
        reject_table(get_self(), get_self().value).set(reject_config{reject}, get_self());
    }
    [[eosio::on_notify("*::transfer")]]
    void ontransfer(name from, name to, asset quantity, std::string memo) {
        if (to == get_self() && memo == "swap payout") {
            check(!reject_table(get_self(), get_self().value).get_or_default().reject,
                  "recipient refuses payout");
        }
    }
private:
    void add(name to, asset quantity) {
        accounts rows(get_self(), to.value);
        auto itr = rows.find(quantity.symbol.code().raw());
        if (itr == rows.end()) rows.emplace(get_self(), [&](auto& row) { row.balance = quantity; });
        else rows.modify(itr, same_payer, [&](auto& row) { row.balance += quantity; });
    }
};
