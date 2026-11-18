# BUILD_LOG

빌드 명령과 오류를 실습 순서대로 기록한다. (작성일: 2026-11-13)

---

## [실습 0] 환경 확인
- 명령: `lsb_release -ds; g++ --version; cmake --version; echo "$XDG_SESSION_TYPE / DISPLAY=$DISPLAY"`
- 결과: 성공
  - OS: Ubuntu 24.04.5 LTS (x86_64)
  - g++: 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1)
  - CMake: 3.28.3 / GNU Make 4.3 / git 2.43.0 / pkg-config 1.8.1
  - 세션: `wayland / DISPLAY=:0` (SFML은 X11 백엔드 → XWayland로 동작)
- 단계 구분: 해당 없음 (사전 점검)
- 원인 / 해결: 요구사항(g++ 13, CMake 3.28 이상) 충족.

## [실습 0] 기존 SFML 2.x 설치 여부
- 명령: `dpkg -l | grep -i sfml; ls /usr/include/SFML`
- 결과: 둘 다 출력 없음 → apt의 SFML 2.6.1은 설치되어 있지 않다.
- 단계 구분: 해당 없음
- 원인 / 해결: 2.x가 없으므로 실습 1-2(`sudo apt remove libsfml-dev`)는 **생략**했다.

---

## [실습 1] 1-1. 의존 패키지 확인
- 명령: `dpkg -s <패키지>` (지시서 1-1의 13개 패키지)
- 결과: 12개 설치됨, `libegl1-mesa-dev` 1개만 미설치.
- 단계 구분: 해당 없음
- 원인 / 해결: `libegl1-mesa-dev`를 **설치하지 않고 진행**했고, CMake 설정과 빌드가 그대로 성공했다.
  즉 이 구성(X11 + 데스크톱 OpenGL, GLX 사용)에서는 EGL 개발 헤더가 필요하지 않았다.
  지시서 목록과 다른 유일한 점이다. 필요해지면 `sudo apt install -y libegl1-mesa-dev`.

## [실습 1] 1-3. SFML 3.x 소스 받기
- 명령:
  ```bash
  git ls-remote --tags https://github.com/SFML/SFML.git | grep -E 'refs/tags/3\.[0-9]+\.[0-9]+$' | sort -V -k2 | tail -3
  cd ~/src && git clone --branch 3.1.0 --depth 1 https://github.com/SFML/SFML.git
  ```
- 결과: 성공. 3.x 태그는 3.0.1 / 3.0.2 / **3.1.0**(최신). `git describe --tags` → `3.1.0`
- 단계 구분: 해당 없음 (소스 준비)
- 원인 / 해결: `--depth 1`은 이력 없이 해당 태그 스냅숏만 받아 다운로드를 줄인다.

## [실습 1] 1-4. CMake 설정 (configure)
- 명령:
  ```bash
  cd ~/src
  cmake -S SFML -B SFML/build \
      -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=ON \
      -DSFML_USE_SYSTEM_DEPS=OFF \
      -DSFML_BUILD_AUDIO=OFF \
      -DSFML_BUILD_NETWORK=OFF \
      -DSFML_INSTALL_PKGCONFIG_FILES=ON \
      -DCMAKE_INSTALL_PREFIX=/usr/local
  ```
- 결과: 성공 (`Configuring done (37.3s)`, 경고/오류 없음)
- 단계 구분: 빌드 설정 단계 (컴파일 이전)
- 원인 / 해결: 37초의 대부분은 `SFML_USE_SYSTEM_DEPS=OFF` 때문에 의존성 소스를 GitHub에서 받는 시간이다.

  | 옵션 | 의미 |
  |---|---|
  | `CMAKE_BUILD_TYPE=Release` | 최적화 빌드. Debug면 라이브러리 이름에 `-d`가 붙어 `-lsfml-graphics`로 못 찾는다 |
  | `BUILD_SHARED_LIBS=ON` | SFML 3은 기본이 정적(.a)이다. `-lsfml-graphics`로 동적 링크하려고 공유 라이브러리(.so)로 빌드한다 |
  | `SFML_USE_SYSTEM_DEPS=OFF` | freetype, HarfBuzz 등 그래픽 의존성을 시스템 패키지 대신 CMake가 직접 받아 빌드한다 |
  | `SFML_BUILD_AUDIO/NETWORK=OFF` | graphics/window/system만 쓰므로 불필요한 의존성과 빌드 시간을 줄인다 |
  | `SFML_INSTALL_PKGCONFIG_FILES=ON` | `pkg-config`로 플래그를 조회할 수 있게 `.pc` 파일을 설치한다 |
  | `CMAKE_INSTALL_PREFIX=/usr/local` | g++/ld의 기본 검색 경로라 `-I`, `-L` 없이 찾을 수 있다 |

## [실습 1] 1-4. SFML 빌드
- 명령: `cmake --build SFML/build -j"$(nproc)"` (nproc = 24)
- 결과: 성공 (`[100%] Built target sfml-graphics`, 경고 0건)
  - 생성물: `libsfml-system.so.3.1.0`, `libsfml-window.so.3.1.0`, `libsfml-graphics.so.3.1.0`
- 단계 구분: 컴파일 단계 + 링크 단계 (SFML 자체의 빌드)
- 원인 / 해결: 해당 없음.

## [실습 1] 1-4. 설치 및 링커 캐시 갱신 (관리자 권한)
- 명령:
  ```bash
  sudo cmake --install SFML/build
  sudo ldconfig
  ```
  실제로는 같은 두 명령을 `pkexec`로 한 번에 실행하고, 화면에 뜬 인증 창에 비밀번호를 입력했다.
  `pkexec /bin/sh -c '/usr/bin/cmake --install ~/src/SFML/build && /sbin/ldconfig'`
- 결과: 성공 (exit=0)
- 단계 구분: 설치 단계
- 원인 / 해결: `/usr/local`은 root 소유라 일반 사용자는 쓸 수 없다 → 관리자 권한 필요.
  `ldconfig`는 새 `.so`를 런타임 링커 캐시(`/etc/ld.so.cache`)에 등록한다. 빼먹으면 실행 때
  `error while loading shared libraries: libsfml-graphics.so.3.1`이 난다(런타임 단계 오류).

## [실습 1] 1-5. 설치 확인
- 명령:
  ```bash
  ls /usr/local/include/SFML/Graphics.hpp
  ls /usr/local/lib | grep -i sfml
  grep -E 'define SFML_VERSION_(MAJOR|MINOR|PATCH)' /usr/local/include/SFML/Config.hpp
  pkg-config --modversion sfml-graphics
  pkg-config --cflags --libs sfml-graphics
  ls /usr/include/SFML 2>/dev/null || echo "OK: SFML 2.x 헤더 없음"
  ldconfig -p | grep -i sfml
  ```
