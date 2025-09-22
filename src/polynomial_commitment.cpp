#include "polynomial_commitment.h"
#include <iostream>
#include <sstream>
#include <algorithm>

// ==================== Polynomial 实现 ====================

Polynomial::Polynomial(const BigInt& mod) : modulus(mod) {
    coefficients.push_back(BigInt("0"));
}

Polynomial::Polynomial(const std::vector<BigInt>& coeffs, const BigInt& mod) 
    : coefficients(coeffs), modulus(mod) {
    // 移除前导零
    while (coefficients.size() > 1 && coefficients.back().is_zero()) {
        coefficients.pop_back();
    }
}

Polynomial::Polynomial(const Polynomial& other) 
    : coefficients(other.coefficients), modulus(other.modulus) {}

Polynomial::Polynomial(Polynomial&& other) noexcept 
    : coefficients(std::move(other.coefficients)), modulus(std::move(other.modulus)) {}

Polynomial& Polynomial::operator=(const Polynomial& other) {
    if (this != &other) {
        coefficients = other.coefficients;
        modulus = other.modulus;
    }
    return *this;
}

Polynomial& Polynomial::operator=(Polynomial&& other) noexcept {
    if (this != &other) {
        coefficients = std::move(other.coefficients);
        modulus = std::move(other.modulus);
    }
    return *this;
}

BigInt Polynomial::evaluate(const BigInt& x) const {
    BigInt result = BigInt("0");
    BigInt x_power = BigInt("1");
    
    for (size_t i = 0; i < coefficients.size(); ++i) {
        BigInt term = (coefficients[i] * x_power) % modulus;
        result = (result + term) % modulus;
        x_power = (x_power * x) % modulus;
    }
    
    return result;
}

Polynomial Polynomial::operator+(const Polynomial& other) const {
    if (modulus != other.modulus) {
        throw std::invalid_argument("多项式模数不匹配");
    }
    
    size_t max_size = std::max(coefficients.size(), other.coefficients.size());
    std::vector<BigInt> result_coeffs(max_size, BigInt("0"));
    
    for (size_t i = 0; i < max_size; ++i) {
        BigInt coeff1 = (i < coefficients.size()) ? coefficients[i] : BigInt("0");
        BigInt coeff2 = (i < other.coefficients.size()) ? other.coefficients[i] : BigInt("0");
        result_coeffs[i] = (coeff1 + coeff2) % modulus;
    }
    
    return Polynomial(result_coeffs, modulus);
}

Polynomial Polynomial::operator-(const Polynomial& other) const {
    if (modulus != other.modulus) {
        throw std::invalid_argument("多项式模数不匹配");
    }
    
    size_t max_size = std::max(coefficients.size(), other.coefficients.size());
    std::vector<BigInt> result_coeffs(max_size, BigInt("0"));
    
    for (size_t i = 0; i < max_size; ++i) {
        BigInt coeff1 = (i < coefficients.size()) ? coefficients[i] : BigInt("0");
        BigInt coeff2 = (i < other.coefficients.size()) ? other.coefficients[i] : BigInt("0");
        result_coeffs[i] = (coeff1 - coeff2 + modulus) % modulus;
    }
    
    return Polynomial(result_coeffs, modulus);
}

Polynomial Polynomial::operator*(const Polynomial& other) const {
    if (modulus != other.modulus) {
        throw std::invalid_argument("多项式模数不匹配");
    }
    
    size_t result_size = coefficients.size() + other.coefficients.size() - 1;
    std::vector<BigInt> result_coeffs(result_size, BigInt("0"));
    
    for (size_t i = 0; i < coefficients.size(); ++i) {
        for (size_t j = 0; j < other.coefficients.size(); ++j) {
            BigInt product = (coefficients[i] * other.coefficients[j]) % modulus;
            result_coeffs[i + j] = (result_coeffs[i + j] + product) % modulus;
        }
    }
    
    return Polynomial(result_coeffs, modulus);
}

