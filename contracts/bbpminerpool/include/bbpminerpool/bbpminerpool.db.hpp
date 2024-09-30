#pragma once

#include <amax.ntoken/amax.nasset.hpp>
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

#define TBL struct [[eosio::table, eosio::contract("bbpminerpool")]]
#define NTBL(name) struct [[eosio::table(name), eosio::contract("bbpminerpool")]]

static constexpr name      SYS_BANK                 = "amax.token"_n;
static constexpr symbol    AMAX_SYMBOL              = symbol(symbol_code("AMAX"), 8);
static constexpr symbol    VOTE_SYMBOL              = symbol(symbol_code("VOTE"), 4);

static constexpr uint32_t MAX_LOGO_SIZE             = 512;
static constexpr uint32_t MAX_TITLE_SIZE            = 2048;


static constexpr uint32_t CHECK_UNFINISHED          = 0;
static constexpr uint32_t CHECK_NEED_REFUND         = 1;
static constexpr uint32_t CHECK_FINISHED            = 2;


NTBL("global") global_t {
    name        admin; 
    name        pool_main_voter;
    asset       pool_vote_quota     = asset(1200, AMAX_SYMBOL);  // 总额度
    asset       total_vote_recd     = asset(0, AMAX_SYMBOL);    // 已经接收的AMAX数量
    asset       total_rewarded      = asset(0, AMAX_SYMBOL);    // 总奖励
    asset       total_claimed       = asset(0, AMAX_SYMBOL);    // 总领取
    asset       main_voter_claimed  = asset(0, AMAX_SYMBOL);    // main voter获得奖励
    set<name>   rewarders;                                      // 奖励来源账号

    EOSLIB_SERIALIZE( global_t, (admin)(pool_main_voter)(pool_vote_quota)(total_vote_recd)
                                (total_rewarded)(total_claimed)(main_voter_claimed)(rewarders))
};

typedef eosio::singleton< "global"_n, global_t > global_singleton;

//scope _self
TBL voter_t {
    name            account;        //PK
    asset           amount;         //投入多少AMAX
    asset           total_claimed;   

    time_point_sec  created_at;
    time_point_sec  updated_at;
    
    voter_t(){}
    uint64_t primary_key()const { return account.value; }
    
    typedef eosio::multi_index<"voters"_n, voter_t> idx_t;

    EOSLIB_SERIALIZE(voter_t, (account)(amount)(total_claimed)(created_at)(updated_at))
};


} //namespace amax