#include "basic_types.h"
#include <openssl/bn.h>
#include <openssl/rand.h>
#include <sstream>
#include <iomanip>

// BigInt 实现
BigInt::BigInt() {
    value = BN_new();
    BN_zero(value);
}

BigInt::BigInt(const std::string& str, int base) {
    value = BN_new();
    if (base == 16) {
        BN_hex2bn(&value, str.c_str());
    } else {
        BN_dec2bn(&value, str.c_str());
    }
}

BigInt::BigInt(const BigInt& other) {
    value = BN_new();
    BN_copy(value, other.value);
}

BigInt::BigInt(BigInt&& other) noexcept : value(other.value) {
    other.value = nullptr;
}

BigInt::~BigInt() {
    if (value) {
        BN_free(value);
    }
}

BigInt& BigInt::operator=(const BigInt& other) {
    if (this != &other) {
        BN_copy(value, other.value);
    }
    return *this;
}

BigInt& BigInt::operator=(BigInt&& other) noexcept {
    if (this != &other) {
        if (value) {
            BN_free(value);
        }
        value = other.value;
        other.value = nullptr;
    }
    return *this;
}

// 算术运算
BigInt BigInt::operator+(const BigInt& other) const {
    BigInt result;
    BN_add(result.value, value, other.value);
    return result;
}

BigInt BigInt::operator-(const BigInt& other) const {
    BigInt result;
    BN_sub(result.value, value, other.value);
    return result;
}

BigInt BigInt::operator*(const BigInt& other) const {
    BigInt result;
    BN_CTX* ctx = BN_CTX_new();
    BN_mul(result.value, value, other.value, ctx);
    BN_CTX_free(ctx);
    return result;
}

BigInt BigInt::operator/(const BigInt& other) const {
    BigInt result;
    BN_CTX* ctx = BN_CTX_new();
    BN_div(result.value, nullptr, value, other.value, ctx);
    BN_CTX_free(ctx);
    return result;
}

BigInt BigInt::operator%(const BigInt& other) const {
    BigInt result;
    BN_CTX* ctx = BN_CTX_new();
    BN_mod(result.value, value, other.value, ctx);
    BN_CTX_free(ctx);
    return result;
}

BigInt BigInt::operator^(const BigInt& other) const {
    BigInt result;
    BN_CTX* ctx = BN_CTX_new();
    BN_mod_exp(result.value, value, other.value, other.value, ctx);
    BN_CTX_free(ctx);
    return result;
}

// 比较运算
bool BigInt::operator==(const BigInt& other) const {
    return BN_cmp(value, other.value) == 0;
}

bool BigInt::operator!=(const BigInt& other) const {
    return BN_cmp(value, other.value) != 0;
}

bool BigInt::operator<(const BigInt& other) const {
    return BN_cmp(value, other.value) < 0;
}

bool BigInt::operator>(const BigInt& other) const {
    return BN_cmp(value, other.value) > 0;
}

bool BigInt::operator<=(const BigInt& other) const {
    return BN_cmp(value, other.value) <= 0;
}

bool BigInt::operator>=(const BigInt& other) const {
    return BN_cmp(value, other.value) >= 0;
}

// 赋值运算
BigInt& BigInt::operator+=(const BigInt& other) {
    BN_add(value, value, other.value);
    return *this;
}

BigInt& BigInt::operator-=(const BigInt& other) {
    BN_sub(value, value, other.value);
    return *this;
}

BigInt& BigInt::operator*=(const BigInt& other) {
    BN_CTX* ctx = BN_CTX_new();
    BN_mul(value, value, other.value, ctx);
    BN_CTX_free(ctx);
    return *this;
}

BigInt& BigInt::operator%=(const BigInt& other) {
    BN_CTX* ctx = BN_CTX_new();
    BN_mod(value, value, other.value, ctx);
    BN_CTX_free(ctx);
    return *this;
}

// 工具函数
std::string BigInt::to_string(int base) const {
    char* str = nullptr;
    if (base == 16) {
        str = BN_bn2hex(value);
    } else {
        str = BN_bn2dec(value);
    }
    
    std::string result(str);
    OPENSSL_free(str);
    return result;
}

