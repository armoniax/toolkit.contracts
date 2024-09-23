#pragma once

#include <eosio/asset.hpp>
#include <eosio/action.hpp>

#include <string>

#include <amaxapplybbp/amaxapplybbp.db.hpp>
#include <amaxapplybps.hpp>
#include <wasm_db.hpp>
#include <optional>
#include <map>
#include <amax.token.hpp>

namespace amax {

using std::string;
using std::vector;
         
using namespace wasm::db;
using namespace eosio;

static constexpr name      NFT_BANK    = "did.ntoken"_n;
static constexpr eosio::name active_perm{"active"_n};


enum class err: uint8_t {
   NONE                 = 0,
   RECORD_NOT_FOUND     = 1,
   RECORD_EXISTING      = 2,
   SYMBOL_MISMATCH      = 4,
   PARAM_ERROR          = 5,
   MEMO_FORMAT_ERROR    = 6,
   PAUSED               = 7,
   NO_AUTH              = 8,
   NOT_POSITIVE         = 9,
   NOT_STARTED          = 10,
   OVERSIZED            = 11,
   TIME_EXPIRED         = 12,
   NOTIFY_UNRELATED     = 13,
   ACTION_REDUNDANT     = 14,
   ACCOUNT_INVALID      = 15,
   FEE_INSUFFICIENT     = 16,
   FIRST_CREATOR        = 17,
   STATUS_ERROR         = 18,
   SCORE_NOT_ENOUGH     = 19,
   NEED_REQUIRED_CHECK  = 20,
   INSUFFICIENT_FUNDS   = 21,
   TIME_ERROR           = 22,

};

/**
 * The `amaxapplybbp` sample system contract defines the structures and actions that allow users to create, issue, and manage tokens for AMAX based blockchains. It demonstrates one way to implement a smart contract which allows for creation and management of tokens. It is possible for one to create a similar contract which suits different needs. However, it is recommended that if one only needs a token with the below listed actions, that one uses the `amaxapplybbp` contract instead of developing their own.
 *
 * The `amaxapplybbp` contract class also implements two useful public static methods: `get_supply` and `get_balance`. The first allows one to check the total supply of a specified token, created by an account and the second allows one to check the balance of a token for a specified account (the token creator account has to be specified as well).
 *
 * The `amaxapplybbp` contract manages the set of tokens, accounts and their corresponding balances, by using two internal multi-index structures: the `accounts` and `stats`. The `accounts` multi-index table holds, for each row, instances of `account` object and the `account` object holds information about the balance of one token. The `accounts` table is scoped to an eosio account, and it keeps the rows indexed based on the token's symbol.  This means that when one queries the `accounts` multi-index table for an account name the result is all the tokens that account holds at the moment.
 *
 * Similarly, the `stats` multi-index table, holds instances of `currency_stats` objects for each row, which contains information about current supply, maximum supply, and the creator account for a symbol token. The `stats` table is scoped to the token symbol.  Therefore, when one queries the `stats` table for a token symbol the result is one single entry/row corresponding to the queried symbol token if it was previously created, or nothing, otherwise.
 */
class [[eosio::contract("amaxapplybbp")]] amaxapplybbp : public contract {

   #define CHECKC(exp, code, msg) \
   { if (!(exp)) eosio::check(false, string("[[") + to_string((int)code) + string("]] ")  \
                                    + string("[[") + _self.to_string() + string("]] ") + msg); }

   static constexpr eosio::name MT_BANK{"amax.mtoken"_n};
   static constexpr eosio::name AMAX_BANK{"amax.token"_n};

   private:
      dbc                 _dbc;
   public:
      using contract::contract;
  
   amaxapplybbp(eosio::name receiver, eosio::name code, datastream<const char*> ds): contract(receiver, code, ds),
         _dbc(get_self()),
         _global(get_self(), get_self().value),
         _bbp_t(get_self(), get_self().value),
         _voter_t(get_self(), get_self().value),
         _plan_t(get_self(), get_self().value),
         _ibbp_t(get_self(), get_self().value),
         _globalclaim(get_self(), get_self().value)

    {
        _gstate = _global.exists() ? _global.get() : global_t{};
         _gstateclaim = _globalclaim.exists() ? _globalclaim.get() : globalclm_t{};
    }

    ~amaxapplybbp() { 
      _global.set( _gstate, get_self() );
      _globalclaim.set( _gstateclaim, get_self() );
     }
   