Polynomial Polynomial::operator*(const BigInt& scalar) const {
    std::vector<BigInt> result_coeffs;
    for (const auto& coeff : coefficients) {
        result_coeffs.push_back((coeff * scalar) % modulus);
    }
    return Polynomial(result_coeffs, modulus);
}

Polynomial Polynomial::derivative() const {
    if (coefficients.size() <= 1) {
        return Polynomial(modulus);
    }
    
    std::vector<BigInt> result_coeffs;
    for (size_t i = 1; i < coefficients.size(); ++i) {
        BigInt coeff = (coefficients[i] * BigInt(std::to_string(i))) % modulus;
        result_coeffs.push_back(coeff);
    }
    
    return Polynomial(result_coeffs, modulus);
}

Polynomial Polynomial::integrate() const {
    std::vector<BigInt> result_coeffs;
    result_coeffs.push_back(BigInt("0"));  // 常数项
    
    for (size_t i = 0; i < coefficients.size(); ++i) {
        BigInt coeff = (coefficients[i] * CryptoUtils::mod_inverse(BigInt(std::to_string(i + 1)), modulus)) % modulus;
        result_coeffs.push_back(coeff);
    }
    
    return Polynomial(result_coeffs, modulus);
}

std::pair<Polynomial, Polynomial> Polynomial::divide(const Polynomial& divisor) const {
    if (divisor.degree() == 0 && divisor.coefficients[0].is_zero()) {
        throw std::invalid_argument("除数不能为零多项式");
    }
    
    Polynomial dividend = *this;
    std::vector<BigInt> quotient_coeffs;
    
    while (dividend.degree() >= divisor.degree()) {
        BigInt leading_coeff = dividend.coefficients.back();
        BigInt divisor_leading = divisor.coefficients.back();
        BigInt coeff = (leading_coeff * CryptoUtils::mod_inverse(divisor_leading, modulus)) % modulus;
        
        quotient_coeffs.push_back(coeff);
        
        // 从被除数中减去 coeff * divisor * x^(dividend.degree() - divisor.degree())
        Polynomial term = divisor * coeff;
        for (size_t i = 0; i < dividend.degree() - divisor.degree(); ++i) {
            term.coefficients.insert(term.coefficients.begin(), BigInt("0"));
        }
        
        dividend = dividend - term;
    }
    
    std::reverse(quotient_coeffs.begin(), quotient_coeffs.end());
    return {Polynomial(quotient_coeffs, modulus), dividend};
}

size_t Polynomial::degree() const {
    if (coefficients.size() <= 1) {
        return 0;
    }
    return coefficients.size() - 1;
}

BigInt Polynomial::get_coefficient(size_t index) const {
    if (index >= coefficients.size()) {
        return BigInt("0");
    }
    return coefficients[index];
}

void Polynomial::set_coefficient(size_t index, const BigInt& value) {
    if (index >= coefficients.size()) {
        coefficients.resize(index + 1, BigInt("0"));
    }
    coefficients[index] = value % modulus;
}

Polynomial Polynomial::from_roots(const std::vector<BigInt>& roots, const BigInt& modulus) {
    if (roots.empty()) {
        return Polynomial(modulus);
    }
    
    Polynomial result(std::vector<BigInt>{BigInt("1")}, modulus);
    
    for (const auto& root : roots) {
        // 乘以 (x - root)
        Polynomial factor(std::vector<BigInt>{modulus - root, BigInt("1")}, modulus);
        result = result * factor;
    }
    
    return result;
}

