#pragma once

#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>
#include <eosio/binary_extension.hpp> 
#include <utils.hpp>

#include <optional>
#include <string>
#include <map>
#include <set>
#include <type_traits>


namespace amax {

using namespace std;
using namespace eosio;

#define HASH256(str) sha256(const_cast<char*>(str.c_str()), str.size())

static constexpr uint32_t MAX_LOGO_SIZE        = 512;
static constexpr uint32_t MAX_TITLE_SIZE        = 2048;

#define TBL struct [[eosio::table, eosio::contract("amaxapplybps")]]
#define NTBL(name) struct [[eosio::table(name), eosio::contract("amaxapplybps")]]

NTBL("global") global_t {
    name                admin;   
    name                bbp_contract;
    name                sys_contract = name("amax");
    uint32_t            total_bbps_count;
    uint32_t            applied_bbps_count  = 0;
    
    EOSLIB_SERIALIZE( global_t, (admin)(bbp_contract)(sys_contract)(total_bbps_count)(applied_bbps_count))

};

typedef eosio::singleton< "global"_n, global_t > global_singleton;

} //namespace amax