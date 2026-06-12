# BLUSK Programming Language

**차세대 고성능 프로그래밍 언어**
저수준 제어력 + 고수준 생산성 + 정적 분석 + AI 연산 확장

> `저수준 속도(Rust급) + 고수준 편의(Python급) + AI 내장`

---

## Language Switch

| Language | Link              |
| -------- | ----------------- |
| 한국어      | [Open](#korean)   |
| 中文       | [Open](#chinese)  |
| 日本語      | [Open](#japanese) |
| English  | [Open](#english)  |

---

## Quick Overview

BLUSK는 **빠른 실행**, **간결한 문법**, **컴파일 전 정적 분석**, **AI 연산 친화 구조**를 목표로 설계된 언어입니다.

이 프로젝트의 핵심 방향은 다음과 같습니다.

* 개발자가 큰 구조를 직접 통제할 수 있어야 한다.
* 컴파일러가 가능한 많은 결정을 미리 처리해야 한다.
* 런타임 비용은 줄이고, 실행 성능은 높여야 한다.
* AI 및 수치 연산을 장기적으로 확장할 수 있어야 한다.

---

## Core Highlights

| 항목     | 내용                          |
| ------ | --------------------------- |
| 실행 구조  | Register VM + `@native` AOT |
| 정적 분석  | BLUSK Checker               |
| 메모리 전략 | Checker + Runtime GC 하이브리드  |
| 목표 속도  | Go급 ~ Rust급 사이              |
| 저수준 제어 | C/C++ 수준                    |
| AI 확장  | Saturday AI / llama.cpp 연동  |

---

## Architecture at a Glance

```text
Lex → Parse → Checker → Compile → SVM 실행
```

BLUSK는 단순히 “코드를 실행하는 언어”가 아니라,
**코드가 실행되기 전에 최대한 많은 비용을 줄이는 언어**를 지향합니다.

### 1) Hybrid Execution Model

* **Register VM**: 기본 실행 구조
* **@native AOT**: LLVM 백엔드 기반 네이티브 컴파일
* **@vm / @native**: 개발자가 실행 의도를 직접 표현

### 2) BLUSK Checker

컴파일 전 단계에서 코드의 구조와 메모리 특성을 분석합니다.

Checker가 담당하는 일:

* RC 생략 가능성 판단
* dead code 제거
* 순환 참조 탐지
* Borrow 상태 위반 검사
* `@Newmemorycancel` 보호 구역 검증

### 3) Memory Strategy

```text
[컴파일 전 - Checker]
  └─ dead object 제거
  └─ RC 생략 마킹
  └─ 순환 참조 사전 탐지

[런타임 - SVM]
  └─ 기본: Reference Counting
  └─ 보조: Trial Deletion Cycle GC
  └─ @Newmemorycancel: GC 제외 / 쓰기 보호
```

---

## Why BLUSK Exists

BLUSK는 “문법만 예쁜 언어”가 아니라,
**실제로 빠르고, 관리 가능하고, 장기적으로 커질 수 있는 언어**를 목표로 합니다.

원하는 방향은 다음과 같습니다.

* 코드가 길어져도 구조가 무너지지 않을 것
* 실행 성능이 장난 아니게 나오도록 할 것
* 컴파일러가 메모리와 참조 관계를 최대한 미리 정리할 것
* AI 계산과 수치 처리도 자연스럽게 얹을 수 있을 것

---

# 한국어

<a id="korean"></a>

## 1. BLUSK 소개

BLUSK는 **저수준 제어력**과 **고수준 생산성**을 동시에 추구하는 프로그래밍 언어입니다.

핵심 목표는 네 가지입니다.

* 빠른 실행 속도
* 간결한 문법
* 컴파일 전 정적 분석
* AI 연산까지 고려한 구조

## 2. 핵심 설계

### 2-1. 하이브리드 실행 구조

BLUSK는 하나의 실행 방식에만 의존하지 않습니다.

* **Register VM**: 기본 실행 구조
* **@native AOT**: LLVM 백엔드로 네이티브 기계어 컴파일
* **실행 힌트**: `@vm`, `@native`로 개발자가 실행 방식을 직접 제어

### 2-2. BLUSK Checker

컴파일 전에 코드를 분석하는 정적 분석기입니다.

```text
Lex → Parse → Checker → Compile → SVM 실행
```

Checker가 하는 일:

* RC 생략 가능 변수 판별
* dead code 제거
* 순환 참조 탐지
* Borrow 상태 위반 검사
* `@Newmemorycancel` 구역 보호

### 2-3. GC 전략

BLUSK는 정적 분석과 런타임 GC를 함께 사용합니다.

```text
[컴파일 전 - Checker]
  └─ dead object 제거
  └─ RC 생략 마킹
  └─ 순환 참조 사전 탐지

[런타임 - SVM GC]
  └─ 기본: Reference Counting
  └─ 보조: Trial Deletion Cycle GC
  └─ @Newmemorycancel: GC 제외
```

## 3. 목표 성능

| 항목       | 목표             |
| -------- | -------------- |
| AI 행렬 연산 | Mojo 동급 이상     |
| 데이터 연산   | NumPy 대비 2~3배  |
| 전체 평균 속도 | Go급 ~ Rust급 사이 |
| 저수준 제어   | C/C++ 수준       |
| AI 내장    | Saturday AI    |

## 4. 프로젝트 구조

```text
BLUSK/
├── compiler/src/
│   ├── main.cpp          # 진입점
│   ├── lexer.h/cpp       # 토크나이저
│   ├── parser.h/cpp      # AST 생성
│   ├── checker.h/cpp     # 정적 분석기
│   ├── compiler.h/cpp    # Register VM 바이트코드 생성
│   ├── vm.h/cpp          # SVM 실행기
│   ├── value.h/cpp       # 타입 / 객체 표현
│   ├── ast.h/cpp         # AST 노드
│   ├── opcode.h          # 명령어 셋
│   └── error.h           # 에러 리포트
│
├── test_basic.blusk      # 기본 기능 테스트
├── test_checker.blusk    # Checker 기능 테스트
└── README.md             # 문서
```

## 5. 빌드 방법

### Windows (Visual Studio)

```bash
# BLUSK.sln 열기
# 빌드 → x64 Release
```

### Linux / macOS

```bash
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --config Release
```

### 의존성

* C++17 이상
* (선택) `llama.cpp` — `LLAMA_EMBED` 정의 시 `.gguf` 모델 지원
* (선택) `httplib.h` — Saturday AI API 모드용

## 6. 실행 예제

```bash
# 기본 테스트
blusk.exe test_basic.blusk

# Checker 기능 확인
blusk.exe test_checker.blusk
```

예시 출력:

```text
[BLUSK] Compiling: test_checker.blusk
[Checker] Analyzing: test_checker.blusk
[Checker] Done. errors=0 warnings=1 dead=1 rcSkip=3 cycleSkip=3 memProt=0
[Checker] RC-skip (no ref-counting): local_only used_var result
[Checker] Dead vars (skipped in compile): never_used
[BLUSK] Compilation succeeded. 42 instructions.
[BLUSK] Running...

Local only: 42
Max size: 100
Result: 20
Data[1]: 200

[BLUSK] Execution finished.
```

## 7. 주요 기능

### 변수 선언

```blusk
gg x = 10;          // 타입 추론 변수
num PI = 3.14;      // 상수 (재대입 불가)
int age = 25;       // 명시적 타입 (향후 지원)
```

### 어노테이션

```blusk
@native
void fast_loop() { ... }

@Newmemorycancel
gg protected = 100;
```

### Saturday AI

```blusk
import ai;

gg ai = new AI();
ai.load("model.gguf");
// ai.load("api:openai:gpt-4o");
response = ai.ask("What is BLUSK?", 0.7, 3);
print(response);
```

### Math 내장 함수

```blusk
import Blusk.num.Math;

gg sqrt_val = Math.sqrt(16);
gg pi = Math.PI;
gg result = Math.sin(pi / 2);
```

### 배열

```blusk
gg[] numbers = {1, 2, 3, 4, 5};
print(numbers[2]);
numbers[2] = 99;
```

### 제어 흐름

```blusk
if (x < y) {
    print("Less");
} else {
    print("Greater or equal");
};

for (gg i = 0; i < 10; i++) {
    print(i);
};

while (condition) {
    // ...
};

loop(task.time = 5000, task.interval = 1000) {
    // 5초간 1초 간격 루프
};
```

## 8. Checker 동작 원리

### RC 생략 판단 기준

```blusk
gg local = 10;     // ✅ rcSkip = true
                   // 이유: 함수 안에서만 살고 참조 1회

gg escaped;
void foo() {
    escaped = 20;  // ❌ rcSkip = false
}                  // 이유: 함수 밖으로 탈출
```

### Dead Code 제거

```blusk
gg never_used = 999;   // ⚠️ Checker 경고 + 컴파일 제외
```

### 순환 참조 탐지

```blusk
// A → B → C → A 순환 구조
// → Checker: Cycle GC 필요 (경고)
// → 런타임: Trial Deletion으로 처리
```

## 9. 설계 철학

1. **저수준 제어 + 고수준 편의**

   * 어셈블리 접근 가능
   * Python급 간결함 지향

2. **컴파일러가 똑똑하게, 개발자는 힌트만**

   * Checker가 RC 생략 / Cycle 판단 자동 처리
   * `@native`, `@vm`으로 직접 제어 가능

3. **AI가 언어의 일부**

   * Saturday AI를 단순 라이브러리가 아닌 언어 기능으로 취급
   * `OP_AI_LOAD`, `OP_AI_ASK` 같은 VM 명령어 고려

4. **장기 생태계 목표**

   * C, C++, Java, Python, BLUSK를 잇는 범용 생태계 지향
   * 속도 + 편의 + AI 최적화를 동시에 추구

## 10. 로드맵

* ✅ **0.7 Alpha** — Register VM + Checker + 기본 기능
* ⬜ **0.9 Beta** — JIT + SIMD tensor 완성 + 표준 라이브러리 확장
* ⬜ **1.0 Release** — `@native` LLVM 완성 + BDK 배포 + 문서화
* ⬜ **2.0** — 자체 백엔드 + GPU 연산 지원

## 11. 라이선스

* BLUSK 언어 자체: MIT (예정)
* 내장 llama.cpp: MIT License (고지 필요)

## 12. 기여

* 현재 상태: 0.7 Alpha 개발 중
* 연락: 프로젝트 정보 추가 예정

> **"The language that thinks ahead."**
> BLUSK가 컴파일 전에 생각하고, 런타임에서 실행한다.

---

# 中文

<a id="chinese"></a>

## 1. 什么是 BLUSK

BLUSK 是一门同时追求 **底层控制** 与 **高级开发效率** 的编程语言。

它的目标很清楚：

* 执行速度快
* 语法简洁
* 编译前可做静态分析
* 适合扩展 AI 计算能力

## 2. 核心设计

### 2-1. 混合执行结构

BLUSK 不依赖单一执行方式。

* **Register VM**：默认执行模型
* **@native AOT**：通过 LLVM 后端编译成本地机器码
* **运行时提示**：使用 `@vm` / `@native` 控制执行方式

### 2-2. BLUSK Checker

这是编译前运行的静态分析器。

```text
Lex → Parse → Checker → Compile → SVM
```

Checker 负责：

* 判断 RC 是否可跳过
* 删除 dead code
* 检测循环引用
* 检查 Borrow 状态错误
* 保护 `@Newmemorycancel` 区域

### 2-3. GC 策略

BLUSK 结合静态分析与运行时 GC。

```text
[编译前 - Checker]
  └─ 删除 dead object
  └─ 标记 RC 跳过
  └─ 预先检测循环引用

[运行时 - SVM GC]
  └─ 默认：Reference Counting
  └─ 辅助：Trial Deletion Cycle GC
  └─ @Newmemorycancel：排除 GC
```

## 3. 性能目标

| 项目      | 目标              |
| ------- | --------------- |
| AI 矩阵运算 | Mojo 同级或更高      |
| 数据运算    | 比 NumPy 快 2~3 倍 |
| 平均速度    | 介于 Go 和 Rust 之间 |
| 底层控制    | C/C++ 级别        |
| 内建 AI   | Saturday AI     |

## 4. 项目结构

```text
BLUSK/
├── compiler/src/
│   ├── main.cpp
│   ├── lexer.h/cpp
│   ├── parser.h/cpp
│   ├── checker.h/cpp
│   ├── compiler.h/cpp
│   ├── vm.h/cpp
│   ├── value.h/cpp
│   ├── ast.h/cpp
│   ├── opcode.h
│   └── error.h
│
├── test_basic.blusk
├── test_checker.blusk
└── README.md
```

## 5. 构建方法

### Windows (Visual Studio)

```bash
# 打开 BLUSK.sln
# 构建 → x64 Release
```

### Linux / macOS

```bash
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --config Release
```

## 6. 使用示例

```bash
blusk.exe test_basic.blusk
blusk.exe test_checker.blusk
```

## 7. 主要功能

* 变量声明
* 注解（`@native`, `@Newmemorycancel`）
* Saturday AI
* 数学内建函数
* 数组
* 控制流

## 8. Checker 逻辑

* RC 跳过判断
* dead code 删除
* 循环引用检测

## 9. 设计理念

1. 底层控制 + 高级便利
2. 编译器更聪明，开发者只提供提示
3. AI 是语言的一部分
4. 面向长期生态

## 10. 路线图

* 0.7 Alpha：Register VM + Checker + 基本功能
* 0.9 Beta：JIT + SIMD tensor + 标准库扩展
* 1.0 Release：`@native` LLVM + 文档
* 2.0：自研后端 + GPU 支持

## 11. 许可证

* BLUSK：MIT（计划中）
* llama.cpp：MIT License

---

# 日本語

<a id="japanese"></a>

## 1. BLUSK とは

BLUSK は、**低レベル制御** と **高レベルの書きやすさ** を両立することを目指したプログラミング言語です。

主な目標は次のとおりです。

* 高速な実行
* わかりやすい文法
* コンパイル前の静的解析
* AI 計算への対応

## 2. コア設計

### 2-1. ハイブリッド実行構造

BLUSK は 1 つの方式だけに依存しません。

* **Register VM**: 基本の実行方式
* **@native AOT**: LLVM バックエンドでネイティブコード化
* **ランタイムヒント**: `@vm` / `@native` で制御

### 2-2. BLUSK Checker

コンパイル前にコードを解析する静的解析器です。

```text
Lex → Parse → Checker → Compile → SVM
```

Checker の役割:

* RC スキップ判定
* dead code の削除
* 循環参照の検出
* Borrow 状態の検査
* `@Newmemorycancel` 領域の保護

### 2-3. GC 戦略

BLUSK は静的解析とランタイム GC を組み合わせます。

```text
[コンパイル前 - Checker]
  └─ dead object の削除
  └─ RC スキップ判定
  └─ 循環参照の事前検出

[ランタイム - SVM GC]
  └─ 基本: Reference Counting
  └─ 補助: Trial Deletion Cycle GC
  └─ @Newmemorycancel 領域: GC 対象外
```

## 3. 目標性能

| 項目      | 目標            |
| ------- | ------------- |
| AI 行列計算 | Mojo 同等以上     |
| データ処理   | NumPy の 2〜3 倍 |
| 平均速度    | Go と Rust の間  |
| 低レベル制御  | C/C++ レベル     |
| AI 内蔵   | Saturday AI   |

## 4. プロジェクト構成

```text
BLUSK/
├── compiler/src/
│   ├── main.cpp
│   ├── lexer.h/cpp
│   ├── parser.h/cpp
│   ├── checker.h/cpp
│   ├── compiler.h/cpp
│   ├── vm.h/cpp
│   ├── value.h/cpp
│   ├── ast.h/cpp
│   ├── opcode.h
│   └── error.h
│
├── test_basic.blusk
├── test_checker.blusk
└── README.md
```

## 5. ビルド方法

### Windows (Visual Studio)

```bash
# BLUSK.sln を開く
# ビルド → x64 Release
```

### Linux / macOS

```bash
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --config Release
```

## 6. 使用例

```bash
blusk.exe test_basic.blusk
blusk.exe test_checker.blusk
```

## 7. 主な機能

* 変数宣言
* アノテーション（`@native`, `@Newmemorycancel`）
* Saturday AI
* 数学関数
* 配列
* 制御構文

## 8. Checker の仕組み

* RC スキップ判定
* Dead Code の削除
* 循環参照の検出

## 9. 設計思想

1. 低レベル制御 + 高レベルの便利さ
2. コンパイラが賢く、開発者はヒントを出すだけ
3. AI は言語の一部
4. 長く使えるエコシステムを目指す

## 10. ロードマップ

* 0.7 Alpha: Register VM + Checker + 基本機能
* 0.9 Beta: JIT + SIMD tensor + 標準ライブラリ拡張
* 1.0 Release: `@native` LLVM + ドキュメント
* 2.0: 自作バックエンド + GPU 対応

## 11. ライセンス

* BLUSK: MIT（予定）
* llama.cpp: MIT License

---

# English

<a id="english"></a>

## 1. What is BLUSK?

BLUSK is a programming language designed to combine **low-level control** with **high-level productivity**.

Its main goals are:

* Fast execution
* Simple syntax
* Static analysis before compilation
* AI-oriented computation support

## 2. Core Design

### 2-1. Hybrid Execution Model

BLUSK does not rely on a single execution path.

* **Register VM**: default execution model
* **@native AOT**: native machine code via LLVM backend
* **Runtime hints**: `@vm` / `@native` let the developer choose

### 2-2. BLUSK Checker

A static analyzer that runs before compilation.

```text
Lex → Parse → Checker → Compile → SVM
```

The Checker handles:

* RC skip analysis
* dead code removal
* cyclic reference detection
* Borrow state validation
* `@Newmemorycancel` protection

### 2-3. GC Strategy

BLUSK combines static analysis with runtime GC.

```text
[Before compilation - Checker]
  └─ dead object removal
  └─ RC skip marking
  └─ cyclic reference pre-detection

[Runtime - SVM GC]
  └─ Default: Reference Counting
  └─ Backup: Trial Deletion Cycle GC
  └─ @Newmemorycancel region: excluded from GC
```

## 3. Performance Targets

| Item              | Target               |
| ----------------- | -------------------- |
| AI matrix ops     | Mojo-level or better |
| Data processing   | 2–3x NumPy           |
| Average speed     | Between Go and Rust  |
| Low-level control | C/C++ level          |
| Built-in AI       | Saturday AI          |

## 4. Project Structure

```text
BLUSK/
├── compiler/src/
│   ├── main.cpp
│   ├── lexer.h/cpp
│   ├── parser.h/cpp
│   ├── checker.h/cpp
│   ├── compiler.h/cpp
│   ├── vm.h/cpp
│   ├── value.h/cpp
│   ├── ast.h/cpp
│   ├── opcode.h
│   └── error.h
│
├── test_basic.blusk
├── test_checker.blusk
└── README.md
```

## 5. Build

### Windows (Visual Studio)

```bash
# Open BLUSK.sln
# Build → x64 Release
```

### Linux / macOS

```bash
cmake -S . -B cmake-build-release -DCMAKE_BUILD_TYPE=Release
cmake --build cmake-build-release --config Release
```

## 6. Examples

```bash
blusk.exe test_basic.blusk
blusk.exe test_checker.blusk
```

## 7. Main Features

* Variable declarations
* Annotations (`@native`, `@Newmemorycancel`)
* Saturday AI
* Math built-ins
* Arrays
* Control flow

## 8. Checker Logic

* RC skip decisions
* dead code elimination
* cyclic reference detection

## 9. Design Philosophy

1. Low-level control + high-level convenience
2. Smarter compiler, developer-provided hints
3. AI is part of the language
4. Built for a long-term ecosystem

## 10. Roadmap

* 0.7 Alpha: Register VM + Checker + basic features
* 0.9 Beta: JIT + SIMD tensor + stdlib expansion
* 1.0 Release: `@native` LLVM + documentation
* 2.0: custom backend + GPU support

## 11. License

* BLUSK: MIT (planned)
* llama.cpp: MIT License

---

## Edit Notes

* 언어별 섹션은 같은 구조라서 한 군데만 고쳐도 다른 언어로 옮기기 쉽다.
* 실제 기능이 바뀌면 먼저 한국어 섹션을 고친 뒤, 나머지를 맞추면 된다.
* 섹션 추가가 필요하면 `API`, `Installation`, `Examples`, `FAQ`를 덧붙이면 된다.
