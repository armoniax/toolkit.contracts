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

#define TBL struct [[eosio::table, eosio::contract("amaxapplybbp")]]
#define NTBL(name) struct [[eosio::table(name), eosio::contract("amaxapplybbp")]]

static constexpr name      SYS_BANK                 = "amax.token"_n;
static constexpr symbol    AMAX_SYMBOL              = symbol(symbol_code("AMAX"), 8);
static constexpr symbol    VOTE_SYMBOL              = symbol(symbol_code("VOTE"), 4);

static constexpr uint32_t MAX_LOGO_SIZE             = 512;
static constexpr uint32_t MAX_TITLE_SIZE            = 2048;


static constexpr uint32_t CHECK_UNFINISHED          = 0;
static constexpr uint32_t CHECK_NEED_REFUND         = 1;
static constexpr uint32_t CHECK_FINISHED            = 2;

namespace BbpStatus {
    static constexpr eosio::name INIT           { "init"_n   };
    static constexpr eosio::name REFUNDING      { "refunding"_n   };
    static constexpr eosio::name FINISHED       { "finished"_n  };
}

NTBL("global") global_t {
    name                    admin; 
    uint32_t                voter_idx       = 0;
    uint64_t                total_voter_cnt = 0;
    name                    bps_contract;
    name                    sys_contract    =name("amax");
    eosio::public_key       bbp_mkey;

    EOSLIB_SERIALIZE( global_t, (admin)(voter_idx)(total_voter_cnt)(bps_contract)(sys_contract)(bbp_mkey))
};

typedef eosio::singleton< "global"_n, global_t > global_singleton;


NTBL("globalclaim") globalclm_t {              
    asset    total_claimed = asset(0, AMAX_SYMBOL); 
    name     last_idx; 
    uint32_t bbp_count     = 0;

    EOSLIB_SERIALIZE( globalclm_t, (total_claimed)(last_idx)(bbp_count) )
};
typedef eosio::singleton< "globalclaim"_n, globalclm_t > globalclaim_singleton;



//scope _self
TBL voter_t {
    uint64_t        id;                 //PK
    name            voter_account;
    name            bbp_account;
    time_point_sec  created_at;
    time_point_sec  updated_at;
    
    voter_t(){}
    uint64_t primary_key()const { return id ; }

    uint64_t  by_voter_account() const { return voter_account.value; }
    uint64_t  by_bbp_account() const { return bbp_account.value; }
    
    typedef eosio::multi_index<"voters"_n, voter_t,
        indexed_by<"voteridx"_n, const_mem_fun<voter_t, uint64_t, &voter_t::by_voter_account> >,
        indexed_by<"bbpidx"_n, const_mem_fun<voter_t, uint64_t, &voter_t::by_bbp_account> >
    > idx_t;

    EOSLIB_SERIALIZE(voter_t, (id)(voter_account)(bbp_account)(created_at)(updated_at))
};

typedef eosio::multi_index< "voters"_n, voter_t > voters;
inline static voters make_voter_table( const name& self ) { return voters(self, self.value); }


TBL bbp_t {
    name                            owner;                 //PK
    checksum256                     amc_txid;
    uint64_t                        plan_id;
    string                          logo_uri;       
    string                          org_name;                   // cn:xxx|en:xxX
    string                          org_info;                   // web:xxx|tw:xxx|tg:xxX
    name                            dao_code;   
    string                          manifesto;                  // cn:xxx|en:xxx
    string                          email;
    string                          url;
    uint64_t                        location;
    name                            status;

    time_point_sec                  created_at;
    time_point_sec                  updated_at;
    map<extended_symbol, asset>     quants;
    map<extended_nsymbol, nasset>   nfts;
    eosio::public_key               mkey;

    bbp_t(){}
    uint64_t primary_key()const { return owner.value ; }
    uint64_t by_plan_id() const { return plan_id<<32 | created_at.utc_seconds; }

    typedef eosio::multi_index< "bbps"_n,  bbp_t,
        indexed_by<"planidx"_n, const_mem_fun<bbp_t, uint64_t, &bbp_t::by_plan_id> >
                     > idx_t;

    EOSLIB_SERIALIZE(bbp_t, (owner)(amc_txid)(plan_id)(logo_uri)(org_name)(org_info)(dao_code)
                            (manifesto)(email)(url)(location)(status)
                            (created_at)(updated_at)(quants)(nfts)(mkey))
};

TBL ibbp_t {
    name            account;
    name            rewarder;
    time_point_sec  created_at;
    time_point_sec  updated_at;

    ibbp_t() {};

    uint64_t    primary_key()const { return account.value; }

    typedef eosio::multi_index<"ibbps"_n,ibbp_t> idx_t;

    EOSLIB_SERIALIZE( ibbp_t, (account)(rewarder)(created_at)(updated_at) )
};

TBL rewarder_t {
    name            account;
    asset           rewarded;
    time_point_sec  created_at;
    time_point_sec  updated_at;

    rewarder_t() {};

    uint64_t    primary_key()const { return account.value; }

    typedef eosio::multi_index<"rewarders"_n,rewarder_t> idx_t;

    EOSLIB_SERIALIZE( rewarder_t, (account)(rewarded)(created_at)(updated_at) )
};

TBL gstats_t {
    uint64_t  plan_id;
    map<extended_symbol, asset> quants;
    map<extended_nsymbol, nasset> nfts;
    time_point_sec                  created_at;
    time_point_sec                  updated_at;

    gstats_t(){}
    uint64_t primary_key()const { return plan_id; }

    typedef eosio::multi_index< "stats"_n,  gstats_t > idx_t;

    EOSLIB_SERIALIZE(gstats_t, (plan_id)(quants)(nfts)(created_at)(updated_at))
};

TBL plan_t {
    uint64_t                        id;                 //PK
    uint64_t                        total_bbp_quota;
    uint64_t                        applied_bbp_quota;
    uint64_t                        fulfilled_bbp_quota;
    uint64_t                        min_sum_quant = 1200;
    map<extended_symbol, asset>     quants;                 //{amax: 600, amae:0 }
    map<extended_nsymbol, nasset>   nfts;
    time_point_sec                  started_at;
    time_point_sec                  ended_at;
    time_point_sec                  created_at;
    time_point_sec                  updated_at;

    plan_t(){
    }

    plan_t(const uint64_t& plan_id ):id(plan_id){
    }

    uint64_t primary_key()const { return id ; }

    typedef eosio::multi_index< "bbpplans"_n,  plan_t > idx_t;


    EOSLIB_SERIALIZE(plan_t, (id)(total_bbp_quota)(applied_bbp_quota)(fulfilled_bbp_quota)(min_sum_quant)(quants)(nfts)
        (started_at)(ended_at)(created_at)(updated_at))

};



} //namespace amax