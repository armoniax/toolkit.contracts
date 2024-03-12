#include <l2amc.owner/l2amc.owner.hpp>
#include <variant>
namespace amax {
   using namespace std;

   #define CHECKC(exp, code, msg) \
      { if (!(exp)) eosio::check(false, string("[[") + to_string((int)code) + string("]] ")  \
                                    + string("[[") + _self.to_string() + string("]] ") + msg); }
 
   void l2amc_owner::_newaccount( const name& creator, const name& account) {
      auto perm = creator != get_self()? OWNER_PERM : ACTIVE_PERM;
      amax_system::newaccount_action  act(SYS_CONTRACT, { {creator, perm} }) ;
      authority owner_auth  = { 1, {}, {{{get_self(), OWNER_PERM}, 1}}, {} }; 
      authority active_auth  = { 1, {}, {{{get_self(), ACTIVE_PERM}, 1}}, {} }; 
      act.send( creator, account, owner_auth, active_auth);

      amax_system::buyrambytes_action buy_ram_act(SYS_CONTRACT, { {get_self(), ACTIVE_PERM} });
      buy_ram_act.send( get_self(), account, _gstate.ram_bytes );

      amax_system::delegatebw_action delegatebw_act(SYS_CONTRACT, { {get_self(), ACTIVE_PERM} });
      delegatebw_act.send( get_self(), account,  _gstate.stake_net_quantity,  _gstate.stake_cpu_quantity, false );
   }

   void l2amc_owner::updateauth(const name& proxy_contract, const name& owner, const eosio::public_key& amc_pubkey){
      require_auth(proxy_contract);
      CHECKC( _gstate.amc_authers.find(proxy_contract) != _gstate.amc_authers.end(), err::NO_AUTH, "no auth to operate" )
      _updateauth(owner,proxy_contract, amc_pubkey);
   }

   void l2amc_owner::_updateauth(const name& owner, const name& proxy_contract, const eosio::public_key& amc_pubkey){
      account_pubkey_t::idx_t account_pubkeys(get_self(), owner.value );
      auto idx = account_pubkeys.get_index<"amcpubkeyidx"_n>();
      auto itr = idx.find(sha256pk(amc_pubkey));
      if ( itr != idx.end()) return;
      // CHECKC( lower_itr ==idx.end() || lower_itr->amc_pubkey != amc_pubkey, err::STATUS_ERROR, "amc_pubkey already exist");
      // auto pubkey_ptr_beg = idx.begin();

      auto pubkey_vector = vector<eosio::public_key>();

      // pubkey_vector.push_back(amc_pubkey);
      // while( pubkey_ptr_beg != idx.end() ) {
      //    pubkey_vector.push_back(pubkey_ptr_beg->amc_pubkey);
      //    pubkey_ptr_beg++;
      // }

      // if(pubkey_vector.size() > _gstate.max_pubkey_count) {
      //    auto older_idx = account_pubkeys.begin();
      //    account_pubkeys.erase(older_idx);
      //    for (auto it = pubkey_vector.begin(); it != pubkey_vector.end();) {
      //       if (*it == pubkey_ptr_beg->amc_pubkey) {
      //          it = pubkey_vector.erase(it);
      //          break;
      //       } else {
      //          ++it;
      //       }
      //    }
      // }

      // std::sort(pubkey_vector.begin(), pubkey_vector.end(), [](const eosio::public_key& p1, const eosio::public_key& p2){
      //    return (memcmp( std::get<0>(p1).data(), std::get<0>(p2).data(), 33) < 0);
      // });

      authority auth = { 1, {}, {}, {} };
      // for( auto p : pubkey_vector ) {
      //    auth.keys.push_back({p, 1});
      // }
      checksum256 txid;
      _txid(txid);
      account_pubkeys.emplace( _self, [&]( auto& a ){
         a.id              = account_pubkeys.available_primary_key();
         a.account         = owner;
         a.amc_txid        = txid;
         a.amc_pubkey      = amc_pubkey;
         a.created_at      = time_point_sec( current_time_point() );
      });
      
      // auto key_itr = account_pubkeys.rbegin();
      int size = 0;
      set<eosio::public_key> keys;
      uint64_t old_id;
      for ( auto key_itr = account_pubkeys.rbegin(); key_itr != account_pubkeys.rend(); key_itr++){
         if ( size >= _gstate.max_pubkey_count){
            old_id = key_itr -> id;
            break;
         }else {
            pubkey_vector.push_back(key_itr->amc_pubkey);
            size ++;
         }
      }

      auto b_itr = account_pubkeys.begin();
      while ( b_itr != account_pubkeys.end()){
         if ( b_itr -> id > old_id) break;
         b_itr = account_pubkeys.erase(b_itr);
      }
      
      std::sort(pubkey_vector.begin(), pubkey_vector.end(), [](const eosio::public_key& p1, const eosio::public_key& p2){
         return (memcmp( std::get<0>(p1).data(), std::get<0>(p2).data(), 33) < 0);
      });

      for( auto p : pubkey_vector ) {
         auth.keys.push_back({p, 1});
      }
      amax_system::updateauth_action updateauth_act(SYS_CONTRACT, {{ owner, ACTIVE_PERM}});
      updateauth_act.send( owner, "submitperm"_n, ACTIVE_PERM, auth);
      if(auth.keys.size() > 1) {
         return;
      }
      amax_system::linkauth_action linkauth_act(SYS_CONTRACT,{ {owner, ACTIVE_PERM} } );
      linkauth_act.send( owner, proxy_contract, "submitaction"_n, "submitperm"_n);
   }