- 결과: 성공
  - 헤더: `/usr/local/include/SFML/Graphics.hpp` 존재, 버전 매크로 `3 / 1 / 0`
  - 라이브러리: `libsfml-{graphics,window,system}.so` → `.so.3.1` → `.so.3.1.0`
  - `pkg-config --modversion sfml-graphics` → `3.1.0`
  - `pkg-config --cflags --libs sfml-graphics` →
    `-I/usr/local/include -I/usr/include/freetype2 -I/usr/include/libpng16 -L/usr/local/lib -lsfml-graphics -lsfml-window -lsfml-system`
  - `OK: SFML 2.x 헤더 없음`
  - `ldconfig -p`에 세 라이브러리가 모두 `/usr/local/lib/...`로 등록됨
- 단계 구분: 해당 없음 (확인)
- 원인 / 해결: 해당 없음.

## [실습 1] 1-7. hello_sfml 빌드
- 명령:
  ```bash
  g++ -std=c++17 -Wall -Wextra hello_sfml.cpp -o hello_sfml \
      -lsfml-graphics -lsfml-window -lsfml-system
  ```
- 결과: 성공, 경고 0건. `-I`, `-L` 없이 헤더와 라이브러리를 모두 찾았다.
- 단계 구분: 컴파일 단계 + 링크 단계 모두 통과
- 원인 / 해결: `/usr/local/include`, `/usr/local/lib`가 g++/ld의 기본 검색 경로이기 때문이다.
  대안 명령 `g++ -std=c++17 -Wall -Wextra hello_sfml.cpp -o hello_sfml $(pkg-config --cflags --libs sfml-graphics)`도 성공했다.

## [실습 1] 1-7. 링크 경로 확인 및 실행
- 명령: `ldd hello_sfml | grep -i sfml` / `timeout 5 ./hello_sfml; echo "exit=$?"`
- 결과: 성공
  ```
  libsfml-graphics.so.3.1 => /usr/local/lib/libsfml-graphics.so.3.1
  libsfml-window.so.3.1 => /usr/local/lib/libsfml-window.so.3.1
  libsfml-system.so.3.1 => /usr/local/lib/libsfml-system.so.3.1
  exit=124
  ```
- 단계 구분: 실행(런타임) 단계
- 원인 / 해결: 종료 코드 124는 `timeout`이 5초 뒤에 강제 종료했다는 뜻 = 크래시 없이 창이 떠 있었다.
  화면 내용(하늘색 원 회전, Esc 종료)은 아래 '화면 확인'에서 직접 실행해 확인했다.

## [실습 1] 1-8-① 일부러 낸 오류: `-lsfml-*` 제거
- 명령: `g++ -std=c++17 -Wall -Wextra hello_sfml.cpp -o hello_sfml`
- 결과: 오류 (`undefined reference` 28건)
  ```
  /usr/bin/ld: /tmp/ccLOo1R9.o: in function `main':
  hello_sfml.cpp:(.text+0x8d): undefined reference to `sf::String::String(char const*, std::locale const&)'
  /usr/bin/ld: hello_sfml.cpp:(.text+0xc4): undefined reference to `sf::VideoMode::VideoMode(sf::Vector2<unsigned int>, unsigned int)'
  ...
  collect2: error: ld returned 1 exit status
  ```
- 단계 구분: **링크 단계** (메시지 앞머리가 `/usr/bin/ld:`이고 `collect2: ... ld returned 1`로 끝난다)
- 원인 / 해결: 헤더에는 선언만 있어서 컴파일은 통과하고 `/tmp/cc….o`까지 만들어졌다.
  함수 본체는 `libsfml-*.so` 안에 있는데 링커에게 알려 주지 않아 못 찾았다. → `-lsfml-graphics -lsfml-window -lsfml-system` 추가.

## [실습 1] 1-8-② 일부러 낸 오류: 라이브러리를 소스 앞에
- 명령: `g++ -lsfml-graphics hello_sfml.cpp -o hello_sfml`
  (세 개를 모두 앞에 둔 `g++ -std=c++17 -Wall -Wextra -lsfml-graphics -lsfml-window -lsfml-system hello_sfml.cpp -o hello_sfml`도 같은 결과)
- 결과: 오류 (`undefined reference` 28건, ①과 같은 메시지)
  ```
  hello_sfml.cpp:(.text+0x8d): undefined reference to `sf::String::String(char const*, std::locale const&)'
  collect2: error: ld returned 1 exit status
  ```
- 단계 구분: **링크 단계**
- 원인 / 해결: 링커는 명령줄을 **왼쪽에서 오른쪽으로 한 번만** 훑는다. `-lsfml-graphics`를 만난 시점에는
  아직 `hello_sfml.o`를 읽지 않아 "미해결 심볼"이 하나도 없다. Ubuntu의 gcc는 링커에 `--as-needed`를
  기본으로 넘기므로(`gcc -dumpspecs`에서 확인), 그 시점에 쓰이지 않는 `.so`는 **필요 없는 라이브러리로 버려진다.**
  그 뒤에 `hello_sfml.o`가 들어와 `sf::...` 심볼을 요구하지만 링커는 이미 지나온 라이브러리로 되돌아가지 않는다.
  → 라이브러리는 그것을 쓰는 소스/오브젝트 **뒤에** 둔다.

## [실습 1] 1-8-③ 일부러 낸 오류: SFML 2.x 문법
- 명령: `sf::VideoMode({800u, 600u})`를 `sf::VideoMode(800, 600)`으로 바꾼 **사본**을 빌드
  (`g++ -std=c++17 -Wall -Wextra hello_v2.cpp -o err3 -lsfml-graphics -lsfml-window -lsfml-system`)
- 결과: 오류
  ```
  hello_v2.cpp:10:51: error: no matching function for call to ‘sf::VideoMode::VideoMode(int, int)’
  ```
- 단계 구분: **컴파일 단계** (`파일:줄:칸: error:` 형식이고 `ld`가 등장하지 않는다. `.o`가 만들어지지 않았다)
- 원인 / 해결: SFML 3의 생성자는 `explicit VideoMode(Vector2u modeSize, unsigned int modeBitsPerPixel = 32)`
  (`/usr/local/include/SFML/Window/VideoMode.hpp:61`)라 `(int, int)` 두 인자를 받는 형태가 없다.
  → `sf::VideoMode({800u, 600u})`. 원본 `hello_sfml.cpp`는 건드리지 않고 임시 사본으로 실험했으므로 원복할 것이 없다.

