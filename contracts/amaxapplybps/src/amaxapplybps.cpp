
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


   void amaxapplybps::refreshbbp(const uint32_t& count) {
      
      producers_table producttable( _gstate.sys_contract, _gstate.sys_contract.value );
      auto idx = producttable.get_index<"prototalvote"_n>();

      if(_gstate_scan.current_producer == ""_n) _gstate_scan.current_producer = producttable.begin()->owner;
      if(_gstate_scan.current_producer == producttable.begin()->owner) {
         CHECKC(_gstate_scan.scan_started_at.sec_since_epoch() + _gstate_scan.scan_interval_minutes * 60 < current_time_point().sec_since_epoch(),
          err::NOT_STARTED, "time not reached yet")
      }
      auto itr = producttable.find(_gstate_scan.current_producer.value);
      auto execute_count = 0;
      auto last_producer = _gstate_scan.current_producer;

      amax_system::addproducer_action addproducer_act( _gstate.sys_contract, {_self, "active"_n} );
      while (itr != producttable.end() && execute_count < count) {
         block_signing_authority producer_authority = convert_to_block_signing_authority(itr->producer_key);
         auto reward_shared_ratio = 0;
         if(itr->ext) {
            reward_shared_ratio = itr->ext->reward_shared_ratio;
         }
         addproducer_act.send(itr->owner, producer_authority, itr->url, itr->location, reward_shared_ratio);
         
         execute_count++;
         _gstate_scan.current_producer = itr->owner;
         itr++;
      }
      if(itr == producttable.end()) {
         _gstate_scan.current_producer = producttable.begin()->owner;
      }
      CHECKC( execute_count > 0, err::RECORD_NOT_FOUND, "no bbp need claim: " + last_producer.to_string());
   }


}//namespace amax
