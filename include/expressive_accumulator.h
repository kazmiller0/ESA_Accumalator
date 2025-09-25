#ifndef EXPRESSIVE_ACCUMULATOR_H
#define EXPRESSIVE_ACCUMULATOR_H

#pragma once

#include <mcl/bls12_381.hpp>
#include <vector>
#include <string>
#include <unordered_set>
#include <set>
#include <memory>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <algorithm>

using namespace mcl::bls12;

namespace expressive_accumulator {

// 全局初始化函数
void initMcl();
void initFlintContext();

class CharacteristicPolynomial; // 前向声明

/**
 * @brief 累加器摘要，代表一个集合的密码学承诺。
 * @details 该摘要是集合特征多项式 P(z) 在秘密点 s 的值在 G1 群上的体现，即 g1^P(s)。
 *          由于离散对数难题，从该值无法反推出集合内容。
 */
struct AccumulatorDigest {
    G1 value; // 累加器值，例如 g^P(s)

    AccumulatorDigest() {
        value.clear();
    }
    
    void initialize(const G1& generator) {
        value = generator; // 初始化为生成元
    }
    
    bool is_identity() const {
        return value.isZero();
    }
    
    // 序列化与反序列化 (可以根据需要实现)
    std::string serialize() const;
    void deserialize(const std::string& data);
    
    bool operator==(const AccumulatorDigest& other) const {
        return value == other.value;
    }

    // 用于在 verifier 中重新计算摘要
    const CharacteristicPolynomial& getPolynomial() const;
};

/**
 * @brief G2 群上的累加器摘要。
 */
struct AccumulatorDigestG2 {
    G2 value;

    AccumulatorDigestG2() {
        value.clear();
    }

    void initialize(const G2& generator) {
        value = generator; // 初始化为生成元
    }

    bool is_identity() const {
        return value.isZero();
    }
};

/**
 * @brief 集合交集证明。
 * @details 包含 I ⊆ A 和 I ⊆ B 的子集关系证明，
 *          以及 (A\\I) 和 (B\\I) 的不相交性证明，从而完整地证明 I = A ∩ B。
 */
struct IntersectionProof {
    AccumulatorDigest intersection_digest_g1; ///< 交集 I(s) 在 G1 上的承诺 g1^I(s)
    G2 witness_QA_g2; ///< 商多项式 Q_A(s) = A(s)/I(s) 在 G2 上的证据 g2^Q_A(s)
    G2 witness_QB_g2; ///< 商多项式 Q_B(s) = B(s)/I(s) 在 G2 上的证据 g2^Q_B(s)

    // 用于不相交证明的贝祖等式见证
    G1 witness_a_g1;  ///< 贝祖系数 a(s) 在 G1 上的承诺 g1^a(s)
    G1 witness_b_g1;  ///< 贝祖系数 b(s) 在 G1 上的承诺 g1^b(s)

    bool is_valid;
    
    IntersectionProof() : is_valid(false) {}
};

/**
 * @brief 成员关系证明，使用更高效的商多项式方法。
 */
struct MembershipProof {
    G2 witness_g2;                          ///< 见证 W = g2^((P(s)-P_x(s))/(s-x))
    bool is_member;                         ///< 声明元素是否是成员
    
    MembershipProof() : is_member(false) {}
};

/**
 * @brief 定义累加器的动态操作类型。
 */
enum class UpdateOperation { ADD, DELETE };

/**
 * @brief 动态操作的证明。
 * @details 用于记录集合的增删操作，并提供相应的密码学证据。
 */
struct UpdateProof {
    UpdateOperation op_type;                // 操作类型 (增/删)
    AccumulatorDigest old_digest;           // 操作前的摘要
    AccumulatorDigest new_digest;           // 操作后的摘要
    int element;                            // 被操作的元素
    MembershipProof membership_proof;       // 仅在删除时使用，证明删除的“权利”
    bool is_valid;

    UpdateProof() : element(0), is_valid(false) {}
};

// 计数证明
struct CountProof {
    AccumulatorDigest count_digest;
    Fr count_value;
    std::vector<G1> commitments;
    bool is_valid;
    
    CountProof() : is_valid(false) {}
};

/**
 * @brief 密码学累加器的可信设置。
 * @details 负责生成和存储系统级的秘密参数（s, r）和由它们衍生的公开参数（g^{s^i}）。
 *          该设置的安全性是整个系统安全性的基石。
 */
class ExpressiveTrustedSetup {
private:
    Fr secret_s;
    Fr secret_r;
    size_t max_degree;
    G1 g1_generator;
    G2 g2_generator;

public:
    std::vector<G1> g1_s_powers;
    std::vector<G2> g2_s_powers;

    /**
     * @brief 构造函数。
     * @param s 秘密参数 s。
     * @param r 秘密参数 r。
     * @param max_deg 多项式的最大次数。
     */
    ExpressiveTrustedSetup(const Fr& s, const Fr& r, size_t max_deg = 1000);
    void generatePowers();

