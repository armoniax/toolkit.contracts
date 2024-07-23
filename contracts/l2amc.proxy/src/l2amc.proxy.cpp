#include "l2amc.proxy/l2amc.proxy.hpp"
#include <chrono>
#include <vector>
#include <string>
#include "l2amc.owner.hpp"
#include "utils.hpp"

const std::string MESSAGE_MAGIC = "Bitcoin Signed Message:\n";
const std::string BIND_MSG = "Armonia";
static constexpr eosio::name active_permission{"active"_n};
using namespace amax;
using namespace wasm;

using namespace std;

#define CHECKC(exp, code, msg) \
      { if (!(exp)) eosio::check(false, string("[[") + to_string((int)code) + string("]] ")  \
                                    + string("[[") + _self.to_string() + string("]] ") + msg); }
std::string to_hex( const char* d, uint32_t s ) 
{
    std::string r;
    const char* to_hex="0123456789abcdef";
    uint8_t* c = (uint8_t*)d;
    for( uint32_t i = 0; i < s; ++i )
        (r += to_hex[(c[i]>>4)]) += to_hex[(c[i] &0x0f)];
    return r;
}
std::vector<char> encode(const int val,const int minlen) {
    std::vector<char> result;
    // 0-255 的ASCII码值
    int base = 256;
    std::vector<char> chars(base);
    for (int i = 0; i < 256; ++i) {
        chars[i] = static_cast<char>(i);
    }
    //使用模运算符 % 和除法运算符 /，通过将 val 除以 base 来迭代编码过程。每次迭代，我们都会得到一个余数，该余数对应于 chars 向量中的一个字符
    int value = val;
    while (value > 0) {
        auto curcode = chars[value % base];
        result.insert(result.begin(), curcode);
        value /= base;
    }
    // 检查是否需要用零进行填充以达到最小长度 minlen
    int pad_size = minlen - result.size();
    if (pad_size > 0) {
        for (int i = 0; i < pad_size; i++) {
            result.insert(result.begin(), 0);
        }
    }
    return result;
}

std::vector<char> num_to_var_int(const uint64_t x) {
    std::vector<char> result;
    if (x < 253) {
        result.push_back(x);
    } else if (x < 65536) {
        result.push_back((char)253);
        auto encode_bytes = encode(x, 2);
        reverse(encode_bytes.begin(),encode_bytes.end());
        result.insert(result.end(), encode_bytes.begin(), encode_bytes.end());
    } else if (x < 4294967296) {
        result.push_back((char)254);
        auto encode_bytes = encode(x, 4);
        reverse(encode_bytes.begin(),encode_bytes.end());
        result.insert(result.end(), encode_bytes.begin(), encode_bytes.end());
    } else {
        result.push_back((char)255);
        auto encode_bytes = encode(x, 8);
        reverse(encode_bytes.begin(),encode_bytes.end());
        result.insert(result.end(), encode_bytes.begin(), encode_bytes.end());
    }
    
    return result;
}


eosio::checksum256 sha256sha256(const vector<char> data){

    auto sha_1 = sha256(data.data(),data.size());
    auto byte_array = sha_1.extract_as_byte_array();
    auto chars = reinterpret_cast<const char*>(byte_array.data());
    return sha256(chars,byte_array.size());
}
void proxy::init(const name& admin, const name& owner_contract){
    require_auth(_self);
    CHECKC( is_account(admin), err::ACCOUNT_INVALID,"admin account invalid");
    CHECKC( is_account(owner_contract), err::ACCOUNT_INVALID,"owner_contract account invalid");
  
    _gstate.admin = admin; 
    _gstate.owner_contract = owner_contract;
}
void proxy::activate( const name& account, 
                    const string& btc_pub_key,
                    const eosio::signature& signature,
                    const public_key& temp_amc_pub){
    require_auth(_gstate.admin);

    auto msg_packed = pack(temp_amc_pub);
    auto packed_data = pack(msg_packed_t(MESSAGE_MAGIC,to_hex(msg_packed.data(),msg_packed.size())));
    
    auto public_key = recover_key(sha256sha256(packed_data),signature);

    auto accs = l2amc_owner::l2amc_account_t::idx_t( _gstate.owner_contract, L2AMC_BTC_NAME.value);
    auto itr = accs.find(account.value);
    if ( itr == accs.end()){
        l2amc_owner::bind_action newaccount_act(_gstate.owner_contract,{ {get_self(), ACTIVE_PERM} });
        newaccount_act.send(_self,L2AMC_BTC_NAME,btc_pub_key,account,L2AMC_BTC_NAME,public_key);
    }else {
        CHECKC( itr -> xchain_pubkey == btc_pub_key, err::PARAM_ERROR,"l2amc_acct already exist l2amc pubkey: " + btc_pub_key )
        CHECKC( itr -> recovered_public_key == public_key, err::PARAM_ERROR,"l2amc_acct already exist l2amc pubkey: " + account.to_string() )
    }
    
    l2amc_owner::updateauth_action setauth_act(_gstate.owner_contract,{ {get_self(), ACTIVE_PERM} });
    setauth_act.send(_self,account,temp_amc_pub);
}


void proxy::submitaction(const name& account, const vector<char> packed_action,const eosio::signature& sign){
    
    require_auth(account);
    string msg = to_hex(packed_action.data(),packed_action.size());

    vector<char> packed = pack(MESSAGE_MAGIC);
    vector<char> msg_packed = {num_to_var_int(msg.size())};
    
    for (auto it = msg.begin(); it != msg.end(); ++it) {
        msg_packed.push_back(*it);
    }
    packed.insert(packed.end(),msg_packed.begin(),msg_packed.end());
    auto recover_pub_key =  recover_key( sha256sha256(packed), sign);
    auto accs = l2amc_owner::l2amc_account_t::idx_t( _gstate.owner_contract, L2AMC_BTC_NAME.value);
    
    auto itr = accs.find(account.value);
    CHECKC( itr != accs.end(), err::RECORD_NOT_FOUND,"[proxy] account not found")
    CHECKC( itr-> recovered_public_key == recover_pub_key, err::DATA_MISMATCH,"Public key mismatch")
    unpacked_action_t unpacked_action = unpack<unpacked_action_t>(packed_action.data(),packed_action.size());
    CHECKC( unpacked_action.actions.size() > 0 , err::OVERSIZED,"There are no executable actions")

    vector<eosio::action> actions;
    for ( auto at : unpacked_action.actions){
        eosio::action send_action;
        send_action.account = at.account;
        send_action.name = at.name;
        send_action.data = at.data;
        send_action.authorization.emplace_back(permission_level{ account, active_permission });
        actions.push_back(send_action);
        
    }
    l2amc_owner::execaction_action exec_act(_gstate.owner_contract,{ {get_self(), ACTIVE_PERM} });
    exec_act.send( _self,L2AMC_BTC_NAME, account , actions, unpacked_action.nonce);

}