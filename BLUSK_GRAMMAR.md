# BLUSK 문법 레퍼런스 (0.7 alpha)

---

## 1. 기본 구조

```blusk
root package.패키지명;     // 선택적 패키지 선언

import blusk26;            // 필수 (항상 첫 번째)
import io;
import Blusk.num.Math;

@entry
Blusk public void main() {
    // 코드
    the end;
}
```

---

## 2. 변수 선언

| 키워드 | 의미 | 예시 |
|---|---|---|
| `gg` | 타입 추론 변수 | `gg x = 10;` |
| `num` | 상수 (재대입 불가) | `num MAX = 100;` |
| `int` | 정수 (gg와 동일, 명시적) | `int count = 0;` |
| `str` | 문자열 (gg와 동일, 명시적) | `str name = "BLUSK";` |
| `float` | 실수 (gg와 동일, 명시적) | `float pi = 3.14;` |

```blusk
gg x = 10;
gg y = 3.14;
gg name = "hello";
num MAX = 999;        // 상수, 이후 MAX = 1; 하면 Checker 에러

int a = 5;            // 구버전 호환
str msg = "world";    // 구버전 호환
```

---

## 3. 출력

```blusk
import io;

print("hello");               // 문자열 그대로
print("%d", x);               // 정수 포맷
print("%s", name);            // 문자열 포맷
print(f"{x} + {y} = {z}");    // f-string (Python 스타일)
```

---

## 4. 입력

```blusk
import io;

io.read("입력하세요: ", com name);   // name 변수에 저장
```

---

## 5. 산술 연산자

| 연산자 | 의미 | 예시 |
|---|---|---|
| `+` | 더하기 / 문자열 연결 | `x + y` |
| `-` | 빼기 | `x - y` |
| `*` | 곱하기 | `x * y` |
| `/` | 나누기 | `x / y` |
| `%` | 나머지 | `x % y` |
| `**` | 제곱 | `2 ** 8` |
| `-x` | 단항 음수 | `-x` |

---

## 6. 비교 연산자 (조건식에서 사용)

| 연산자 | 의미 |
|---|---|
| `==` | 같음 |
| `!=` | 다름 |
| `<` | 미만 |
| `<=` | 이하 |
| `>` | 초과 |
| `>=` | 이상 |

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

`else if` (공백 포함)도 동일하게 작동.

---

## 8. 반복문

### for
```blusk
for (gg i = 0; i < 10; i++) {
    print("%d", i);
};
```
`i++` / `i--` / `i = i + 1` 모두 지원.

### while
```blusk
while (x < 100) {
    x = x + 1;
};
```

### loop (시간 기반)
```blusk
// 신버전
loop(task.time = 5, task.interval = 1) {
    print("1초마다 실행, 5초 후 종료");
};

// 구버전 호환
loop(task.time, 5, 1) {
    print("동일");
};
```

### break / continue
```blusk
for (gg i = 0; i < 10; i++) {
    if (i == 5) { break; };
    if (i == 3) { continue; };
    print("%d", i);
};
```

---

## 9. 종료 명령

```blusk
the end;                    // 프로그램 즉시 종료 (return 0 대체)
the end : if (x < 0);      // 조건부 종료
the end : return x;        // 값 반환 후 종료
```

---

## 10. 배열

```blusk
gg[] nums = {1, 2, 3, 4, 5};

print("%d", nums[0]);       // 읽기
nums[2] = 99;               // 쓰기
print("%d", size.nums);     // 길이
```

---

## 11. OOP

### 클래스 선언
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
```

### 상속 (forkfrom)
```blusk
Blusk public class Dog : forkfrom Animal {
    gg breed;

    Dog(gg n, gg b) {
        name  = n;
        sound = "Woof";
        breed = b;
    }
}
```

### 인스턴스 생성 및 호출
```blusk
gg dog = new Dog("Rex", "Labrador");
dog.speak();
```

---

## 12. Math 라이브러리

```blusk
import Blusk.num.Math;

gg a = Math.sqrt(16);       // 4.0
gg b = Math.abs(-5);        // 5
gg c = Math.pow(2, 10);     // 1024 (향후)
gg d = Math.sin(0);         // 0.0
gg e = Math.cos(0);         // 1.0
gg f = Math.log(Math.E);    // 1.0
gg g = Math.PI;             // 3.14159...
gg h = Math.E;              // 2.71828...
gg i = Math.I;              // "i" (허수단위)
gg j = Math.floor(3.7);     // 3
gg k = Math.ceil(3.2);      // 4
gg l = Math.round(3.5);     // 4
```

---

## 13. 어노테이션

| 어노테이션 | 의미 | 상태 |
|---|---|---|
| `@entry` | 엔트리포인트 (멀티 main 지원) | ✅ 작동 |
| `@Entry` | 대소문자 무관 | ✅ 작동 |
| `@native` | 해당 함수 네이티브 컴파일 | 🔧 stub |
| `@vm` | 해당 함수 VM으로 강제 실행 | 🔧 stub |
| `@unsafe` | 저수준 레지스터/GPU 접근 허용 | 🔧 stub |
| `@Newmemorycancel` | 메모리 보호 구역 선언 | ✅ Checker 처리 |
| `@noalias` | 포인터 앨리어싱 없음 선언 | 🔧 stub |

```blusk
@entry
Blusk public void main() { ... }

@native
Blusk public void fastCalc(gg a, gg b) {
    the end : return a + b;
}

