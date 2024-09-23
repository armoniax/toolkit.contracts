#include <amaxapplybbp/amaxapplybbp.hpp>

#include <math.hpp>
#include <utils.hpp>
#include "mdao.info/mdao.info.db.hpp"
#include "amax.ntoken/amax.ntoken.hpp"

namespace amax {

#define TRANSFEREX(bank, from, to, quantity, memo) \
    {	token::transfer_action act{ bank, { {from, active_perm} } };\
			act.send( from, to, quantity , memo );}
         
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



   void amaxapplybbp::applybbp(
                  const name&      owner,
                  const uint32_t&   plan_id,
                  const string&     logo_uri,
                  const string&     org_name,
                  const string&     org_info,
                  const string&     email,
                  const string&     manifesto,
                  const string&     url,
                  const uint32_t&   location,
                  const std::optional<eosio::public_key> pub_mkey){
      require_auth( owner );
      _set_producer(owner, plan_id, logo_uri, org_name, org_info, email, manifesto, url, location, pub_mkey);
   }

   // void amaxapplybbp::updatebbp(const name& owner,
   //                            const uint32_t& plan_id,
   //                            const string& logo_uri,
   //                            const string& org_name,
   //                            const string& org_info,
   //                            const name& dao_code,
   //                            const string& reward_shared_plan,
   //                            const string& manifesto,
   //                            const string& issuance_plan){

   //    require_auth( owner );
   //    auto bbp_itr = _producer_tbl.find(owner.value);
   //    CHECKC( bbp_itr != _producer_tbl.end(),err::RECORD_EXISTING,"Application submitted:" + owner.to_string())

   //    _set_producer(owner,logo_uri,org_name,org_info,dao_code,reward_shared_plan,manifesto,issuance_plan);
   // }

   void amaxapplybbp::_set_producer(
                  const name&      owner,
                  const uint32_t&   plan_id,
                  const string&     logo_uri,
                  const string&     org_name,
                  const string&     org_info,
                  const string&     email,
                  const string&     manifesto,
                  const string&     url,
                  const uint32_t&   location,
                  const std::optional<eosio::public_key> pub_mkey){
      auto plan_itr = _plan_t.find(plan_id);
      CHECKC( plan_itr != _plan_t.end(), err::RECORD_NOT_FOUND, "plan not found symbol" )
      CHECKC( plan_itr->started_at <= current_time_point(), err::TIME_ERROR, "plan not started" ) 

      CHECKC( logo_uri.size() <= MAX_LOGO_SIZE ,err::OVERSIZED ,"logo size must be <= " + to_string(MAX_LOGO_SIZE))
      CHECKC( org_name.size() <= MAX_TITLE_SIZE ,err::OVERSIZED ,"org_name size must be <= " + to_string(MAX_TITLE_SIZE))
      CHECKC( org_info.size() <= MAX_TITLE_SIZE ,err::OVERSIZED ,"org_info size must be <= " + to_string(MAX_TITLE_SIZE))

      auto bbp_itr = _bbp_t.find(owner.value);
      if(bbp_itr !=  _bbp_t.end() && (bbp_itr->status != BbpStatus::INIT)){
         CHECKC( false, err::STATUS_ERROR, "Information cant been changed")
      } 

      if(bbp_itr ==_bbp_t.end()){
         //update plan
         db::set(_plan_t, plan_itr, _self, [&]( auto& p, bool is_new ) {
            p.applied_bbp_quota =  plan_itr->applied_bbp_quota + 1;
         });
      }

      checksum256 txid;
      _txid(txid);
      db::set(_bbp_t, bbp_itr, _self, [&]( auto& p, bool is_new ) {
         if (is_new) {
            p.owner        = owner;
            p.created_at   = current_time_point();
            p.status       = BbpStatus::INIT;
         }
         p.amc_txid              = txid;
         p.plan_id               = plan_id;
         p.logo_uri              = logo_uri;
         p.org_name              = org_name;
         p.org_info              = org_info;
         p.manifesto             = manifesto;
         p.email                 = email;
         p.url                   = url;
         p.location              = location;
         p.updated_at            = current_time_point();
         if(pub_mkey.has_value()) 
            p.mkey               = pub_mkey.value();
         else 
            p.mkey              = _gstate.bbp_mkey;
      });
   }

