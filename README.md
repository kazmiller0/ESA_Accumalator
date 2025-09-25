# 表现力累加器 (Expressive Accumulator)

一个基于BLS12-381椭圆曲线的密码学累加器实现，支持动态集合操作和零知识证明。

## 功能特性

- **动态集合操作**: 支持元素的添加和删除
- **成员关系证明**: 零知识证明元素是否属于集合
- **集合交集证明**: 零知识证明两个集合的交集
- **高效性能**: 基于FLINT库的高效多项式运算
- **密码学安全**: 基于BLS12-381椭圆曲线配对

## 项目结构

```
├── include/                    # 头文件
│   └── expressive_accumulator.h
├── src/                       # 源代码
│   └── expressive_accumulator.cpp
├── examples/                  # 示例和测试
│   ├── comprehensive_test.cpp # 综合功能测试
│   └── performance_test.cpp  # 性能基准测试
├── third_party/              # 第三方库
│   └── mcl/                  # MCL密码学库
├── paper/                    # 相关论文
├── CMakeLists.txt           # 构建配置
└── README.md               # 项目说明
```

## 依赖项

- **MCL**: 密码学库，提供BLS12-381椭圆曲线支持
- **FLINT**: 数论库，用于高效多项式运算
- **GMP**: 大整数运算库
- **OpenSSL**: 密码学函数库

## 构建说明

### 前置要求

- macOS (推荐使用Homebrew安装依赖)
- CMake 3.16+
- C++17编译器

### 安装依赖

```bash
# 安装Homebrew依赖
brew install cmake gmp flint openssl
```

### 构建项目

```bash
# 创建构建目录
mkdir build_flint && cd build_flint

# 配置项目
cmake ..

# 编译
make -j4

# 运行测试
./comprehensive_test
./performance_test
```

## 性能基准

基于1000个元素的集合，100次操作的平均性能：

| 操作 | 性能 | 说明 |
|------|------|------|
| 添加元素 | 361 µs/op | 基本集合操作 |
| 成员关系证明 | 1,228 µs/op | 零知识证明生成 |
| 删除元素 | 2,539 µs/op | 包含权利验证的删除 |
| 交集证明验证 | 15,040 µs/op | 集合交集证明验证 |
| 交集证明生成 | 261,000 µs/op | 最复杂的密码学操作 |

## 使用示例

```cpp
#include "expressive_accumulator.h"

// 初始化
expressive_accumulator::initMcl();
expressive_accumulator::initFlintContext();

// 创建可信设置
Fr s, r;
// ... 设置秘密参数
ExpressiveTrustedSetup setup(s, r, 1000);
setup.generatePowers();

// 创建累加器
ExpressiveAccumulator acc(setup, G1_TYPE);

// 添加元素
UpdateProof add_proof = acc.addElement(42);

// 生成成员关系证明
MembershipProof membership_proof = acc.generateMembershipProof(42);

// 验证证明
bool is_valid = ExpressiveAccumulator::verifyMembershipProof(
    acc.getDigest(), 42, membership_proof, setup);
```

## 算法原理

本项目实现了基于多项式承诺的累加器，核心思想是：

1. **特征多项式**: 集合S的特征多项式为 P(z) = ∏(z - x_i)，其中x_i ∈ S
2. **累加器值**: 在秘密点s处评估多项式，得到累加器值 g^P(s)
3. **零知识证明**: 使用配对和多项式承诺技术生成各种零知识证明

## 相关论文

- [An Expressive Zero-Knowledge Set Accumulator](./paper/An_Expressive_Zero-Knowledge_Set_Accumulator.pdf)
- [vChain](./paper/vChain.pdf)

## 许可证

本项目仅供学术研究使用。

## 贡献

欢迎提交Issue和Pull Request来改进项目。
