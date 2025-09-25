#include <iostream>
#include <vector>
#include <string>
#include <set>

extern "C" {
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpz_mod.h>
#include <flint/fmpz_mod_poly.h>
}

#include "expressive_accumulator.h"
#include <NTL/ZZ_p.h>

using namespace expressive_accumulator;
using namespace NTL;

// 声明外部定义的 FLINT 上下文
extern "C" {
    extern fmpz_mod_ctx_t flint_ctx;
}

// 辅助函数，用于打印测试结果
void printTestResult(const std::string& test_name, bool success) {
    std::cout << "[TEST] " << test_name << ": " << (success ? "PASSED" : "FAILED") << std::endl;
}

// 辅助函数，用于打印集合内容
void printSet(const std::string& name, const std::set<int>& s) {
    std::cout << name << " = { ";
    bool first = true;
    for (int el : s) {
        if (!first) {
            std::cout << ", ";
        }
        std::cout << el;
        first = false;
    }
    std::cout << " }" << std::endl;
}

//
// Created by parallels on 9/11/23.
//

void test_membership_proof(
    const ExpressiveTrustedSetup& setup,
    const ExpressiveAccumulator& acc,
    int member_element,
    int non_member_element
) {
    std::cout << "为元素 " << member_element << " 生成成员关系证明 (预期: 存在)..." << std::endl;
    MembershipProof member_proof = acc.generateMembershipProof(member_element);
    bool member_verify = ExpressiveAccumulator::verifyMembershipProof(acc.getDigest(), member_element, member_proof, setup);
    printTestResult("验证成员关系 (元素存在)", member_verify && member_proof.is_member);
    
    // 测试一个不存在的成员
    std::cout << "为元素 " << non_member_element << " 生成成员关系证明 (预期: 不存在)..." << std::endl;
    MembershipProof non_member_proof = acc.generateMembershipProof(non_member_element);
    // 对于一个不存在的元素，is_member 应该是 false，并且 verify 函数应该返回 false
    bool non_member_verify_correct_behavior = !non_member_proof.is_member && !ExpressiveAccumulator::verifyMembershipProof(acc.getDigest(), non_member_element, non_member_proof, setup);
    printTestResult("验证非成员关系 (元素不存在)", non_member_verify_correct_behavior);
    std::cout << std::endl;
}

