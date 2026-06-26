# Mini Algorithmic Information Theory（迷你算法信息论）

**从零开始、零依赖的 C 语言实现**，涵盖大学级算法信息论（AIT）。每个模块对应 MIT（及其他顶尖大学）的一门或多门课程，将柯尔莫哥洛夫复杂度、所罗门诺夫归纳、蔡廷 Ω 常数和不可压缩性方法转化为可运行的 C 代码，实现理论与实践的桥接。

## 子模块总览

| 子模块 | 主题 | 参考课程 |
|-----------|--------|-------------|
| [mini-algorithmic-randomness-ml](mini-algorithmic-randomness-ml/) | 基于机器学习的算法随机性：Martin-Löf 测试、GAN 随机性检测、基于神经压缩的 K(x) 估计 | MIT 6.867, Stanford CS236, Berkeley CS294-158 |
| [mini-chaitin-omega-constant](mini-chaitin-omega-constant/) | 蔡廷 Ω 常数：停机概率、无前缀通用图灵机、Solovay 归约、左 c.e. 实数、Calude 定理 | MIT 6.841, Stanford CS254, Berkeley CS278 |
| [mini-incompressibility-method](mini-incompressibility-method/) | 不可压缩性方法：Li & Vitányi §6 证明技术、排序下界、基于 K(x) 的组合证明 | MIT 6.841, Oxford Advanced Complexity, Cambridge Part III |
| [mini-kolmogorov-complexity-k](mini-kolmogorov-complexity-k/) | 柯尔莫哥洛夫复杂度 K(x)：普通/前缀定义、LZ/Huffman/BWT 压缩器、香农熵、标准化压缩距离应用 | MIT 6.841, Stanford CS254, CMU 15-855 |
| [mini-minimum-description-length](mini-minimum-description-length/) | 最小描述长度：两段码、NML 归一化最大似然码、精炼 MDL、模型选择与 AIC/BIC 对比 | MIT 6.441, Stanford EE376A, Helsinki CS |
| [mini-prefix-complexity-machine](mini-prefix-complexity-machine/) | 前缀复杂度：前缀图灵机、通用先验分布 m(x)、编码定理、单调机复杂度 Km(x) | MIT 6.841, Stanford CS254, MIT 6.441 |
| [mini-resource-bounded-kolmogorov](mini-resource-bounded-kolmogorov/) | 资源受限复杂度 K^t(x)/K^s(x)：时间/空间受限、Levin 通用搜索、标准化压缩距离近似 | MIT 6.841, Stanford CS254, CMU 15-855 |
| [mini-solomonoff-universal-prediction](mini-solomonoff-universal-prediction/) | 所罗门诺夫通用归纳：算法概率 M(x)、燕尾调度枚举、预测收敛性 | MIT 6.841/18.400, CMU 15-751, Cambridge Part III |

## 设计理念

- **零外部依赖** — 纯 C（C99/C11），仅使用 `libc` 和 `libm`
- **模块自包含** — 每个目录自带 `Makefile`、`include/`、`src/`、`examples/`、`demos/`、`tests/`
- **理论到代码的映射** — 每个模块包含 `docs/` 目录，内有课程对齐说明和教科书引用（Li & Vitányi, Cover & Thomas, Nies, Downey & Hirschfeldt）
- **实用演示程序** — 程序枚举、压缩基准测试、Martin-Löf 测试模拟、蔡廷 Ω 近似、通用搜索等

## 构建方式

每个模块相互独立。进入模块目录后运行：

```bash
cd mini-algorithmic-randomness-ml
make all    # 构建全部
make test   # 运行测试
```

需要 **GCC** 和 **GNU Make**。

## 项目结构

```
mini-algorithmic-info-theory/
├── mini-algorithmic-randomness-ml/     # 基于 ML 的 Martin-Löf 测试、GAN 随机性检测、神经网络 K(x) 估计
├── mini-chaitin-omega-constant/        # 蔡廷 Ω：停机概率、无前缀通用图灵机、Solovay 归约、左 c.e. 实数
├── mini-incompressibility-method/      # 不可压缩性证明：组合下界、Li & Vitányi §6 方法
├── mini-kolmogorov-complexity-k/       # 核心 K(x)：普通与前缀复杂度、LZ/Huffman/BWT 压缩器、熵度量
├── mini-minimum-description-length/    # MDL 原理：两段码、NML 通用码、精炼 MDL 模型选择
├── mini-prefix-complexity-machine/     # 前缀图灵机：前缀码与 Kraft 不等式、通用分布 m(x)、Km(x)
├── mini-resource-bounded-kolmogorov/   # K^t(x) 与 K^s(x)：时间/空间受限复杂度、Levin 通用搜索、NCD
└── mini-solomonoff-universal-prediction/ # 所罗门诺夫归纳：算法概率 M(x)、燕尾调度、预测
```

## 许可证

MIT
