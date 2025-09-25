#include <iostream>
#include <set>
#include <vector>
#include <chrono>
#include <functional> // 需要包含 functional 头文件
#include "../include/expressive_accumulator.h"
extern "C" {
#include <flint/flint.h>
#include <flint/fmpz.h>
#include <flint/fmpz_mod.h>
#include <flint/fmpz_mod_poly.h>
}

using namespace expressive_accumulator;

// 声明外部定义的 FLINT 上下文
extern "C" {
    extern fmpz_mod_ctx_t flint_ctx;
}

// 新增：通用的基准测试运行器
void run_benchmark(const std::string& name, int num_ops, const std::function<void()>& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    double avg_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / (double)num_ops;
    
    std::cout << "\n  [Benchmark] " << name << ":" << std::endl;
    std::cout << "    " << std::fixed << std::setprecision(2) << avg_time << " µs/op  (" << num_ops << " operations)" << std::endl;
}


int main() {
    // 初始化 MCL 库
    mcl::bn::initPairing(mcl::BLS12_381);
    
    // 初始化 FLINT 的全局上下文
    initFlintContext();

    std::cout << "--- Performance Test ---" << std::endl;

    try {
        // ============================================================
        // 1. Setup
        // ============================================================
        const size_t UNIVERSE_SIZE = 2000;
        const int INITIAL_SET_SIZE = 1000;
        const int NUM_OPS = 100;

        Fr secret_s, secret_r;
        secret_s.setHashOf("perf_test_secret_s");
        secret_r.setHashOf("perf_test_secret_r");
        ExpressiveTrustedSetup setup(secret_s, secret_r, UNIVERSE_SIZE);
        setup.generatePowers();
        
        std::cout << "\n============================================================\n";
        std::cout << "  Performance Benchmark\n";
        std::cout << "  (Initial Set Size: " << INITIAL_SET_SIZE << ", Operations: " << NUM_OPS << ")\n";
        std::cout << "============================================================\n" << std::endl;

        // ============================================================
        // 2. Test Add/Delete (Prover)
        // ============================================================
        ExpressiveAccumulator acc_add(setup, G1_TYPE);
        for (int i = 0; i < INITIAL_SET_SIZE; ++i) acc_add.addElement(i);

        run_benchmark("addElement", NUM_OPS, [&]() {
            for (int i = 0; i < NUM_OPS; ++i) {
                acc_add.addElement(INITIAL_SET_SIZE + i);
            }
        });

        run_benchmark("deleteElement (with proof)", NUM_OPS, [&]() {
            // 删除之前存在的元素以确保逻辑正确
            for (int i = 0; i < NUM_OPS; ++i) {
                acc_add.deleteElement(i);
            }
        });
        
        // ============================================================
        // 3. Test Membership Proof Generation (Prover)
        // ============================================================
        ExpressiveAccumulator acc_prove(setup, G1_TYPE);
        for (int i = 0; i < INITIAL_SET_SIZE; ++i) acc_prove.addElement(i);
        
        run_benchmark("generateMembershipProof", NUM_OPS, [&]() {
            for (int i = 0; i < NUM_OPS; ++i) {
                acc_prove.generateMembershipProof(i);
            }
        });

        // ============================================================
        // 4. 精心构造测试集合以确保有交集
        // ============================================================
        std::cout << "\n构造测试数据..." << std::endl;
        std::set<int> common_elements;
        for (int i = 0; i < INITIAL_SET_SIZE / 2; ++i) {
            common_elements.insert(i);
        }

        std::set<int> a_only_elements;
        for (int i = INITIAL_SET_SIZE / 2; i < INITIAL_SET_SIZE; ++i) {
            a_only_elements.insert(i);
        }

        std::set<int> b_only_elements;
        for (int i = INITIAL_SET_SIZE; i < INITIAL_SET_SIZE + INITIAL_SET_SIZE / 2; ++i) {
            b_only_elements.insert(i);
        }

        ExpressiveAccumulator acc_b(setup, G1_TYPE);
        for (int el : common_elements) acc_b.addElement(el);
        for (int el : b_only_elements) acc_b.addElement(el);
        
        ExpressiveAccumulator acc_intersection(setup, G1_TYPE);
        for (int el : common_elements) acc_intersection.addElement(el);
        
        std::cout << "测试数据构造完成。" << std::endl;
        std::cout << "  - 集合 A 大小: " << acc_prove.getElements().size() << std::endl;
        std::cout << "  - 集合 B 大小: " << acc_b.getElements().size() << std::endl;
        std::cout << "  - 交集大小: " << acc_intersection.getElements().size() << std::endl;

        // ============================================================
        // 4. Test Intersection Proof Generation
        // ============================================================
        run_benchmark("generateIntersectionProof", NUM_OPS, [&]() {
            for (int i = 0; i < NUM_OPS; ++i) {
                auto intersect_proof = ExpressiveAccumulator::generateIntersectionProof(acc_prove, acc_b, setup);
                (void)intersect_proof; // 避免 "unused variable" 警告
            }
        });

        // ============================================================
        // 5. Test Intersection Proof Verification
        // ============================================================
        auto intersect_proof = ExpressiveAccumulator::generateIntersectionProof(acc_prove, acc_b, setup);
        auto expected_digest = acc_intersection.getDigest();

        run_benchmark("verifyIntersectionProof", NUM_OPS, [&]() {
            for (int i = 0; i < NUM_OPS; ++i) {
                volatile bool result = ExpressiveAccumulator::verifyIntersectionProof(
                    acc_prove.getDigest(), acc_b.getDigest(), intersect_proof, setup);
                (void)result; // 避免 "unused variable" 警告
            }
        });

    } catch (const std::exception& e) {
        std::cerr << "程序异常: " << e.what() << std::endl;
        return 1;
    }

    // 清理 FLINT 资源
    fmpz_mod_ctx_clear(flint_ctx);

    return 0;
}
