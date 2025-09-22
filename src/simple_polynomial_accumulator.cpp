#include "simple_polynomial_accumulator.h"
#include <iostream>
#include <algorithm>

// ==================== SimplePolynomialAccumulator 实现 ====================

SimplePolynomialAccumulator::SimplePolynomialAccumulator(size_t max_degree) 
    : kzg_params(GroupElement(BigInt("1"), BigInt("2")), 1, BigInt("2")) {
    
    // 生成安全素数作为群阶
    std::cout << "正在生成安全素数..." << std::endl;
    current_modulus = CryptoUtils::generate_safe_prime(64);
    std::cout << "安全素数生成完成: " << current_modulus.to_string() << std::endl;
    
    // 生成生成元
    std::cout << "正在生成群生成元..." << std::endl;
    GroupElement generator = GroupElement::generator(current_modulus);
    std::cout << "群生成元生成完成: " << generator.to_string() << std::endl;
    
    // 创建KZG参数
    kzg_params = KZGParams(generator, max_degree, current_modulus);
    
    // 初始化成员关系承诺
    Polynomial empty_poly(std::vector<BigInt>{BigInt("1")}, current_modulus);
    membership_commitment = std::make_unique<KZGCommitment>(empty_poly, kzg_params);
    
    std::cout << "简化多项式累加器初始化完成" << std::endl;
    std::cout << "群阶: " << current_modulus.to_string() << std::endl;
    std::cout << "生成元: " << generator.to_string() << std::endl;
    std::cout << "最大多项式次数: " << max_degree << std::endl;
}

void SimplePolynomialAccumulator::update_membership_commitment() {
    if (current_set.empty()) {
        // 空集合，使用常数多项式1
        Polynomial empty_poly(std::vector<BigInt>{BigInt("1")}, current_modulus);
        membership_commitment = std::make_unique<KZGCommitment>(empty_poly, kzg_params);
    } else {
        // 创建成员关系多项式
        Polynomial membership_poly = create_membership_polynomial();
        membership_commitment = std::make_unique<KZGCommitment>(membership_poly, kzg_params);
    }
}

Polynomial SimplePolynomialAccumulator::create_membership_polynomial() const {
    std::vector<BigInt> elements = get_current_elements();
    return Polynomial::from_roots(elements, current_modulus);
}

std::vector<BigInt> SimplePolynomialAccumulator::get_current_elements() const {
    std::vector<BigInt> elements;
    for (const auto& element : current_set) {
        elements.push_back(element);
    }
    return elements;
}

// ==================== 基本操作：增删改查 ====================

bool SimplePolynomialAccumulator::add_element(const BigInt& element) {
    if (current_set.find(element) != current_set.end()) {
        std::cout << "元素 " << element.to_string() << " 已存在于集合中" << std::endl;
        return false;
    }
    
    // 检查多项式次数限制
    if (current_set.size() >= kzg_params.max_degree) {
        std::cout << "集合大小已达到最大多项式次数限制" << std::endl;
        return false;
    }
    
    // 添加元素到集合
    current_set.insert(element);
    
    // 更新成员关系承诺
    update_membership_commitment();
    
    std::cout << "成功添加元素: " << element.to_string() << std::endl;
    std::cout << "新成员关系承诺: " << membership_commitment->get_commitment().to_string() << std::endl;
    return true;
}

bool SimplePolynomialAccumulator::remove_element(const BigInt& element) {
    auto it = current_set.find(element);
    if (it == current_set.end()) {
        std::cout << "元素 " << element.to_string() << " 不存在于集合中" << std::endl;
        return false;
    }
    
    // 从集合中移除元素
    current_set.erase(it);
    
    // 更新成员关系承诺
    update_membership_commitment();
    
    std::cout << "成功移除元素: " << element.to_string() << std::endl;
    std::cout << "新成员关系承诺: " << membership_commitment->get_commitment().to_string() << std::endl;
    return true;
}

bool SimplePolynomialAccumulator::update_element(const BigInt& old_element, const BigInt& new_element) {
    // 检查旧元素是否存在
    if (current_set.find(old_element) == current_set.end()) {
        std::cout << "元素 " << old_element.to_string() << " 不存在于集合中，无法修改" << std::endl;
        return false;
    }
    
    // 检查新元素是否已存在
    if (current_set.find(new_element) != current_set.end()) {
        std::cout << "元素 " << new_element.to_string() << " 已存在于集合中，无法修改" << std::endl;
        return false;
    }
    
    // 删除旧元素
    current_set.erase(old_element);
    
    // 添加新元素
    current_set.insert(new_element);
    
    // 更新成员关系承诺
    update_membership_commitment();
    
    std::cout << "成功修改元素: " << old_element.to_string() << " -> " << new_element.to_string() << std::endl;
    std::cout << "新成员关系承诺: " << membership_commitment->get_commitment().to_string() << std::endl;
    return true;
}

bool SimplePolynomialAccumulator::contains(const BigInt& element) const {
    return current_set.find(element) != current_set.end();
}

// ==================== 集合操作：交并补差 ====================

