#ifndef POLYNOMIAL_COMMITMENT_H
#define POLYNOMIAL_COMMITMENT_H

#include "basic_types.h"
#include <vector>
#include <map>
#include <memory>

// 多项式类
class Polynomial {
private:
    std::vector<BigInt> coefficients;  // 系数，从低次项到高次项
    BigInt modulus;                    // 模数
    
public:
    Polynomial(const BigInt& mod);
    Polynomial(const std::vector<BigInt>& coeffs, const BigInt& mod);
    Polynomial(const Polynomial& other);
    Polynomial(Polynomial&& other) noexcept;
    
    Polynomial& operator=(const Polynomial& other);
    Polynomial& operator=(Polynomial&& other) noexcept;
    
    // 基本操作
    BigInt evaluate(const BigInt& x) const;
    Polynomial operator+(const Polynomial& other) const;
    Polynomial operator-(const Polynomial& other) const;
    Polynomial operator*(const Polynomial& other) const;
    Polynomial operator*(const BigInt& scalar) const;
    
    // 多项式运算
    Polynomial derivative() const;
    Polynomial integrate() const;
    std::pair<Polynomial, Polynomial> divide(const Polynomial& divisor) const;
    
    // 工具函数
    size_t degree() const;
    const std::vector<BigInt>& get_coefficients() const { return coefficients; }
    const BigInt& get_modulus() const { return modulus; }
    BigInt get_coefficient(size_t index) const;
    void set_coefficient(size_t index, const BigInt& value);
    
    // 序列化
    std::string serialize() const;
    static Polynomial deserialize(const std::string& data, const BigInt& modulus);
    
    // 从根构造多项式
    static Polynomial from_roots(const std::vector<BigInt>& roots, const BigInt& modulus);
    
    // 拉格朗日插值
    static Polynomial lagrange_interpolation(const std::vector<BigInt>& x_values, 
                                           const std::vector<BigInt>& y_values, 
                                           const BigInt& modulus);
};

// KZG多项式承诺参数
struct KZGParams {
    GroupElement g;           // 生成元
    std::vector<GroupElement> g_powers;  // g^1, g^2, ..., g^d
    BigInt modulus;           // 群模数
    size_t max_degree;        // 最大多项式次数
    
    KZGParams(const GroupElement& generator, size_t max_deg, const BigInt& mod);
    bool is_valid() const;
};

// KZG多项式承诺
class KZGCommitment {
private:
    GroupElement commitment;  // 承诺值
    Polynomial polynomial;    // 被承诺的多项式
    KZGParams params;        // KZG参数
    
public:
    KZGCommitment(const Polynomial& poly, const KZGParams& kzg_params);
    
    // 获取器
    const GroupElement& get_commitment() const { return commitment; }
    const Polynomial& get_polynomial() const { return polynomial; }
    const KZGParams& get_params() const { return params; }
    
    // 打开承诺（在特定点）
    struct OpeningProof {
        GroupElement quotient_commitment;  // 商多项式的承诺
        BigInt quotient_evaluation;        // 商多项式在挑战点的值
        bool is_valid;
        
        OpeningProof() : is_valid(false) {}
    };
    
    OpeningProof open(const BigInt& point) const;
    bool verify_opening(const OpeningProof& proof, const BigInt& point, const BigInt& value) const;
    
    // 批量打开
    struct BatchOpeningProof {
        std::vector<GroupElement> quotient_commitments;
        std::vector<BigInt> quotient_evaluations;
        BigInt random_challenge;
        bool is_valid;
        
        BatchOpeningProof() : is_valid(false) {}
    };
    
    BatchOpeningProof batch_open(const std::vector<BigInt>& points) const;
    bool verify_batch_opening(const BatchOpeningProof& proof, 
                             const std::vector<BigInt>& points, 
                             const std::vector<BigInt>& values) const;
    
    // 承诺更新
    void update_polynomial(const Polynomial& new_poly);
    
    // 序列化
    std::string serialize() const;
    static KZGCommitment deserialize(const std::string& data, const KZGParams& params);
};

