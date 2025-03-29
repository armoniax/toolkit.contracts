#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/permission.hpp>
#include <eosio/action.hpp>

#include <string>
#include <amaxapplybps/amaxapplybps.db.hpp>

#include <amax_system.hpp>
#include <wasm_db.hpp>

namespace amax {

using std::string;
using std::vector;

#define TRANSFER(bank, to, quantity, memo) \
    {	mtoken::transfer_action act{ bank, { {_self, active_perm} } };\
			act.send( _self, to, quantity , memo );}
         
using namespace wasm::db;
using namespace eosio;

static constexpr name      NFT_BANK    = "did.ntoken"_n;
static constexpr name      SYS_CONTRACT= "amax"_n;
static constexpr eosio::name active_perm{"active"_n};


enum class err: uint8_t {
   NONE                 = 0,
   RECORD_NOT_FOUND     = 1,
   RECORD_EXISTING      = 2,
   SYMBOL_MISMATCH      = 4,
   PARAM_ERROR          = 5,
   MEMO_FORMAT_ERROR    = 6,
   PAUSED               = 7,
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
   NEED_REQUIRED_CHECK  = 20

};

/**
 * The `amaxapplybps` sample system contract defines the structures and actions that allow users to create, issue, and manage tokens for AMAX based blockchains. It demonstrates one way to implement a smart contract which allows for creation and management of tokens. It is possible for one to create a similar contract which suits different needs. However, it is recommended that if one only needs a token with the below listed actions, that one uses the `amaxapplybps` contract instead of developing their own.
 *
 * The `amaxapplybps` contract class also implements two useful public static methods: `get_supply` and `get_balance`. The first allows one to check the total supply of a specified token, created by an account and the second allows one to check the balance of a token for a specified account (the token creator account has to be specified as well).
 *
 * The `amaxapplybps` contract manages the set of tokens, accounts and their corresponding balances, by using two internal multi-index structures: the `accounts` and `stats`. The `accounts` multi-index table holds, for each row, instances of `account` object and the `account` object holds information about the balance of one token. The `accounts` table is scoped to an eosio account, and it keeps the rows indexed based on the token's symbol.  This means that when one queries the `accounts` multi-index table for an account name the result is all the tokens that account holds at the moment.
 *
 * Similarly, the `stats` multi-index table, holds instances of `currency_stats` objects for each row, which contains information about current supply, maximum supply, and the creator account for a symbol token. The `stats` table is scoped to the token symbol.  Therefore, when one queries the `stats` table for a token symbol the result is one single entry/row corresponding to the queried symbol token if it was previously created, or nothing, otherwise.
 */
class [[eosio::contract("amaxapplybps")]] amaxapplybps : public contract {
   
   private:
      dbc                 _dbc;
   public:
      using contract::contract;
  
   amaxapplybps(eosio::name receiver, eosio::name code, datastream<const char*> ds): contract(receiver, code, ds),
         _dbc(get_self()),
         _global(get_self(), get_self().value),
         _globalscan(get_self(), get_self().value)
    {
        _gstate = _global.exists() ? _global.get() : global_t{};
         _gstate_scan = _globalscan.exists() ? _globalscan.get() : globalscan_t{};
        
    }

    ~amaxapplybps() { _global.set( _gstate, get_self() );
                     _globalscan.set( _gstate_scan, get_self() ); }     

   ACTION init( const name& admin, const name& bbp_contract, const uint32_t& total_bbps_count){
      require_auth( _self );
      _gstate.admin              = admin;
      _gstate.bbp_contract       = bbp_contract;
      _gstate.total_bbps_count   = total_bbps_count;
   }

   ACTION addproducer(const name& submiter, const name& producter,
                     const eosio::public_key& mpubkey,
                     const string& url, uint16_t location,
                     std::optional<uint32_t> reward_shared_ratio);

   ACTION refreshbbp(const uint32_t& count);

   ACTION setinterval(const uint32_t& scan_interval_minutes);

   // ACTION setstatus( const name& submiter, const name& owner, const name& status);


   private:
      global_singleton    _global;
      global_t            _gstate;
      globalscan_singleton _globalscan;
      globalscan_t        _gstate_scan;


      inline eosio::block_signing_authority convert_to_block_signing_authority( const eosio::public_key& producer_key ) {
         return eosio::block_signing_authority_v0{ .threshold = 1, .keys = {{producer_key, 1}} };
      }

      void _set_producer(const name& owner,
                  const string& logo_uri,
                  const string& org_name,
                  const string& org_info,
                  const name& dao_code,
                  const string& reward_shared_plan,
                  const string& manifesto,
                  const string& issuance_plan);
};
} //namespace amax
