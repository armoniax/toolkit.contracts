#pragma once

#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>


#include <optional>
#include <string>
#include <map>
#include <set>
#include <type_traits>


namespace amax {

using namespace std;
using namespace eosio;
#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

#define hash(str) sha256(const_cast<char*>(str.c_str()), str.size())
static constexpr eosio::symbol SYS_SYMB         = SYMBOL("AMAX", 8);
static constexpr eosio::name SYS_CONTRACT       = "amax"_n;
static constexpr eosio::name OWNER_PERM         = "owner"_n;
static constexpr eosio::name ACTIVE_PERM        = "active"_n;

#define TBL struct [[eosio::table, eosio::contract("l2amc.owner")]]
#define NTBL(name) struct [[eosio::table(name), eosio::contract("l2amc.owner")]]

NTBL("global") global_t {
    name        admin                   = "armoniaadmin"_n;
    set<name>   amc_authers;
    uint64_t    ram_bytes               = 5000;
    asset       stake_net_quantity      = asset(10000, SYS_SYMB); //TODO: set params in global table
    asset       stake_cpu_quantity      = asset(40000, SYS_SYMB);
    uint64_t    max_pubkey_count        = 5;
    EOSLIB_SERIALIZE( global_t, (admin)(amc_authers)(ram_bytes)
                                (stake_net_quantity)(stake_cpu_quantity)
                                (max_pubkey_count))
};
typedef eosio::singleton< "global"_n, global_t > global_singleton;


NTBL("global2") global2_t {
    name      proxy_action  = "submitaction"_n;
};
typedef eosio::singleton< "global2"_n, global2_t > global2_singleton;

namespace ChainType {
    static constexpr eosio::name BTC            { "btc"_n       };
    static constexpr eosio::name ETH            { "eth"_n       };
    static constexpr eosio::name BSC            { "bsc"_n       };
    static constexpr eosio::name TRON           { "tron"_n      };
};

namespace BindStatus {
    static constexpr eosio::name REQUESTED      { "requested"_n };
    static constexpr eosio::name APPROVED       { "approved"_n  }; //delete upon disapprove
};

//Scope: l2amc, E.g. btc, eth, bsc, tron
TBL l2amc_account_t {
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

//Scope:account
TBL account_pubkey_t {
    uint64_t            id;
    name                account;        
    checksum256         amc_txid;
    time_point_sec      created_at;
    eosio::public_key   amc_pubkey;  

    account_pubkey_t() {}
    account_pubkey_t( const name& a ): account(a) {}

    uint64_t primary_key()const { return  id; }

    eosio::checksum256 by_amc_pubkey() const  { 
        auto i = tapos_block_prefix();

        auto packed_data = pack(amc_pubkey); 
        return sha256(packed_data.data(),packed_data.size());
     } 
    typedef eosio::multi_index
    < "accpubkeys"_n,  account_pubkey_t
        ,indexed_by<"amcpubkeyidx"_n, const_mem_fun<account_pubkey_t, eosio::checksum256, &account_pubkey_t::by_amc_pubkey>>
    > idx_t;

    EOSLIB_SERIALIZE( account_pubkey_t, (id)(account)(amc_txid)(created_at)(amc_pubkey) )
};

} //namespace amax