Polynomial Polynomial::lagrange_interpolation(const std::vector<BigInt>& x_values, 
                                            const std::vector<BigInt>& y_values, 
                                            const BigInt& modulus) {
    if (x_values.size() != y_values.size()) {
        throw std::invalid_argument("x值和y值数量不匹配");
    }
    
    size_t n = x_values.size();
    std::vector<BigInt> result_coeffs(n, BigInt("0"));
    
    for (size_t i = 0; i < n; ++i) {
        // 计算拉格朗日基多项式 L_i(x)
        Polynomial basis(std::vector<BigInt>{BigInt("1")}, modulus);
        BigInt denominator = BigInt("1");
        
        for (size_t j = 0; j < n; ++j) {
            if (i != j) {
                // 乘以 (x - x_j)
                Polynomial factor(std::vector<BigInt>{modulus - x_values[j], BigInt("1")}, modulus);
                basis = basis * factor;
                
                // 计算分母
                denominator = (denominator * (x_values[i] - x_values[j] + modulus)) % modulus;
            }
        }
        
        // 除以分母
        BigInt inv_denominator = CryptoUtils::mod_inverse(denominator, modulus);
        basis = basis * inv_denominator;
        
        // 乘以 y_i
        basis = basis * y_values[i];
        
        // 加到结果中
        for (size_t k = 0; k < basis.coefficients.size(); ++k) {
            result_coeffs[k] = (result_coeffs[k] + basis.coefficients[k]) % modulus;
        }
    }
    
    return Polynomial(result_coeffs, modulus);
}

// ==================== KZGParams 实现 ====================

KZGParams::KZGParams(const GroupElement& generator, size_t max_deg, const BigInt& mod) 
    : g(generator), modulus(mod), max_degree(max_deg) {
    
    // 预计算 g^1, g^2, ..., g^max_degree
    g_powers.reserve(max_degree + 1);
    g_powers.push_back(GroupElement::identity(mod));  // g^0 = 1
    g_powers.push_back(g);  // g^1
    
    for (size_t i = 2; i <= max_degree; ++i) {
        g_powers.push_back(g_powers[i-1] * g);
    }
}

bool KZGParams::is_valid() const {
    return g.valid() && !g_powers.empty() && max_degree > 0;
}

// ==================== KZGCommitment 实现 ====================

KZGCommitment::KZGCommitment(const Polynomial& poly, const KZGParams& kzg_params) 
    : polynomial(poly), params(kzg_params) {
    
    if (!params.is_valid()) {
        throw std::invalid_argument("KZG参数无效");
    }
    
    if (poly.degree() > params.max_degree) {
        throw std::invalid_argument("多项式次数超过最大限制");
    }
    
    // 计算承诺: C = g^c_0 * g^c_1 * ... * g^(c_d * d)
    commitment = GroupElement::identity(params.modulus);
    
    for (size_t i = 0; i < polynomial.get_coefficients().size(); ++i) {
        if (!polynomial.get_coefficient(i).is_zero()) {
            GroupElement term = params.g_powers[i] ^ polynomial.get_coefficient(i);
            commitment = commitment * term;
        }
    }
}

KZGCommitment::OpeningProof KZGCommitment::open(const BigInt& point) const {
    OpeningProof proof;
    
    // 计算多项式在point处的值
    BigInt value = polynomial.evaluate(point);
    
    // 构造商多项式 q(x) = (f(x) - f(z)) / (x - z)
    Polynomial numerator = polynomial - Polynomial(std::vector<BigInt>{value}, polynomial.get_modulus());
    Polynomial denominator(std::vector<BigInt>{polynomial.get_modulus() - point, BigInt("1")}, polynomial.get_modulus());
    
    auto [quotient, remainder] = numerator.divide(denominator);
    
    if (!remainder.get_coefficient(0).is_zero()) {
        // 余数不为零，说明point不是根
        proof.is_valid = false;
        return proof;
    }
    
    // 计算商多项式的承诺
    KZGCommitment quotient_commitment(quotient, params);
    proof.quotient_commitment = quotient_commitment.get_commitment();
    proof.quotient_evaluation = quotient.evaluate(point);
    proof.is_valid = true;
    
    return proof;
}