   ACTION version() {
      map<extended_nsymbol, nasset> nfts;
      nsymbol nsymb= nsymbol(1001,0);
      auto exnsymbol= extended_nsymbol(nsymb, "amax.ntoken"_n);
      nfts[exnsymbol] = nasset{1, nsymb};

      nsymbol nsymb2= nsymbol(1001,0);
      auto exnsymbol2= extended_nsymbol(nsymb2, "amax.ntoken"_n);
      auto count = nfts.count(exnsymbol2);
      CHECKC(false, err::STATUS_ERROR, to_string(count) + " " + exnsymbol2.get_contract().to_string() + " " + to_string(exnsymbol2.get_nsymbol().id) )
      // CHECKC(false, err::STATUS_ERROR, "version 1.0.0")
   }

   ACTION init( const name& admin, const eosio::public_key& bbp_mkey, const name& bps_contract){
      require_auth( _self );
      _gstate.admin           = admin;
      _gstate.bbp_mkey        = bbp_mkey;
      _gstate.bps_contract   = bps_contract;
   }

   ACTION applybbp(const name&      owner,
                  const uint32_t&   plan_id,
                  const string&     logo_uri,
                  const string&     org_name,
                  const string&     org_info,
                  const string&     mail,
                  const string&     manifesto,
                  const string&     url,
                  const uint32_t&   location,
                  const std::optional<eosio::public_key> pub_mkey);


   [[eosio::on_notify("*::transfer")]]
   void ontoken_transfer( name from, name to, asset quantity, string memo );

   [[eosio::on_notify("amax.ntoken::transfer")]]
   void onrecv_nft( name from, name to, const std::vector<nasset>& assets, string memo );

   ACTION claimbbps(const uint32_t& count);

   ACTION intransfer(const name& bpp, const name& target);

   ACTION addvoters(const std::vector<name> &voters){
      _check_admin();
      CHECKC( voters.size() > 0 && voters.size() <= 50, err::OVERSIZED, "accounts oversized: " + std::to_string( voters.size()) )
      voter_t::idx_t voter_accts( _self, _self.value );
      auto voter_idx   = voter_accts.get_index<"voteridx"_n>();
      auto addcount = 0;
      for (auto& target : voters) {
         CHECKC(is_account(target), err::ACCOUNT_INVALID, "account not existed: " + target.to_string() );
         auto itr = voter_idx.find( target.value );
         if (itr != voter_idx.end()) {
            continue;   //found and skip
         }
         _gstate.total_voter_cnt++;
         voter_accts.emplace( _self, [&]( auto& a ){
            a.id              = _gstate.total_voter_cnt;
            a.voter_account   = target;
            a.created_at      = current_time_point();
         });
         addcount++;
      }
      CHECKC( addcount > 0, err::ACTION_REDUNDANT, "no new voter added" )
   }

   ACTION setplan(const uint64_t& plan_id, const uint64_t& bbp_quota, const uint64_t& min_sum_quant,
               time_point_sec start_at, time_point_sec ended_at,
               map<extended_symbol, asset> quants, 
               map<extended_nsymbol, nasset> nfts){
      _check_admin();
      CHECKC( plan_id > 0, err::PARAM_ERROR, "plan_id invalid" )
      CHECKC( bbp_quota >= 0, err::PARAM_ERROR, "bbp_quota invalid" )
      
      auto plan_itr = _plan_t.find( plan_id );
       if(plan_itr == _plan_t.end()) {
         _plan_t.emplace( _self, [&]( auto& a ){
            a.id                 = plan_id;
            a.total_bbp_quota    = bbp_quota;
            a.applied_bbp_quota = 0;
            a.fulfilled_bbp_quota   = 0;
            a.min_sum_quant      = min_sum_quant;
            a.quants             = quants;
            a.nfts               = nfts;
            a.started_at           = start_at;
            a.ended_at           = ended_at;
            a.created_at         = current_time_point();
         });
       } else {
         _plan_t.modify( plan_itr, _self, [&]( auto& a ){
            a.total_bbp_quota       = bbp_quota;
            a.quants                = quants;
            a.nfts                  = nfts;
            a.min_sum_quant         = min_sum_quant;
            a.started_at              = start_at;
            a.ended_at              = ended_at;
            a.updated_at            = current_time_point();
         });
       }
   }