@Newmemorycancel
gg hardwareBuffer = 0;
```

---

## 14. Saturday AI (내장)

```blusk
import ai;

// 모델 로드
modelName.load("model.gguf");          // 로컬 llama.cpp
// modelName.load("api:openai:gpt-4o");   // OpenAI API
// modelName.load("api:anthropic:claude-sonnet-4-20250514"); // Anthropic

// 질문
modelName.ask("질문", 0.7, 3);         // (prompt, temperature, sentences)
modelName.think("질문", 0.7, 3);       // ask 동의어

// 학습 / 저장
modelName.learn("data.txt");
modelName.save("model.dat");
modelName.status();
```

모델 이름은 사용자가 자유롭게 지정:
```blusk
brain.load("brain.gguf");
saturday.load("saturday.model");
pipi.load("pipi.dat");
```

---

## 15. 시간 관련

```blusk
task.sleep(1000);   // 1000ms 대기
```

---

## 16. 패키지 선언

```blusk
root package.example.sky;   // 이 파일의 패키지
```

---

## 17. import 시스템

```blusk
import blusk26;                    // 필수 기본 라이브러리
import io;                         // 입출력
import Blusk.num.Math;             // 수학 함수
import oop;                        // OOP 지원
import ai;                         // Saturday AI
import time;                       // 시간 함수 (향후)
import string;                     // 문자열 함수 (향후)
import collections;                // 컬렉션 함수 (향후)

// 자바 스타일 패키지 경로도 지원 (정규화됨)
import Blusk.org.blusk.26.*;       // → blusk26
import Blusk.org.others.Devices.*; // → 외부 .sky 파일 (향후)
```

---

## 18. 멀티 @entry (멀티 메인 클래스)

```blusk
@entry
Blusk public void main() {
    print("기본 진입점");
    the end;
}

@entry
Blusk public void serverMain() {
    print("서버 진입점");
    the end;
}
```

`blusk.exe file.blusk` → 첫 번째 `@entry` 실행.

---

## 19. 실행 명령어

```bash
blusk file.blusk           # 파일 실행
blusk --repl               # 대화형 모드
blusk --dis file.blusk     # 바이트코드 디스어셈블
blusk --debug file.blusk   # 디버그 모드 (GC/JIT 통계 출력)
blusk --version            # 버전 정보
blusk --meta               # bluskmeta.json 출력
```

---

## 20. 아직 미구현 / 추가 예정 문법

| 기능 | 상태 | 우선순위 |
|---|---|---|
| 소켓 통신 / HTTP 서버 생성 | ❌ 미구현 | 1.0 |
| 논리 연산자 `&&` `\|\|` `!` | ❌ 미구현 | 높음 |
| `as` 타입 캐스팅 (`i as double`) | ❌ 미구현 | 중간 |
| 문자열 내장 메서드 (`str.len()`) | ❌ 미구현 | 중간 |
| `try / catch / finally` 예외처리 | ❌ 미구현 | 중간 |
| 람다 / 클로저 | ❌ 미구현 | 낮음 |
| `parallel for` 멀티스레드 루프 | ❌ 미구현 | 1.0 |
| `simd for` SIMD 자동 벡터화 | 🔧 구조만 | 진행중 |
| `tensor<N, T>` 고정 크기 SIMD 배열 | 🔧 구조만 | 진행중 |
| `@native` 실제 LLVM 컴파일 | 🔧 stub | 1.0 |
| `@unsafe` 저수준 접근 | 🔧 stub | 1.0 |
| `.sky` 사용자 정의 모듈 | 🔧 stub | 1.0 |
| `forkfrom` 다중 상속 | ❌ 미구현 | 낮음 |
| `interface` / `abstract` | ❌ 미구현 | 낮음 |
| 제네릭 (`<T>`) | ❌ 미구현 | 낮음 |
| 범위 연산자 `a..b` | ❌ 미구현 | 낮음 |
| 파이프 연산자 `\|>` | ❌ 미구현 | 검토 중 |

---

## 21. Checker 동작

```
[컴파일 전 자동 실행]
- 미사용 변수 → 경고 + 컴파일 제외
- 상수 재대입 → 에러
- @Newmemorycancel 영역 쓰기 → 에러
- 수명 확정 변수 → RC 생략 마킹 (성능 향상)
- 순환 참조 가능성 → 경고 (Cycle GC가 런타임 처리)

[단축키]
Alt + Shift + F12  → 수동 Checker 실행 (IDE 플러그인 연동 예정)
1분마다 자동 실행  → (IDE 플러그인 연동 예정)
```

---

## 22. 파일 확장자

| 확장자 | 용도 |
|---|---|
| `.blusk` | BLUSK 소스 파일 |
| `.sky` | 사용자 정의 헤더/모듈 파일 (향후) |
| `bluskmeta.json` | 프로젝트 메타데이터 |

---

## 알려진 버그 (0.7 alpha)

1. **`@Entry` 대문자** → parser.cpp `parseAnnotation`에서 소문자 정규화 필요
2. **`Blusk void main()` (public 없음)** → `public` 선택적으로 수정 필요
3. **`brain.think()`** → AI 메서드에 `think` 추가 필요
4. **논리 연산자 없음** → `&&`, `||`, `!` 미구현
5. **조건식이 단순 비교만 가능** → 복합 조건 `(a > 0 && b < 10)` 미지원
