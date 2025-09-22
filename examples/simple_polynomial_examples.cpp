#include "simple_polynomial_accumulator.h"
#include <iostream>
#include <vector>

void demonstrate_basic_operations() {
    std::cout << "=== 基本操作演示：增删改查 ===" << std::endl;
    
    SimplePolynomialAccumulator acc(100);
    
    // 添加元素
    acc.add_element(BigInt("1001"));
    acc.add_element(BigInt("1002"));
    acc.add_element(BigInt("1003"));
    
    std::cout << "集合大小: " << acc.size() << std::endl;
    std::cout << "1001在集合中: " << (acc.contains(BigInt("1001")) ? "是" : "否") << std::endl;
    std::cout << "1005在集合中: " << (acc.contains(BigInt("1005")) ? "是" : "否") << std::endl;
    
    // 移除元素
    acc.remove_element(BigInt("1002"));
    std::cout << "移除1002后集合大小: " << acc.size() << std::endl;
    std::cout << "1002在集合中: " << (acc.contains(BigInt("1002")) ? "是" : "否") << std::endl;
    
    // 更新元素
    acc.update_element(BigInt("1003"), BigInt("1004"));
    std::cout << "更新1003->1004后集合大小: " << acc.size() << std::endl;
    std::cout << "1003在集合中: " << (acc.contains(BigInt("1003")) ? "是" : "否") << std::endl;
    std::cout << "1004在集合中: " << (acc.contains(BigInt("1004")) ? "是" : "否") << std::endl;
}

void demonstrate_set_operations() {
    std::cout << "\n=== 集合操作演示：交并补差 ===" << std::endl;
    
    // 创建两个累加器
    SimplePolynomialAccumulator acc1(100);
    SimplePolynomialAccumulator acc2(100);
    
    // 添加元素到第一个累加器
    std::vector<BigInt> elements1 = {BigInt("1"), BigInt("2"), BigInt("3"), BigInt("4")};
    for (const auto& elem : elements1) {
        acc1.add_element(elem);
    }
    
    // 添加元素到第二个累加器
    std::vector<BigInt> elements2 = {BigInt("3"), BigInt("4"), BigInt("5"), BigInt("6")};
    for (const auto& elem : elements2) {
        acc2.add_element(elem);
    }
    
    std::cout << "累加器1大小: " << acc1.size() << std::endl;
    std::cout << "累加器2大小: " << acc2.size() << std::endl;
    
    // 计算并集
    auto union_result = acc1.compute_union(acc2.get_current_set());
    if (union_result.is_valid) {
        std::cout << "并集计算成功，结果大小: " << union_result.result_set.size() << std::endl;
        std::cout << "并集元素: ";
        for (const auto& elem : union_result.result_set) {
            std::cout << elem.to_string() << " ";
        }
        std::cout << std::endl;
    }
    
    // 计算交集
    auto intersection_result = acc1.compute_intersection(acc2.get_current_set());
    if (intersection_result.is_valid) {
        std::cout << "交集计算成功，结果大小: " << intersection_result.result_set.size() << std::endl;
        std::cout << "交集元素: ";
        for (const auto& elem : intersection_result.result_set) {
            std::cout << elem.to_string() << " ";
        }
        std::cout << std::endl;
    }
    
    // 计算差集
    auto difference_result = acc1.compute_difference(acc2.get_current_set());
    if (difference_result.is_valid) {
        std::cout << "差集计算成功，结果大小: " << difference_result.result_set.size() << std::endl;
        std::cout << "差集元素: ";
        for (const auto& elem : difference_result.result_set) {
            std::cout << elem.to_string() << " ";
        }
        std::cout << std::endl;
    }
    
    // 计算补集
    auto complement_result = acc1.compute_complement(acc2.get_current_set());
    if (complement_result.is_valid) {
        std::cout << "补集计算成功，结果大小: " << complement_result.result_set.size() << std::endl;
        std::cout << "补集元素: ";
        for (const auto& elem : complement_result.result_set) {
            std::cout << elem.to_string() << " ";
        }
        std::cout << std::endl;
    }
}

