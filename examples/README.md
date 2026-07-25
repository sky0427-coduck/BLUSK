# BLUSK Examples

번호 순서대로 보면 BLUSK 문법을 단계적으로 익힐 수 있습니다.

| 파일 | 내용 |
|---|---|
| `01_hello.blusk` | 기본 출력, 조건문, for/while |
| `02_variables.blusk` | 변수, 상수(num), `as` 캐스팅, 논리 연산자 `&&`/`\|\|`/`!` |
| `03_control_flow.blusk` | switch-case, for-in(범위 기반), the end 조건부 종료 |
| `04_oop.blusk` | 클래스, forkfrom 상속 |
| `05_math.blusk` | Math 내장 라이브러리 |
| `06_matrix_simd.blusk` | Matrix 타입 + `@simd for` (AVX2 가속) |
| `07_arrays.blusk` | 배열 선언/인덱싱/순회 |
| `08_annotations.blusk` | `@native`, `@unsafe`, `@Newmemorycancel` |
| `09_checker_test.blusk` | BLUSK Checker 동작 확인 (RC-skip, dead code, 상수 보호) |

## 실행

```bash
blusk examples/01_hello.blusk
blusk --debug examples/09_checker_test.blusk   # Checker/GC 통계 포함
blusk --dis examples/06_matrix_simd.blusk      # 바이트코드 확인
```

## 참고

- AI(Saturday) 관련 예제는 사용자가 직접 AI 엔진을 만들기로 하여 **보류** 상태입니다.
- `@native`, `@unsafe`는 현재 어노테이션 마킹만 되어 있고 실제 LLVM 컴파일은 1.0 목표입니다.
