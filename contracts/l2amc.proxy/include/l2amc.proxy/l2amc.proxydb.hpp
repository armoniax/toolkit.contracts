#pragma once

#include <eosio/crypto.hpp>
#include <eosio/datastream.hpp>
#include <eosio/eosio.hpp>
#include <eosio/multi_index.hpp>
#include <eosio/permission.hpp>
#include <eosio/privileged.hpp>
#include <eosio/serialize.hpp>
#include <eosio/singleton.hpp>
#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/privileged.hpp>
#include <eosio/singleton.hpp>
#include <eosio/system.hpp>
#include <eosio/time.hpp>
#include <eosio/binary_extension.hpp>
#include <cmath>
#include <map>
#include <optional>
#include <set>
#include <chrono>
#include <algorithm>
#include <string>
#include <type_traits>
#include "amax_system.hpp"

using namespace eosio;
using namespace std;
using std::string;

// using namespace wasm;

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
   RATE_OVERLOAD        = 19,
   DATA_MISMATCH        = 20,
   MISC                 = 255,

   DEX_ORDER_ERROR      = 21,
   POOL_ORDER_ERROR     = 22,
   INSUFFICIENT_QUOTA   = 23,
   LOWER_LIMIT_EXCEEDED = 24,
   UPPER_LIMIT_EXCEEDED = 25,
   TIMES_EXCEEDED        = 26
};


namespace wasm { namespace db {

#define PROXY_TBL struct [[eosio::table, eosio::contract("l2amc.proxy")]]
#define PROXY_TBL_NAME(name) struct [[eosio::table(name), eosio::contract("l2amc.proxy")]]
#define HASH256(str) sha256(const_cast<char*>(str.c_str()), str.size())
#define SYMBOL(sym_code, precision) symbol(symbol_code(sym_code), precision)

static constexpr symbol    MUSDT_SYMBOL       = symbol(symbol_code("MUSDT"), 6);
static constexpr name      MTOKEN               { "amax.mtoken"_n };
static constexpr name      SPLIT_ACCOUNT        { "amax.split"_n };


static constexpr uint32_t  MAX_TITLE_SIZE        = 256;
static constexpr int128_t  RATIO_PRECISION       = 10000;
static constexpr eosio::name SYS_CONTRACT       = "amax"_n;
static constexpr eosio::name OWNER_PERM         = "owner"_n;
static constexpr eosio::name ACTIVE_PERM        = "active"_n;
static constexpr eosio::name AMAX_CODE_PERM        = "amax.code"_n;
static constexpr eosio::name L2AMC_BTC_NAME        = "btc"_n;
static constexpr eosio::symbol SYS_SYMB         = SYMBOL("AMAX", 8);

PROXY_TBL_NAME("global") global_t {
   name              admin;
   set<name>         amc_authers;
   name              owner_contract;
   typedef eosio::singleton< "global"_n, global_t > tbl_t;
};

struct [[eosio::action]] action_t{
   name           account;
   name           name;
   vector<char>   data; 
   EOSLIB_SERIALIZE(action_t,(account)(name)(data))
};


struct [[eosio::action]] unpacked_action_t{
   vector<action_t>          actions;
   string                           nonce;
   EOSLIB_SERIALIZE(unpacked_action_t,(actions)(nonce))
};

struct [[eosio::action]] msg_packed_t{
   string                  message_magic;
   string                  msg;

   msg_packed_t() {};
   msg_packed_t( const string& s1, const string& s2): message_magic(s1),msg(s2) {};

   EOSLIB_SERIALIZE(msg_packed_t,  (message_magic)(msg))
};

}}