void demonstrate_witness_system() {
    std::cout << "\n=== 见证系统演示 ===" << std::endl;
    
    SimplePolynomialAccumulator acc(100);
    acc.add_element(BigInt("100"));
    acc.add_element(BigInt("200"));
    acc.add_element(BigInt("300"));
    
    // 生成见证
    BigInt element = BigInt("200");
    auto witness = acc.generate_witness(element);
    
    if (witness.is_valid) {
        std::cout << "见证生成成功" << std::endl;
        
        // 验证见证
        bool witness_valid = acc.verify_witness(witness, element);
        std::cout << "见证验证: " << (witness_valid ? "成功" : "失败") << std::endl;
        
        // 更新见证
        acc.add_element(BigInt("400"));
        bool update_success = acc.update_witness(witness, BigInt("400"), true);
        std::cout << "见证更新: " << (update_success ? "成功" : "失败") << std::endl;
        
        // 再次验证更新后的见证
        witness_valid = acc.verify_witness(witness, BigInt("400"));
        std::cout << "更新后见证验证: " << (witness_valid ? "成功" : "失败") << std::endl;
    } else {
        std::cout << "见证生成失败" << std::endl;
    }
}

void demonstrate_comprehensive_example() {
    std::cout << "\n=== 综合示例演示 ===" << std::endl;
    
    SimplePolynomialAccumulator acc(50);
    
    // 添加一些元素
    std::vector<BigInt> elements = {BigInt("10"), BigInt("20"), BigInt("30"), BigInt("40")};
    for (const auto& elem : elements) {
        acc.add_element(elem);
    }
    
    std::cout << "初始集合大小: " << acc.size() << std::endl;
    acc.print_state();
    
    // 生成多个见证
    std::vector<BigInt> witness_elements = {BigInt("10"), BigInt("20"), BigInt("30")};
    std::vector<SimplePolynomialAccumulator::Witness> witnesses;
    
    for (const auto& elem : witness_elements) {
        auto witness = acc.generate_witness(elem);
        witnesses.push_back(witness);
        std::cout << "为元素 " << elem.to_string() << " 生成见证: " << (witness.is_valid ? "成功" : "失败") << std::endl;
    }
    
    // 验证所有见证
    bool all_witnesses_valid = true;
    for (size_t i = 0; i < witnesses.size(); ++i) {
        bool valid = acc.verify_witness(witnesses[i], witness_elements[i]);
        if (!valid) {
            all_witnesses_valid = false;
        }
        std::cout << "见证 " << witness_elements[i].to_string() << " 验证: " << (valid ? "成功" : "失败") << std::endl;
    }
    
    std::cout << "所有见证验证: " << (all_witnesses_valid ? "全部成功" : "部分失败") << std::endl;
    
    // 执行一些集合操作
    std::unordered_set<BigInt, BigInt::Hash> other_set = {BigInt("20"), BigInt("30"), BigInt("50"), BigInt("60")};
    
    auto union_result = acc.compute_union(other_set);
    auto intersection_result = acc.compute_intersection(other_set);
    auto difference_result = acc.compute_difference(other_set);
    
    std::cout << "\n集合操作结果:" << std::endl;
    std::cout << "并集大小: " << union_result.result_set.size() << std::endl;
    std::cout << "交集大小: " << intersection_result.result_set.size() << std::endl;
    std::cout << "差集大小: " << difference_result.result_set.size() << std::endl;
}

int main() {
    std::cout << "=== 简化多项式累加器核心功能演示 ===" << std::endl;
    
    try {
        // 基本操作：增删改查
        demonstrate_basic_operations();
        
        // 集合操作：交并补差
        demonstrate_set_operations();
        
        // 见证系统
        demonstrate_witness_system();
        
        // 综合示例
        demonstrate_comprehensive_example();
        
        std::cout << "\n=== 所有核心功能演示完成 ===" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "错误: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