size_t BigInt::bit_length() const {
    return BN_num_bits(value);
}

bool BigInt::is_zero() const {
    return BN_is_zero(value);
}

bool BigInt::is_one() const {
    return BN_is_one(value);
}

// 静态函数
BigInt BigInt::random(size_t bits) {
    BigInt result;
    BN_rand(result.value, bits, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);
    return result;
}

BigInt BigInt::random_range(const BigInt& min, const BigInt& max) {
    BigInt range = max - min;
    BigInt random_val = random(range.bit_length());
    return min + (random_val % range);
}

BigInt BigInt::from_hex(const std::string& hex) {
    return BigInt(hex, 16);
}

BigInt BigInt::from_bytes(const std::vector<uint8_t>& bytes) {
    BigInt result;
    BN_bin2bn(bytes.data(), bytes.size(), result.value);
    return result;
}

std::vector<uint8_t> BigInt::to_bytes() const {
    std::vector<uint8_t> result(BN_num_bytes(value));
    BN_bn2bin(value, result.data());
    return result;
}

// 密码学工具函数实现
namespace CryptoUtils {
    BigInt sha256(const BigInt& input) {
        std::vector<uint8_t> input_bytes = input.to_bytes();
        uint8_t hash[SHA256_DIGEST_LENGTH];
        SHA256(input_bytes.data(), input_bytes.size(), hash);
        
        return BigInt::from_bytes(std::vector<uint8_t>(hash, hash + SHA256_DIGEST_LENGTH));
    }
    
    BigInt sha3_256(const BigInt& input) {
        std::vector<uint8_t> input_bytes = input.to_bytes();
        uint8_t hash[32];
        
        EVP_MD_CTX* ctx = EVP_MD_CTX_new();
        const EVP_MD* md = EVP_sha3_256();
        
        EVP_DigestInit_ex(ctx, md, nullptr);
        EVP_DigestUpdate(ctx, input_bytes.data(), input_bytes.size());
        EVP_DigestFinal_ex(ctx, hash, nullptr);
        EVP_MD_CTX_free(ctx);
        
        return BigInt::from_bytes(std::vector<uint8_t>(hash, hash + 32));
    }
    
    BigInt hash_to_group(const BigInt& input, const BigInt& modulus) {
        BigInt hash_result = sha256(input);
        return hash_result % modulus;
    }
    
    bool miller_rabin(const BigInt& n, int rounds) {
        if (n.is_zero() || n.is_one()) return false;
        if (n == BigInt("2")) return true;
        if (n % BigInt("2") == BigInt("0")) return false;
        
        BigInt d = n - BigInt("1");
        int s = 0;
        while (d % BigInt("2") == BigInt("0")) {
            d = d / BigInt("2");
            s++;
        }
        
        for (int i = 0; i < rounds; i++) {
            BigInt a = BigInt::random_range(BigInt("2"), n - BigInt("1"));
            BigInt x = mod_pow(a, d, n);
            
            if (x == BigInt("1") || x == n - BigInt("1")) {
                continue;
            }
            
            bool composite = true;
            for (int j = 0; j < s - 1; j++) {
                x = (x * x) % n;
                if (x == n - BigInt("1")) {
                    composite = false;
                    break;
                }
            }
            
            if (composite) {
                return false;
            }
        }
        
        return true;
    }
    
    bool is_prime(const BigInt& n, int rounds) {
        if (n < BigInt("2")) return false;
        if (n == BigInt("2")) return true;
        if (n % BigInt("2") == BigInt("0")) return false;
        
        return miller_rabin(n, rounds);
    }
    
    BigInt generate_prime(size_t bits) {
        BigInt candidate;
        do {
            candidate = BigInt::random(bits);
            // 确保是奇数
            if (candidate % BigInt("2") == BigInt("0")) {
                candidate += BigInt("1");
            }
        } while (!is_prime(candidate, 40));
        
        return candidate;
    }
    