   void l2amc_owner::bind( 
            const name& proxy_contract,
            const name& l2amc_name,        //eth,bsc,btc,trx
            const string& xchain_pubkey, 
            const name& owner,
            const name& creator,
            const eosio::public_key& recovered_public_key){  
      require_auth(proxy_contract);
      CHECKC( _gstate.amc_authers.find(proxy_contract) != _gstate.amc_authers.end(), err::NO_AUTH, "no auth to operate" )
      l2amc_account_t::idx_t l2amc_accts (get_self(), l2amc_name.value );
      auto idx = l2amc_accts.get_index<"xchpubkeyidx"_n>();
      auto itr = idx.find(hash(xchain_pubkey));
      CHECKC( l2amc_accts.find(owner.value) == l2amc_accts.end(),err::RECORD_EXISTING,"l2amc_acct already exist account:" +owner.to_string())
      checksum256 txid;
      _txid(txid);
      if( itr == idx.end()) {
         _newaccount(creator, owner);
         l2amc_accts.emplace( _self, [&]( auto& a ){
            a.account         = owner;
            a.xchain_pubkey   = xchain_pubkey;
            a.recovered_public_key = recovered_public_key;
            a.amc_txid        = txid;
            a.created_at      = time_point_sec( current_time_point() );
         });
         return;
      }
      CHECKC(false, err::STATUS_ERROR, "l2amc_acct already exist l2amc pubkey: " + xchain_pubkey );
   }

   void l2amc_owner::init( const name& admin, 
                        const name& amc_auther, 
                        const asset& stake_net_quantity, 
                        const asset& stake_cpu_quantity) {
      CHECKC( has_auth(_self),  err::NO_AUTH, "no auth to operate" )      

      CHECKC( is_account( admin ), err::ACCOUNT_INVALID, admin.to_string() + ": invalid account" )
      CHECKC( is_account( amc_auther ), err::ACCOUNT_INVALID, amc_auther.to_string() + ": invalid account" )
      _gstate.admin                 = admin;
      _gstate.amc_authers.insert( amc_auther );
      _gstate.stake_net_quantity    = stake_net_quantity;
      _gstate.stake_cpu_quantity    = stake_cpu_quantity;
   }

   void l2amc_owner::execaction(const name& proxy_contract,
                  const name& l2amc_name, const name& owner,
                  const vector<eosio::action>& actions,
                  const string& nonce){
      require_auth(proxy_contract);
      CHECKC( _gstate.amc_authers.find(proxy_contract) != _gstate.amc_authers.end(), err::NO_AUTH, "no auth to operate" )

      l2amc_account_t::idx_t l2amc_accts (get_self(), l2amc_name.value );
      auto itr = l2amc_accts.find(owner.value);
      CHECKC( itr != l2amc_accts.end(), err::RECORD_EXISTING,"l2amc_acct not exist l2amc account: " + proxy_contract.to_string())
      CHECKC( nonce == to_string(itr -> next_nonce), err::PARAM_ERROR,"nonce does not match")

      for ( eosio::action a : actions){
         a.send();
      }

      l2amc_accts.modify( itr,same_payer, [&]( auto& a ){
            a.next_nonce  ++;
      });
   }

   void l2amc_owner::_txid(checksum256& txid) {
      size_t tx_size = transaction_size();
      char* buffer = (char*)malloc( tx_size );
      read_transaction( buffer, tx_size );
      txid = sha256( buffer, tx_size );
   }

}
