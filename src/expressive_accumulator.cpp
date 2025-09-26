/**
 * @file expressive_accumulator.cpp
 * @brief 表现力累加器及相关类的实现。
 * @details 本文件包含密码学累加器的核心实现，包括元素操作、
 *          证明生成和证明验证的逻辑。
 */
#include "expressive_accumulator.h"
#include <iostream>
#include <vector>
#include <numeric>
#include <algorithm>
#include <cassert>

// 引入 FLINT C 语言头文件
extern "C" {
#include <flint/fmpz.h>
#include <flint/fmpz_mod.h>
#include <flint/fmpz_mod_poly.h>
}

// 使用 mcl 的命名空间
using namespace mcl::bls12;

namespace expressive_accumulator {

// 全局初始化函数
void initMcl() {
    mcl::bn::initPairing(mcl::BLS12_381);
}

// 定义 FLINT 的全局有限域上下文
extern "C" {
    fmpz_mod_ctx_t flint_ctx;
}

// 初始化 FLINT 上下文的函数
void initFlintContext() {
    // 使用 BLS12-381 的标量域模数 (Fr 的模数)
    try {
        auto p_str = mcl::bls12::Fr::getModulo();
        std::cout << "MCL modulo string: " << p_str << std::endl;
        
        fmpz_t p;
        fmpz_init(p);
        int result = fmpz_set_str(p, p_str.c_str(), 10);
        
        if (result != 0) {
            throw std::runtime_error("Failed to parse MCL modulo string");
        }
        
        fmpz_mod_ctx_init(flint_ctx, p);
        fmpz_clear(p);
        std::cout << "FLINT context initialized successfully" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error initializing FLINT context: " << e.what() << std::endl;
        // 使用硬编码的模数作为后备
        const char* p_str_hardcoded = "52435875175126190479447740508185965837690552500527637822603658699938581184513";
        std::cout << "Using hardcoded modulo: " << p_str_hardcoded << std::endl;
        
        fmpz_t p;
        fmpz_init(p);
        int result = fmpz_set_str(p, p_str_hardcoded, 10);
        
        if (result != 0) {
            throw std::runtime_error("Failed to parse hardcoded modulo string");
        }
        
        fmpz_mod_ctx_init(flint_ctx, p);
        fmpz_clear(p);
        std::cout << "FLINT context initialized with hardcoded modulo" << std::endl;
    }
}

// PolynomialUtils 命名空间，用于封装 FLINT 多项式操作
namespace PolynomialUtils {
    // 辅助函数：将 mcl::Fr 转换为 FLINT 的 fmpz_t
    void frToFmpz(fmpz_t result, const mcl::Fr& fr) {
        fmpz_set_str(result, fr.getStr(10).c_str(), 10);
    }

    // 辅助函数：将 FLINT 的 fmpz_t 转换为 mcl::Fr
    mcl::Fr fmpzToFr(const fmpz_t f) {
        char* s = fmpz_get_str(nullptr, 10, f);
        mcl::Fr fr;
        try {
            fr.setStr(s);
        } catch (const std::exception& e) {
            std::cerr << "错误: 无法将FLINT整数转换为MCL Fr: " << s << std::endl;
            std::cerr << "异常: " << e.what() << std::endl;
            free(s);
            throw;
        }
        free(s);
        return fr;
    }

    // 从根集合创建 FLINT 多项式 P(z) = (z - r1)(z - r2)...
    void fromRoots(fmpz_mod_poly_t poly, const std::set<int>& roots, const fmpz_mod_ctx_t ctx) {
        fmpz_mod_poly_init(poly, ctx);
        fmpz_mod_poly_set_coeff_si(poly, 0, 1, ctx); // P(z) = 1

        fmpz_mod_poly_t temp_term;
        fmpz_mod_poly_init(temp_term, ctx);

        fmpz_t r_fmpz;
        fmpz_init(r_fmpz);

        for (int root : roots) {
            // temp_term = z - root
            fmpz_set_si(r_fmpz, root);
            fmpz_mod_poly_set_coeff_si(temp_term, 1, 1, ctx); // temp_term = z
            fmpz_mod_poly_set_coeff_si(temp_term, 0, 0, ctx); // 清零常数项
            fmpz_mod_poly_sub_fmpz(temp_term, temp_term, r_fmpz, ctx);

            fmpz_mod_poly_mul(poly, poly, temp_term, ctx);
        }
        
        fmpz_clear(r_fmpz);
        fmpz_mod_poly_clear(temp_term, ctx);
    }
    