void test_all() {
    // 初始化 MCL 库
    mcl::bn::initPairing(mcl::BLS12_381);
    
    // FLINT a modern C library for number theory.
    // We need to initialize the prime field `Fr` for FLINT.
    // auto p_str = mcl::bls12::Fr::getModulo();
    // fmpz_t p;
    // fmpz_init(p);
    // fmpz_set_str(p, p_str.c_str(), 10);
    // fmpz_mod_ctx_init(flint_ctx, p);


    // 1. 设置阶段
    std::cout << "--- 1. 可信设置阶段 ---" << std::endl;
    const size_t UNIVERSE_SIZE = 100;
    mcl::bls12::Fr secret_s, secret_r;
    secret_s.setHashOf("test_secret_s");
    secret_r.setHashOf("test_secret_r");
    ExpressiveTrustedSetup setup(secret_s, secret_r, UNIVERSE_SIZE);
    setup.generatePowers();
    std::cout << "可信设置完成，元素宇宙大小: " << UNIVERSE_SIZE << std::endl << std::endl;

    // 2. 累加器初始化
    std::cout << "--- 2. 累加器初始化 ---" << std::endl;
    std::set<int> set_a_elements = {1, 3, 5, 7, 9};
    ExpressiveAccumulator acc_a(setup, G1_TYPE);
    for (int el : set_a_elements) {
        acc_a.addElement(el);
    }
    printSet("集合 A", acc_a.getElements());

    std::set<int> set_b_elements = {2, 3, 5, 8, 9};
    ExpressiveAccumulator acc_b(setup, G1_TYPE);
    for (int el : set_b_elements) {
        acc_b.addElement(el);
    }
    printSet("集合 B", acc_b.getElements());
    std::cout << std::endl;

    // 3. 动态操作测试: 添加元素
    std::cout << "--- 3. 动态操作测试: 添加元素 ---" << std::endl;
    int element_to_add = 10;
    std::cout << "向累加器 A 添加元素 " << element_to_add << "..." << std::endl;
    UpdateProof add_proof = acc_a.addElement(element_to_add);
    printSet("新的集合 A", acc_a.getElements());
    bool add_verify = ExpressiveAccumulator::verifyUpdateProof(add_proof, setup);
    printTestResult("验证添加元素证明", add_verify);
    std::cout << std::endl;

    // 4. 动态操作测试: 删除元素
    std::cout << "--- 4. 动态操作测试: 删除元素 ---" << std::endl;
    int element_to_delete = 7;
    std::cout << "从累加器 A 删除元素 " << element_to_delete << "..." << std::endl;
    UpdateProof delete_proof = acc_a.deleteElement(element_to_delete);
    printSet("新的集合 A", acc_a.getElements());
    bool delete_verify = ExpressiveAccumulator::verifyUpdateProof(delete_proof, setup);
    printTestResult("验证删除元素证明", delete_verify);
    std::cout << std::endl;

    // 5. 成员关系证明测试
    std::cout << "--- 5. 成员关系证明测试 ---" << std::endl;
    // 测试一个存在的成员
    int member_element = 5;
    test_membership_proof(setup, acc_a, member_element, 6);
    
    // 6. 集合交集证明测试 (精确验证模型)
    std::cout << "--- 6. 集合交集证明测试 (精确验证模型) ---" << std::endl;
    printSet("当前集合 A", acc_a.getElements());
    printSet("当前集合 B", acc_b.getElements());
    
    // 证明者方: 生成交集证明
    std::cout << "证明者: 生成交集证明..." << std::endl;
    std::cout << "调试: 开始生成交集证明..." << std::endl;
    try {
        IntersectionProof intersection_proof = ExpressiveAccumulator::generateIntersectionProof(acc_a, acc_b, setup);
        std::cout << "调试: 交集证明生成完成" << std::endl;
        
        // 验证者方: 直接验证收到的证明
        std::cout << "验证者: 验证交集证明..." << std::endl;
        bool intersection_verify = ExpressiveAccumulator::verifyIntersectionProof(
            acc_a.getDigest(),
            acc_b.getDigest(),
            intersection_proof,
            setup
        );
        printTestResult("验证交集证明", intersection_verify);

    // (可选) 为了人类可读，我们打印出证明中包含的交集，并与实际交集对比
    if (intersection_verify) {
        std::cout << "验证通过。正在从证明中解析交集..." << std::endl;
        std::set<int> intersection_set = CharacteristicPolynomial::intersection(acc_a.getElements(), acc_b.getElements());
        CharacteristicPolynomial poly_I(intersection_set);
        Fr I_s = poly_I.evaluate(setup.getSecretS());
        G1 digest_I_from_proof = intersection_proof.intersection_digest_g1.value;
        G1 digest_I_recalculated;
        G1::mul(digest_I_recalculated, setup.getG1Generator(), I_s);
        if (digest_I_from_proof == digest_I_recalculated) {
            printSet("从证明中验证的交集与实际交集一致", intersection_set);
        } else {
            std::cout << "错误：证明有效，但解析出的交集与实际不符！" << std::endl;
        }
    }
    } catch (const std::exception& e) {
        std::cout << "调试: 交集证明生成失败: " << e.what() << std::endl;
        return;
    }
    std::cout << std::endl;
    
    std::cout << "--- 所有测试已完成 ---" << std::endl;
}

int main() {
    initMcl();
    initFlintContext();

    test_all();

    // 清理 FLINT 资源
    fmpz_mod_ctx_clear(flint_ctx);

    return 0;
}
