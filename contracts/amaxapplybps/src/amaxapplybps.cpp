
#include <amaxapplybps/amaxapplybps.hpp>
#include <variant>
#include <math.hpp>
#include <utils.hpp>

namespace amax {

using namespace std;
using namespace amax;

#define CHECKC(exp, code, msg) \
   { if (!(exp)) eosio::check(false, string("[[") + to_string((int)code) + string("]] ")  \
                                    + string("[[") + _self.to_string() + string("]] ") + msg); }

   void amaxapplybps::addproducer(const name& submiter, const name& producter,
                              const eosio::public_key& mpubkey,
                              const std::string& url, uint16_t location, std::optional<uint32_t> reward_shared_ratio){
      require_auth( submiter );
      CHECKC( submiter == _gstate.admin || submiter == _gstate.bbp_contract, err::NO_AUTH,
               "no auth permission: " + submiter.to_string())
      amax_system::addproducer_action addproducer_act( _gstate.sys_contract, {_self, "active"_n} );
      block_signing_authority producer_authority = convert_to_block_signing_authority(mpubkey);

      addproducer_act.send(producter, producer_authority, url, location, reward_shared_ratio);

      _gstate.applied_bbps_count++;

   }

}//namespace amax
