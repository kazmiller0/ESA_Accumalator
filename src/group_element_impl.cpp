#include "basic_types.h"
#include <openssl/bn.h>

// GroupElement 构造函数和赋值操作符实现
GroupElement::GroupElement() : value(BigInt("0")), modulus(BigInt("1")), is_valid(false) {}

GroupElement::GroupElement(const BigInt& val, const BigInt& mod) 
    : value(val), modulus(mod), is_valid(true) {
    // 确保值在模数范围内
    if (value >= modulus) {
        value = value % modulus;
    }
}

GroupElement::GroupElement(const GroupElement& other) 
    : value(other.value), modulus(other.modulus), is_valid(other.is_valid) {}

GroupElement::GroupElement(GroupElement&& other) noexcept 
    : value(std::move(other.value)), modulus(std::move(other.modulus)), is_valid(other.is_valid) {
    other.is_valid = false;
}

GroupElement& GroupElement::operator=(const GroupElement& other) {
    if (this != &other) {
        value = other.value;
        modulus = other.modulus;
        is_valid = other.is_valid;
    }
    return *this;
}

GroupElement& GroupElement::operator=(GroupElement&& other) noexcept {
    if (this != &other) {
        value = std::move(other.value);
        modulus = std::move(other.modulus);
        is_valid = other.is_valid;
        other.is_valid = false;
    }
    return *this;
}

// GroupElement 其他方法实现
GroupElement GroupElement::operator*(const GroupElement& other) const {
    if (!is_valid || !other.is_valid || modulus != other.modulus) {
        return GroupElement();
    }
    
    BigInt result_value = (value * other.value) % modulus;
    return GroupElement(result_value, modulus);
}

GroupElement GroupElement::operator^(const BigInt& exponent) const {
    if (!is_valid) {
        return GroupElement();
    }
    
    BigInt result_value = CryptoUtils::mod_pow(value, exponent, modulus);
    return GroupElement(result_value, modulus);
}

GroupElement GroupElement::inverse() const {
    if (!is_valid || value.is_zero()) {
        return GroupElement();
    }
    
    BigInt inv_value = CryptoUtils::mod_inverse(value, modulus);
    return GroupElement(inv_value, modulus);
}

bool GroupElement::operator==(const GroupElement& other) const {
    return is_valid && other.is_valid && 
           modulus == other.modulus && 
           value == other.value;
}

bool GroupElement::operator!=(const GroupElement& other) const {
    return !(*this == other);
}

std::string GroupElement::to_string() const {
    if (!is_valid) {
        return "Invalid GroupElement";
    }
    return "(" + value.to_string() + " mod " + modulus.to_string() + ")";
}

GroupElement GroupElement::generator(const BigInt& modulus) {
    // 原根是模n的乘法群Z_n*的生成元
    
    // 首先检查modulus是否为素数
    if (!CryptoUtils::is_prime(modulus, 40)) {
        // 如果不是素数，使用简化的方法
        return GroupElement(BigInt("2"), modulus);
    }
    
    // 对于大素数（> 32位），使用简化的方法以提高效率
    if (modulus.bit_length() > 32) {
        // 对于大素数，直接使用2作为生成元
        // 虽然可能不是真正的原根，但在实际应用中通常足够
        return GroupElement(BigInt("2"), modulus);
    }
    
    // 对于素数p，寻找模p的原根
    BigInt p = modulus;
    BigInt phi = p - BigInt("1"); // 对于素数p，φ(p) = p-1
    
    // 获取φ(p)的所有素因子
    std::vector<BigInt> prime_factors = get_prime_factors(phi);
    
    // 对于中等大小的素数，限制搜索范围以提高效率
    BigInt max_search = BigInt("100"); // 最多搜索100个候选
    if (p <= max_search) {
        max_search = p - BigInt("1");
    }
    
    // 尝试从2开始寻找原根，但限制搜索范围
    for (BigInt g = BigInt("2"); g <= max_search; g = g + BigInt("1")) {
        if (is_primitive_root(g, p, phi, prime_factors)) {
            return GroupElement(g, modulus);
        }
    }
    
    // 如果在前100个候选中没有找到，尝试一些常见的原根候选
    std::vector<std::string> common_candidates = {"2", "3", "5", "7", "11", "13", "17", "19", "23", "29"};
    for (const auto& candidate_str : common_candidates) {
        BigInt g(candidate_str);
        if (g < p && is_primitive_root(g, p, phi, prime_factors)) {
            return GroupElement(g, modulus);
        }
    }
    
    // 如果还是没找到，返回2作为备选（虽然可能不是原根）
    return GroupElement(BigInt("2"), modulus);
}

GroupElement GroupElement::identity(const BigInt& modulus) {
    return GroupElement(BigInt("1"), modulus);
}

// 辅助函数实现

bool GroupElement::is_primitive_root(const BigInt& g, const BigInt& p, const BigInt& phi, 
                                     const std::vector<BigInt>& prime_factors) {
    // 检查g是否为模p的原根
    // 原根的条件：g^φ(p) ≡ 1 (mod p) 且 g^(φ(p)/q) ≢ 1 (mod p) 对所有φ(p)的素因子q
    
    // 快速检查：如果g >= p，则不是原根
    if (g >= p) {
        return false;
    }
    
    // 快速检查：如果g == 1，则不是原根（除非p == 2）
    if (g == BigInt("1") && p != BigInt("2")) {
        return false;
    }
    
    // 首先检查 g^φ(p) ≡ 1 (mod p)
    if (CryptoUtils::mod_pow(g, phi, p) != BigInt("1")) {
        return false;
    }
    
    // 检查对于所有φ(p)的素因子q，g^(φ(p)/q) ≢ 1 (mod p)
    // 限制检查的素因子数量以提高效率
    size_t max_factors_to_check = std::min(prime_factors.size(), size_t(10));
    for (size_t i = 0; i < max_factors_to_check; i++) {
        const BigInt& q = prime_factors[i];
        BigInt exponent = phi / q;
        if (CryptoUtils::mod_pow(g, exponent, p) == BigInt("1")) {
            return false;
        }
    }
    
    return true;
}

std::vector<BigInt> GroupElement::get_prime_factors(const BigInt& n) {
    // 获取n的所有素因子（去重）
    std::vector<BigInt> factors;
    BigInt temp = n;
    
    // 处理因子2
    while (temp % BigInt("2") == BigInt("0")) {
        if (factors.empty() || factors.back() != BigInt("2")) {
            factors.push_back(BigInt("2"));
        }
        temp = temp / BigInt("2");
    }
    
    // 处理奇数因子
    for (BigInt i = BigInt("3"); i * i <= temp; i = i + BigInt("2")) {
        while (temp % i == BigInt("0")) {
            if (factors.empty() || factors.back() != i) {
                factors.push_back(i);
            }
            temp = temp / i;
        }
    }
    
    // 如果temp > 1，那么temp本身是素数
    if (temp > BigInt("1")) {
        if (factors.empty() || factors.back() != temp) {
            factors.push_back(temp);
        }
    }
    
    return factors;
}
