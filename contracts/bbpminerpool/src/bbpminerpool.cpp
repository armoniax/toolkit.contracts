#include <bbpminerpool/bbpminerpool.hpp>

#include <math.hpp>
#include <utils.hpp>
#include "mdao.info/mdao.info.db.hpp"
#include "amax.ntoken/amax.ntoken.hpp"

namespace amax {

#define TRANSFEREX(bank, from, to, quantity, memo) \
    {	token::transfer_action act{ bank, { {from, active_perm} } };\
			act.send( from, to, quantity , memo );}

using namespace std;
using namespace amax;
using namespace mdao;

[[eosio::on_notify("amax.token::transfer")]]
void bbpminerpool::ontoken_transfer( name from, name to, asset quantity, string memo ){
   if (from == get_self()) { return; }
   if (to != _self) { return; }

   CHECKC( quantity.symbol == AMAX_SYMBOL, err::INVALID_PARAM, "invalid quantity" );
   CHECKC( quantity.amount > 0, err::INVALID_PARAM, "invalid quantity" );

   //CASE-1: main-voter refuel for voter to redeem
   if(memo == "refuel") { return; }

   //CASE-2: voter 领取奖励
   if(_gstate.rewarders.count(from)) {
      _gstate.total_rewarded += quantity;
      _global.set( _gstate, get_self() );
      auto itr = _voter_t.begin();
      auto total_claimed = asset(0, AMAX_SYMBOL);
      while (itr != _voter_t.end()) {
         auto reward = _get_reward(_gstate.pool_vote_quota, itr->amount, quantity);
         if(reward.amount == 0) { itr++; continue; }

         total_claimed += reward;
         _voter_t.modify( itr, get_self(), [&]( auto& r ) {
            r.total_claimed   += reward;
            r.updated_at      = current_time_point();
         });
         //转账
         TRANSFER( AMAX_BANK, from, reward, "");
         itr++;
      }

      auto remain_reward = quantity - total_claimed;
      if(remain_reward.amount > 0) {
         _gstate.main_voter_claimed += remain_reward;
         TRANSFER( AMAX_BANK, _gstate.pool_main_voter, remain_reward, "");
      }
      _gstate.total_claimed += total_claimed;

      return;
   } 

   //CASE-3: 接收voter AMAX 
   auto itr = _voter_t.find( from.value );
   CHECKC( itr != _voter_t.end(), err::RECORD_NOT_FOUND, "not a voter" );
   _voter_t.modify( itr, get_self(), [&]( auto& r ) {
      r.amount += quantity;
      r.updated_at = current_time_point();
   });
   CHECKC( _gstate.total_vote_recd + quantity <= _gstate.pool_vote_quota, err::OVER_QUOTA, "over quota" );
   _gstate.total_vote_recd += quantity;
   _global.set( _gstate, get_self() );

}

}//namespace amax
