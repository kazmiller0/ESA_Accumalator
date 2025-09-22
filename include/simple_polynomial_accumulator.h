#ifndef SIMPLE_POLYNOMIAL_ACCUMULATOR_H
#define SIMPLE_POLYNOMIAL_ACCUMULATOR_H

#include "polynomial_commitment.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <memory>

// 简化的多项式累加器 - 只包含核心功能
class SimplePolynomialAccumulator {
private:
    // 多项式承诺参数
    KZGParams kzg_params;
    
    // 当前集合
    std::unordered_set<BigInt, BigInt::Hash> current_set;
    
    // 成员关系承诺
    std::unique_ptr<KZGCommitment> membership_commitment;
    
    // 辅助数据结构
    BigInt current_modulus;
    
    // 内部方法
    void update_membership_commitment();
    Polynomial create_membership_polynomial() const;
    std::vector<BigInt> get_current_elements() const;
    
public:
    // 构造函数
    SimplePolynomialAccumulator(size_t max_degree = 1000);
    ~SimplePolynomialAccumulator() = default;
    
    // 基本操作：增删改查
    bool add_element(const BigInt& element);
    bool remove_element(const BigInt& element);
    bool update_element(const BigInt& old_element, const BigInt& new_element);
    bool contains(const BigInt& element) const;
    
    // 集合操作：交并补差
    struct SetOperationResult {
        std::unordered_set<BigInt, BigInt::Hash> result_set;
        KZGCommitment result_commitment;
        bool is_valid;
        
        SetOperationResult() : result_commitment(Polynomial(std::vector<BigInt>{BigInt("0")}, BigInt("2")), 
                                                KZGParams(GroupElement(BigInt("1"), BigInt("2")), 1, BigInt("2"))), 
                              is_valid(false) {}
    };
    
    SetOperationResult compute_union(const std::unordered_set<BigInt, BigInt::Hash>& other_set);
    SetOperationResult compute_intersection(const std::unordered_set<BigInt, BigInt::Hash>& other_set);
    SetOperationResult compute_difference(const std::unordered_set<BigInt, BigInt::Hash>& other_set);
    SetOperationResult compute_complement(const std::unordered_set<BigInt, BigInt::Hash>& other_set);
    
    // 见证系统
    struct Witness {
        KZGCommitment witness_commitment;
        bool is_valid;
        
        Witness() : witness_commitment(Polynomial(std::vector<BigInt>{BigInt("0")}, BigInt("2")), 
                                      KZGParams(GroupElement(BigInt("1"), BigInt("2")), 1, BigInt("2"))), 
                   is_valid(false) {}
    };
    
    Witness generate_witness(const BigInt& element);
    bool verify_witness(const Witness& witness, const BigInt& element);
    bool update_witness(Witness& witness, const BigInt& element, bool is_addition);
    
    // 获取器
    const std::unordered_set<BigInt, BigInt::Hash>& get_current_set() const { return current_set; }
    const KZGCommitment& get_membership_commitment() const { return *membership_commitment; }
    size_t size() const { return current_set.size(); }
    size_t get_max_degree() const { return kzg_params.max_degree; }
    
    // 调试
    void print_state() const;
};

#endif // SIMPLE_POLYNOMIAL_ACCUMULATOR_H