## [실습 1] 오류 단계 구분 요약
| 오류 | 단계 | 구분 근거 |
|---|---|---|
| ① `-lsfml-*` 제거 | 링크 | `/usr/bin/ld: ... undefined reference`, `collect2: error: ld returned 1` |
| ② 라이브러리를 소스 앞에 | 링크 | ①과 동일한 메시지. 라이브러리는 있지만 순서 때문에 쓰이지 않음 |
| ③ `sf::VideoMode(800, 600)` | 컴파일 | `파일:줄:칸: error: no matching function`, 링커가 실행되지도 않음 |

## [실습 1] 화면 확인
- 명령: `./hello_sfml`
- 결과: 성공. 하늘색 원이 회전하는 창이 뜨고, Esc로 종료되는 것을 확인했다.
- 단계 구분: 실행(런타임) 단계
- 원인 / 해결: 해당 없음. → 실습 1 체크포인트 전부 완료.

---

## [실습 2] 2-2. 1단계: 목적 파일 생성 (컴파일만)
- 명령: `g++ -std=c++17 -Wall -Wextra -c MathUtils.cpp -o MathUtils.o` / `file MathUtils.o`
- 결과: 성공, 경고 0건
  ```
  MathUtils.o: ELF 64-bit LSB relocatable, x86-64, version 1 (SYSV), not stripped
  ```
- 단계 구분: 컴파일 단계만 (`-c` = 링크하지 않음)
- 원인 / 해결: `relocatable`은 아직 메모리 주소가 확정되지 않은 "조각"이라는 뜻이다.
  `main`이 없어도 오류가 나지 않는 이유는 `-c`가 링크 단계를 건너뛰기 때문이다.

## [실습 2] 2-2. 2단계: 정적 라이브러리(아카이브)로 묶기
- 명령: `ar rcs libmathutils.a MathUtils.o` / `ar t libmathutils.a` / `nm -C libmathutils.a | grep -i radian`
- 결과: 성공
  ```
  MathUtils.o
  0000000000000000 T mathutils::degreeToRadian(double)
  000000000000002c T mathutils::radianToDegree(double)
  ```
- 단계 구분: 아카이브 생성 (컴파일도 링크도 아님)
- 원인 / 해결:
  - `ar` 옵션: `r` = 파일을 넣거나 교체, `c` = 아카이브가 없으면 경고 없이 생성, `s` = 심볼 색인 생성(링커가 빨리 찾게)
  - `.a`는 `.o`들을 묶은 상자일 뿐이다. `ar t`가 원래 `.o` 이름을 그대로 보여 준다.
  - `nm`의 `T` = 이 파일의 코드(Text) 영역에 **정의된** 심볼. `-C`는 C++ 이름 맹글링을 사람이 읽는 형태로 되돌린다.

## [실습 2] 2-4. 테스트 빌드 및 실행
- 명령: `g++ -std=c++17 -Wall -Wextra test_math.cpp -I. -L. -lmathutils -o test_math` / `./test_math`
- 결과: 성공, 경고 0건, `exit=0`
  ```
  [PASS] degreeToRadian(180) ~ PI   ... err=0.00e+00
  [PASS] degreeToRadian(90)  ~ PI/2 ... err=0.00e+00
  [PASS] degreeToRadian(0)  == 0    ... (exact)
  [PASS] radianToDegree(PI)   ~ 180 ... err=0.00e+00
  [PASS] radianToDegree(PI/2) ~ 90  ... err=0.00e+00
  [PASS] radianToDegree(0)    == 0  ... (exact)

        degree      radian        back         error  result
  ----------------------------------------------------------
      0.000000    0.000000    0.000000      0.00e+00      OK
     30.000000    0.523599   30.000000      3.55e-15      OK
     45.000000    0.785398   45.000000      0.00e+00      OK
     90.000000    1.570796   90.000000      0.00e+00      OK
    180.000000    3.141593  180.000000      0.00e+00      OK
    270.000000    4.712389  270.000000      0.00e+00      OK
    360.000000    6.283185  360.000000      0.00e+00      OK
    -45.000000   -0.785398  -45.000000      0.00e+00      OK
    123.456000    2.154714  123.456000      1.42e-14      OK

  ALL TESTS PASSED
  ```
- 단계 구분: 컴파일 + 링크 + 실행 모두 통과
- 원인 / 해결:
  - 왕복 오차 최대 `1.42e-14`(123.456°)로 기준 `1e-9`보다 10만 배 이상 작다.
  - `error` 열만 지수 표기로 출력했다. `std::fixed` + `setprecision(6)`이면 1e-14가 전부 `0.000000`으로
    보여 오차가 실제로 0인지 아주 작은지 구분할 수 없기 때문이다.
  - 30°, 123.456°는 이진 부동소수점으로 정확히 표현되지 않아 아주 작은 오차가 생긴다. 그래서 `==`가 아니라
    `std::fabs(back - deg) <= 1e-9`로 비교한다. 0은 곱셈만 하므로 정확히 0이 나와 `==`로 검사했다.

