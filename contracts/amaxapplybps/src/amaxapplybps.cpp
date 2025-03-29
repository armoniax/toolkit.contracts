
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

   void amaxapplybps::setinterval(const uint32_t& scan_interval_minutes) {
      require_auth( _self );
      CHECKC(scan_interval_minutes > 0, err::INVALID_PARAM, "scan_interval_minutes must be positive");
      _gstate_scan.scan_interval_minutes = scan_interval_minutes;
   }
      
   void amaxapplybps::refreshbbp(const uint32_t& count) {
      CHECKC(count > 0, err::INVALID_PARAM, "count must be positive");
      
      producers_table producttable(_gstate.sys_contract, _gstate.sys_contract.value);
      auto idx = producttable.get_index<"electedprod"_n>();
      
      // 获取起始迭代器
      auto itr = (_gstate_scan.current_producer == ""_n) ? idx.begin() 
                                                         : idx.find(_gstate_scan.current_producer_key);
      CHECKC(itr != idx.end(), err::RECORD_NOT_FOUND, "no bbp need claim");
   
      // 检查扫描间隔
      if (_gstate_scan.current_producer == ""_n) {
            CHECKC(_gstate_scan.scan_started_at.sec_since_epoch() + _gstate_scan.scan_interval_minutes * 60 < current_time_point().sec_since_epoch(),
                  err::NOT_STARTED, "time not reached yet");
            _gstate_scan.scan_started_at = current_time_point();
      }
   
      amax_system::addproducer_action addproducer_act(_gstate.sys_contract, {_self, "active"_n});
      uint32_t execute_count = 0;
      name last_processed_producer = ""_n;
   
      while (itr != idx.end() && execute_count < count) {
            if (!itr->is_active) {
               ++itr;
               continue;
            }
   
            // 先保存下一个迭代器，防止send后失效
            auto next_itr = itr;
            ++next_itr;
   
            // 执行操作
            uint16_t reward_shared_ratio = itr->ext ? itr->ext->reward_shared_ratio : 0;
            addproducer_act.send(itr->owner, itr->producer_authority, itr->url, itr->location, reward_shared_ratio);
            
            last_processed_producer = itr->owner;
            execute_count++;
            itr = next_itr;
      }
   
      // 更新状态
      if (itr == idx.end()) {
            _gstate_scan.current_producer = ""_n;
            _gstate_scan.current_producer_key = 0;
      } else {
            _gstate_scan.current_producer = itr->owner;
            _gstate_scan.current_producer_key = itr->by_elected_prod();
      }
   }


}//namespace amax
