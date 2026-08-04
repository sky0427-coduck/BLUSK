# BLUSK 문법 레퍼런스 (0.7-alpha)

> 이 문서는 실제 `compiler/src/` 소스(lexer/parser/checker/compiler/vm)를 직접 읽고 검증하여 작성했습니다.
> 각 항목에 상태 태그를 붙였습니다:
> - ✅ **동작 확인** : 파서 → 컴파일러 → VM까지 실제로 실행됨
> - 🔧 **파싱만 됨** : 문법은 인식되지만 컴파일러/VM이 무시하거나 stub 처리함 (실행 안 되거나 부분 실행)
> - ❌ **미구현** : 아예 문법 자체가 없음

---

## 0. 현재 상태 요약

| 항목 | 값 |
|---|---|
| 버전 | 0.7-alpha (`bluskmeta.json`, `CMakeLists.txt`, `main.cpp` 일치) |
| AI 내장 | **보류**. llama.cpp/httplib 의존성 제거됨. `opcode.h`에 `OP_AI_*` 주석 처리로 남아있음 |
| 실행 구조 | Register VM(SVM) + OOP 전용 AST 트리워킹 인터프리터의 하이브리드 |
| self-hosting 목표 | v1.5 |

---

## 1. 기본 구조

```blusk
root package.패키지명;     // ✅ 선택적 패키지 선언

import blusk26;            // ✅ 필수 (없으면 컴파일러가 fatal 에러)
import io;
import Blusk.num.Math;

@entry
Blusk public void main() {
    // 코드
    the end;
}
```

- `@Entry`, `@ENTRY` 등 대소문자 무관하게 인식됨 (파서가 소문자로 정규화).
- `Blusk public void main()` 에서 `public`은 선택 — 있어도 없어도 동일하게 동작.
- 클래스 메서드가 아닌 **독립(top-level) 함수는 선언 문법은 파싱되지만 호출할 방법이 없습니다.** `@native`로 감싸도 컴파일러가 `ANNOTATION` 노드를 통째로 건너뛰기 때문에 함수 본문 자체가 바이트코드로 생성되지 않고, `funcName(args)` 형태의 최상위 호출 문법도 파서에 없습니다. 함수가 필요하면 지금은 **클래스 메서드**로 작성하세요. (🔧 파싱만 됨 — `examples/08_annotations.blusk`는 이 문제로 실제로는 동작하지 않는 예제입니다.)

---

## 2. 변수 선언

| 키워드 | 의미 | 폭/정밀도 |
|---|---|---|
| `gg` | 타입 추론 변수 | 정수 리터럴 → 64bit(long), 실수 리터럴 → 64bit(double) |
| `num` | 상수 (재대입 불가, 위반 시 Checker 에러) | gg와 동일 |
| `int` | 32bit 정수 | 오버플로 시 C처럼 wrap-around |
| `long` | 64bit 정수 | |
| `float` | 32bit 실수 | |
| `double` | 64bit 실수 | |
| `bool` | 불리언 | |
| `str` / `string` | 문자열 (`string`은 `str`의 동의어) | |

```blusk
gg x = 10;
num MAX = 999;        // 이후 MAX = 1; 하면 Checker 에러

int a = 5;             // 32bit — a + b (long) 연산 시 자동 승격 후 재대입 시 다시 int로 캐스팅됨
long big = 10000000000;
float f = 3.14;
double d = 3.14159265358979;
str msg = "world";
```

- ✅ `int`/`long`/`float`/`double`은 단순 별칭이 아니라 실제로 폭이 다른 값으로 저장됩니다 (`Value::Int32`/`Int64`/`Float32`/`Float64`). 산술 연산 시 더 넓은 타입으로 자동 승격되고, 선언된 타입 변수에 재대입할 때는 원래 폭으로 다시 캐스팅됩니다.
- ✅ Checker가 narrowing(정밀도 손실 가능성)을 내부적으로 판단하는 로직이 있습니다 (`typeWidthCompare`).

---

## 3. 출력