bool KZGCommitment::verify_opening(const OpeningProof& proof, const BigInt& point, const BigInt& value) const {
    if (!proof.is_valid) {
        return false;
    }
    
    // 验证: e(C - g^value, g) = e(q_commitment, g^point - g)
    // 这里简化为检查承诺的一致性
    
    // 重新计算商多项式
    Polynomial numerator = polynomial - Polynomial(std::vector<BigInt>{value}, polynomial.get_modulus());
    Polynomial denominator(std::vector<BigInt>{polynomial.get_modulus() - point, BigInt("1")}, polynomial.get_modulus());
    
    auto [quotient, remainder] = numerator.divide(denominator);
    
    if (!remainder.get_coefficient(0).is_zero()) {
        return false;
    }
    
    // 验证商多项式承诺
    KZGCommitment expected_quotient_commitment(quotient, params);
    return proof.quotient_commitment == expected_quotient_commitment.get_commitment();
}

// ==================== PolynomialAccumulator 实现 ====================

PolynomialAccumulator::PolynomialAccumulator(const GroupElement& generator, size_t max_degree, const BigInt& modulus) 
    : kzg_params(generator, max_degree, modulus), current_modulus(modulus) {}

bool PolynomialAccumulator::add_element(const BigInt& element) {
    if (element_to_commitment_index.find(element) != element_to_commitment_index.end()) {
        return false;  // 元素已存在
    }
    
    // 创建成员关系多项式
    std::vector<BigInt> elements = {element};
    Polynomial membership_poly = create_membership_polynomial(elements);
    
    // 创建承诺
    KZGCommitment commitment(membership_poly, kzg_params);
    commitments.push_back(commitment);
    element_to_commitment_index[element] = commitments.size() - 1;
    
    return true;
}

bool PolynomialAccumulator::remove_element(const BigInt& element) {
    auto it = element_to_commitment_index.find(element);
    if (it == element_to_commitment_index.end()) {
        return false;  // 元素不存在
    }
    
    size_t index = it->second;
    commitments.erase(commitments.begin() + index);
    element_to_commitment_index.erase(it);
    
    // 更新索引
    for (auto& pair : element_to_commitment_index) {
        if (pair.second > index) {
            pair.second--;
        }
    }
    
    return true;
}

bool PolynomialAccumulator::contains(const BigInt& element) const {
    return element_to_commitment_index.find(element) != element_to_commitment_index.end();
}

Polynomial PolynomialAccumulator::create_membership_polynomial(const std::vector<BigInt>& elements) {
    // 创建成员关系多项式: f(x) = (x - e1)(x - e2)...(x - en)
    return Polynomial::from_roots(elements, current_modulus);
}

Polynomial PolynomialAccumulator::create_non_membership_polynomial(const BigInt& element, const std::vector<BigInt>& elements) {
    // 创建非成员关系多项式: f(x) = (x - e1)(x - e2)...(x - en) / (x - element)
    Polynomial membership_poly = create_membership_polynomial(elements);
    Polynomial divisor(std::vector<BigInt>{current_modulus - element, BigInt("1")}, current_modulus);
    
    auto [quotient, remainder] = membership_poly.divide(divisor);
    return quotient;
}

KZGCommitment PolynomialAccumulator::create_membership_commitment(const std::vector<BigInt>& elements) {
    Polynomial poly = create_membership_polynomial(elements);
    return KZGCommitment(poly, kzg_params);
}

KZGCommitment PolynomialAccumulator::create_non_membership_commitment(const BigInt& element, const std::vector<BigInt>& elements) {
    Polynomial poly = create_non_membership_polynomial(element, elements);
    return KZGCommitment(poly, kzg_params);
}