   [[eosio::on_notify("*::transfer")]]
   void amaxapplybbp::ontoken_transfer( name from, name to, asset quantity, string memo ){
      if (from == get_self()) { return; }
      if (to != _self) { return; }
      if(memo == "refuel") { return; }

      auto from_bank = get_first_receiver();
      asset amax_quant;
      auto ret =  _on_receive_asset(from, to, from_bank, quantity, nasset{0, nsymbol{1, 1}}, amax_quant);
      if(!ret) return;

      _on_asset_finished(from, from_bank, amax_quant);
   }


   [[eosio::on_notify("amax.ntoken::transfer")]]
   void amaxapplybbp::onrecv_nft( name from, name to, const std::vector<nasset>& assets, string memo ){
      if (from == get_self()) { return; }
      if (to != _self) { return; }
      if(memo == "refuel") { return; }
      
      auto from_bank = get_first_receiver();

      asset amax_quant;
      auto ret = _on_receive_asset(from, to, from_bank, asset(0, AMAX_SYMBOL), assets[0], amax_quant);
      if(!ret) return;

      _on_asset_finished(from, from_bank, amax_quant);
   }

   bool amaxapplybbp::_on_receive_asset(const name& from, const name& to, const name& from_bank,
          const asset& quantity, const nasset& nquantity, asset& amax_quant) {
         
      auto bbp_itr = _bbp_t.find(from.value);
      CHECKC( bbp_itr != _bbp_t.end(), err::STATUS_ERROR, "bbp not found:" + from.to_string())
      //进行中
      CHECKC( bbp_itr->status == BbpStatus::INIT, err::STATUS_ERROR, "Information cant been changed")

      auto plan_itr = _plan_t.find(bbp_itr->plan_id);
      CHECKC( plan_itr != _plan_t.end(), err::RECORD_NOT_FOUND, "plan not found symbol" )
      CHECKC( plan_itr->fulfilled_bbp_quota < plan_itr->total_bbp_quota, err::STATUS_ERROR, "this plan already finished")

      auto quants = bbp_itr->quants;
      auto nfts = bbp_itr->nfts;
      const auto& symb = extended_symbol(quantity.symbol, from_bank);
      const auto& nsymb = extended_nsymbol(nquantity.symbol, from_bank);


      map<extended_symbol, asset> plan_quants;
      map<extended_nsymbol, nasset> plan_nfts; 
      if(quantity.amount > 0){
         //check project symbol required
         plan_quants = plan_itr->quants;
         CHECKC(plan_quants.count(symb) > 0, err::RECORD_NOT_FOUND, "plan not found symbol: ")
         if(quants.count(symb) == 0){ 
            quants[symb] = quantity;
         } else {
            quants[symb] += quantity;
         }
         //update global stats
         _add_quant_stats(plan_itr->id, symb, quantity);
      } else if(nquantity.amount > 0) {
         plan_nfts = plan_itr->nfts;

         //check project symbol required
         // CHECKC(false, err::PARAM_ERROR, "Invalid param: " + from_bank.to_string() + " " + to_string(plan_nfts.size()) + " " + 
         //                               nsymb.get_contract().to_string() + " " + to_string(nsymb.get_nsymbol().id) + " " + to_string(nsymb.get_nsymbol().parent_id)
         //                               )
         CHECKC(plan_nfts.count(nsymb) > 0, err::RECORD_NOT_FOUND, "plan not found symbol: ")
         if(nfts.count(nsymb) == 0){ 
            nfts[nsymb] = nquantity;
         } else {
            nfts[nsymb] += nquantity;
         }
         _add_nquant_stats(plan_itr->id, nsymb, nquantity);
      
      } else {
         CHECKC(false, err::PARAM_ERROR, "Invalid param: " + quantity.to_string())
      }

      auto check_ret       = _check_request_quant(plan_itr->quants, quants, plan_itr->min_sum_quant);
      auto nft_check_ret   = _check_request_nft(plan_itr->nfts, nfts);
      if( check_ret == CHECK_UNFINISHED || nft_check_ret == CHECK_UNFINISHED ) {
         db::set(_bbp_t, bbp_itr, _self, [&]( auto& p, bool is_new ) {
            p.quants       = quants;
            p.nfts         = nfts;
            p.updated_at   = current_time_point();
         });
         return false;
      }

      //paid finished
       db::set(_bbp_t, bbp_itr, _self, [&]( auto& p, bool is_new ) {
         p.quants = quants;
         p.nfts   = nfts;
         if(check_ret == CHECK_FINISHED && nft_check_ret == CHECK_FINISHED){
            p.status = BbpStatus::FINISHED;
         } else {
            p.status = BbpStatus::REFUNDING;
         }
         p.updated_at = current_time_point();
      });

      db::set(_plan_t, plan_itr, _self, [&]( auto& p, bool is_new ) {
         p.fulfilled_bbp_quota = plan_itr->fulfilled_bbp_quota + 1;
      });

      amax_quant = quants.at(extended_symbol(AMAX_SYMBOL, AMAX_BANK));
      auto amax_quant_int = amax_quant.amount/calc_precision(amax_quant.symbol.precision());
      if( amax_quant_int > plan_itr->min_sum_quant) {
         amax_quant = asset(plan_itr->min_sum_quant * calc_precision(AMAX_SYMBOL.precision()), AMAX_SYMBOL);
      }
      return true;
   }