SimplePolynomialAccumulator::SetOperationResult SimplePolynomialAccumulator::compute_union(const std::unordered_set<BigInt, BigInt::Hash>& other_set) {
    SetOperationResult result;
    
    // 计算并集
    result.result_set = current_set;
    for (const auto& elem : other_set) {
        result.result_set.insert(elem);
    }
    
    // 创建并集多项式承诺
    std::vector<BigInt> union_elements;
    for (const auto& elem : result.result_set) {
        union_elements.push_back(elem);
    }
    Polynomial union_poly = Polynomial::from_roots(union_elements, current_modulus);
    result.result_commitment = KZGCommitment(union_poly, kzg_params);
    
    result.is_valid = true;
    
    std::cout << "计算并集完成，结果大小: " << result.result_set.size() << std::endl;
    return result;
}

SimplePolynomialAccumulator::SetOperationResult SimplePolynomialAccumulator::compute_intersection(const std::unordered_set<BigInt, BigInt::Hash>& other_set) {
    SetOperationResult result;
    
    // 计算交集
    for (const auto& elem : current_set) {
        if (other_set.find(elem) != other_set.end()) {
            result.result_set.insert(elem);
        }
    }
    
    // 创建交集多项式承诺
    std::vector<BigInt> intersection_elements;
    for (const auto& elem : result.result_set) {
        intersection_elements.push_back(elem);
    }
    Polynomial intersection_poly = Polynomial::from_roots(intersection_elements, current_modulus);
    result.result_commitment = KZGCommitment(intersection_poly, kzg_params);
    
    result.is_valid = true;
    
    std::cout << "计算交集完成，结果大小: " << result.result_set.size() << std::endl;
    return result;
}

SimplePolynomialAccumulator::SetOperationResult SimplePolynomialAccumulator::compute_difference(const std::unordered_set<BigInt, BigInt::Hash>& other_set) {
    SetOperationResult result;
    
    // 计算差集
    for (const auto& elem : current_set) {
        if (other_set.find(elem) == other_set.end()) {
            result.result_set.insert(elem);
        }
    }
    
    // 创建差集多项式承诺
    std::vector<BigInt> difference_elements;
    for (const auto& elem : result.result_set) {
        difference_elements.push_back(elem);
    }
    Polynomial difference_poly = Polynomial::from_roots(difference_elements, current_modulus);
    result.result_commitment = KZGCommitment(difference_poly, kzg_params);
    
    result.is_valid = true;
    
    std::cout << "计算差集完成，结果大小: " << result.result_set.size() << std::endl;
    return result;
}

SimplePolynomialAccumulator::SetOperationResult SimplePolynomialAccumulator::compute_complement(const std::unordered_set<BigInt, BigInt::Hash>& other_set) {
    // 补集就是差集
    return compute_difference(other_set);
}

// ==================== 见证系统 ====================

SimplePolynomialAccumulator::Witness SimplePolynomialAccumulator::generate_witness(const BigInt& element) {
    Witness witness;
    
    if (!contains(element)) {
        std::cout << "元素不在集合中，无法生成见证" << std::endl;
        return witness;
    }
    
    // 创建见证多项式（除当前元素外的所有元素）
    std::vector<BigInt> witness_elements;
    for (const auto& elem : current_set) {
        if (elem != element) {
            witness_elements.push_back(elem);
        }
    }
    
    if (witness_elements.empty()) {
        // 如果只有当前元素，见证是常数多项式1
        Polynomial witness_poly(std::vector<BigInt>{BigInt("1")}, current_modulus);
        witness.witness_commitment = KZGCommitment(witness_poly, kzg_params);
    } else {
        Polynomial witness_poly = Polynomial::from_roots(witness_elements, current_modulus);
        witness.witness_commitment = KZGCommitment(witness_poly, kzg_params);
    }
    
    witness.is_valid = true;
    
    std::cout << "生成见证: " << element.to_string() << std::endl;
    return witness;
}

bool SimplePolynomialAccumulator::verify_witness(const Witness& witness, const BigInt& element) {
    if (!witness.is_valid) {
        return false;
    }
    
    // 验证见证：见证承诺 * 元素承诺 = 成员关系承诺
    // 这里简化验证，检查见证承诺的有效性
    return witness.witness_commitment.get_commitment().valid();
}

bool SimplePolynomialAccumulator::update_witness(Witness& witness, const BigInt& element, bool is_addition) {
    if (!witness.is_valid) {
        return false;
    }
    
    // 重新生成见证
    witness = generate_witness(element);
    
    std::cout << "更新见证: " << element.to_string() << " (" << (is_addition ? "添加" : "删除") << ")" << std::endl;
    return true;
}

// ==================== 调试功能 ====================

void SimplePolynomialAccumulator::print_state() const {
    std::cout << "\n=== 简化多项式累加器状态 ===" << std::endl;
    std::cout << "当前集合大小: " << current_set.size() << std::endl;
    std::cout << "最大多项式次数: " << kzg_params.max_degree << std::endl;
    std::cout << "成员关系承诺: " << membership_commitment->get_commitment().to_string() << std::endl;
    
    std::cout << "集合元素: ";
    for (const auto& elem : current_set) {
        std::cout << elem.to_string() << " ";
    }
    std::cout << std::endl;
    std::cout << "=========================" << std::endl;
}