PolynomialAccumulator::ZKProof PolynomialAccumulator::generate_membership_proof(const BigInt& element) {
    ZKProof proof;
    
    if (!contains(element)) {
        return proof;  // 元素不存在，无法生成证明
    }
    
    // 获取所有元素
    std::vector<BigInt> elements;
    for (const auto& pair : element_to_commitment_index) {
        elements.push_back(pair.first);
    }
    
    // 创建成员关系承诺
    KZGCommitment commitment = create_membership_commitment(elements);
    
    // 生成打开证明
    proof.opening_proof = commitment.open(element);
    proof.challenge = generate_challenge("membership_" + element.to_string());
    proof.response = generate_challenge("response_" + proof.challenge.to_string());
    proof.is_valid = proof.opening_proof.is_valid;
    
    return proof;
}

PolynomialAccumulator::ZKProof PolynomialAccumulator::generate_non_membership_proof(const BigInt& element) {
    ZKProof proof;
    
    if (contains(element)) {
        return proof;  // 元素存在，无法生成非成员关系证明
    }
    
    // 获取所有元素
    std::vector<BigInt> elements;
    for (const auto& pair : element_to_commitment_index) {
        elements.push_back(pair.first);
    }
    
    // 创建非成员关系承诺
    KZGCommitment commitment = create_non_membership_commitment(element, elements);
    
    // 生成打开证明
    proof.opening_proof = commitment.open(element);
    proof.challenge = generate_challenge("non_membership_" + element.to_string());
    proof.response = generate_challenge("response_" + proof.challenge.to_string());
    proof.is_valid = proof.opening_proof.is_valid;
    
    return proof;
}

bool PolynomialAccumulator::verify_membership_proof(const ZKProof& proof, const BigInt& element) {
    if (!proof.is_valid) {
        return false;
    }
    
    // 获取所有元素
    std::vector<BigInt> elements;
    for (const auto& pair : element_to_commitment_index) {
        elements.push_back(pair.first);
    }
    
    // 创建期望的承诺
    KZGCommitment expected_commitment = create_membership_commitment(elements);
    
    // 验证打开证明
    return expected_commitment.verify_opening(proof.opening_proof, element, BigInt("0"));
}

bool PolynomialAccumulator::verify_non_membership_proof(const ZKProof& proof, const BigInt& element) {
    if (!proof.is_valid) {
        return false;
    }
    
    // 获取所有元素
    std::vector<BigInt> elements;
    for (const auto& pair : element_to_commitment_index) {
        elements.push_back(pair.first);
    }
    
    // 创建期望的承诺
    KZGCommitment expected_commitment = create_non_membership_commitment(element, elements);
    
    // 验证打开证明
    return expected_commitment.verify_opening(proof.opening_proof, element, BigInt("0"));
}

BigInt PolynomialAccumulator::generate_challenge(const std::string& context) {
    // 使用哈希函数生成挑战
    BigInt hash_input = CryptoUtils::sha256(BigInt(context));
    return hash_input % current_modulus;
}

PolynomialAccumulator::SetOperationProof PolynomialAccumulator::prove_union(const std::vector<BigInt>& set1, const std::vector<BigInt>& set2) {
    SetOperationProof proof;
    
    // 计算并集
    std::vector<BigInt> union_elements = set1;
    for (const auto& elem : set2) {
        if (std::find(union_elements.begin(), union_elements.end(), elem) == union_elements.end()) {
            union_elements.push_back(elem);
        }
    }
    
    // 创建并集承诺
    proof.result_commitment = create_membership_commitment(union_elements);
    
    // 生成打开证明
    BigInt challenge_point = generate_challenge("union_proof");
    proof.opening_proof = proof.result_commitment.open(challenge_point);
    
    proof.challenge = generate_challenge("union_challenge");
    proof.response = generate_challenge("union_response");
    proof.is_valid = proof.opening_proof.is_valid;
    
    return proof;
}