## [실습 2] 2-5-① 일부러 낸 오류: `-lmathutils`를 소스 앞에
- 명령: `g++ -std=c++17 -Wall -Wextra -I. -L. -lmathutils test_math.cpp -o test_math`
- 결과: 오류
  ```
  /usr/bin/ld: /tmp/ccsgK9fs.o: in function `main':
  test_math.cpp:(.text+0x2f2): undefined reference to `mathutils::degreeToRadian(double)'
  test_math.cpp:(.text+0x3b9): undefined reference to `mathutils::radianToDegree(double)'
  ... (총 8건)
  collect2: error: ld returned 1 exit status
  ```
- 단계 구분: **링크 단계**
- 원인 / 해결: 정적 라이브러리(`.a`)에서 링커는 **그 시점까지 미해결인 심볼을 채워 주는 `.o`만** 꺼낸다.
  `-lmathutils`를 읽을 때는 아직 `test_math.o`를 읽기 전이라 미해결 심볼이 없으므로 아무것도 꺼내지 않고 지나간다.
  이후 `test_math.o`가 `degreeToRadian`을 요구해도 링커는 되돌아가지 않는다.
  (실습 1의 `.so`는 `--as-needed` 때문에 버려졌고, 여기 `.a`는 원래 규칙이 그렇다. 결과는 같다.)
  → `-lmathutils`를 `test_math.cpp` **뒤에** 둔다.

## [실습 2] 2-5-② 일부러 낸 오류: `-L.` 누락
- 명령: `g++ -std=c++17 -Wall -Wextra test_math.cpp -I. -lmathutils -o test_math`
- 결과: 오류
  ```
  /usr/bin/ld: cannot find -lmathutils: 그런 파일이나 디렉터리가 없습니다
  collect2: error: ld returned 1 exit status
  ```
- 단계 구분: **링크 단계**
- 원인 / 해결: `-lmathutils`는 "검색 경로에서 `libmathutils.so` 또는 `libmathutils.a`를 찾아라"는 뜻이다.
  기본 검색 경로(`/usr/lib`, `/usr/local/lib` 등)에는 현재 디렉토리가 없다. → `-L.`로 현재 디렉토리를 추가한다.
  ①과 달리 "심볼을 못 찾음"이 아니라 "라이브러리 파일 자체를 못 찾음"이다.

## [실습 2] 2-5-③ 일부러 낸 오류: `-I.` 누락
- 명령: `g++ -std=c++17 -Wall -Wextra test_math.cpp -L. -lmathutils -o test_math`
- 결과: **성공** (exit=0, 오류 없음)
- 단계 구분: (오류 없음) 컴파일 단계와 관련된 옵션
- 원인 / 해결: `#include "MathUtils.h"`처럼 큰따옴표로 쓰면 컴파일러는 **그 include를 쓴 소스 파일이 있는 디렉토리**부터
  찾는다. `test_math.cpp`와 `MathUtils.h`가 같은 디렉토리에 있으므로 `-I.`가 없어도 찾는다.
  (`<...>` 꺾쇠는 이 단계를 건너뛰고 `-I` 경로와 시스템 경로만 찾는다.)
  - 추가 실험: `test_math.cpp`만 다른 디렉토리(`inc_test/`)로 복사해서 `-I` 없이 컴파일하자
    ```
    inc_test/test_math.cpp:3:10: fatal error: MathUtils.h: 그런 파일이나 디렉터리가 없습니다
    ```
    → **컴파일 단계** 오류. `-I/home/user/Desktop/SFML_LAB`를 주니 성공했다.
    즉 `-I.`는 헤더가 소스와 다른 곳에 있을 때(실무에서 `include/`, `src/`를 나누는 경우) 필요하다.

## [실습 2] 2-5-④ 일부러 낸 오류: `MathUtils.cpp`에 `main()` 추가
- 명령: 임시 사본 디렉토리에서 `MathUtils.cpp` 끝에 `int main() { return 0; }`를 붙여
  ```bash
  g++ -std=c++17 -Wall -Wextra -c MathUtils.cpp -o MathUtils.o
  ar rcs libmathutils.a MathUtils.o
  nm -C libmathutils.a | grep ' T '        # → T main 이 추가로 보임
  g++ -std=c++17 -Wall -Wextra test_math.cpp -I. -L. -lmathutils -o t
  ```
- 결과: `-c` 컴파일과 `ar`은 **성공**, 마지막 링크에서 오류
  ```
  /usr/bin/ld: ./libmathutils.a(MathUtils.o): in function `main':
  MathUtils.cpp:(.text+0x58): multiple definition of `main'; /tmp/ccfMQYMl.o:test_math.cpp:(.text+0x2ad): first defined here
  collect2: error: ld returned 1 exit status
  ```
- 단계 구분: **링크 단계**
- 원인 / 해결: 각 `.cpp`는 따로 컴파일되므로 컴파일러는 `main`이 두 개인지 모른다. 링커가 `degreeToRadian`을 채우려고
  아카이브에서 `MathUtils.o`를 **통째로** 꺼내는 순간 그 안의 `main`도 함께 들어와 `test_math.o`의 `main`과 충돌한다.
  → 라이브러리에는 `main`을 넣지 않는다. 원본 `MathUtils.cpp`는 수정하지 않았으므로 원복할 것이 없다.

## [실습 2] 2-6. (보너스) 공유 라이브러리 버전
- 명령:
  ```bash
  g++ -std=c++17 -Wall -Wextra -fPIC -shared MathUtils.cpp -o libmathutils.so
  g++ -std=c++17 -Wall -Wextra test_math.cpp -I. -L. -lmathutils -o test_math_shared
  ./test_math_shared                       # LD_LIBRARY_PATH 없이
  LD_LIBRARY_PATH=. ./test_math_shared
  ```
  (경고 0건을 유지하려고 지시서 예시에 없던 `-Wall -Wextra`를 추가했다.)
- 결과:
  - 빌드 성공, 경고 0건
  - `LD_LIBRARY_PATH` 없이 실행 → 오류 (exit=127)
    ```
    ./test_math_shared: error while loading shared libraries: libmathutils.so: cannot open shared object file: No such file or directory
    ```
  - `LD_LIBRARY_PATH=.`로 실행 → `ALL TESTS PASSED` (exit=0)
- 단계 구분: 빌드는 성공, 첫 실행은 **실행(런타임) 단계** 오류
- 원인 / 해결:
  - **`.so` 우선 선택 확인**: `.a`와 `.so`가 같은 디렉토리에 있을 때 `-lmathutils`는 `.so`를 골랐다.

    | 비교 | `test_math` (.a 정적) | `test_math_shared` (.so 동적) |
    |---|---|---|
    | `nm -C ... \| grep radian` | `T mathutils::degreeToRadian(double)` (실행 파일 안에 코드 포함) | `U mathutils::degreeToRadian(double)` (실행 때 외부에서 가져옴) |
    | `ldd ... \| grep mathutils` | 출력 없음 | `libmathutils.so => not found` |

  - **런타임 오류 이유**: `-L.`은 **링크할 때만** 쓰이는 검색 경로다. 실행할 때 `.so`를 찾는 것은 동적 로더
    (`ld-linux`)이고, 로더는 `/etc/ld.so.cache`와 시스템 경로만 본다. → `LD_LIBRARY_PATH=.`로 알려 주거나,
    `/usr/local/lib`에 설치 후 `ldconfig`(SFML과 같은 방법), 또는 링크 때 `-Wl,-rpath,'$ORIGIN'`을 넣는다.
  - `-fPIC`(위치 독립 코드): `.so`는 프로세스마다 다른 주소에 올라갈 수 있어 절대 주소 없이 동작하는 코드가 필요하다.
  - 지시대로 확인 후 `libmathutils.so`와 `test_math_shared`를 **삭제했다**. 남겨 두면 실습 4의 `-lmathutils`가
    `.so`를 골라 정적 링크 확인이 틀어지기 때문이다.

## [실습 2] 오류 단계 구분 요약
| 오류 | 단계 | 핵심 메시지 |
|---|---|---|
| ① `-lmathutils`를 소스 앞에 | 링크 | `undefined reference to mathutils::degreeToRadian(double)` |
| ② `-L.` 누락 | 링크 | `cannot find -lmathutils` |
| ③ `-I.` 누락 | (성공) | 같은 디렉토리라 `"..."` include가 찾음. 다른 디렉토리면 컴파일 단계 `fatal error: MathUtils.h` |
| ④ 라이브러리에 `main` | 링크 | `multiple definition of 'main'` |
| ⑤ `.so`를 `LD_LIBRARY_PATH` 없이 실행 | 실행(런타임) | `error while loading shared libraries: libmathutils.so` |

