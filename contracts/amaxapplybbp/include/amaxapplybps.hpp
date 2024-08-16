#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/permission.hpp>
#include <eosio/action.hpp>

#include <string>

#include <amax_system.hpp>
#include <wasm_db.hpp>

namespace amax {


class amaxapplybps {
   public:
   [[eosio::action]]
   void addproducer(const name& submiter, const name& producter,
                     const eosio::public_key& mpubkey,
                     const string& url, uint16_t location, 
                     std::optional<uint32_t> reward_shared_ratio);

   using addproducer_action = eosio::action_wrapper<"addproducer"_n, &amaxapplybps::addproducer>;

};
} //namespace amax