PolynomialAccumulator::SetOperationProof PolynomialAccumulator::prove_intersection(const std::vector<BigInt>& set1, const std::vector<BigInt>& set2) {
    SetOperationProof proof;
    
    // 计算交集
    std::vector<BigInt> intersection_elements;
    for (const auto& elem : set1) {
        if (std::find(set2.begin(), set2.end(), elem) != set2.end()) {
            intersection_elements.push_back(elem);
        }
    }
    
    // 创建交集承诺
    proof.result_commitment = create_membership_commitment(intersection_elements);
    
    // 生成打开证明
    BigInt challenge_point = generate_challenge("intersection_proof");
    proof.opening_proof = proof.result_commitment.open(challenge_point);
    
    proof.challenge = generate_challenge("intersection_challenge");
    proof.response = generate_challenge("intersection_response");
    proof.is_valid = proof.opening_proof.is_valid;
    
    return proof;
}

PolynomialAccumulator::SetOperationProof PolynomialAccumulator::prove_difference(const std::vector<BigInt>& set1, const std::vector<BigInt>& set2) {
    SetOperationProof proof;
    
    // 计算差集
    std::vector<BigInt> difference_elements;
    for (const auto& elem : set1) {
        if (std::find(set2.begin(), set2.end(), elem) == set2.end()) {
            difference_elements.push_back(elem);
        }
    }
    
    // 创建差集承诺
    proof.result_commitment = create_membership_commitment(difference_elements);
    
    // 生成打开证明
    BigInt challenge_point = generate_challenge("difference_proof");
    proof.opening_proof = proof.result_commitment.open(challenge_point);
    
    proof.challenge = generate_challenge("difference_challenge");
    proof.response = generate_challenge("difference_response");
    proof.is_valid = proof.opening_proof.is_valid;
    
    return proof;
}

void PolynomialAccumulator::print_state() const {
    std::cout << "\n=== 多项式累加器状态 ===" << std::endl;
    std::cout << "承诺数量: " << commitments.size() << std::endl;
    std::cout << "元素数量: " << element_to_commitment_index.size() << std::endl;
    std::cout << "最大多项式次数: " << kzg_params.max_degree << std::endl;
    
    std::cout << "元素列表: ";
    for (const auto& pair : element_to_commitment_index) {
        std::cout << pair.first.to_string() << " ";
    }
    std::cout << std::endl;
    std::cout << "========================" << std::endl;
}

// ==================== PolynomialUtils 实现 ====================

namespace PolynomialUtils {
    Polynomial add_polynomials(const Polynomial& p1, const Polynomial& p2) {
        return p1 + p2;
    }
    
    Polynomial multiply_polynomials(const Polynomial& p1, const Polynomial& p2) {
        return p1 * p2;
    }
    
    Polynomial scalar_multiply(const Polynomial& poly, const BigInt& scalar) {
        return poly * scalar;
    }
    
    Polynomial create_vanishing_polynomial(const std::vector<BigInt>& roots, const BigInt& modulus) {
        return Polynomial::from_roots(roots, modulus);
    }
    
    BigInt evaluate_polynomial(const Polynomial& poly, const BigInt& point) {
        return poly.evaluate(point);
    }
    
    std::vector<BigInt> evaluate_polynomial_batch(const Polynomial& poly, const std::vector<BigInt>& points) {
        std::vector<BigInt> results;
        for (const auto& point : points) {
            results.push_back(poly.evaluate(point));
        }
        return results;
    }
    
    Polynomial interpolate(const std::vector<BigInt>& x_values, const std::vector<BigInt>& y_values, const BigInt& modulus) {
        return Polynomial::lagrange_interpolation(x_values, y_values, modulus);
    }
    
    Polynomial random_polynomial(size_t degree, const BigInt& modulus) {
        std::vector<BigInt> coeffs;
        for (size_t i = 0; i <= degree; ++i) {
            coeffs.push_back(CryptoUtils::random_range(BigInt("0"), modulus - BigInt("1")));
        }
        return Polynomial(coeffs, modulus);
    }
}
