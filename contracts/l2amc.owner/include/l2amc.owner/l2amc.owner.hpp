#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/permission.hpp>
#include <eosio/action.hpp>

#include <string>

#include <l2amc.owner/l2amc.owner.db.hpp>
#include <wasm_db.hpp>
#include <amax_system.hpp>

namespace amax {

using std::string;
using std::vector;


#define TRANSFER(bank, to, quantity, memo) \
    {	mtoken::transfer_action act{ bank, { {_self, active_perm} } };\
			act.send( _self, to, quantity , memo );}
         
using namespace wasm::db;
using namespace eosio;
using namespace wasm;
#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

static constexpr name      NFT_BANK    = "did.ntoken"_n;
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
   NEED_MANUAL_CHECK    = 20
};
namespace vendor_info_status {
    static constexpr eosio::name RUNNING            = "running"_n;
    static constexpr eosio::name STOP               = "stop"_n;
};

/**
 * The `l2amc.owner` sample system contract defines the structures and actions that allow users to create, issue, and manage tokens for AMAX based blockchains. It demonstrates one way to implement a smart contract which allows for creation and management of tokens. It is possible for one to create a similar contract which suits different needs. However, it is recommended that if one only needs a token with the below listed actions, that one uses the `l2amc.owner` contract instead of developing their own.
 *
 * The `l2amc.owner` contract class also implements two useful public static methods: `get_supply` and `get_balance`. The first allows one to check the total supply of a specified token, created by an account and the second allows one to check the balance of a token for a specified account (the token creator account has to be specified as well).
 *
 * The `l2amc.owner` contract manages the set of tokens, accounts and their corresponding balances, by using two internal multi-index structures: the `accounts` and `stats`. The `accounts` multi-index table holds, for each row, instances of `account` object and the `account` object holds information about the balance of one token. The `accounts` table is scoped to an eosio account, and it keeps the rows indexed based on the token's symbol.  This means that when one queries the `accounts` multi-index table for an account name the result is all the tokens that account holds at the moment.
 *
 * Similarly, the `stats` multi-index table, holds instances of `currency_stats` objects for each row, which contains information about current supply, maximum supply, and the creator account for a symbol token. The `stats` table is scoped to the token symbol.  Therefore, when one queries the `stats` table for a token symbol the result is one single entry/row corresponding to the queried symbol token if it was previously created, or nothing, otherwise.
 */
class [[eosio::contract("l2amc.owner")]] l2amc_owner : public contract {
   
   private:
      dbc                 _dbc;
   public:
      using contract::contract;
  
   l2amc_owner(eosio::name receiver, eosio::name code, datastream<const char*> ds): contract(receiver, code, ds),
         _dbc(get_self()),
         _global(get_self(), get_self().value),
         _global2(get_self(), get_self().value)
    {
        _gstate = _global.exists() ? _global.get() : global_t{};
        _gstate2 = _global2.exists() ? _global2.get() : global2_t{};
    }
    ~l2amc_owner() { _global.set( _gstate, get_self() ); }

   ACTION init(         const name& admin, 
                        const name& amc_auther, 
                        const asset& stake_net_quantity, 
                        const asset& stake_cpu_quantity);

   ACTION bind( 
            const name& proxy_contract,
            const name& l2amc_name,        //eth,bsc,btc,trx
            const string& xchain_pubkey, 
            const name& owner,
            const name& creator,
            const eosio::public_key& recovered_public_key);


   ACTION updateauth(const name& proxy_contract, const name& owner, const eosio::public_key& amc_pubkey);

   ACTION execaction(const name& proxy_contract, const name& owner, const name& l2amc_name, const vector<eosio::action>& actions,const string& nonce);

   ACTION setoracle( const name& oracle, const bool& to_add ) {
      require_auth( _gstate.admin );
      check( is_account( oracle ), oracle.to_string() + ": invalid account" );
         bool found = ( _gstate.amc_authers.find( oracle ) != _gstate.amc_authers.end() );
         if (to_add) {
            check( !found, "oracle already added" );
            _gstate.amc_authers.insert( oracle );
         } else {
            check( found, "oracle not found" );
            _gstate.amc_authers.erase( oracle );
         }
   
   }

   ACTION setadmin( const name& admin ) {
      require_auth(_self) ;
      check( is_account( admin ), admin.to_string() + ": invalid account" );
      _gstate.admin = admin;
   }


   ACTION setproxyact( const name& proxy_action ) {
      require_auth(_self) ;
      _gstate2.proxy_action = proxy_action;
      _global2.set( _gstate2, get_self() ); 
   }

    private:
        global_singleton    _global;
        global_t            _gstate;

        global2_singleton    _global2;
        global2_t            _gstate2;

   private:
      void _updateauth( const name& owner,const name& proxy_contract,  const eosio::public_key& amc_pubkey);

      void _newaccount(  const name& creator, const name& account);

      checksum256 sha256pk(eosio::public_key amc_pubkey) {
         auto packed_data = pack(amc_pubkey); 
         return sha256(packed_data.data(),packed_data.size());
      }  
       void _txid(checksum256& txid);
};


} //namespace amax