   void amaxapplybbp::_call_set_producer(
            const name& owner, const name& from_bank,
            const name& voter_account, const asset& quantity){
      //transfer to voter
      TRANSFER( AMAX_BANK, voter_account, quantity, "bbp");

      //add producer
      auto itr = _bbp_t.find(owner.value);
      CHECKC( itr !=  _bbp_t.end(),err::RECORD_NOT_FOUND ,"bbp not found:" + owner.to_string())
      amaxapplybps::addproducer_action addproducer_act(_gstate.bps_contract, {get_self(), "active"_n});
      addproducer_act.send(get_self(), owner, itr->mkey, itr->url, itr->location, 0);

      //add vote 
      auto vote_quant = asset(quantity.amount/10000, VOTE_SYMBOL);
      amax_system::addvote_action add_vote_act(_gstate.sys_contract, {voter_account, "active"_n});
      add_vote_act.send(voter_account, vote_quant);

      //vote 
      amax_system::vote_action vote_act(_gstate.sys_contract, {voter_account, "active"_n});
      std::vector<name> producers;
      producers.push_back(owner);
      vote_act.send( voter_account,producers);
   }
   
   void amaxapplybbp::_on_asset_finished(const name& owner, const name& from_bank, const asset& amax_quant)
   {
      _gstate.voter_idx = _gstate.voter_idx + 1;
      auto voter_itr = _voter_t.find(_gstate.voter_idx);
      CHECKC( voter_itr != _voter_t.end(), err::RECORD_NOT_FOUND, "voter not found" )
      db::set(_voter_t, voter_itr, _self, [&]( auto& p, bool is_new ) {
         p.bbp_account = owner;
         p.updated_at = current_time_point();
      });
      _call_set_producer(owner, from_bank, voter_itr->voter_account, amax_quant);
   };

   void amaxapplybbp::_refund(const name& owner, const extended_symbol& symbol, const asset& refund_quant){
      auto bbp_itr = _bbp_t.find(owner.value);
      CHECKC( bbp_itr != _bbp_t.end(), err::RECORD_NOT_FOUND, "bbp not found:" + owner.to_string())
      CHECKC( bbp_itr->status == BbpStatus::REFUNDING, err::STATUS_ERROR, "Information cant been changed")
      CHECKC( refund_quant.amount > 0, err::PARAM_ERROR, "Invalid param: " + refund_quant.to_string())

      auto plan_itr = _plan_t.find(bbp_itr->plan_id);
      CHECKC( plan_itr != _plan_t.end(), err::RECORD_NOT_FOUND, "plan not found symbol" )
      auto plan_quants  = plan_itr->quants;
      auto quants       = bbp_itr->quants;

      auto total_amount = 0;
      for(auto& [symb, plan_quant] : plan_quants) {
         CHECKC(quants.count(symb) > 0, err::RECORD_NOT_FOUND, "plan not found")
         auto quant = quants.at(symb);
         if(symb == symbol) {
            quant -=refund_quant;
            CHECKC(quant >= plan_quant, err::INSUFFICIENT_FUNDS, "Insufficient funds")
            TRANSFER( symbol.get_contract(), owner, refund_quant, "refund");
         }
         total_amount += quant.amount/calc_precision(quant.symbol.precision());
         quants[symb] = quant;
      }
      CHECKC(total_amount >= plan_itr->min_sum_quant, err::INSUFFICIENT_FUNDS, "Insufficient funds")
      if(total_amount == plan_itr->min_sum_quant) {
         db::set(_bbp_t, bbp_itr, _self, [&]( auto& p, bool is_new ) {
            p.status = BbpStatus::FINISHED;
            p.quants = quants;
            p.updated_at = current_time_point();
         });
      }

   }

