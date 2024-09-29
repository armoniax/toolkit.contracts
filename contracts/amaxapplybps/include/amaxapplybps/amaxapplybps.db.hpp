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


NTBL("globalscan") globalscan_t {
    name                current_producer;  
    uint128_t           current_producer_key; 
    time_point_sec      scan_started_at;
    int32_t             scan_interval_minutes = 60*12;  
    
    EOSLIB_SERIALIZE( globalscan_t, (current_producer)(current_producer_key)(scan_started_at)(scan_interval_minutes))
};

typedef eosio::singleton< "global"_n, global_t > global_singleton;
typedef eosio::singleton< "globalscan"_n, globalscan_t > globalscan_singleton;

} //namespace amax