```blusk
import io;

print("hello");               // ✅ 문자열 그대로
print("%d", x);                // ✅ 정수 포맷
print("%s", name);              // ✅ 문자열 포맷
print(f"{x} + {y} = {z}");      // ✅ f-string
print(someExpr);                // ✅ print(단일 표현식) — 변수, Matrix, 산술식 등
```

`print`를 쓰려면 `import io;`가 반드시 있어야 합니다 (없으면 컴파일 에러 리포트).

---

## 4. 입력

```blusk
import io;

io.read("입력하세요: ", com name);   // ✅ name 변수에 저장
```

이 하드코딩된 `io.read(...)` 형태만 지원됩니다. `stdlib/blusk_io.*`에 `read_file`, `write_file`, `append_file`, `file_exists`, `mmap_read` 등이 구현되어 있지만, **BLUSK 소스에서 이걸 호출하는 문법 경로가 아직 연결되지 않았습니다** (🔧 — 모듈 레지스트리엔 등록되어 있으나 컴파일러가 `OP_NATIVE_CALL`을 생성하는 코드가 없음). `string`/`collections`/`time` 모듈도 동일한 상태입니다.

---

## 5. 산술 연산자

| 연산자 | 의미 |
|---|---|
| `+` `-` `*` `/` `%` `**` | 사칙연산 / 나머지 / 제곱 |
| `-x` | 단항 음수 |

모두 ✅ 동작 확인.

---

## 6. 비교 / 논리 연산자

| 연산자 | 의미 | 상태 |
|---|---|---|
| `==` `!=` `<` `<=` `>` `>=` | 비교 | ✅ |
| `&&` | AND | ✅ |
| `\|\|` | OR | ✅ |
| `!` | NOT | ✅ |

우선순위(낮음→높음): `\|\|` < `&&` < 비교 < `+ -` < `* / %` < `**` < 단항.

```blusk
if (a > 0 && b < 10) { print("범위 안"); };
if (!ready || retry) { print("재시도"); };
```

> ⚠️ **주의**: `if`/`while`/`switch`의 조건식은 위 연산자를 자유롭게 조합한 완전한 표현식을 지원하지만, **C스타일 `for`문의 조건절(두 번째 `;` 사이)은 `왼쪽 <연산자> 오른쪽` 단일 비교 토큰 하나만 지원합니다** (`&&`/`||`로 여러 조건을 묶을 수 없음). 아래 7번 참고.

---

## 7. 조건문

```blusk
if (x > 0) {
    print("양수");
} elseif (x == 0) {
    print("영");
} else {
    print("음수");
};
```

`else if` (공백 포함)도 동일하게 동작. ✅

---

## 8. 반복문

### for (C스타일)

```blusk
for (int i = 0; i <= 10; i++) {
    print("%d", i);
};

for (gg i = 0; i < 10; i++) { ... }   // gg도 그대로 가능
```

- ✅ 초기화부는 `gg`, `int`, `long`, `float`, `double`, `bool`, `str` 아무 타입 키워드나 사용 가능 — C++처럼 `for (int i = 0; ...)` 형태 그대로 씁니다.
- `i++` / `i--` / `i = i + 1` 모두 지원.
- ⚠️ **조건절은 `변수 비교연산자 값` 형태의 단일 비교만 지원합니다.** 예: `i <= 10`은 되지만 `i <= 10 && flag`처럼 조건을 두 개 이상 묶는 건 지금 for문 조건절에서는 안 됩니다 (if/while과 달리 조건절 파서가 토큰 3개만 읽음). 복합 조건이 필요하면 for 안에 `if`로 처리하세요.

### for-in
```blusk
for (gg x in arr) {
    print(f"{x}");
};
```
✅ 동작 확인.

### while
```blusk
while (x < 100) {
    x = x + 1;
};
```
✅ 완전한 표현식(`&&`, `||`, `!` 포함) 조건 지원.

### loop (횟수/시간 기반)
```blusk
loop(5, 1) {           // ✅ 신버전: 5회 반복, 매 회 후 1초 대기
    print("5번 실행");
};

loop(task.time = 5, task.interval = 1) { ... };  // ✅ 구버전 호환 문법도 동일하게 지원

loop { print("무한 반복, break 필요"); };          // ✅ 괄호 없으면 무한 루프 (break로만 탈출)
```