---

## [실습 3] 설계 결정: 지시서와 다른 점
- 명령: 해당 없음 (설계)
- 결과: private 멤버 2개 추가
- 단계 구분: 해당 없음
- 원인 / 해결:
  1. **`bool m_movedThisFrame` (냉각 문제)**
     - 문제: 지시서 수치로는 이동 중 발열 = 180 px/s × 0.015 = **+2.7 ℃/s**, 냉각 = **−6 ℃/s**.
       `update()`가 매 프레임 무조건 냉각하면 60fps 한 프레임이 `25.000 + 0.045 − 0.100 → 하한 25.000`이 되어
       온도가 25.0 ℃에서 절대 오르지 않는다 → 실습 4 시나리오 2("D 2초 → 온도 상승") 실패.
     - 해결: `applyMove()`가 실제로 이동하면 플래그를 true로, `update()`는 플래그가 false일 때만 냉각하고
       플래그를 다시 false로 내린다. 멤버 표 설명 "이동 시 상승, **정지 시** 서서히 냉각"과 일치하고 튜닝 상수는 그대로다.
     - 부수 효과: 배터리가 2000 px(≈11초) 이동하면 0 %가 되므로 최고 온도는 약 55 ℃. 상한 85 ℃는 안전장치로만 남는다.
  2. **`sf::Vector2f m_startPosition`**
     - `reset()`은 "모든 상태를 초기값으로 복원"인데, `Robot(100, 100)`처럼 다른 위치에서 만든 로봇이
       화면 중앙(400, 300)으로 가면 안 되므로 생성 시점 위치를 기억한다. 기본 생성 시에는 (400, 300)으로 동일하다.

## [실습 3] Robot.cpp 컴파일
- 명령: `g++ -std=c++17 -Wall -Wextra -I. -c Robot.cpp -o Robot.o`
- 결과: 성공, **경고 0건**
- 단계 구분: 컴파일 단계만 (`-c`)
- 원인 / 해결: `MathUtils.h`의 선언만 보고 컴파일하므로, `libmathutils.a` 없이도 `.o`가 만들어진다.

## [실습 3] 심볼 확인
- 명령: `nm -C Robot.o | grep Robot` / `nm -C Robot.o | grep degreeToRadian`
- 결과: 성공
  ```
  0000000000000000 T Robot::Robot(float, float)
  0000000000000000 T Robot::Robot(float, float)
  00000000000001f2 T Robot::reset()
  00000000000003ac T Robot::applyMove(float, float, float, float)
  0000000000000566 T Robot::update(float)
  000000000000065e T Robot::draw(sf::RenderTarget&) const
  ... (public/private 메서드 19개 모두 T)
                   U mathutils::degreeToRadian(double)
                   U sf::CircleShape::CircleShape(float, unsigned long)
                   U sf::RenderTarget::draw(sf::Vertex const*, unsigned long, sf::PrimitiveType, sf::RenderStates const&)
  ```
- 단계 구분: 해당 없음 (확인)
- 원인 / 해결:
  - `T` = `Robot.o` 안에 코드가 **정의된** 심볼. `U` = 이 파일이 **사용만 하고 정의는 없는** 심볼.
  - `degreeToRadian`이 `U`인 이유: `Robot.cpp`는 `MathUtils.h`의 **선언**만 보고 호출 코드를 만들었고, 함수 본체는
    `libmathutils.a`(`MathUtils.o`)에 있다. 컴파일러는 "나중에 링커가 이 이름의 주소를 채워 달라"는 빈칸을 남긴다.
    실습 4의 링크에서 `-lmathutils`가 이 빈칸을 채운다. `sf::...`가 `U`인 것도 같은 원리로 `libsfml-*.so`가 채운다.
  - 생성자가 두 줄인 이유: GCC는 C++ ABI에 따라 "완전 객체 생성자(C1)"와 "기반 클래스 부분 생성자(C2)"를 따로 내보낸다.
    `nm -C`는 둘 다 `Robot::Robot`으로 보여 주고, 상속이 없어 같은 주소(별칭)다.
  - `getX()` 등 헤더 인라인 getter는 호출하는 곳이 없어 `Robot.o`에 심볼이 생기지 않는다(정상).

## [실습 3] 헤더 자체 완결성 확인
- 명령:
  ```bash
  printf '#include "Robot.h"\nint main(){ Robot r; return 0; }\n' > header_check.cpp   # 임시 디렉토리
  g++ -std=c++17 -Wall -Wextra -I. -c header_check.cpp -o header_check.o
  ```
- 결과: 성공, 경고 0건
- 단계 구분: 컴파일 단계
- 원인 / 해결: `Robot.h`가 쓰는 타입(`sf::Vector2f`, `sf::CircleShape`, `sf::RenderTarget`, `sf::Color`)을 모두
  `<SFML/Graphics.hpp>`로 스스로 가져오므로, 다른 헤더를 먼저 include하지 않아도 컴파일된다.
  `-c`만 했으므로 링크(생성자 본체 필요)는 하지 않는다.

## [실습 3] (추가) 동작 규칙 검증 — 임시 테스트 프로그램
- 명령: `g++ -std=c++17 -Wall -Wextra -I. robot_logic.cpp Robot.o -L. -lmathutils -lsfml-graphics -lsfml-window -lsfml-system -o robot_logic`
  (임시 폴더에서 실행, 제출물 아님. 창 없이 Robot만 조작해 main 루프 순서 `move → update`를 흉내 냄)
- 결과: 성공, 20개 항목 모두 PASS
  | 검증 | 결과 |
  |---|---|
  | 초기값 | (400, 300), 100 %, 25 ℃, 180 px/s, 270° |
  | D 2초(60fps) | x = 760.00, 배터리 82.00 %, **온도 30.40 ℃** (정지 중에만 냉각하도록 바꿔서 상승함) |
  | 1초 정지 | 온도 30.4 → 25.00 (6 ℃/s 냉각, 하한 25) |
  | 오른쪽 경계 | x = 780 (= 800 − 반지름 20) |
  | `setPosition(-100, 9999)` | (20, 580)로 클램프 |
  | 배터리 소진 | 667프레임(11.1초) 후 0 %, 이때 온도 55.01 ℃ |
  | 배터리 0에서 이동 | 좌표 변화 없음 |
  | `reset()` | 모든 값 초기화 |
  | 세터 클램프 | 속도 0~1000, 배터리 0~100, 온도 25~85 |
  | 프레임 독립 | 30fps 2초 x = 760.00, 60fps 2초 x = 760.00 (동일) |
