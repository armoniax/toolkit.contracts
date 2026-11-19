#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <string>

class [[eosio::contract("musdtoxousdt")]] musdtoxousdt : public eosio::contract {
public:
    using contract::contract;

    [[eosio::action]]
    void setconfig(eosio::name admin, eosio::asset musdt_rate, eosio::asset usdt_rate, uint16_t fee_bps);

    [[eosio::action]]
    void claimfees(eosio::name to, eosio::asset quantity);

    [[eosio::on_notify("*::transfer")]]
    void ontransfer(eosio::name from, eosio::name to,
                    eosio::asset quantity, std::string memo);

private:
    static constexpr eosio::name musdt_contract = eosio::name("amax.mtoken");
    static constexpr eosio::name usdt_contract = eosio::name("usdt.token");
    static constexpr eosio::symbol musdt_symbol = eosio::symbol("MUSDT", 6);
    static constexpr eosio::symbol usdt_symbol = eosio::symbol("USDT", 6);

    struct [[eosio::table("config")]] config_row {
        eosio::name admin;
        eosio::asset musdt_rate{1000000, musdt_symbol};
        eosio::asset usdt_rate{1000000, usdt_symbol};
        uint16_t fee_bps = 30;
        uint64_t next_id = 1;
        eosio::asset musdt_liquidity{0, musdt_symbol};
        eosio::asset usdt_liquidity{0, usdt_symbol};
        eosio::asset musdt_fees{0, musdt_symbol};
        eosio::asset usdt_fees{0, usdt_symbol};
        EOSLIB_SERIALIZE(config_row, (admin)(musdt_rate)(usdt_rate)(fee_bps)(next_id)(musdt_liquidity)(usdt_liquidity)(musdt_fees)(usdt_fees))
    };
    using config_table = eosio::singleton<"config"_n, config_row>;

    struct [[eosio::table("orders")]] order_row {
        uint64_t id;
        eosio::name owner;
        eosio::asset input;
        eosio::asset gross_output;
        eosio::asset fee;
        eosio::time_point_sec created_at;
        uint64_t primary_key() const { return id; }
        uint128_t by_direction() const {
            return (static_cast<uint128_t>(input.symbol == musdt_symbol ? 0 : 1) << 64) | id;
        }
        EOSLIB_SERIALIZE(order_row, (id)(owner)(input)(gross_output)(fee)(created_at))
    };
    using orders_table = eosio::multi_index<"orders"_n, order_row,
        eosio::indexed_by<"bydirection"_n, eosio::const_mem_fun<order_row, uint128_t, &order_row::by_direction>>>;

    // External token layout only; excluded from our published ABI by clean_abi.py.
    struct token_account {
        eosio::asset balance;
        uint64_t primary_key() const { return balance.symbol.code().raw(); }
        EOSLIB_SERIALIZE(token_account, (balance))
    };
    using token_accounts = eosio::multi_index<"accounts"_n, token_account>;

    void process_queue();
    eosio::asset balance(eosio::name token_contract, eosio::symbol symbol) const;
    config_row config() const;
    void send(eosio::name to, eosio::asset quantity, const std::string& memo) const;
};
