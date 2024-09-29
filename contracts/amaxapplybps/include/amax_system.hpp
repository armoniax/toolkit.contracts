#pragma once

#include <eosio/action.hpp>
#include <eosio/print.hpp>


namespace amax {

   using eosio::ignore;
   using eosio::name;
   using eosio::permission_level;

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
   static constexpr symbol    vote_symbol       = symbol("VOTE", 4);
   static const asset         vote_asset_0      = asset(0, vote_symbol);
   struct producer_info_ext {
      asset          elected_votes        = vote_asset_0;
      uint32_t       reward_shared_ratio  = 0;
   };
   
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

      inline uint128_t by_elected_prod() const {
         return ext ? by_elected_prod(owner, is_active, ext->elected_votes)
                    : by_elected_prod(owner, is_active, vote_asset_0);
      }

      bool     active()const      { return is_active;                               }

      // explicit serialization macro is not necessary, used here only to improve compilation time
      EOSLIB_SERIALIZE( producer_info, (owner)(total_votes)(producer_key)(is_active)(url)(location)
                                       (last_claimed_time)(unclaimed_rewards)
                                       (producer_authority)(ext) )
   };
   typedef eosio::multi_index< "producers"_n, producer_info,
                               indexed_by<"prototalvote"_n, const_mem_fun<producer_info, double, &producer_info::by_votes>>,
                               indexed_by<"electedprod"_n,  const_mem_fun<producer_info, uint128_t, &producer_info::by_elected_prod>, /*Nullable*/ true >
                             > producers_table;

class amax_system {
      public: 
            [[eosio::action]]
            void newaccount(  const name&       creator,
                              const name&       name,
                              authority         owner,
                              authority         active);

            [[eosio::action]]
            void updateauth(     name        account,
                              name        permission,
                              name        parent,
                              authority   auth );
            
            [[eosio::action]]
            void buyrambytes( const name& payer, const name& receiver, uint32_t bytes );

            [[eosio::action]]
            void delegatebw( const name& from, const name& receiver,
                        const asset& stake_net_quantity, const asset& stake_cpu_quantity, bool transfer );

            [[eosio::action]]
            void addvote( const name& voter, const asset& votes );

            [[eosio::action]]
            void vote( const name& voter, const std::vector<name>& producers );
            
            [[eosio::action]]
            void addproducer( const name& producer, const block_signing_authority& producer_authority,
                                 const std::string& url, uint16_t location, std::optional<uint32_t> reward_shared_ratio );
            using newaccount_action = eosio::action_wrapper<"newaccount"_n, &amax_system::newaccount>;
            using updateauth_action = eosio::action_wrapper<"updateauth"_n, &amax_system::updateauth>;

            using buyrambytes_action = eosio::action_wrapper<"buyrambytes"_n, &amax_system::buyrambytes>;
            using delegatebw_action = eosio::action_wrapper<"delegatebw"_n,   &amax_system::delegatebw>;
            using vote_action = eosio::action_wrapper<"vote"_n, &amax_system::vote>;
            using addvote_action = eosio::action_wrapper<"addvote"_n, &amax_system::addvote>;
            using addproducer_action = eosio::action_wrapper<"addproducer"_n, &amax_system::addproducer>;
   };

}