    // 在点 s 处评估 FLINT 多项式
    mcl::Fr evaluate(const fmpz_mod_poly_t poly, const mcl::Fr& s, const fmpz_mod_ctx_t ctx) {
        fmpz_t s_fmpz, res_fmpz;
        fmpz_init(s_fmpz);
        fmpz_init(res_fmpz);
        
        frToFmpz(s_fmpz, s);
        fmpz_mod_poly_evaluate_fmpz(res_fmpz, poly, s_fmpz, ctx);
        
        mcl::Fr result = fmpzToFr(res_fmpz);
        
        fmpz_clear(s_fmpz);
        fmpz_clear(res_fmpz);
        
        return result;
    }
}


// ==========================================================================================
// ExpressiveTrustedSetup - 方法实现
// ==========================================================================================

ExpressiveTrustedSetup::ExpressiveTrustedSetup(const Fr& s, const Fr& r, size_t max_deg)
    : secret_s(s), secret_r(r), max_degree(max_deg) {
    // 构造函数体为空，所有计算都在 generatePowers 中进行
}

/**
 * @brief 生成并预计算公开参数。
 * @details 这是一个一次性的设置操作，计算 g1^{s^i} 和 g2^{s^i}
 *          直到所需的最大次数。这些预计算的值是所有后续累加器操作和证明的基础。
 */
void ExpressiveTrustedSetup::generatePowers() {
    hashAndMapToG1(g1_generator, "expressive_generator_g1", 12);
    hashAndMapToG2(g2_generator, "expressive_generator_g2", 12);
    
    g1_s_powers.resize(max_degree + 2);
    g2_s_powers.resize(max_degree + 2);
    
    for (size_t i = 0; i <= max_degree + 1; ++i) {
        Fr s_power;
        Fr::pow(s_power, secret_s, i);
        G1::mul(g1_s_powers[i], g1_generator, s_power);
        G2::mul(g2_s_powers[i], g2_generator, s_power);
    }
}


// ==========================================================================================
// CharacteristicPolynomial - 方法实现
// ==========================================================================================

void CharacteristicPolynomial::addElement(int element) {
    elements.insert(element);
}

void CharacteristicPolynomial::removeElement(int element) {
    elements.erase(element);
}

/**
 * @brief 在给定点 a 处评估特征多项式。
 * @details 计算 P(a) = product_{x_i in elements} (a - x_i)。
 * @param a 评估点 (通常是秘密值 s)。
 * @return Fr 评估结果 P(a)。
 */
Fr CharacteristicPolynomial::evaluate(const Fr& a) const {
    if (elements.empty()) {
        return Fr(1); // 空集的多项式是 P(z) = 1
    }
    
    Fr res(1);
    for (int r : elements) {
        Fr r_fr = r;
        res *= (a - r_fr);
    }
    return res;
}

std::set<int> CharacteristicPolynomial::intersection(const std::set<int>& set1, const std::set<int>& set2) {
    std::set<int> result;
    std::set_intersection(set1.begin(), set1.end(), 
                        set2.begin(), set2.end(),
                        std::inserter(result, result.begin()));
    return result;
}

// ==========================================================================================
// ExpressiveAccumulator - 方法实现
// ==========================================================================================

ExpressiveAccumulator::ExpressiveAccumulator(const ExpressiveTrustedSetup& setup, GroupType type)
    : trusted_setup(setup), group_type(type) {
    polynomial = std::make_unique<CharacteristicPolynomial>(std::set<int>());
    if (type == G1_TYPE) {
        digest_g1.initialize(setup.getG1Generator());
    } else {
        digest_g2.initialize(setup.getG2Generator());
    }
}

/**
 * @brief 根据当前的元素集合更新累加器的摘要值。
 * @details 在任何元素变更（添加/删除）后调用此函数。
 *          它会重新评估特征多项式 P(s) 并将摘要更新为 g^P(s)。
 */
void ExpressiveAccumulator::updateAccumulatorValue() {
    Fr poly_eval = polynomial->evaluate(trusted_setup.getSecretS());
    
    if (group_type == G1_TYPE) {
        G1::mul(digest_g1.value, trusted_setup.getG1Generator(), poly_eval);
    } else { // G2_TYPE
        G2::mul(digest_g2.value, trusted_setup.getG2Generator(), poly_eval);
    }
}

/**
 * @brief 添加元素并返回操作证明。
 * @details 更新多项式的根集合，并重新计算累加器摘要。
 */
UpdateProof ExpressiveAccumulator::addElement(int element) {
    UpdateProof proof;
    proof.op_type = UpdateOperation::ADD;
    proof.element = element;
    proof.old_digest = this->getDigest();

    if (elements.find(element) == elements.end()) {
        elements.insert(element);
        polynomial->addElement(element);
        updateAccumulatorValue();
    }
    
    proof.new_digest = this->getDigest();
    proof.is_valid = true;
    return proof;
}

/**
 * @brief 删除元素并返回操作证明。
 * @details 在删除前，会先生成一个成员关系证明来证明删除的“权利”。
 */
UpdateProof ExpressiveAccumulator::deleteElement(int element) {
    UpdateProof proof;
    proof.op_type = UpdateOperation::DELETE;
    proof.old_digest = this->getDigest();
    proof.element = element;

    if (elements.find(element) == elements.end()) {
        proof.is_valid = false;
        proof.new_digest = this->getDigest(); 
        return proof;
    }
    
    proof.membership_proof = this->generateMembershipProof(element);
    if (!proof.membership_proof.is_member) {
        proof.is_valid = false;
        proof.new_digest = this->getDigest(); 
        return proof;
    }

    polynomial->removeElement(element);
    elements.erase(element);
    updateAccumulatorValue();
    proof.new_digest = this->getDigest();
    proof.is_valid = true;
    
    return proof;
}

MembershipProof ExpressiveAccumulator::generateMembershipProof(int element) const {
    MembershipProof proof;
    if (elements.find(element) == elements.end()) {
        proof.is_member = false;
        return proof;
    }
    proof.is_member = true;

    // 计算商多项式 Q(z) = (P(z) - P_x(z)) / (z - x)
    // P_x(z) = z-x. P_x(s) = s-x
    // Q(z) 实际上就是除了 (z-x) 之外的所有其他 (z-r) 项的乘积
    std::set<int> witness_elements = elements;
    witness_elements.erase(element);
    CharacteristicPolynomial witness_poly(witness_elements);
    
    Fr witness_s = witness_poly.evaluate(trusted_setup.getSecretS());
    G2::mul(proof.witness_g2, trusted_setup.getG2Generator(), witness_s);
    
    return proof;
}

/**
 * @brief 验证一个给定元素的成员关系证明。
 * @details 成员关系证明是交集证明的一个特例，用于证明主集合 A
 *          与单例集合 {x} 的交集。验证过程包括两个步骤：
 *          1. 一致性检查：确保证明中包含的交集摘要与期望的摘要（{x} 或空集的摘要）匹配。
 *          2. 密码学检查：使用配对验证底层的交集证明。
 * @param acc_digest 需要被验证的累加器的摘要。
 * @param proof 证明者提供的成员关系证明。
 * @param element 被检查的元素。
 * @param setup 可信设置对象的引用。
 * @return 如果证明有效则返回 true，否则返回 false。
 */
bool ExpressiveAccumulator::verifyMembershipProof(
    const AccumulatorDigest& acc_digest, 
    int element, 
    const MembershipProof& proof, 
    const ExpressiveTrustedSetup& setup) {

    if (!proof.is_member) {
        return false;
    }

    // 验证 e(A, g2) == e(g1^{s-x}, W) * e(x, g2)
    // 原始等式: e(g1^P(s), g2) == e(g1^(s-x), g2^Q(s))
    // 变换后: e(A, g2) == e(g1^s * g1^{-x}, W)
    // 再变换: e(A, g2) == e(g1^s, W) * e(g1^{-x}, W)
    // 更简单的验证: e(A / g1^{P_x(s)}, g2) == e(g1, W)
    // A / g1^{P_x(s)} = g1^{P(s)} / g1^{s-x} = g1^{P(s)-(s-x)}
    // W = g2^Q(s)
    // e(g1^{P(s)-(s-x)}, g2) == e(g1, g2^Q(s))
    // P(s)-(s-x) == Q(s)  -> (P(s)-(s-x))/(s-x) != Q(s)
    
    // 正确的验证: e(A, g2) / e(g1, P_x(g2)) == e(g1^{s-x}, W)
    // e(digest, g2) == e(g1^(s-x), witness)
    
    GT lhs, rhs;
    
    // lhs = e(A, g2)
    pairing(lhs, acc_digest.value, setup.getG2Generator());

    // rhs = e(g1^{s-x}, W)
    Fr s = setup.getSecretS();
    Fr x;
    x = element;
    G1 sx_g1;
    G1::mul(sx_g1, setup.getG1Generator(), s - x);
    pairing(rhs, sx_g1, proof.witness_g2);

    return lhs == rhs;
}


IntersectionProof ExpressiveAccumulator::generateIntersectionProof(
    const ExpressiveAccumulator& acc1,
    const ExpressiveAccumulator& acc2,
    const ExpressiveTrustedSetup& setup)
{
    IntersectionProof proof;
    const Fr& secret_s = setup.getSecretS();

    // 1. 计算交集和差集
    std::set<int> intersection_set = CharacteristicPolynomial::intersection(acc1.getElements(), acc2.getElements());
    std::set<int> diff_A_set, diff_B_set;
    std::set_difference(acc1.getElements().begin(), acc1.getElements().end(), intersection_set.begin(), intersection_set.end(), std::inserter(diff_A_set, diff_A_set.begin()));
    std::set_difference(acc2.getElements().begin(), acc2.getElements().end(), intersection_set.begin(), intersection_set.end(), std::inserter(diff_B_set, diff_B_set.begin()));

    // 2. 使用 FLINT 构建多项式
    fmpz_mod_poly_t poly_I, poly_QA, poly_QB;
    PolynomialUtils::fromRoots(poly_I, intersection_set, flint_ctx);
    PolynomialUtils::fromRoots(poly_QA, diff_A_set, flint_ctx);
    PolynomialUtils::fromRoots(poly_QB, diff_B_set, flint_ctx);

    // 3. 计算 I, Q_A, Q_B 多项式在 s 处的值
    Fr I_s = PolynomialUtils::evaluate(poly_I, secret_s, flint_ctx);
    Fr QA_s = PolynomialUtils::evaluate(poly_QA, secret_s, flint_ctx);
    Fr QB_s = PolynomialUtils::evaluate(poly_QB, secret_s, flint_ctx);
    
    // 4. 创建子集证明的承诺
    G1::mul(proof.intersection_digest_g1.value, setup.getG1Generator(), I_s);
    G2::mul(proof.witness_QA_g2, setup.getG2Generator(), QA_s);
    G2::mul(proof.witness_QB_g2, setup.getG2Generator(), QB_s);

    // 5. 使用 FLINT 计算不相交证明
    fmpz_mod_poly_t gcd, a, b;
    fmpz_mod_poly_init(gcd, flint_ctx);
    fmpz_mod_poly_init(a, flint_ctx);
    fmpz_mod_poly_init(b, flint_ctx);

    fmpz_mod_poly_xgcd(gcd, a, b, poly_QA, poly_QB, flint_ctx);

    if (!fmpz_mod_poly_is_one(gcd, flint_ctx)) {
        proof.is_valid = false;
    } else {
        Fr a_s = PolynomialUtils::evaluate(a, secret_s, flint_ctx);
        Fr b_s = PolynomialUtils::evaluate(b, secret_s, flint_ctx);

        // 6. 创建不相交证明的承诺
        G1::mul(proof.witness_a_g1, setup.getG1Generator(), a_s);
        G1::mul(proof.witness_b_g1, setup.getG1Generator(), b_s);
        proof.is_valid = true;
    }

    // 7. 清理所有 FLINT 对象
    fmpz_mod_poly_clear(poly_I, flint_ctx);
    fmpz_mod_poly_clear(poly_QA, flint_ctx);
    fmpz_mod_poly_clear(poly_QB, flint_ctx);
    fmpz_mod_poly_clear(gcd, flint_ctx);
    fmpz_mod_poly_clear(a, flint_ctx);
    fmpz_mod_poly_clear(b, flint_ctx);

    return proof;
}

bool ExpressiveAccumulator::verifyIntersectionProof(
    const AccumulatorDigest& digest_A,
    const AccumulatorDigest& digest_B,
    const IntersectionProof& proof,
    const ExpressiveTrustedSetup& setup)
{
    if (!proof.is_valid) return false;

    GT e1, e2, e3, e4, e5;
    G1 g1_gen = setup.getG1Generator();
    G2 g2_gen = setup.getG2Generator();

    // 1. 验证 I ⊆ A: e(A, g2) == e(I, W_QA)
    pairing(e1, digest_A.value, g2_gen);
    pairing(e2, proof.intersection_digest_g1.value, proof.witness_QA_g2);
    if (e1 != e2) return false;

    // 2. 验证 I ⊆ B: e(B, g2) == e(I, W_QB)
    pairing(e1, digest_B.value, g2_gen);
    pairing(e2, proof.intersection_digest_g1.value, proof.witness_QB_g2);
    if (e1 != e2) return false;

    // 3. 验证 (A\\I) 和 (B\\I) 不相交: e(W_a, W_QA) * e(W_b, W_QB) == e(g1, g2)
    pairing(e3, proof.witness_a_g1, proof.witness_QA_g2);
    pairing(e4, proof.witness_b_g1, proof.witness_QB_g2);
    pairing(e5, g1_gen, g2_gen);
    if (e3 * e4 != e5) return false;

    return true;
}

/**
 * @brief 验证动态操作（增/删）的证明。
 * @details 对于添加操作，验证 P_new(s) = P_old(s) * (s-x)。
 *          对于删除操作，首先验证 x 确实在 P_old(s) 中（权利证明），
 *          然后验证 P_old(s) = P_new(s) * (s-x)。
 * @param proof 包含操作细节和密码学证据的更新证明。
 * @param setup 可信设置对象的引用。
 * @return 如果证明有效则返回 true，否则返回 false。
 */
bool ExpressiveAccumulator::verifyUpdateProof(const UpdateProof& proof, const ExpressiveTrustedSetup& setup) {
    if (!proof.is_valid) return false;

    Fr element_fr(proof.element);
    G2 g2_gen = setup.getG2Generator();
    G2 g2_s = setup.g2_s_powers[1]; // g2^s

    if (proof.op_type == UpdateOperation::ADD) {
        // 验证 e(new, g2) == e(old, g2^s) * e(old, g2)^(-x)
        Fp12 lhs, rhs1, rhs2_base, rhs2, rhs;
        pairing(lhs, proof.new_digest.value, g2_gen);
        pairing(rhs1, proof.old_digest.value, g2_s);
        
        pairing(rhs2_base, proof.old_digest.value, g2_gen);
        Fp12::pow(rhs2, rhs2_base, -element_fr); // 指数上的减法

        Fp12::mul(rhs, rhs1, rhs2);
        return lhs == rhs;
    }

    if (proof.op_type == UpdateOperation::DELETE) {
        // 对于删除操作，首先验证调用者确实有权删除该元素（即该元素确实在旧集合中）
        bool has_right_to_delete = verifyMembershipProof(proof.old_digest, proof.element, proof.membership_proof, setup);
        if (!has_right_to_delete) {
            std::cerr << "Verification failed: proof of membership for deleted element is invalid." << std::endl;
        return false;
    }
        
        // 2. 验证代数关系 e(old, g2) == e(new, g2^s) * e(new, g2)^(-x)
        Fp12 lhs, rhs1, rhs2_base, rhs2, rhs;
        pairing(lhs, proof.old_digest.value, g2_gen);
        pairing(rhs1, proof.new_digest.value, g2_s);

        pairing(rhs2_base, proof.new_digest.value, g2_gen);
        Fp12::pow(rhs2, rhs2_base, -element_fr);

        Fp12::mul(rhs, rhs1, rhs2);
        return lhs == rhs;
    }

        return false;
}

// --- 调试和测试 ---
void ExpressiveAccumulator::printDigest() const {
    if (group_type == G1_TYPE) {
        std::cout << "  Digest G1: " << digest_g1.value << std::endl;
    } else {
        std::cout << "  Digest G2: " << digest_g2.value << std::endl;
    }
}

} // namespace expressive_accumulator