### break / continue
```blusk
for (gg i = 0; i < 10; i++) {
    if (i == 5) { break; };
    if (i == 3) { continue; };
    print("%d", i);
};
```
✅ 중첩 루프(for 안에 for, if 안에 break 등) 안에서도 정확한 루프 경계로 점프하도록 처리되어 있습니다.

---

## 9. 종료 명령

```blusk
the end;                        // ✅ 즉시 종료
the end : if (x < 0);           // ✅ 조건부 종료 — 괄호 있어도 없어도 동작
the end : if x < 0;             // ✅ 괄호 생략도 가능
the end : return x;             // ✅ 값 반환 후 종료
```

---

## 10. 배열

```blusk
gg[] nums = {1, 2, 3, 4, 5};

print("%d", nums[0]);       // ✅ 읽기
nums[2] = 99;                // ✅ 쓰기
print("%d", size.nums);      // ✅ 길이
```
범위를 벗어난 인덱스 접근은 런타임 에러로 리포트됩니다.

---

## 11. OOP

```blusk
import oop;

Blusk public class Animal {
    gg name;
    gg sound;

    Animal(gg n, gg s) {
        name  = n;
        sound = s;
    }

    Blusk public void speak() {
        print(f"{name} says {sound}!");
    }
}

Blusk public class Dog : forkfrom Animal {
    gg breed;

    Dog(gg n, gg b) {
        name  = n;
        sound = "Woof";
        breed = b;
    }
}

gg dog = new Dog("Rex", "Labrador");
dog.speak();
dog = new Dog("Max", "Poodle");  // ✅ 재대입의 new도 지원 (이전에는 VAR_DECL에서만 되고 ASSIGN에서는 안 됨)
```

- ✅ 생성자, 단일 상속(`forkfrom`), 메서드 오버라이드(자식에 없으면 부모 체인을 재귀적으로 탐색) 모두 동작.
- ⚠️ **구현 구조가 다릅니다**: 클래스 메서드 본문은 메인 Register VM 바이트코드로 컴파일되는 게 아니라, 별도의 **AST 트리워킹 인터프리터**(`SVM::runBlock`)가 직접 실행합니다. 그 결과 메서드 본문 안에서는 `print`(f-string 포함, ✅ 확인됨), 변수 선언/대입, `if/elseif/else`, `while`, `for`(C스타일), `task.sleep`만 지원됩니다 — `switch`, `Matrix`, 배열, `for-in`, 논리연산자(`&&`/`||`/`!`)는 메서드 본문 안에서는 아직 처리되지 않습니다 (🔧). 최상위 `main()` 안에서는 이런 제약이 없습니다.
- `forkfrom` 다중 상속은 아직 없음 (❌, 단일 부모만).

---

## 12. Math 라이브러리

```blusk
import Blusk.num.Math;

gg a = Math.sqrt(16);       // ✅ 4.0
gg b = Math.abs(-5);        // ✅ 5
gg d = Math.sin(0);         // ✅ 0.0
gg e = Math.cos(0);         // ✅ 1.0
gg f = Math.log(Math.E);    // ✅ 1.0
gg g = Math.PI;             // ✅ 3.14159...
gg h = Math.E;              // ✅ 2.71828...
gg j = Math.floor(3.7);     // ✅ 3
gg k = Math.ceil(3.2);      // ✅ 4
gg l = Math.round(3.5);     // ✅ 4
gg c = Math.pow(2, 10);     // ❌ 아직 미구현 (파서는 통과하지만 컴파일러가 "Unknown Math" 에러 리포트)
gg i = Math.I;              // 🔧 버그: "허수단위"로 문서화되어 있었지만 실제로는 Math.E 값이 반환됨 (컴파일러 분기 누락)
```

