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