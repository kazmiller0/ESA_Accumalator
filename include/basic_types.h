#ifndef BASIC_TYPES_H
#define BASIC_TYPES_H

#include <openssl/bn.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <string>
#include <vector>
#include <functional>
#include <unordered_set>

// 前向声明
class GroupElement;

// BigInt 类 - 大整数实现
class BigInt {
private:
    BIGNUM* value;
    
public:
    // 构造函数和析构函数
    BigInt();
    BigInt(const std::string& str, int base = 10);
    BigInt(const BigInt& other);
    BigInt(BigInt&& other) noexcept;
    ~BigInt();
    
    BigInt& operator=(const BigInt& other);
    BigInt& operator=(BigInt&& other) noexcept;
    
    // 算术运算
    BigInt operator+(const BigInt& other) const;
    BigInt operator-(const BigInt& other) const;
    BigInt operator*(const BigInt& other) const;
    BigInt operator/(const BigInt& other) const;
    BigInt operator%(const BigInt& other) const;
    BigInt operator^(const BigInt& other) const;
    
    // 比较运算
    bool operator==(const BigInt& other) const;
    bool operator!=(const BigInt& other) const;
    bool operator<(const BigInt& other) const;
    bool operator>(const BigInt& other) const;
    bool operator<=(const BigInt& other) const;
    bool operator>=(const BigInt& other) const;
    
    // 赋值运算
    BigInt& operator+=(const BigInt& other);
    BigInt& operator-=(const BigInt& other);
    BigInt& operator*=(const BigInt& other);
    BigInt& operator%=(const BigInt& other);
    
    // 工具函数
    std::string to_string(int base = 10) const;
    size_t bit_length() const;
    bool is_zero() const;
    bool is_one() const;
    
    // 静态函数
    static BigInt random(size_t bits);
    static BigInt random_range(const BigInt& min, const BigInt& max);
    static BigInt from_hex(const std::string& hex);
    static BigInt from_bytes(const std::vector<uint8_t>& bytes);
    std::vector<uint8_t> to_bytes() const;
    
    // 哈希支持
    struct Hash {
        std::size_t operator()(const BigInt& bi) const {
            return std::hash<std::string>{}(bi.to_string());
        }
    };
    
    // 内部访问器
    BIGNUM* get_bn() const { return value; }
};

// GroupElement 类 - 群元素实现
class GroupElement {
private:
    BigInt value;
    BigInt modulus;
    bool is_valid;
    
public:
    GroupElement();
    GroupElement(const BigInt& val, const BigInt& mod);
    GroupElement(const GroupElement& other);
    GroupElement(GroupElement&& other) noexcept;
    
    GroupElement& operator=(const GroupElement& other);
    GroupElement& operator=(GroupElement&& other) noexcept;
    
    // 群运算
    GroupElement operator*(const GroupElement& other) const;
    GroupElement operator^(const BigInt& exponent) const;
    GroupElement inverse() const;
    
    // 比较运算
    bool operator==(const GroupElement& other) const;
    bool operator!=(const GroupElement& other) const;
    
    // 工具函数
    std::string to_string() const;
    bool valid() const { return is_valid; }
    
    // 获取器
    const BigInt& get_value() const { return value; }
    const BigInt& get_modulus() const { return modulus; }
    
    // 静态函数
    static GroupElement generator(const BigInt& modulus);
    static GroupElement identity(const BigInt& modulus);
    static GroupElement random(const BigInt& modulus);
    
    // 辅助函数
    static bool is_primitive_root(const BigInt& g, const BigInt& p, const BigInt& phi, 
                                 const std::vector<BigInt>& prime_factors);
    static std::vector<BigInt> get_prime_factors(const BigInt& n);
};

// 零知识证明类型
enum class ProofType {
    MEMBERSHIP,
    NON_MEMBERSHIP,
    SET_OPERATION
};

// ZeroKnowledgeProof 类 - 零知识证明实现
class ZeroKnowledgeProof {
public:
    ProofType type;
    GroupElement commitment;
    BigInt challenge;
    BigInt response;
    BigInt randomness;
    std::vector<GroupElement> auxiliary_data;
    bool is_valid;
    
    ZeroKnowledgeProof(ProofType t = ProofType::MEMBERSHIP);
    ZeroKnowledgeProof(const ZeroKnowledgeProof& other);
    ZeroKnowledgeProof(ZeroKnowledgeProof&& other) noexcept;
    
    ZeroKnowledgeProof& operator=(const ZeroKnowledgeProof& other);
    ZeroKnowledgeProof& operator=(ZeroKnowledgeProof&& other) noexcept;
    
    // 序列化
    std::string serialize() const;
    static ZeroKnowledgeProof deserialize(const std::string& data);
    
    // 验证
    bool verify() const;
};

// 密码学工具函数命名空间
namespace CryptoUtils {
    BigInt sha256(const BigInt& input);
    BigInt sha3_256(const BigInt& input);
    BigInt hash_to_group(const BigInt& input, const BigInt& modulus);
    
    bool miller_rabin(const BigInt& n, int rounds = 40);
    bool is_prime(const BigInt& n, int rounds = 40);
    BigInt generate_prime(size_t bits);
    BigInt generate_safe_prime(size_t bits);
    
    BigInt mod_inverse(const BigInt& a, const BigInt& m);
    BigInt mod_pow(const BigInt& base, const BigInt& exp, const BigInt& mod);
    BigInt mod_sqrt(const BigInt& a, const BigInt& p);
    
    BigInt random_bits(size_t bits);
    BigInt random_range(const BigInt& min, const BigInt& max);
    
    std::string to_hex(const BigInt& value);
    BigInt from_hex(const std::string& hex);
    std::vector<uint8_t> to_bytes(const BigInt& value);
    BigInt from_bytes(const std::vector<uint8_t>& bytes);
    
    GroupElement hash_to_elliptic_curve(const BigInt& input, const BigInt& p, const BigInt& a, const BigInt& b);
    bool is_quadratic_residue(const BigInt& a, const BigInt& p);
}

#endif // BASIC_TYPES_H
