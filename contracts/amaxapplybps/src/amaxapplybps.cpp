#include <amax.applybp/amax.applybp.hpp>

#include <math.hpp>
#include <utils.hpp>

namespace amax {


namespace db {

    template<typename table, typename Lambda>
    inline void set(table &tbl,  typename table::const_iterator& itr, const eosio::name& emplaced_payer,
            const eosio::name& modified_payer, Lambda&& setter )
   {
        if (itr == tbl.end()) {
            tbl.emplace(emplaced_payer, [&]( auto& p ) {
               setter(p, true);
            });
        } else {
            tbl.modify(itr, modified_payer, [&]( auto& p ) {
               setter(p, false);
            });
        }
    }

    template<typename table, typename Lambda>
    inline void set(table &tbl,  typename table::const_iterator& itr, const eosio::name& emplaced_payer,
               Lambda&& setter )
   {
      set(tbl, itr, emplaced_payer, eosio::same_payer, setter);
   }

}// namespace db


using namespace std;
using namespace amax;
using namespace mdao;

#define CHECKC(exp, code, msg) \
   { if (!(exp)) eosio::check(false, string("[[") + to_string((int)code) + string("]] ")  \
                                    + string("[[") + _self.to_string() + string("]] ") + msg); }


   void amaxapplybps::init( const name& admin){
      require_auth( _self );

      CHECKC( is_account(admin),err::ACCOUNT_INVALID,"admin invalid:" + admin.to_string())
      // CHECKC( is_account(dao_contract),err::ACCOUNT_INVALID,"dao_contract invalid:" + dao_contract.to_string())

      _gstate.admin = admin;
      // _gstate.dao_contract = dao_contract;

   }
   

   void amaxapplybps::applybp(const name& owner,
                              const string& logo_uri,
                              const string& org_name,
                              const string& org_info,
                              const name& dao_code,
                              const string& reward_shared_plan,
                              const string& manifesto,
                              const string& issuance_plan){
      require_auth( owner );

      auto prod_itr = _producer_tbl.find(owner.value);
      CHECKC( prod_itr == _producer_tbl.end(),err::RECORD_EXISTING,"Application submitted:" + owner.to_string())

      _set_producer(owner,logo_uri,org_name,org_info,dao_code,reward_shared_plan,manifesto,issuance_plan);
   }

   void amaxapplybps::addproducer(const name& submiter,
                              const name& owner,
                              const string& logo_uri,
                              const string& org_name,
                              const string& org_info,
                              const name& dao_code,
                              const string& reward_shared_plan,
                              const string& manifesto,
                              const string& issuance_plan){
      require_auth( submiter );
      CHECKC( submiter == _gstate.admin,err::NO_AUTH,"Missing required authority of admin" )

      _set_producer(owner,logo_uri,org_name,org_info,dao_code,reward_shared_plan,manifesto,issuance_plan);
   }

}//namespace amax
