#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/permission.hpp>
#include <eosio/action.hpp>
#include <eosio/crypto.hpp>
#include <string>

namespace amax {

using std::string;
using std::vector;
using namespace eosio;

#define hash(str) sha256(const_cast<char*>(str.c_str()), str.size())
/**
 * The `l2amc.owner` sample system contract defines the structures and actions that allow users to create, issue, and manage tokens for AMAX based blockchains. It demonstrates one way to implement a smart contract which allows for creation and management of tokens. It is possible for one to create a similar contract which suits different needs. However, it is recommended that if one only needs a token with the below listed actions, that one uses the `l2amc.owner` contract instead of developing their own.
 *
 * The `l2amc.owner` contract class also implements two useful public static methods: `get_supply` and `get_balance`. The first allows one to check the total supply of a specified token, created by an account and the second allows one to check the balance of a token for a specified account (the token creator account has to be specified as well).
 *
 * The `l2amc.owner` contract manages the set of tokens, accounts and their corresponding balances, by using two internal multi-index structures: the `accounts` and `stats`. The `accounts` multi-index table holds, for each row, instances of `account` object and the `account` object holds information about the balance of one token. The `accounts` table is scoped to an eosio account, and it keeps the rows indexed based on the token's symbol.  This means that when one queries the `accounts` multi-index table for an account name the result is all the tokens that account holds at the moment.
 *
 * Similarly, the `stats` multi-index table, holds instances of `currency_stats` objects for each row, which contains information about current supply, maximum supply, and the creator account for a symbol token. The `stats` table is scoped to the token symbol.  Therefore, when one queries the `stats` table for a token symbol the result is one single entry/row corresponding to the queried symbol token if it was previously created, or nothing, otherwise.
 */
class l2amc_owner {
   
   public:
      
   
      ACTION init(         const name& admin, 
                           const name& oracler, 
                           const asset& stake_net_quantity, 
                           const asset& stake_cpu_quantity);

      //inline action
      ACTION bind( 
               const name& submitter,
               const name& l2amc_name,        //eth,bsc,btc,trx
               const string& xchain_pubkey, 
               const name& owner,
               const name& creator,
               const eosio::public_key& recovered_public_key);

      ACTION updateauth(const name& submitter, const name& owner, const eosio::public_key& amc_pubkey);

      [[eosio::action]]
      void execaction(const name& submitter, const name& l2amc_name, const name& account, const vector<eosio::action>& actions,const string& nonce);

      using bind_action = eosio::action_wrapper<"bind"_n,   &l2amc_owner::bind>;
      using updateauth_action = eosio::action_wrapper<"updateauth"_n,   &l2amc_owner::updateauth>;
      using execaction_action = eosio::action_wrapper<"execaction"_n,   &l2amc_owner::execaction>;
   
   
   struct l2amc_account_t {
      name                account;                //PK
      string              xchain_pubkey;          //UK: hash(xchain_pubkey)
      uint64_t            next_nonce = 1;
      checksum256         amc_txid;
      time_point_sec      created_at;
      eosio::public_key   recovered_public_key;
      l2amc_account_t() {}
      l2amc_account_t( const name& a ): account(a) {}

      uint64_t primary_key()const { return account.value ; }

      eosio::checksum256 by_xchain_pubkey() const  { return hash(xchain_pubkey);  } 
      eosio::checksum256 by_recovered_pubkey() const  { 
         auto packed_data = pack(recovered_public_key); 
         return sha256(packed_data.data(),packed_data.size());
      } 

      typedef eosio::multi_index
      < "l2amcaccts"_n,  l2amc_account_t,
         indexed_by<"xchpubkeyidx"_n,    const_mem_fun<l2amc_account_t, eosio::checksum256, &l2amc_account_t::by_xchain_pubkey> >,
         indexed_by<"recpubkeyidx"_n,    const_mem_fun<l2amc_account_t, eosio::checksum256, &l2amc_account_t::by_recovered_pubkey>>
      > idx_t;

      EOSLIB_SERIALIZE( l2amc_account_t, (account)(xchain_pubkey)(next_nonce)(amc_txid)(created_at) (recovered_public_key) )
   };
};


} //namespace amax
