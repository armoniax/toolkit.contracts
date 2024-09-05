#include <eosio/print.hpp>
#include "l2amc.proxy/l2amc.proxy.hpp"

#include <chrono>
#include <vector>
#include <string>
#include "l2amc.owner.hpp"
#include "utils.hpp"
#include  "ed25519.h"
#include  "ed25519_signature.h"

const std::string MESSAGE_MAGIC = "Bitcoin Signed Message:\n";
const std::string TON_MESSAGE_MAGIC = "TON Signed Message:\n";
const std::string BIND_MSG = "Armonia";
static constexpr eosio::name active_permission{"active"_n};
using namespace amax;
using namespace wasm;

using namespace std;

#define CHECKC(exp, code, msg) \
      { if (!(exp)) eosio::check(false, string("[[") + to_string((int)code) + string("]] ")  \
                                    + string("[[") + _self.to_string() + string("]] ") + msg); }
                                    
                                    

unsigned char* hexstr_to_char(const char* hexstr)
{
    size_t len = strlen(hexstr);
    size_t final_len = len / 2;
    unsigned char* chrs = (unsigned char*)malloc((final_len+1) * sizeof(*chrs));
    for (size_t i=0, j=0; j<final_len; i+=2, j++)
        chrs[j] = (hexstr[i] % 32 + 9) % 25 * 16 + (hexstr[i+1] % 32 + 9) % 25;
    chrs[final_len] = '\0';
    return chrs;
}
 char* hexstr_to_char2(const char* hexstr)
{
    size_t len = strlen(hexstr);
    size_t final_len = len / 2;
    char* chrs = ( char*)malloc((final_len+1) * sizeof(*chrs));
    for (size_t i=0, j=0; j<final_len; i+=2, j++)
        chrs[j] = (hexstr[i] % 32 + 9) % 25 * 16 + (hexstr[i+1] % 32 + 9) % 25;
    chrs[final_len] = '\0';
    return chrs;
}

                                    
std::string to_hex( const char* d, uint32_t s ) 
{
    std::string r;
    const char* to_hex="0123456789abcdef";
    uint8_t* c = (uint8_t*)d;
    for( uint32_t i = 0; i < s; ++i )
        (r += to_hex[(c[i]>>4)]) += to_hex[(c[i] &0x0f)];
    return r;
}

std::string to_hex_upper( const char* d, uint32_t s ) 
{
    std::string r;
    const char* to_hex="0123456789ABCDEF";
    uint8_t* c = (uint8_t*)d;
    for( uint32_t i = 0; i < s; ++i )
        (r += to_hex[(c[i]>>4)]) += to_hex[(c[i] &0x0f)];
    return r;
}

template<typename CharT>
static std::string to_hex_pk(const CharT* d, uint32_t s) {
    std::string r;
    const char* to_hex="0123456789abcdef";
    uint8_t* c = (uint8_t*)d;
    for( uint32_t i = 0; i < s; ++i ) {
        (r += to_hex[(c[i] >> 4)]) += to_hex[(c[i] & 0x0f)];
    }
    return r;
}

#define ed25519_public_key_size     32
#define ed25519_secret_key_size     32
#define ed25519_private_key_size    64
#define ed25519_signature_size      64

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


eosio::checksum256 sha256(const vector<char> data){
    auto sha_1 = eosio::sha256(data.data(),data.size());
    return sha_1;
}

