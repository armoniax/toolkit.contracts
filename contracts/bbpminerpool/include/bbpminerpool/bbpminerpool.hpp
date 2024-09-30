#pragma once

#include <eosio/asset.hpp>
#include <eosio/action.hpp>

#include <string>

#include <bbpminerpool/bbpminerpool.db.hpp>
#include <amaxapplybps.hpp>
#include <wasm_db.hpp>
#include <optional>
#include <map>
#include <amax.token.hpp>

namespace amax {

using std::string;
using std::vector;
         
using namespace wasm::db;
using namespace eosio;

static constexpr name      AMAX_BANK    = "amax.token"_n;
static constexpr eosio::name active_perm{"active"_n};


enum class err: uint8_t {
   NONE                 = 0,
   RECORD_NOT_FOUND     = 1,
   RECORD_EXISTING      = 2,
   SYMBOL_MISMATCH      = 4,
   INVALID_PARAM        = 5,
   MEMO_FORMAT_ERROR    = 6,
   OVER_QUOTA           = 7,
   NO_AUTH              = 8,
   NOT_POSITIVE         = 9,
   NOT_STARTED          = 10,
   OVERSIZED            = 11,
   TIME_EXPIRED         = 12,
   NOTIFY_UNRELATED     = 13,
   ACTION_REDUNDANT     = 14,
   ACCOUNT_INVALID      = 15,
   FEE_INSUFFICIENT     = 16,
   FIRST_CREATOR        = 17,
   STATUS_ERROR         = 18,
   SCORE_NOT_ENOUGH     = 19,
   NEED_REQUIRED_CHECK  = 20,
   INSUFFICIENT_FUNDS   = 21,
   TIME_ERROR           = 22,

};

/**
 * The `bbpminerpool` sample system contract defines the structures and actions that allow users to create, issue, and manage tokens for AMAX based blockchains. It demonstrates one way to implement a smart contract which allows for creation and management of tokens. It is possible for one to create a similar contract which suits different needs. However, it is recommended that if one only needs a token with the below listed actions, that one uses the `bbpminerpool` contract instead of developing their own.
 *
 * The `bbpminerpool` contract class also implements two useful public static methods: `get_supply` and `get_balance`. The first allows one to check the total supply of a specified token, created by an account and the second allows one to check the balance of a token for a specified account (the token creator account has to be specified as well).
 *
 * The `bbpminerpool` contract manages the set of tokens, accounts and their corresponding balances, by using two internal multi-index structures: the `accounts` and `stats`. The `accounts` multi-index table holds, for each row, instances of `account` object and the `account` object holds information about the balance of one token. The `accounts` table is scoped to an eosio account, and it keeps the rows indexed based on the token's symbol.  This means that when one queries the `accounts` multi-index table for an account name the result is all the tokens that account holds at the moment.
 *
 * Similarly, the `stats` multi-index table, holds instances of `currency_stats` objects for each row, which contains information about current supply, maximum supply, and the creator account for a symbol token. The `stats` table is scoped to the token symbol.  Therefore, when one queries the `stats` table for a token symbol the result is one single entry/row corresponding to the queried symbol token if it was previously created, or nothing, otherwise.
 */
class [[eosio::contract("bbpminerpool")]] bbpminerpool : public contract {

   #define CHECKC(exp, code, msg) \
   { if (!(exp)) eosio::check(false, string("[[") + to_string((int)code) + string("]] ")  \
                                    + string("[[") + _self.to_string() + string("]] ") + msg); }

   static constexpr eosio::name AMAX_BANK{"amax.token"_n};

   private:
      dbc                 _dbc;
   public:
      using contract::contract;
  
   bbpminerpool(eosio::name receiver, eosio::name code, datastream<const char*> ds): contract(receiver, code, ds),
         _dbc(get_self()),
         _global(get_self(), get_self().value),
         _voter_t(get_self(), get_self().value)

    {
        _gstate = _global.exists() ? _global.get() : global_t{};
    }

    ~bbpminerpool() { 
      _global.set( _gstate, get_self() );
     }
   
   ACTION init( const name& admin, const asset& pool_vote_quota, const name& pool_main_voter){
      require_auth( _self );
      _gstate.admin              = admin;
      _gstate.pool_vote_quota    = pool_vote_quota;
      _gstate.pool_main_voter    = pool_main_voter;
   }

   ACTION addrewarder( const name& rewarder){
      require_auth( _self );
      _gstate.rewarders.insert(rewarder);
   }
   ACTION delrewarder( const name& rewarder){
      require_auth( _self );
      _gstate.rewarders.erase(rewarder);
   } 

   ACTION addvoter( const name& voter){
      require_auth( _self );

      CHECKC(is_account(voter), err::INVALID_PARAM, "invalid account");
      _voter_t.emplace( _self, [&]( auto& r ) {
         r.account         = voter;
         r.amount          = asset(0, AMAX_SYMBOL);
         r.total_claimed   = asset(0, AMAX_SYMBOL);
         r.created_at      = current_time_point();
      });
   }

   ACTION delvoter( const name& voter){
      require_auth( _self );
      auto itr = _voter_t.find( voter.value );
      CHECKC( itr != _voter_t.end(), err::RECORD_NOT_FOUND, "voter not found" );
      CHECKC( itr->amount.amount == 0, err::INVALID_PARAM, "invalid amount" );
      _voter_t.erase( itr );
   }

   [[eosio::on_notify("*::transfer")]]
   void ontoken_transfer( name from, name to, asset quantity, string memo );

   ACTION redeem(const name& account) {
      require_auth( account );
      auto itr = _voter_t.find( account.value );
      CHECKC( itr != _voter_t.end(),  err::RECORD_NOT_FOUND,   "not a voter" );
      CHECKC( itr->amount.amount > 0, err::INVALID_PARAM,      "nothing to redeem amount" );
      auto amount = itr->amount;
      _voter_t.modify( itr, get_self(), [&]( auto& r ) {
         r.amount = asset(0, AMAX_SYMBOL);
         r.updated_at = current_time_point();
      });
      _gstate.total_vote_recd -= amount;
      TRANSFER( AMAX_BANK, account, amount, "reward" );
   }

   private:
      global_singleton           _global;
      global_t                   _gstate;
      voter_t::idx_t             _voter_t;
      asset _get_reward(const asset& pool_vote_quota, const asset& vote_quant, const asset& reward) {
         auto amount = (vote_quant.amount *10000 / pool_vote_quota.amount) * reward.amount / 10000;
         return asset(amount, AMAX_SYMBOL);
      }

};
} //namespace amax