    // 获取器
    Fr getSecretS() const { return secret_s; }
    Fr getSecretR() const { return secret_r; }
    int getQ() const { return static_cast<int>(max_degree); }
    G1 getG1Generator() const { return g1_generator; }
    G2 getG2Generator() const { return g2_generator; }
    G2 getG2_s_pow(int i) const { return g2_s_powers.at(i); }
};

// 群类型枚举
enum GroupType { G1_TYPE, G2_TYPE };

/**
 * @brief 表现力累加器核心类。
 * @details 实现了一个支持动态增、删、改、查以及集合交集零知识证明的密码学累加器。
 */
class ExpressiveAccumulator {
private:
    /**
     * @brief [私有] 根据当前的元素集合更新累加器的摘要值。
     * @details 在任何元素变更（添加/删除）后调用此函数。
     *          它会重新评估特征多项式 P(s) 并将摘要更新为 g^P(s)。
     */
    void updateAccumulatorValue();
    const ExpressiveTrustedSetup& trusted_setup;
    std::set<int> elements;
    std::unique_ptr<CharacteristicPolynomial> polynomial; // 使用智能指针
    GroupType group_type;

public:
    AccumulatorDigest digest_g1;
    AccumulatorDigestG2 digest_g2;

    /**
     * @brief 构造函数。
     * @param setup 可信设置对象的引用。
     * @param type 累加器所在的群类型 (G1_TYPE 或 G2_TYPE)。
     */
    ExpressiveAccumulator(const ExpressiveTrustedSetup& setup, GroupType type);
    
    // 返回更新证明
    /**
     * @brief 动态操作 - 添加一个元素到集合。
     * @return 一个 UpdateProof 对象，包含操作的证明。
     */
    UpdateProof addElement(int element);
    /**
     * @brief 从集合中删除一个元素。
     * @param element 要删除的元素。
     * @return 一个 UpdateProof 对象，包含操作的证明。
     */
    UpdateProof deleteElement(int element);
    
    const std::set<int>& getElements() const { return elements; }
    const CharacteristicPolynomial& getPolynomial() const { return *polynomial; } // 解引用指针

    // 成员关系证明 (专用版本)
    static bool verifyMembershipProof(const AccumulatorDigest& acc_digest, 
                                      int element, 
                                      const MembershipProof& proof, 
                                      const ExpressiveTrustedSetup& setup);
    MembershipProof generateMembershipProof(int element) const;
    
    // 集合运算
    /**
     * @brief 生成两个累加器的交集证明。
     * @param acc1 第一个累加器。
     * @param acc2 第二个累加器。
     * @param setup 可信设置对象。
     * @return 一个 IntersectionProof 对象。
     */
    static IntersectionProof generateIntersectionProof(
        const ExpressiveAccumulator& acc1,
        const ExpressiveAccumulator& acc2,
        const ExpressiveTrustedSetup& setup);
    
    /**
     * @brief [静态] 验证集合交集证明 (精确模型)。
     * @details 无需预知结果，可独立验证 I = A ∩ B。
     */
    static bool verifyIntersectionProof(
        const AccumulatorDigest& digest_A, 
        const AccumulatorDigest& digest_B,
        const IntersectionProof& proof,
        const ExpressiveTrustedSetup& setup);
    
    /**
     * @brief 获取器
     */
    const AccumulatorDigest& getDigest() const { return digest_g1; }
    const AccumulatorDigestG2& getDigestG2() const { return digest_g2; }
    const ExpressiveTrustedSetup& getTrustedSetup() const { return trusted_setup; }
    GroupType getGroupType() const { return group_type; }
    
    /**
     * @brief 调试和测试
     */
    void printDigest() const;

    /**
     * @brief [静态] 验证动态操作（增/删）的证明。
     * @details (description)
     */
    static bool verifyUpdateProof(const UpdateProof& proof, const ExpressiveTrustedSetup& setup);

    /**
     * @brief 生成一个元素的成员关系证明。
     * @param element 要查询的元素。
     * @return 一个 MembershipProof 对象。
     */
    // MembershipProof generateMembershipProof(int element) const; // 已移到上方专用区域
};

// 特征多项式类
class CharacteristicPolynomial {
private:
    std::set<int> elements;
        
public:
    CharacteristicPolynomial(const std::set<int>& elems) : elements(elems) {}
    
    void addElement(int element);
    void removeElement(int element);
    Fr evaluate(const Fr& a) const;

    const std::set<int>& getElements() const {
        return elements;
    }

    static std::set<int> intersection(const std::set<int>& set1, const std::set<int>& set2);
};

// 二维项结构
struct Term2D {
    int s_exp;
    int r_exp;
    
    Term2D(int s, int r) : s_exp(s), r_exp(r) {}
};

} // namespace expressive_accumulator

#endif // EXPRESSIVE_ACCUMULATOR_H