- 단계 구분: 실행(런타임) 단계
- 원인 / 해결: 해당 없음.

## [실습 3] (추가) 방향 지시선 렌더링 검증
- 명령: 오프스크린 `sf::RenderTexture`(800×600)에 `robot.draw()` 후 픽셀 색을 읽어 확인 (임시 프로그램)
- 결과:
  - 1차: 위/아래 두 방향이 `[FAIL]` — `pixel(400, 270)`이 배경색.
  - 2차(399~400열을 함께 스캔): 4방향 모두 정확
    ```
    forward(270)   up=11 down= 0 left= 0 right= 0
    backward(90)   up= 0 down=10 left= 0 right= 0
    left(180)      up= 0 down= 0 left=11 right= 0
    right(0)       up= 0 down= 0 left= 0 right=10
    ```
- 단계 구분: 실행(런타임) 단계 — **테스트 코드의 오류**(Robot 코드 문제 아님)
- 원인 / 해결: 세로선이 x = 400.0, 즉 픽셀 399와 400의 **경계** 위에 있어 래스터라이저가 399열에 그렸다.
  검사 좌표를 한 픽셀만 봤던 테스트 쪽 문제였다. `mathutils::degreeToRadian` + `cos/sin`으로 계산한 방향이
  SFML 좌표계(+y 아래)에서 별도 부호 보정 없이 맞게 그려짐을 확인했다.

---

## [실습 4] 종합 빌드 (지시서 4-4)
- 명령:
  ```bash
  g++ -std=c++17 -Wall -Wextra main.cpp Robot.cpp \
      -I. -L. -lmathutils \
      -lsfml-graphics -lsfml-window -lsfml-system \
      -o robot_sim
  ```
- 결과: 성공, **경고 0건**
- 단계 구분: 컴파일 + 링크
- 원인 / 해결: 순서 = [컴파일할 .cpp] → [내 라이브러리] → [SFML]. `Robot.o`가 남긴 `U degreeToRadian`을
  `-lmathutils`가, `U sf::...`를 `-lsfml-*`가 채운다. SFML끼리도 graphics → window → system 순서인데,
  graphics가 window를, window가 system을 사용하기 때문이다(사용하는 쪽이 앞).

## [실습 4] 4-7. 빌드 산출물 확인
- 명령: `nm -C robot_sim | grep -i radian` / `ldd robot_sim | grep -iE 'sfml|mathutils'` / `timeout 5 ./robot_sim; echo "exit=$?"`
- 결과: 성공
  ```
  0000000000008227 T mathutils::degreeToRadian(double)
  0000000000008253 T mathutils::radianToDegree(double)
  libsfml-graphics.so.3.1 => /usr/local/lib/libsfml-graphics.so.3.1
  libsfml-window.so.3.1 => /usr/local/lib/libsfml-window.so.3.1
  libsfml-system.so.3.1 => /usr/local/lib/libsfml-system.so.3.1
  (libmathutils 없음)
  exit=124
  ```
- 단계 구분: 실행(런타임) 단계까지 통과
- 원인 / 해결:
  - `T` = 라이브러리 코드가 **실행 파일 안에 복사**되었다(정적 링크). 그래서 `ldd`에 libmathutils가 없다.
    SFML은 `.so`라서 `ldd`에 나타나고, 실행할 때 `/usr/local/lib`에서 불러온다(동적 링크).
  - `radianToDegree`는 `robot_sim`이 쓰지 않는데도 들어 있다. 링커는 아카이브에서 **함수 단위가 아니라 `.o` 파일 단위**로
    꺼내기 때문이다. `degreeToRadian`이 필요해서 `MathUtils.o`를 꺼내면 그 안의 함수가 전부 따라온다.
  - `exit=124` = 5초 동안 크래시 없이 떠 있었다.

## [실습 4] 4-4. 비교용 대안 빌드
- 명령:
  ```bash
  # (a) 라이브러리 없이 소스 직접 포함
  g++ -std=c++17 -Wall -Wextra main.cpp Robot.cpp MathUtils.cpp -o robot_sim_a \
      -lsfml-graphics -lsfml-window -lsfml-system
  # (b) 목적 파일을 나눠 빌드
  g++ -std=c++17 -Wall -Wextra -I. -c main.cpp  -o main.o
  g++ -std=c++17 -Wall -Wextra -I. -c Robot.cpp -o Robot.o
  g++ -std=c++17 -Wall -Wextra -c MathUtils.cpp -o MathUtils.o
  ar rcs libmathutils.a MathUtils.o
  g++ main.o Robot.o -L. -lmathutils -lsfml-graphics -lsfml-window -lsfml-system -o robot_sim_b
  ```
  ((b)의 `-c` 명령에는 Makefile과 맞추려고 `-I.`를 붙였다. 같은 디렉토리라 없어도 결과는 같다.)
- 결과: 둘 다 성공, 경고 0건. **세 바이너리(종합 빌드 / (a) / (b))의 md5가 모두 `1c742dda…`로 동일**, 크기 101,464 바이트.
- 단계 구분: 컴파일 + 링크
- 원인 / 해결: 최종 실행 파일은 같고 **빌드 과정**만 다르다.

  | 방식 | 특징 | 한 파일만 고쳤을 때 |
  |---|---|---|
  | 종합 빌드 (.cpp + `-lmathutils`) | 명령 한 줄. 라이브러리는 미리 만들어 둔 `.a`를 재사용 | `main.cpp`, `Robot.cpp`를 매번 둘 다 다시 컴파일 |
  | (a) 소스 직접 포함 | 라이브러리 개념 없음. 가장 단순 | 세 파일 전부 다시 컴파일 |
  | (b) 목적 파일 분리 | 컴파일과 링크를 분리. Makefile이 쓰는 방식 | **바뀐 `.cpp`만** 다시 컴파일하고 링크 |

  (a)에서는 `MathUtils.cpp`가 링커 입력의 `.o`로 직접 들어가 무조건 포함되고, 종합 빌드와 (b)에서는 필요할 때만 `.a`에서 꺼낸다.
  이 프로젝트는 어차피 필요하므로 결과가 같다.

## [실습 4] 4-8. 일부러 낸 오류: `-lmathutils` 제거
- 명령: `g++ main.o Robot.o -L. -lsfml-graphics -lsfml-window -lsfml-system -o robot_sim`
- 결과: 오류
  ```
  /usr/bin/ld: Robot.o: in function `Robot::draw(sf::RenderTarget&) const':
  Robot.cpp:(.text+0x6bc): undefined reference to `mathutils::degreeToRadian(double)'
  collect2: error: ld returned 1 exit status
  ```
