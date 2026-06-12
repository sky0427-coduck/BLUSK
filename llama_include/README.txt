[ include 폴더 세팅 방법 ]

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
1. httplib.h → 이미 완료! ✅
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
2. llama.cpp 헤더 복사 (PowerShell에서 실행)
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

$src  = "C:\Users\Skyhanry\Desktop\hanry\programming\BLUSK\llama.cpp"
$dest = "C:\Users\Skyhanry\Desktop\hanry\programming\BLUSK\BLUSK\include"

# llama 헤더
Copy-Item "$src\include\llama.h"      $dest
Copy-Item "$src\include\llama-cpp.h"  $dest

# ggml 헤더 (llama.h가 참조함)
Copy-Item "$src\ggml\include\ggml.h"          $dest
Copy-Item "$src\ggml\include\ggml-cpu.h"      $dest
Copy-Item "$src\ggml\include\ggml-backend.h"  $dest
Copy-Item "$src\ggml\include\ggml-opt.h"      $dest
Copy-Item "$src\ggml\include\gguf.h"          $dest
Copy-Item "$src\ggml\include\ggml-alloc.h"    $dest

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
3. Visual Studio 프로젝트 설정
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

[추가 포함 디렉터리]
  프로젝트 우클릭 → 속성 → C/C++ → 추가 포함 디렉터리
  → $(ProjectDir)include  추가

[추가 라이브러리 디렉터리]
  링커 → 일반 → 추가 라이브러리 디렉터리
  → C:\Users\Skyhanry\Desktop\hanry\programming\BLUSK\llama.cpp\build\bin  추가

[추가 종속성 - 링커 → 입력 → 추가 종속성]
  llama.dll.a
  Ws2_32.lib

[전처리기 정의 - C/C++ → 전처리기]
  SATURDAY_LLAMA     ← 로컬 llama 활성화

━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
4. 실행할 때 llama.dll 필요
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━

빌드 후 o2vm.exe 옆에 llama.dll 복사:
  Copy-Item "$src\build\bin\llama.dll" "C:\Users\Skyhanry\Desktop\hanry\programming\BLUSK\BLUSK\"