`stdlib/blusk_math.*`에는 `min/max/clamp/lerp/random/gcd/lcm/is_prime/log2` 등도 구현돼 있지만 4번 항목과 동일한 이유로 BLUSK 문법에서 아직 호출할 수 없습니다.

---

## 13. 어노테이션

| 어노테이션 | 의미 | 상태 |
|---|---|---|
| `@entry` / `@Entry` | 엔트리포인트 (멀티 main 지원, 대소문자 무관) | ✅ |
| `@native` | 네이티브 컴파일 대상 표시 | 🔧 파싱만 됨 — 함수 본문이 컴파일러에서 완전히 스킵됨 (호출 수단도 없음, 1번 섹션 참고) |
| `@vm` | VM 강제 실행 표시 | 🔧 파싱만 됨, 의미 있는 처리 없음 |
| `@unsafe` | 저수준 접근 허용 | 🔧 마킹만 됨 |
| `@Newmemorycancel` | 메모리 보호 구역 선언 | ✅ Checker가 실제로 쓰기를 차단함 |
| `@noalias` | 포인터 앨리어싱 없음 선언 | ❌ 문법조차 없음 |
| `@simd for varName;` | SIMD 최적화 표시 | 🔧 파싱되고 VM까지 전달되지만, 실제 AVX2 연산은 `Matrix` 리터럴 생성 시점(`OP_MATRIX_NEW`)에 이미 끝나 있어서 이 명령어 자체는 사실상 아무 일도 안 함 (연산 뒤 확인용 no-op) |

```blusk
@entry
Blusk public void main() { ... }

@Newmemorycancel
gg hardwareBuffer = 0;
```

---

## 14. Saturday AI — **보류 (0.7 기준 제거됨)**

> 0.7-alpha부터 AI 내장 기능은 **보류**되었습니다. llama.cpp/httplib 의존성이 CMakeLists.txt에서 빠졌고, `opcode.h`의 AI 관련 옵코드는 주석 처리, `compiler.cpp`는 `AI_LOAD`/`AI_ASK`/`AI_LEARN`/`AI_SAVE`/`AI_STATUS` 노드를 전부 무시합니다.

```blusk
import ai;
gg bot = new AI();
bot.load("model.gguf");        // 🔧 파싱은 되지만 아무 동작 없음 (조용히 무시됨)
gg r = bot.ask("질문", 0.7, 3); // 🔧 r은 항상 nil
```

- `brain.think(...)`는 `ask`와 동의어로 파싱은 되지만(파서 레벨 버그는 아님) 위와 동일하게 실행되지 않습니다.
- 자체 AI 엔진 완성 후 복원 예정. `examples/03_ai.blusk`는 지금 실행해도 에러 없이 그냥 빈 결과만 나오는 상태이니 참고만 하세요.

---

## 15. 시간 관련

```blusk
task.sleep(1000);   // ✅ 1000ms 대기
```

---

## 16. 패키지 선언

```blusk
root package.example.sky;   // ✅
```

---

## 17. import 시스템

```blusk
import blusk26;                    // ✅ 필수
import io;                         // ✅
import Blusk.num.Math;             // ✅
import oop;                        // ✅ (플래그 역할, 실제 클래스 파싱엔 필요 없음)
import ai;                         // 🔧 위 14번 참고
import time;                       // 🔧 모듈 등록만 됨, 호출 문법 없음
import string;                     // 🔧 모듈 등록만 됨, 호출 문법 없음
import collections;                // 🔧 모듈 등록만 됨, 호출 문법 없음

import Blusk.org.blusk.26.*;       // ✅ → blusk26으로 정규화
import Blusk.org.others.Devices.*; // ❌ 외부 .sky 파일 — loadSkyFile()이 "not yet implemented" 출력만 함
```

---

## 18. 멀티 @entry

```blusk
@entry
Blusk public void main() { print("기본 진입점"); the end; }

@entry
Blusk public void serverMain() { print("서버 진입점"); the end; }
```
✅ 여러 개 선언 가능. `main.cpp`는 첫 번째로 발견되는 `MAIN_BLOCK`(value=="entry")을 실행합니다.

---

## 19. 실행 명령어