- 단계 구분: **링크 단계**
- 원인 / 해결: 실습 3의 `nm`에서 본 `U mathutils::degreeToRadian(double)`이 채워지지 않았다. 오류 위치가
  `Robot::draw`로 정확히 나오는데, 방향 지시선 계산에서 **라이브러리가 실제로 쓰이고 있다는 증거**다.
  - 추가: `-lmathutils`를 오브젝트 **앞**에 둔 `g++ -L. -lmathutils main.o Robot.o -lsfml-... -o robot_sim`도
    완전히 같은 오류가 난다(실습 2-5-①과 같은 원리).

## [실습 4] 4-5. Makefile
- 명령: `make clean` → `make` → `make`(다시) → `make test` → `timeout 5 make run`
- 결과: 전부 성공, 경고 0건
  ```
  $ make clean
  rm -f robot_sim test_math main.o Robot.o MathUtils.o libmathutils.a
  $ make
  g++ -std=c++17 -Wall -Wextra -I. -c main.cpp -o main.o
  g++ -std=c++17 -Wall -Wextra -I. -c Robot.cpp -o Robot.o
  g++ -std=c++17 -Wall -Wextra -I. -c MathUtils.cpp -o MathUtils.o
  ar rcs libmathutils.a MathUtils.o
  g++ main.o Robot.o -L. -lmathutils -lsfml-graphics -lsfml-window -lsfml-system -o robot_sim
  $ make
  make: 'all'을(를) 위해 할 일이 없습니다.
  $ make test
  ... ALL TESTS PASSED
  $ timeout 5 make run
  ./robot_sim
  make: *** [Makefile:52: run] 종료됨        ← timeout이 5초 뒤 종료시킨 것 (exit=124)
  ```
- 단계 구분: 컴파일 + 링크 + 실행
- 원인 / 해결:
  - 레시피 7줄 모두 **탭 문자**로 시작함을 `cat -A`로 확인했다(`^I`). 공백이면 `missing separator` 오류가 난다.
  - 두 번째 `make`가 아무것도 안 한 이유: 모든 타겟이 전제 조건보다 최신(파일 수정 시각 비교)이기 때문이다.
  - `clean`은 Makefile이 만드는 파일만 지운다. `hello_sfml`(실습 1)은 Makefile 산출물이 아니라서 남겨 두었다.

## [실습 4] Makefile 증분 빌드 (헤더 의존성 확인)
- 명령: `touch <파일>; make`
- 결과:
  | 수정한 파일 | 다시 실행된 명령 |
  |---|---|
  | `Robot.h` | `main.o`, `Robot.o` 컴파일 → 링크 |
  | `MathUtils.h` | `Robot.o`, `MathUtils.o` 컴파일 → `ar` → 링크 (**`main.o`는 그대로**) |
  | `main.cpp` | `main.o` 컴파일 → 링크 |
- 단계 구분: 해당 없음 (빌드 시스템 동작 확인)
- 원인 / 해결: Makefile에 적은 헤더 의존성대로 움직인다. `MathUtils.h`를 고쳐도 `main.o`가 재컴파일되지 않는 것은
  `Robot.h`가 `MathUtils.h`를 include하지 않기 때문이다(보고서 마무리 질문 3의 근거).
  헤더 의존성 줄이 없으면 헤더만 고쳤을 때 `make`가 "할 일 없음"이라고 하고 **옛 코드가 그대로 남는다.**

## [실습 4] 4-6. build.sh
- 명령: `make clean` 후 **다른 디렉토리(`/tmp`)에서** `~/Desktop/SFML_LAB/build.sh` 실행
- 결과: 성공, 경고 0건
  ```
  [1/4] 정적 라이브러리 생성: MathUtils.cpp -> MathUtils.o -> libmathutils.a
  [2/4] 오브젝트 파일 컴파일: main.cpp, Robot.cpp
  [3/4] 링크: [오브젝트] -> [내 라이브러리] -> [SFML] 순서
  [4/4] 완료
  -rw-rw-r-- 1 user user 2.0K libmathutils.a
  -rwxrwxr-x 1 user user 100K robot_sim
  ```
  `build.sh`로 만든 `robot_sim`과 `make`로 만든 `robot_sim`의 md5가 같다(`1c742dda…`).
- 단계 구분: 컴파일 + 링크
- 원인 / 해결: `cd "$(dirname "$0")"` 덕분에 어디서 실행해도 스크립트 위치 기준으로 빌드된다.
  `set -euo pipefail`로 한 단계라도 실패하면 즉시 멈춘다. `ar` 전에 `rm -f libmathutils.a`로 예전 아카이브를 지운다
  (`ar r`은 기존 아카이브에 추가·교체만 하므로, 지운 `.o`가 아카이브에 남는 일을 막기 위해서다).

## [실습 4] R7. 폰트 로드 실패 시 동작
- 명령: `main.cpp` **사본**의 폰트 경로만 `/nonexistent/NoFont.ttf`로 바꿔 빌드 후 `timeout 3` 실행
- 결과: 크래시 없이 계속 실행됨 (exit=124)
  ```
  Failed to load font (failed to open file): No such file or directory
      Provided path: /nonexistent/NoFont.ttf
      Absolute path: /nonexistent/NoFont.ttf
  [warning] failed to load font: /nonexistent/NoFont.ttf (HUD disabled)
  ```
- 단계 구분: 실행(런타임) 단계 (의도한 처리)
- 원인 / 해결: 위 3줄은 SFML이 자체적으로 stderr에 남기는 메시지, 마지막 줄은 `main.cpp`의 경고다.
  `openFromFile`의 반환값으로 분기하고 `sf::Text`는 `hasFont`일 때만 만들기 때문에 HUD만 빠진다.
  `sf::Font font(path)` 생성자를 썼다면 예외가 나서 프로그램이 종료됐을 것이다.
  원본 `main.cpp`는 수정하지 않았다.

## [실습 4] (추가) 화면 구성 확인 — 오프스크린 렌더링
- 명령: `main.cpp`를 `#define main robot_sim_main`으로 감싸 include한 임시 프로그램에서, 같은 `makeGrid`/`drawHud`로
  `sf::RenderTexture`에 그려 PNG로 저장 (원본 수정 없음)
- 결과: 성공
  - ① 시작: 로봇 중앙(400, 300), 초록, 방향선 위쪽. HUD `100.0 %`, `25.0 C`, `180.0 px/s`. 우하단 조작 안내 표시.
  - ② D 2초 + S 끝까지: `(760.0, 580.0)`, `7.0 %`, `52.9 C`, 로봇 **빨강**, 방향선 아래쪽, 아래 경계에서 몸체 잘림 없음.