   ACTION setplanex(const uint64_t& plan_id, const uint64_t& bbp_quota, const uint64_t& min_sum_quant,
               time_point_sec start_at, time_point_sec ended_at,
               uint64_t &applied_bbp_quota, uint64_t &fulfilled_bbp_quota,
               map<extended_symbol, asset> quants, 
               map<extended_nsymbol, nasset> nfts){
      _check_admin();
      CHECKC( plan_id > 0, err::PARAM_ERROR, "plan_id invalid" )
      CHECKC( bbp_quota >= 0, err::PARAM_ERROR, "bbp_quota invalid" )
      
      auto plan_itr = _plan_t.find( plan_id );
       if(plan_itr == _plan_t.end()) {
         _plan_t.emplace( _self, [&]( auto& a ){
            a.id                 = plan_id;
            a.total_bbp_quota    = bbp_quota;
            a.applied_bbp_quota  = applied_bbp_quota;
            a.fulfilled_bbp_quota = fulfilled_bbp_quota;
            a.min_sum_quant      = min_sum_quant;
            a.quants             = quants;
            a.nfts               = nfts;
            a.started_at         = start_at;
            a.ended_at           = ended_at;
            a.created_at         = current_time_point();
         });
       } else {
         _plan_t.modify( plan_itr, _self, [&]( auto& a ){
            a.total_bbp_quota       = bbp_quota;
            a.applied_bbp_quota     = applied_bbp_quota;
            a.fulfilled_bbp_quota   = fulfilled_bbp_quota;
            a.quants                = quants;
            a.nfts                  = nfts;
            a.min_sum_quant         = min_sum_quant;
            a.started_at              = start_at;
            a.ended_at              = ended_at;
            a.updated_at            = current_time_point();
         });
       }
   }

   

   ACTION withdraw(const name& owner,const extended_symbol& symbol, const asset& refund_quant){
      _check_admin();
      _refund(owner,symbol, refund_quant );
   }

   ACTION refund(const name& owner){
      _check_admin();
      _refund_owner(owner );
   }


   ACTION addbbp( const std::vector<name>& bbps,const name& rewarder) {
        _check_admin( );
        CHECKC( is_account(rewarder),err::ACCOUNT_INVALID, "account does not exist: " + rewarder.to_string() );
      
        for (auto& bbp : bbps) {
            auto bbp_itr = _ibbp_t.find( bbp.value );
            CHECKC(bbp_itr ==  _ibbp_t.end(), err::RECORD_EXISTING, "bbp already exists" );
            _ibbp_t.emplace( _self, [&]( auto& row ) {
                row.account     = bbp;
                row.rewarder    = rewarder;
                row.created_at  = current_time_point();
                row.updated_at  = current_time_point();
            });
            _gstateclaim.bbp_count++;
            
        }
    }
    ACTION delbbp( const std::vector<name>& bbps) {
        _check_admin( );

        for (auto& bbp : bbps) {
            auto bbp_itr = _ibbp_t.find( bbp.value );
            CHECKC(bbp_itr !=  _ibbp_t.end(), err::RECORD_NOT_FOUND, "bbp not found:" + bbp.to_string() );
            _ibbp_t.erase( bbp_itr );
            _gstateclaim.bbp_count--;
        }
    }

    ACTION initstats(const uint64_t& plan_id, const map<extended_symbol, asset>& quants){
      _check_admin();

      auto plan = _plan_t.find( plan_id );
      CHECKC( plan != _plan_t.end(), err::RECORD_NOT_FOUND, "plan not found" )
      auto plan_quants = plan->quants;
      for(auto& [symb, quant] : quants) {
         CHECKC( plan_quants.count(symb) > 0, err::RECORD_NOT_FOUND, "quant not found: " + symb.get_symbol().code().to_string() )
      }
      gstats_t::idx_t stats( _self, _self.value );
      auto stat_itr = stats.find( plan_id );
      if(stat_itr != stats.end()) {
         stats.modify( stat_itr, _self, [&]( auto& a ){
            a.quants = quants;
            a.updated_at = current_time_point();
         });
      } else {
         stats.emplace( _self, [&]( auto& a ){
            a.plan_id = plan_id;
            a.quants = quants;
            a.created_at = current_time_point();
            a.updated_at = current_time_point();
         });   
      }
   }

   using intransfer_action = action_wrapper<"intransfer"_n, &amaxapplybbp::intransfer>;
   private:
      global_singleton           _global;
      global_t                   _gstate;
      globalclaim_singleton      _globalclaim;
      globalclm_t                _gstateclaim;

      bbp_t::idx_t               _bbp_t;
      voter_t::idx_t             _voter_t;
      plan_t::idx_t              _plan_t;
      ibbp_t::idx_t              _ibbp_t;


