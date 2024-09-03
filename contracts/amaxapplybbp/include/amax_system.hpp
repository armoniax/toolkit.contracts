#pragma once

#include <eosio/action.hpp>
#include <eosio/print.hpp>
#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/binary_extension.hpp>

using namespace eosio;

namespace amax {

   using eosio::checksum256;
   using eosio::ignore;
   using eosio::name;
   using eosio::permission_level;
   using eosio::public_key;

   /**
    * A weighted permission.
    *
    * Defines a weighted permission, that is a permission which has a weight associated.
    * A permission is defined by an account name plus a permission name.
    */
   struct permission_level_weight {
      permission_level  permission;
      uint16_t          weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( permission_level_weight, (permission)(weight) )
   };

   /**
    * Weighted key.
    *
    * A weighted key is defined by a public key and an associated weight.
    */
   struct key_weight {
      eosio::public_key  key;
      uint16_t           weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( key_weight, (key)(weight) )
   };

   /**
    * Wait weight.
    *
    * A wait weight is defined by a number of seconds to wait for and a weight.
    */
   struct wait_weight {
      uint32_t           wait_sec;
      uint16_t           weight;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( wait_weight, (wait_sec)(weight) )
   };

   /**
    * Blockchain authority.
    *
    * An authority is defined by:
    * - a vector of key_weights (a key_weight is a public key plus a wieght),
    * - a vector of permission_level_weights, (a permission_level is an account name plus a permission name)
    * - a vector of wait_weights (a wait_weight is defined by a number of seconds to wait and a weight)
    * - a threshold value
    */
   struct authority {
      uint32_t                              threshold = 0;
      std::vector<key_weight>               keys;
      std::vector<permission_level_weight>  accounts;
      std::vector<wait_weight>              waits;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( authority, (threshold)(keys)(accounts)(waits) )
   };

   /**
    * Blockchain block header.
    *
    * A block header is defined by:
    * - a timestamp,
    * - the producer that created it,
    * - a confirmed flag default as zero,
    * - a link to previous block,
    * - a link to the transaction merkel root,
    * - a link to action root,
    * - a schedule version,
    * - and a producers' schedule.
    */
   struct block_header {
      uint32_t                                  timestamp;
      name                                      producer;
      uint16_t                                  confirmed = 0;
      checksum256                               previous;
      checksum256                               transaction_mroot;
      checksum256                               action_mroot;
      uint32_t                                  schedule_version = 0;
      std::optional<eosio::producer_schedule>   new_producers;

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE(block_header, (timestamp)(producer)(confirmed)(previous)(transaction_mroot)(action_mroot)
                                     (schedule_version)(new_producers))
   };
   static constexpr symbol    vote_symbol       = symbol("VOTE", 4);
   static const asset         vote_asset_0      = asset(0, vote_symbol);

struct producer_info_ext {
      asset          elected_votes        = vote_asset_0;
      uint32_t       reward_shared_ratio  = 0;
   };

   // Defines `producer_info` structure to be stored in `producer_info` table, added after version 1.0
   struct [[eosio::table]] producer_info {
      name                                                     owner;
      double                                                   total_votes = 0;
      eosio::public_key                                        producer_key; /// a packed public key object
      bool                                                     is_active = true;
      std::string                                              url;
      uint16_t                                                 location = 0;
      time_point                                               last_claimed_time;
      asset                                                    unclaimed_rewards;
      eosio::block_signing_authority                           producer_authority;
      eosio::binary_extension<producer_info_ext, false>        ext;

      uint64_t primary_key()const { return owner.value;                             }
      double   by_votes()const    { return is_active ? -total_votes : total_votes;  }

      inline static uint128_t by_elected_prod(const name& owner, bool is_active, const asset& votes) {
         static constexpr int64_t int64_max = std::numeric_limits<int64_t>::max();
         static constexpr uint64_t uint64_max = std::numeric_limits<uint64_t>::max();
         static_assert( uint64_max - (uint64_t)int64_max == (uint64_t)int64_max + 1 );
         uint64_t amount = votes.amount;
         ASSERT(amount < int64_max);
         uint64_t hi = is_active ? (uint64_t)int64_max - amount : uint64_max - amount;
         return uint128_t(hi) << 64 | owner.value;
      }

      bool     active()const      { return is_active;                               }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( producer_info, (owner)(total_votes)(producer_key)(is_active)(url)(location)
                                       (last_claimed_time)(unclaimed_rewards))
                                       // (producer_authority) (ext) )
   };
   typedef eosio::multi_index< "producers"_n, producer_info> producers_table;

class amax_system {
      public: 
         [[eosio::action]]
         void addvote( const name& voter, const asset& votes );

         [[eosio::action]]
         void vote( const name& voter, const std::vector<name>& producers);

         [[eosio::action]]
         void claimrewards( const name& submitter, const name& owner );
         using claimrewards_action = eosio::action_wrapper<"claimrewards"_n, &amax_system::claimrewards>;
         using addvote_action = eosio::action_wrapper<"addvote"_n, &amax_system::addvote>;
         using vote_action = eosio::action_wrapper<"vote"_n, &amax_system::vote>;
         static asset get_reward( const name& token_contract_account, const name& owner)
         {
            producers_table producttable( token_contract_account, token_contract_account.value );
            const auto& ac = producttable.get( owner.value );
            return ac.unclaimed_rewards;
         }
};

}