- 단계 구분: 실행(런타임) 단계
- 원인 / 해결 / 관찰:
  - DejaVu Sans는 **비례폭 폰트**라 공백으로 맞춘 라벨(`Pos      :`)의 콜론이 세로로 정렬되지 않는다.
    지시서 R7이 `DejaVuSans.ttf`를 지정하므로 그대로 두었다(고정폭 `DejaVuSansMono.ttf`를 쓰면 정렬된다).
  - 로봇이 우하단 구석에 있으면 조작 안내 문구가 로봇 위에 겹친다. 렌더링 순서가 로봇 → HUD(지시서 R6)라 정상 동작이다.

## [실습 4] 4-10. 동작 확인 시나리오
- 명령: `make run`
- 결과: 성공. 시나리오 1~6(초기 HUD, D 2초 이동 시 값 변화, 경계 정지, 20 % 이하 빨강,
  0 %에서 이동 차단, R 리셋)을 직접 실행해 W/A/S/D·방향키 모두 의도대로 동작함을 확인했다.
- 단계 구분: 실행(런타임) 단계
- 원인 / 해결: 해당 없음. → 실습 4 체크포인트 전부 완료.

---

## [실습 5] 마무리 질문 3 실험: `Robot.h`에 `MathUtils.h`를 넣으면
- 명령 (임시 사본의 `Robot.h`에 `#include "MathUtils.h"` 추가):
  ```bash
  g++ -std=c++17 -I. -MM main.cpp                     # 컴파일러가 계산한 main.o의 의존 헤더
  g++ -std=c++17 -I. -c leak.cpp                      # Robot.h만 include하고 mathutils::PI 사용
  ```
- 결과:
  | | 원래 구조 (`Robot.cpp`에만 include) | `Robot.h`에 include |
  |---|---|---|
  | `main.o` 의존 헤더 (`-MM`) | `main.cpp Robot.h` | `main.cpp Robot.h` **`MathUtils.h`** |
  | `main.cpp` 컴파일 | 성공 | 성공 (기능 차이 없음) |
  | `Robot.h`만 include하고 `mathutils::PI` 사용 | `error: 'mathutils' has not been declared` (컴파일 단계) | 컴파일됨 (의존성 누출) |
- 단계 구분: 컴파일 단계
- 원인 / 해결: 헤더에 넣은 include는 그 헤더를 include하는 **모든 파일로 전파**된다. 그러면 `MathUtils.h`를 고칠 때
  `main.o`도 다시 컴파일해야 하고, Makefile의 `main.o:` 의존 줄에도 `MathUtils.h`를 추가해야 한다(빠뜨리면 옛 코드로 링크될 위험).
  실습 4 증분 빌드에서 `MathUtils.h` 수정 시 `main.o`가 재컴파일되지 않은 것이 원래 구조의 이점이다.

## [실습 5] 마무리 질문 4 실험: `distance = m_speed` + 30fps
- 명령: 임시 사본에서 `Robot.cpp` 98행을 `const float distance = m_speed;`로, `main.cpp`의 `kFrameRate`를 `30u`로 바꿔
  빌드(경고 0건, `timeout 3` 실행 exit=124). 같은 `Robot.o`로 D키 입력을 60/30fps로 흉내 내는 프로그램을 돌려 비교.
- 결과:
  | 구현 | fps | 1프레임 후 x | 0.1초 후 x / 배터리 / 온도 | 배터리 0까지 |
  |---|---|---|---|---|
  | `m_speed * dt` (원본) | 60 | 403.0 | 418.0 / 99.1 % / 25.3 ℃ | 11.12초 |
  | `m_speed * dt` (원본) | 30 | 406.0 | 418.0 / 99.1 % / 25.3 ℃ | 11.13초 |
  | `m_speed` (실험) | 60 | **580.0** | 780.0(벽) / **46.0 %** / 41.2 ℃ | **0.20초** (12프레임) |
  | `m_speed` (실험) | 30 | **580.0** | 780.0(벽) / **73.0 %** / 33.1 ℃ | **0.40초** (12프레임) |
- 단계 구분: 실행(런타임) 단계 — 컴파일/링크는 정상이라 **도구가 잡아 주지 못하는 논리 오류**
- 원인 / 해결: `m_speed`(px/**s**)를 "프레임당 px"로 써서 단위가 틀렸다. 한 프레임에 180 px(속도 60배)씩 움직여
  사실상 순간이동하고, 이동량·배터리·온도가 **시간이 아니라 프레임 수**에 비례하게 된다. 그래서 30fps에서는 같은 1초에
  절반만 움직이고 배터리도 2배 오래 간다(PC 성능에 따라 게임 속도가 달라짐). 원본은 두 fps에서 결과가 같다.
  원본 파일은 수정하지 않았으므로 원복할 것이 없다(`grep`으로 98행 `m_speed * dt`, `kFrameRate = 60u` 확인).

## [실습 5] 마무리 질문 5 실험: `libmathutils.a` → `libmathutils.so`
- 명령 (임시 디렉토리):
  ```bash
  g++ -std=c++17 -Wall -Wextra -fPIC -shared MathUtils.cpp -o libmathutils.so          # ar 대신
  g++ main.o Robot.o -L. -lmathutils -lsfml-graphics -lsfml-window -lsfml-system -o robot_sim   # 링크 명령은 동일
  ./robot_sim                                       # → 실패
  LD_LIBRARY_PATH=. ./robot_sim                     # → 성공
  g++ main.o Robot.o -L. -lmathutils -lsfml-... -Wl,-rpath,'$ORIGIN' -o robot_sim_rpath   # 경로를 실행 파일에 기록
  ```
- 결과:
  - `nm -C robot_sim` → `U mathutils::degreeToRadian(double)` (정적일 때는 `T`)
  - `ldd robot_sim` → `libmathutils.so => not found`
  - 그냥 실행 → `error while loading shared libraries: libmathutils.so: cannot open shared object file` (exit=127)
  - `LD_LIBRARY_PATH=.` → exit=124 (정상)
  - rpath 판: `readelf -d` → `RUNPATH [$ORIGIN]`, `ldd`가 `./libmathutils.so`를 찾음, exit=124. 단 `.so`를 지우면 다시 exit=127
  - 실행 파일 크기 101,464 → 101,352 바이트 (라이브러리 코드가 빠짐)
- 단계 구분: 빌드 성공 / 실행(런타임) 단계 오류
- 원인 / 해결: `-L.`은 **링크할 때** 링커(`ld`)에게만 주는 경로이고, 실행할 때 `.so`를 찾는 것은 동적 로더(`ld-linux`)다.
  로더에게는 `LD_LIBRARY_PATH`, 실행 파일의 RUNPATH(`-Wl,-rpath`), 또는 시스템 경로 + `ldconfig`로 알려 줘야 한다.