typedef unsigned char       U8;
typedef signed   char       S8;
typedef uint16_t            U16;
typedef int16_t             S16;
typedef uint32_t            U32;
typedef int32_t             S32;
typedef uint64_t            U64;
typedef int64_t             S64;
int32_t ecp_PrintHexBytes( const char *name,  const U8 *data, U32 size)
{
    printf("%s = 0x", name);
    while (size > 0) printf("%02X", data[--size]);
    printf("\n");
    return 1;
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
    
    auto public_key = recover_key(sha256(packed_data),signature);

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
    auto recover_pub_key =  recover_key( sha256(packed), sign);
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
void printvector(std::string header, const vector<char>& vec){
    printf( "\n %s", header.c_str());
    for (auto it = vec.begin(); it != vec.end(); ++it) {
       printf("%02x",(unsigned char)*it);
    }
    printf( "\n");

}

std::vector<char> hex2arr(const string& hexString) 
{ 
    vector<char> byteArray; 
  
    // Loop through the hex string, two characters at a time 
    for (size_t i = 0; i < hexString.length(); i += 2) { 
        // Extract two characters representing a byte 
        string byteString = hexString.substr(i, 2); 
  
        // Convert the byte string to a uint8_t value 
        char byteValue = static_cast<char>( 
            stoi(byteString, nullptr, 16)); 
  
        // Add the byte to the byte array 
        byteArray.push_back(byteValue); 
    } 
  
    return byteArray; 
} 
void proxy::tonactive(const name& account, 
                    const vector<char>& ton_pub_key,
                    const vector<char>& msg_header_hex,
                    const vector<char>& full_msg_header_hex,
                    const vector<char>& signature,
                    const public_key& temp_amc_pub){
    require_auth(_gstate.admin);
    auto msg_packed = pack(temp_amc_pub);
    auto msg_packed_hex = to_hex_upper(msg_packed.data(),msg_packed.size());
    vector<char> msg_packed_hex_arr(msg_packed_hex.begin(), msg_packed_hex.end());
    printvector("msg_packed: ",msg_packed_hex_arr);

    printvector("ton_pub_key: ",ton_pub_key);
    printvector("msg_header1_hex: ",msg_header_hex);
    printvector("full_msg_head_hex: ",full_msg_header_hex);
    printvector("signature: ",signature);
    _ton_check(ton_pub_key, msg_header_hex, full_msg_header_hex, msg_packed_hex_arr, signature);

    auto owner_contract = _gstate_owner.owner_contracts[L2AMC_TON_NAME];
    auto accs = l2amc_owner::l2amc_account_t::idx_t(owner_contract, L2AMC_TON_NAME.value);
    auto itr = accs.find(account.value);

    std::string str_ton_key(ton_pub_key.begin(), ton_pub_key.end());
    if ( itr == accs.end()){
        public_key key;
        l2amc_owner::bind_action newaccount_act(owner_contract,{ {get_self(), ACTIVE_PERM} });
        print("newaccount_act.send");
        newaccount_act.send(_self,L2AMC_TON_NAME,str_ton_key,account,L2AMC_TON_NAME,key);
        print("newaccount_act.send" + account.to_string());

    }else {
        CHECKC(itr->xchain_pubkey == str_ton_key, err::PARAM_ERROR,"l2amc_acct already exist l2amc pubkey: " + str_ton_key )
    }
    
    l2amc_owner::updateauth_action setauth_act(owner_contract,{ {get_self(), ACTIVE_PERM} });
    setauth_act.send(_self,account,temp_amc_pub);
}

void proxy::tonsubmit(const name& account,
                    const vector<char>& msg_header1_hex,
                    const vector<char>& full_msg_head_hex,
                    const vector<char>& packed_action, 
                    const vector<char>& signature ){
    
    require_auth(account);

    auto accs = l2amc_owner::l2amc_account_t::idx_t( _gstate.owner_contract, L2AMC_BTC_NAME.value);
        
    auto itr = accs.find(account.value);
    CHECKC( itr != accs.end(), err::RECORD_NOT_FOUND,"[proxy] account not found")

    const std::vector<char> pubkey_ton (itr->xchain_pubkey.begin(), itr->xchain_pubkey.end());

    string msg = to_hex(packed_action.data(),packed_action.size());
    vector<char> packed = pack(TON_MESSAGE_MAGIC);
    vector<char> msg_packed = {num_to_var_int(msg.size())};
    for (auto it = msg.begin(); it != msg.end(); ++it) {
        msg_packed.push_back(*it);
    }
    packed.insert(packed.end(),msg_packed.begin(),msg_packed.end());

    _ton_check(pubkey_ton,msg_header1_hex,full_msg_head_hex,packed,signature); 

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
    exec_act.send( _self,L2AMC_TON_NAME, account , actions, unpacked_action.nonce);
}

unsigned char* convertVectorToUnsignedChar(const std::vector<char>& vec) {
    unsigned char* result = new unsigned char[vec.size()];
    std::copy(vec.begin(), vec.end(), result);
    return result;
}
void proxy::_ton_check( const vector<char>& ton_pub_key,
                    const vector<char>& msg_header1_hex,
                    const vector<char>& full_msg_head_hex,
                    const vector<char>& packed_msg, 
                    const vector<char>& signature ) {

    vector<char> full_msg_v1;
    full_msg_v1.insert(full_msg_v1.end(),msg_header1_hex.begin(),msg_header1_hex.end());//将vec1压入
    full_msg_v1.insert(full_msg_v1.end(),packed_msg.begin(),packed_msg.end());//继续将vec2压入
    auto hash_full_msg_v1 =  sha256(full_msg_v1);
    printvector("hash_full_msg_v1: ",full_msg_v1);
    print("hash_full_msg_v1: ");
    hash_full_msg_v1.print();
    print("\n");
    auto hash_full_msg_v1_arr_temp = hash_full_msg_v1.extract_as_byte_array();
    std::vector<char> hash_full_msg_v1_arr(hash_full_msg_v1_arr_temp.data(), hash_full_msg_v1_arr_temp.data() + hash_full_msg_v1_arr_temp.size());
    std::vector<char> full_msg_v2_arr;
    full_msg_v2_arr.insert(full_msg_v2_arr.end(),full_msg_head_hex.begin(),full_msg_head_hex.end());
    full_msg_v2_arr.insert(full_msg_v2_arr.end(),hash_full_msg_v1_arr.begin(),hash_full_msg_v1_arr.end());
    auto full_msg_v2_hash = sha256(full_msg_v2_arr);
    printvector("full_msg_v2_arr: ",full_msg_v2_arr);
    print("full_msg_v2_hash: ");
    full_msg_v2_hash.print();
    print("\n");
    auto full_msg_v2_hash_arr = full_msg_v2_hash.extract_as_byte_array();

    signature_check( convertVectorToUnsignedChar(ton_pub_key),
            full_msg_v2_hash_arr.data(), full_msg_v2_hash_arr.size(),
            convertVectorToUnsignedChar(signature));
}

 void proxy::toncheck(const vector< char>& ton_pub_key,
                    const vector< char>& msg, 
                    const vector< char>& signature ) {
        signature_check( 
                    (const unsigned char*)ton_pub_key.data(),
                    (const unsigned char*)msg.data(), msg.size(),
                    (const unsigned char*)signature.data());
}


void proxy::verify( const vector<unsigned char>& pk, const vector<unsigned char>& msg, const vector<unsigned char>& sig ){
    const unsigned char *pk1 = pk.data();
    const unsigned char *msg1 = msg.data();
    const unsigned char *msg1_sig = sig.data();
    CHECKC( pk.size() == ed25519_public_key_size, err::PARAM_ERROR,"Public key size mismatch")
    CHECKC( sig.size() == ed25519_signature_size, err::PARAM_ERROR,"Signature size mismatch")


    signature_check( pk1, msg1, msg.size(), msg1_sig);
}



extern void ecp_TrimSecretKey(U8 *X);
const unsigned char BasePoint[32] = {9};
int32_t ecp_PrintBytes(const char *name, const U8 *data, U32 size)
{
    U32 i;
    printf("\nstatic const unsigned char %s[%d] =\n  { 0x%02X", name, size, *data++);
    for (i = 1; i < size; i++)
    {
        if ((i & 15) == 0)
            printf(",\n    0x%02X", *data++);
        else
            printf(",0x%02X", *data++);
    }
    printf(" };\n");
    return 1;
}

int proxy::signature_check(
    const unsigned char *expected_pk, 
    const unsigned char *msg, size_t size, 
    const unsigned char *expected_sig)
{
   int rc = 0;
    ecp_PrintHexBytes("print public_key:", expected_pk, ed25519_public_key_size);
    ecp_PrintHexBytes("print msg", msg, size);
    ecp_PrintHexBytes("print expected_sig", expected_sig, ed25519_signature_size);
    print_w_P("begin.....");
    if (!ed25519_VerifySignature(expected_sig, expected_pk, msg, size))
    {
       CHECKC( false, err::ACCOUNT_INVALID, "Signature verification FAILED!!\n");
    }

    return rc;
}