    BigInt generate_safe_prime(size_t bits) {
        BigInt candidate;
        do {
            candidate = BigInt::random(bits - 1);
            candidate = candidate * BigInt("2") + BigInt("1");
        } while (!is_prime(candidate, 40) || !is_prime((candidate - BigInt("1")) / BigInt("2"), 40));
        
        return candidate;
    }
    
    BigInt mod_inverse(const BigInt& a, const BigInt& m) {
        BigInt result;
        BN_CTX* ctx = BN_CTX_new();
        BN_mod_inverse(result.get_bn(), a.get_bn(), m.get_bn(), ctx);
        BN_CTX_free(ctx);
        return result;
    }
    
    BigInt mod_pow(const BigInt& base, const BigInt& exp, const BigInt& mod) {
        BigInt result;
        BN_CTX* ctx = BN_CTX_new();
        BN_mod_exp(result.get_bn(), base.get_bn(), exp.get_bn(), mod.get_bn(), ctx);
        BN_CTX_free(ctx);
        return result;
    }
    
    BigInt mod_sqrt(const BigInt& a, const BigInt& p) {
        // Tonelli-Shanks算法实现平方根
        if (a == BigInt("0")) return BigInt("0");
        if (a == BigInt("1")) return BigInt("1");
        
        BigInt p_minus_1 = p - BigInt("1");
        BigInt q = p_minus_1;
        int s = 0;
        while (q % BigInt("2") == BigInt("0")) {
            q = q / BigInt("2");
            s++;
        }
        
        if (s == 1) {
            return mod_pow(a, (p + BigInt("1")) / BigInt("4"), p);
        }
        
        // 寻找二次非剩余
        BigInt z = BigInt("2");
        while (mod_pow(z, p_minus_1 / BigInt("2"), p) != p_minus_1) {
            z += BigInt("1");
        }
        
        BigInt c = mod_pow(z, q, p);
        BigInt x = mod_pow(a, (q + BigInt("1")) / BigInt("2"), p);
        BigInt t = mod_pow(a, q, p);
        int m = s;
        
        while (t != BigInt("1")) {
            BigInt tt = t;
            int i = 1;
            while (i < m && mod_pow(tt, BigInt("2"), p) != BigInt("1")) {
                tt = (tt * tt) % p;
                i++;
            }
            
            BigInt b = mod_pow(c, mod_pow(BigInt("2"), BigInt(std::to_string(m - i - 1)), p), p);
            x = (x * b) % p;
            t = (t * b * b) % p;
            c = (b * b) % p;
            m = i;
        }
        
        return x;
    }
    
    BigInt random_bits(size_t bits) {
        return BigInt::random(bits);
    }
    
    BigInt random_range(const BigInt& min, const BigInt& max) {
        return BigInt::random_range(min, max);
    }
    
    std::string to_hex(const BigInt& value) {
        return value.to_string(16);
    }
    
    BigInt from_hex(const std::string& hex) {
        return BigInt::from_hex(hex);
    }
    
    std::vector<uint8_t> to_bytes(const BigInt& value) {
        return value.to_bytes();
    }
    
    BigInt from_bytes(const std::vector<uint8_t>& bytes) {
        return BigInt::from_bytes(bytes);
    }
    
    GroupElement hash_to_elliptic_curve(const BigInt& input, const BigInt& p, const BigInt& a, const BigInt& b) {
        // 简化的椭圆曲线哈希实现
        BigInt hash_result = sha256(input);
        BigInt x = hash_result % p;
        
        while (true) {
            BigInt y_squared = (mod_pow(x, BigInt("3"), p) + a * x + b) % p;
            if (is_quadratic_residue(y_squared, p)) {
                BigInt y = mod_sqrt(y_squared, p);
                return GroupElement(y, p);
            }
            x = (x + BigInt("1")) % p;
        }
    }
    
    bool is_quadratic_residue(const BigInt& a, const BigInt& p) {
        return mod_pow(a, (p - BigInt("1")) / BigInt("2"), p) == BigInt("1");
    }
}