      int _check_request_quant(
                     const std::map<extended_symbol, asset>&       plan_quants,
                     const std::map<extended_symbol, asset>&       quants,
                     const uint64_t& min_sum_quant) {
         auto ret = CHECK_FINISHED;
         auto total_quant = 0;
         for(auto& [symb, quant] : plan_quants) {
            if(quants.count(symb) == 0 && quant.amount > 0) {
               return CHECK_UNFINISHED;
            }
            if(quants.count(symb) > 0) {
               if(quants.at(symb) < quant) {
                  return CHECK_UNFINISHED;  
               }
               total_quant += quants.at(symb).amount * calc_precision(4) /calc_precision(quant.symbol.precision());
            }
         }
         if(total_quant < min_sum_quant * calc_precision(4)) {
            return CHECK_UNFINISHED;
         }
         return ret;
      };
      
      bool _on_receive_asset(const name& from, const name& to, const name& from_bank,
         const asset& quantity, const nasset& nquantity, asset& amax_quant);

      void _on_asset_finished(const name& owner, const name& from_bank, const asset& amax_quant);
      int _check_request_nft(
                     const std::map<extended_nsymbol, nasset>&       plan_nfts,
                     const std::map<extended_nsymbol, nasset>&       nfts) {
         auto ret = CHECK_FINISHED;
      
         for(auto& [symb, nft] : plan_nfts) {
            if(nfts.count(symb) == 0) {
               return CHECK_UNFINISHED;
            }
            if(nfts.at(symb) < nft) {
               return CHECK_UNFINISHED;  
            }

            if(nfts.at(symb) > nft) {
               ret = CHECK_NEED_REFUND;  
            }
         }
         return ret;
      };

      void _call_set_producer(
                  const name& owner, const name& from_bank,
                   const name& voter_account, const asset& quantity);

      void _set_producer( 
                  const name&      owner,
                  const uint32_t&   plan_id,
                  const string&     logo_uri,
                  const string&     org_name,
                  const string&     org_info,
                  const string&     email,
                  const string&     manifesto,
                  const string&     url,
                  const uint32_t&   location,
                  const std::optional<eosio::public_key> pub_mkey);

      void _check_admin(){
         CHECKC( has_auth(_self) || has_auth(_gstate.admin), err::NO_AUTH, "no auth for operate" )
      }
      

      void _txid(checksum256& txid) {
         size_t tx_size = transaction_size();
         char* buffer = (char*)malloc( tx_size );
         read_transaction( buffer, tx_size );
         txid = sha256( buffer, tx_size );
      }

      void _add_quant_stats( const uint64_t& plan_id, const extended_symbol ext_sym, const asset& quant){
         gstats_t::idx_t stats( _self, _self.value );
         auto stat_itr = stats.find( plan_id );
         if(stat_itr != stats.end()) {
            auto plan_quants  = stat_itr->quants;
            if(plan_quants.count(ext_sym) == 0) {
               plan_quants[ext_sym] = quant;
            } else{
               plan_quants[ext_sym] += quant;
            }
           stats.modify( stat_itr, _self, [&]( auto& a ){
               a.quants = plan_quants;
               a.updated_at = current_time_point();
           });
         } else {
            stats.emplace( _self, [&]( auto& a ){
               a.plan_id = plan_id;
               a.quants[ext_sym] = quant;
               a.created_at = current_time_point();
               a.updated_at = current_time_point();
            });
         }
      }

      void _add_nquant_stats( const uint64_t& plan_id, const extended_nsymbol ext_sym, const nasset& quant){
         gstats_t::idx_t stats( _self, _self.value );
         auto stat_itr = stats.find( plan_id );
         if(stat_itr != stats.end()) {
            auto nfts  = stat_itr->nfts;
            if(nfts.count(ext_sym) == 0) {
               nfts[ext_sym] = quant;
            } else{
               nfts[ext_sym] += quant;
            }
           stats.modify( stat_itr, _self, [&]( auto& a ){
               a.nfts = nfts;
               a.updated_at = current_time_point();
           });
         } else {
            stats.emplace( _self, [&]( auto& a ){
               a.plan_id = plan_id;
               a.nfts[ext_sym] = quant;
               a.created_at = current_time_point();
               a.updated_at = current_time_point();
            });
         }
      }


      void _refund(const name& owner, const extended_symbol& symbol, const asset& refund_quant);

      void _refund_owner(const name& owner);


      bool _bbp_claim(const name& bpp, const name& claimer);
};
} //namespace amax