// 多项式累加器（基于多项式承诺）
class PolynomialAccumulator {
private:
    KZGParams kzg_params;
    std::vector<KZGCommitment> commitments;
    std::map<BigInt, size_t> element_to_commitment_index;
    BigInt current_modulus;
    
    // 内部方法
    Polynomial create_membership_polynomial(const std::vector<BigInt>& elements);
    Polynomial create_non_membership_polynomial(const BigInt& element, const std::vector<BigInt>& elements);
    BigInt generate_challenge(const std::string& context);
    
public:
    PolynomialAccumulator(const GroupElement& generator, size_t max_degree, const BigInt& modulus);
    
    // 基本操作
    bool add_element(const BigInt& element);
    bool remove_element(const BigInt& element);
    bool contains(const BigInt& element) const;
    
    // 多项式承诺生成
    KZGCommitment create_membership_commitment(const std::vector<BigInt>& elements);
    KZGCommitment create_non_membership_commitment(const BigInt& element, const std::vector<BigInt>& elements);
    
    // 零知识证明
    struct ZKProof {
        KZGCommitment::OpeningProof opening_proof;
        BigInt challenge;
        BigInt response;
        bool is_valid;
        
        ZKProof() : is_valid(false) {}
    };
    
    ZKProof generate_membership_proof(const BigInt& element);
    ZKProof generate_non_membership_proof(const BigInt& element);
    
    // 证明验证
    bool verify_membership_proof(const ZKProof& proof, const BigInt& element);
    bool verify_non_membership_proof(const ZKProof& proof, const BigInt& element);
    
    // 集合操作的多项式证明
    struct SetOperationProof {
        KZGCommitment result_commitment;
        KZGCommitment::OpeningProof opening_proof;
        BigInt challenge;
        BigInt response;
        bool is_valid;
        
        SetOperationProof() : result_commitment(Polynomial(std::vector<BigInt>{BigInt("0")}, BigInt("2")), 
                                               KZGParams(GroupElement(BigInt("1"), BigInt("2")), 1, BigInt("2"))), 
                             is_valid(false) {}
    };
    
    SetOperationProof prove_union(const std::vector<BigInt>& set1, const std::vector<BigInt>& set2);
    SetOperationProof prove_intersection(const std::vector<BigInt>& set1, const std::vector<BigInt>& set2);
    SetOperationProof prove_difference(const std::vector<BigInt>& set1, const std::vector<BigInt>& set2);
    
    // 获取器
    size_t size() const { return commitments.size(); }
    const std::vector<KZGCommitment>& get_commitments() const { return commitments; }
    const KZGParams& get_kzg_params() const { return kzg_params; }
    
    // 调试
    void print_state() const;
};

// 多项式工具函数
namespace PolynomialUtils {
    // 多项式运算
    Polynomial add_polynomials(const Polynomial& p1, const Polynomial& p2);
    Polynomial multiply_polynomials(const Polynomial& p1, const Polynomial& p2);
    Polynomial scalar_multiply(const Polynomial& poly, const BigInt& scalar);
    
    // 特殊多项式构造
    Polynomial create_vanishing_polynomial(const std::vector<BigInt>& roots, const BigInt& modulus);
    Polynomial create_lagrange_basis(const std::vector<BigInt>& points, size_t index, const BigInt& modulus);
    
    // 多项式求值
    BigInt evaluate_polynomial(const Polynomial& poly, const BigInt& point);
    std::vector<BigInt> evaluate_polynomial_batch(const Polynomial& poly, const std::vector<BigInt>& points);
    
    // 多项式插值
    Polynomial interpolate(const std::vector<BigInt>& x_values, const std::vector<BigInt>& y_values, const BigInt& modulus);
    
    // 多项式分解
    std::vector<Polynomial> factor_polynomial(const Polynomial& poly);
    
    // 随机多项式生成
    Polynomial random_polynomial(size_t degree, const BigInt& modulus);
    Polynomial random_polynomial_with_roots(const std::vector<BigInt>& roots, const BigInt& modulus);
}

#endif // POLYNOMIAL_COMMITMENT_H