```bash
blusk file.blusk           # ✅ 파일 실행
blusk --repl                # ✅ 대화형 모드
blusk --dis file.blusk       # ✅ 바이트코드 디스어셈블
blusk --debug file.blusk    # ✅ 디버그 모드 (GC/JIT 통계 출력)
blusk --version              # ✅ 버전 정보
blusk --meta                 # ✅ bluskmeta.json 출력
```

---

## 20. 아직 미구현 / 스텁 (검증 완료 목록)

| 기능 | 상태 | 비고 |
|---|---|---|
| 최상위(top-level) 독립 함수 선언/호출 | ❌ | 클래스 메서드로만 가능 (1번 섹션) |
| 소켓 통신 / HTTP 서버 | ❌ | |
| `try / catch / finally` | ❌ | |
| 람다 / 클로저 | ❌ | |
| `parallel for` | ❌ | |
| `simd for` 실질 최적화 | 🔧 | Matrix 리터럴 생성 시 이미 AVX2 적용됨, 이 키워드 자체는 no-op |
| `tensor<N,T>` | 🔧 | 옵코드(`OP_TENSOR_*`)는 있으나 파서/컴파일러 연결 없음 |
| `@native` 실제 LLVM 컴파일 | 🔧 | `LLVM_ENABLED` 빌드 옵션 있으나 IR emit은 빈 함수 |
| `.sky` 사용자 정의 모듈 | ❌ | |
| `forkfrom` 다중 상속 | ❌ | 단일 부모만 |
| `interface` / `abstract` | ❌ | |
| 제네릭 (`<T>`) | ❌ | |
| 범위 연산자 `a..b` | ❌ | 토큰(`..`)은 렉서에 있으나 파서 미사용 |
| 파이프 연산자 `\|>` | ❌ | |
| Math.pow / Math.I | 🔧 | 12번 섹션 참고 |
| stdlib string/collections/time/io(read_file 등) | 🔧 | 모듈 등록만 됨, 호출 문법 없음 |
| Matrix 실제 행렬곱(matmul) | 🔧 | `OP_MATRIX_MUL` VM에 구현되어 있으나 컴파일러가 절대 emit 안 함 — `Matrix a * b` 리터럴은 실제로는 원소별(Hadamard) 곱임 |
| AI 내장 | 🔧(보류) | 14번 섹션 참고 |

---

## 21. Checker 동작

```
[컴파일 전 자동 실행]
- 미사용 변수 → 경고 + 컴파일 제외
- 상수 재대입 → 에러
- @Newmemorycancel 영역 쓰기 → 에러
- 수명 확정 변수 → RC 생략 마킹
- narrowing(타입 폭 축소) 가능성 → 내부 판단 로직 존재
- 순환 참조 가능성 → 경고 (Cycle GC가 런타임 처리)
```

---

## 22. 파일 확장자

| 확장자 | 용도 |
|---|---|
| `.blusk` | BLUSK 소스 파일 |
| `.sky` | 사용자 정의 헤더/모듈 파일 (미구현) |
| `bluskmeta.json` | 프로젝트 메타데이터 |

---

## 개정 이력

- **이번 개정**: 실제 소스(parser.cpp/lexer.cpp/compiler.cpp/vm.cpp/module.cpp 등) 전수 검토 후 재작성. 이전 버전에 "미구현"으로 적혀있던 논리연산자(`&&`/`||`/`!`), `as` 캐스팅, `switch/case`, `Matrix`, 중첩 루프 대응 `break`/`continue`가 실제로는 이미 구현되어 있음을 확인하여 반영. `patch_notes.txt`의 4개 패치 항목(대문자 `@Entry`, `public` 선택화, `brain.think()`, 구버전 `loop` 문법)도 이미 전부 적용된 상태로 확인됨. 반대로 `@native` 함수 호출 불가, AI 보류, Matrix 실제 행렬곱 미연결, stdlib 대부분 미연결 등 실제로 비어있는 부분을 새로 문서화함.