   void amaxapplybbp::_refund_owner(const name& owner){
      auto bbp_itr = _bbp_t.find(owner.value);
      CHECKC( bbp_itr != _bbp_t.end(), err::RECORD_NOT_FOUND, "bbp not found:" + owner.to_string())
      CHECKC( bbp_itr->status == BbpStatus::INIT, err::STATUS_ERROR, "Information cant been changed")
      auto plan_itr = _plan_t.find(bbp_itr->plan_id);
      CHECKC( plan_itr != _plan_t.end(), err::RECORD_NOT_FOUND, "plan not found symbol" )
      auto quants = bbp_itr->quants;
      auto nfts = bbp_itr->nfts;
      for(auto& [symb, quant] : quants) {
         if(quant.amount > 0) {
            TRANSFER( symb.get_contract(), owner, quant, "refund");
         }
      }
      for(auto& [nsymb, nft] : nfts) {
         if(nft.amount > 0) {
            vector<nasset> nftlist;
            nftlist.push_back(nft);
            amax::ntoken::transfer_action transfer_nft_act(nsymb.get_contract(), {get_self(), "active"_n});
            transfer_nft_act.send(get_self(), owner, nftlist, "refund");
         }
      }
      
      _bbp_t.erase(bbp_itr);
      //更新plan
      db::set(_plan_t, plan_itr, _self, [&]( auto& p, bool is_new ) {
         p.applied_bbp_quota =  plan_itr->applied_bbp_quota - 1;
      });
   }


   void amaxapplybbp::claimbbps( const uint32_t& count)
   {
      if(_gstateclaim.last_idx == ""_n)  _gstateclaim.last_idx = _ibbp_t.begin()->account;
      auto first_account = _gstateclaim.last_idx;
      auto itr = _ibbp_t.find(_gstateclaim.last_idx.value);
      auto excute_count = 0;
      while (itr != _ibbp_t.end() && excute_count < count) {
         if(_bbp_claim(itr->account, itr->rewarder)) {
            excute_count++;
         }
         _gstateclaim.last_idx = itr->account;
         itr++;
      }
      if(itr == _ibbp_t.end()) {
         _gstateclaim.last_idx = _ibbp_t.begin()->account;
      }
      CHECKC( excute_count > 0, err::RECORD_NOT_FOUND, "no bbp need claim: " + first_account.to_string());
   }

   bool amaxapplybbp::_bbp_claim(const name& bbp, const name& claimer){
      
      time_point last_claimed_time;
      auto reward = amax_system::get_reward("amax"_n, bbp, last_claimed_time);

      auto diff =time_point_sec(current_time_point()) - last_claimed_time;
      print( bbp.to_string() + " diff second:"+ to_string(int(diff.to_seconds())) + ",reward:" + reward.to_string() + "\n");
      if(reward.amount == 0 || diff < seconds(3600*24)) {
         return false;
      }
      amax_system::claimrewards_action act{ "amax"_n, { {_self, active_perm} } };\
      act.send( _self, bbp);

      amaxapplybbp::intransfer_action intransfer_act{ _self, { {_self, active_perm} } };
      intransfer_act.send( bbp, claimer);
      return true;
   }

   void amaxapplybbp::intransfer(const name& bbp, const name& target){
      require_auth( _self );
      auto balance = token::get_balance(AMAX_BANK, bbp, AMAX_SYMBOL.code());
      if(balance.amount == 0) {
         return;
      }
   
      TRANSFEREX( AMAX_BANK, bbp, target, balance, "" );
      _gstateclaim.total_claimed += balance;

      rewarder_t::idx_t rewarder_tbl( _self, _self.value );
      auto rewarder_itr = rewarder_tbl.find(target.value);
      if(rewarder_itr == rewarder_tbl.end()) {
         rewarder_tbl.emplace( _self, [&]( auto& a ){
            a.account = target;
            a.rewarded = balance;
            a.created_at = current_time_point();
            a.updated_at = current_time_point();
         });
      } else {
         rewarder_tbl.modify(rewarder_itr, _self, [&]( auto& a ){
            a.rewarded += balance;
            a.updated_at = current_time_point();
         });
      }
   }

}//namespace amax
