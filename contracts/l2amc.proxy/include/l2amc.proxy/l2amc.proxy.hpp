
#include <set>
#include <chrono>
#include "l2amc.proxydb.hpp"

using namespace std;
using namespace eosio;
using namespace wasm::db;

namespace amax {

class [[eosio::contract("l2amc.proxy")]] proxy: public eosio::contract {
private:
    global_t::tbl_t     _global;
    global_t            _gstate;
    globalowner_t::tbl_t     _global_owner;
    globalowner_t            _gstate_owner;
    
public:
    using contract::contract;

    proxy(eosio::name receiver, eosio::name code, datastream<const char*> ds):
        contract(receiver, code, ds)
        ,_global(get_self(), get_self().value)
        ,_global_owner(get_self(), get_self().value)
    {
        _gstate         = _global.exists() ? _global.get() : global_t{};
        _gstate_owner   = _global_owner.exists() ? _global_owner.get() : globalowner_t{};
    }
    
    ~proxy() { _global.set( _gstate, get_self() ); }
    ACTION init( const name& admin, const name& owner_contract);

    ACTION activate( const name& account, 
                    const string& btc_pub_key,
                    const eosio::signature& signature,
                    const public_key& temp_amc_pub);
        
    ACTION submitaction(const name& account,const vector<char> packed_action,const eosio::signature& sign);
    
    ACTION tonactive(const name& account, 
                    const vector<char>& ton_pub_key_hex,
                    const vector<char>& msg_header1_hex,
                    const vector<char>& full_msg_head_hex,
                    const vector<char>& signature,
                    const public_key& temp_amc_pub);

    ACTION tonsubmit(const name& account,
                    const vector<char>& msg_header_hex,
                    const vector<char>& full_msg_header_hex,
                    const vector<char>& packed_action, 
                    const vector<char>& signature_hex );

    ACTION toncheck(const vector<char>& ton_pub_key,
                    const vector< char>& msg, 
                    const vector<char>& signature );

    ACTION verify( const vector<unsigned char>& pk, const vector<unsigned char>& msg, const vector<unsigned char>& sig );

    ACTION addowner(const name& chain,const name& owner){
        require_auth(_gstate.admin);
        _gstate_owner.owner_contracts[chain] = owner;
        _global_owner.set(_gstate_owner, get_self());
    }

private:
    int signature_check(
        const unsigned char *expected_pk, 
        const unsigned char *msg, size_t size, 
        const unsigned char *expected_sig);

    void _ton_check( const vector<char>& ton_pub_key,
                    const vector<char>& msg_header1_hex,
                    const vector<char>& full_msg_head_hex,
                    const vector<char>& packed_msg, 
                    const vector<char>& signature );
    
}; //contract l2amc.proxy

}