- **실제 빌드+실행 테스트 세션 (이번 개정)**: 정적 분석만으로는 드러나지 않던 심각한 런타임 버그 9개를 실제로 빌드해서 찾아 내고 수정함:
  1. **`Lexer::identifier()`가 `.`을 과도하게 병합** — "Blusk.num.Math" 같은 import 경로를 한 토큰으로 만들려던 로직이었지만, 그 부작용으로 `Math.sqrt(...)`, `dog.speak()`, `task.sleep(...)`, `io.read(...)` 등 **언어 전체의 dot 문법이 전부 깨져있었음** (조용히 어떤 오류도 없이 무시됨). 병합 로직 제거로 해결.
  2. `io.read(...)` 파서의 토큰 소비 위치가 엇갈려서 `read` 토큰이 미소비되고 뒤 토큰들이 다 밀리는 버그 수정 (1번과 맞물려 발견).
  3. `for`/`for-in`/`loop(count)` 안에서 `break`/`continue`가 **무한루프를 일으키는** 논리 오류 수정 — `continueTarget`을 본문 컴파일 전에 미리 잡아둔던 것이 원인. `break`처럼 지연 패치(deferred patch) 방식으로 재설계.
  4. Checker가 `print("%d", i)`, `dog.speak()`, C스타일 `for`의 조건절(`i <= n`), `the end : if/return` 안의 변수를 각각 다른 이유로 dead로 오판 — 모두 해당 변수명이 `VAR_REF` 노드가 아니라 전용 노드의 `->value`에 직접 박혀있어 생기는 문제였고, 각각 추적 로직 추가로 해결.
  5. `SVM::runBlock`(OOP 메서드 본문 인터프리터)이 `PRINT_FSTR`(f-string print)을 아예 처리하지 않아 메서드 안의 f-string print가 에러 없이 조용히 아무것도 안 하던 버그 수정.
  6. REPL이 각 줄을 독립된 전체 프로그램으로 감싸서 컴파일하는 구조라, 한 줄에서 선언한 변수가 다음 줄에서 쓰일 때마다 dead로 오판되던 버그 수정 (REPL은 dead-code 제거를 비활성화).
  - 추가로, import된 내장 네임스페이스(`Math`/`io`/`task`/`string`/`time`/`collections`)와 같은 이름으로 변수를 선언하면 Checker가 상당 경고를 띄우는 기능을 새로 추가함.
  - 미해결: 객체 종료 시 GC가 refcount를 줄이지 않아 종료 시 "still live" 경고가 뜨는 문제(기능적 문제는 아님), `Math.pow`/`Math.I` 미구현(문서화됨), float 출력 시 소수점 표기가 생략되는 사소한 포맷팅 문제.

- **RC(참조카운팅) 실제 구현 (추가 개정)**: 이전까지는 `OP_NEW`와 `OP_STORE`가 각각 같은 객체를 "새로 등록"해버려서 refCount가 항상 1로 리셋되고, 별칭(aliasing)이나 재대입 시 증감이 전혀 일어나지 않았음(=RC가 이름만 있고 실질적으로 동작 안 함). 이번에 `OP_NEW`는 추적을 미루고 최초 `OP_STORE`가 "이미 추적 중인지"(`isTracked`)를 확인해서 처음이면 추적(refCount=1), 이미 추적 중이면 증가(incRef), 기존 바인딩을 덮어씁으면 감소(decRef)하도록 재설계함. 실제로 별칭 시 refCount가 2로 증가하고, 두 변수가 모두 다른 객체로 재대입되면 원래 객체가 refCount 0에 도달해 실제로 해제되는 것까지 검증됨 (`--debug` 모드의 GC Stats로 확인 가능). 함께, `a = new Dog(...)` 같은 **재대입 문에서 `new`가 전혀 파싱되지 않던** 별개 버그도 함께 발견되어 수정됨 (`new`는 `gg x = new X()` 선언에서만 동작하던 것을 일반 표현식 위치에서도 동작하도록 확장). 순환 참조(cycle) 거대 객체 간 통합 테스트는 아직 미진행(`obj.field = value;` 필드 대입 문법이 파서에 없음).
