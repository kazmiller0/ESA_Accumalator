#!/bin/bash

# Expressive Accumulator 测试脚本

echo "=== Expressive Accumulator Test Script ==="

# 检查可执行文件
if [ ! -f "bin/comprehensive_test" ] || [ ! -f "bin/performance_test" ]; then
    echo "Error: 可执行文件未找到，请先运行 ./scripts/build.sh"
    exit 1
fi

echo "运行综合功能测试..."
echo "================================"
./bin/comprehensive_test

echo ""
echo "运行性能基准测试..."
echo "================================"
./bin/performance_test

echo ""
echo "[OK] 所有测试已完成！"
