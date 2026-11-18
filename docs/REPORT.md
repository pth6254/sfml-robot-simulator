# SFML 3 기반 2D 로봇 시뮬레이터 빌드 실습 보고서

작성자: pth6254 · 작성일: 2026-11-13 · 상세 기록: [`BUILD_LOG.md`](BUILD_LOG.md)

## 1. 개발 환경

| 항목 | 내용 |
|---|---|
| OS / 세션 | Ubuntu 24.04.5 LTS x86_64, Wayland (SFML은 XWayland로 동작) |
| 컴파일러 / 도구 | g++ 13.3.0 (`-std=c++17 -Wall -Wextra`, 전 과정 **경고 0건**), CMake 3.28.3, GNU Make 4.3 |
| SFML | **3.1.0**을 소스에서 빌드해 `/usr/local`에 공유 라이브러리(`.so`)로 설치 |

**소스 빌드 이유**: apt의 `libsfml-dev`는 2.6.1이다. SFML 3은 이벤트(`std::optional`), 좌표(`Vector2` 인자), 각도(`sf::Angle`), 텍스트(폰트 필수) API가 바뀌어 2.x와 호환되지 않는다.
**CMake 옵션**: `Release`(라이브러리 이름에 `-d`가 붙지 않음), `BUILD_SHARED_LIBS=ON`(SFML 3의 기본은 정적), `SFML_USE_SYSTEM_DEPS=OFF`(freetype 등을 직접 받아 빌드), audio/network `OFF`, `SFML_INSTALL_PKGCONFIG_FILES=ON`. 설치 확인: `pkg-config --modversion sfml-graphics` → `3.1.0`.

## 2. 빌드 명령 (`make` / `./build.sh`가 같은 명령을 실행)

```bash
g++ -std=c++17 -Wall -Wextra -I. -c MathUtils.cpp -o MathUtils.o && ar rcs libmathutils.a MathUtils.o   # 정적 라이브러리
g++ -std=c++17 -Wall -Wextra -I. -c main.cpp -o main.o
g++ -std=c++17 -Wall -Wextra -I. -c Robot.cpp -o Robot.o
g++ main.o Robot.o -L. -lmathutils -lsfml-graphics -lsfml-window -lsfml-system -o robot_sim  # 사용하는 쪽 → 제공하는 쪽
```

확인 결과는 다음과 같다. `nm -C robot_sim`에 `T mathutils::degreeToRadian`이 보여 라이브러리 코드가 실행 파일에 포함되었다. `ldd`에는 `/usr/local/lib/libsfml-*.so.3.1`만 보인다. 종합 빌드, 소스 직접 포함, 목적 파일 분리의 세 방식으로 만든 실행 파일은 md5가 모두 같았다.

## 3. 오류와 해결

| 상황 | 단계 | 메시지 (요약) | 원인 → 해결 |
|---|---|---|---|
| `-lsfml-*` 누락 | 링크 | `undefined reference to sf::VideoMode::VideoMode(...)` | 헤더(선언)만 있고 정의가 없음 → `-l` 추가 |
| `-l`을 소스 **앞**에 둠 (SFML, mathutils) | 링크 | `undefined reference to ...` | 링커는 왼쪽부터 한 번만 훑음 → 라이브러리는 뒤에 |
| `-L.` 누락 | 링크 | `cannot find -lmathutils` | 현재 디렉토리는 검색 경로가 아님 → `-L.` |
| `-I.` 누락 | 성공 | — | `"..."` include는 소스 파일의 디렉토리부터 찾음 |
| 라이브러리에 `main()` | 링크 | `multiple definition of 'main'` | 아카이브에서 꺼낸 `.o`의 `main`이 충돌 |
| `.so`를 경로 없이 실행 | 런타임 | `error while loading shared libraries` | `-L`은 링크 때만 쓰임 → `LD_LIBRARY_PATH`/rpath |
| **2→3 문법** `sf::VideoMode(800, 600)` | 컴파일 | `no matching function for call to VideoMode(int, int)` | 3.x는 `VideoMode({800u, 600u})` |
| (실제) 설치 시 `sudo` 비밀번호 | 설치 | `sudo: 암호가 필요합니다` | `pkexec`로 같은 두 명령(`cmake --install`, `ldconfig`) 실행 |
| (실제) 지시서 수치로는 온도가 오르지 않음 | 런타임 논리 | HUD가 `25.0 C`에 고정 | 발열 2.7 ℃/s < 냉각 6 ℃/s → **정지 중에만 냉각**(`m_movedThisFrame`) |

본 코드는 작성 전에 `/usr/local/include/SFML` 헤더로 API를 확인해서, 의도하지 않은 컴파일·링크 오류는 없었다. 컴파일 오류는 `파일:줄:칸: error`, 링크 오류는 `/usr/bin/ld: ...`와 `collect2: error`로 끝나는 형식으로 구분했다.

## 4. 배운 점

- **선언과 정의**: 헤더는 선언, 라이브러리는 정의다. `nm`에서 `Robot.o`의 `U degreeToRadian`이 링크 후 `robot_sim`의 `T`가 되는 과정으로 연결을 직접 확인했다.
- **정적과 동적**: 정적 라이브러리(`.a`)는 링크할 때 `.o` 단위로 복사되어, 쓰지 않는 `radianToDegree`도 포함된다. 공유 라이브러리(`.so`)는 실행할 때 로더가 찾는다.
- **include 최소화**: Makefile의 헤더 의존성과 include 최소화가 재컴파일 범위를 결정한다. `MathUtils.h`를 고쳐도 `main.o`는 다시 컴파일되지 않았다.
- **숫자 검증**: 사양의 수치를 직접 계산·테스트해야 설계 결함(냉각 > 발열)을 미리 찾을 수 있다.
- **한계**: 대각선 이동은 `move` 호출이 두 번이라 √2배 빠르다. 벽에 막혀도 `m_speed*dt`만큼 배터리가 줄어든다(지시서 규칙).

## 5. 마무리 질문

**Q1.** 링커는 입력을 **왼쪽에서 오른쪽으로 한 번** 처리한다. 정적 라이브러리에서는 *그 시점까지 해결되지 않은 심볼*을 정의한 `.o`만 꺼낸다. `g++ main.cpp Robot.cpp -lmathutils`는 `Robot.o`가 `degreeToRadian`을 필요로 한 뒤 라이브러리를 만나 성공한다. 반대 순서는 라이브러리를 만날 때 필요한 심볼이 없어 아무것도 꺼내지 않고, 되돌아가지도 않으므로 `undefined reference to mathutils::degreeToRadian(double)`이 난다.

**Q2.** 링크 단계 오류. `.cpp`는 따로 컴파일되므로 `-c`와 `ar`은 통과한다.
`/usr/bin/ld: ./libmathutils.a(MathUtils.o): in function 'main': MathUtils.cpp:(.text+0x58): multiple definition of 'main'; /tmp/ccfMQYMl.o:test_math.cpp:(.text+0x2ad): first defined here`

**Q3.** 둘 다 기능은 같다. 하지만 헤더에 넣으면 include가 `Robot.h`를 쓰는 모든 파일로 **전파된다**. `g++ -MM main.cpp`로 확인하니 `main.o`의 의존 헤더에 `MathUtils.h`가 추가되었다. 그 결과 (1) `MathUtils.h`를 고칠 때 `main.o`도 재컴파일해야 하고 Makefile 의존 줄도 늘어난다. (2) `main.cpp`가 include하지 않은 `mathutils::PI`를 쓸 수 있게 되어 숨은 의존이 생긴다. 원래 구조에서는 같은 코드가 `'mathutils' has not been declared` 오류가 난다. 구현에서만 쓰는 의존은 `.cpp`에만 둔다.

**Q4.** 단위가 px/s에서 "px/프레임"으로 바뀌어, 한 프레임에 180 px(60fps에서 속도 60배)를 움직인다. 1프레임 만에 x 400 → 580, 3프레임 안에 벽에 닿아 사실상 순간이동한다. 이동량이 **시간이 아니라 프레임 수에 비례**하게 되어, 배터리가 60fps에서 0.20초, 30fps에서 0.40초 만에 바닥났다. 원본(`m_speed*dt`)은 두 fps 모두 11.1초로 같았다. 컴파일러가 잡지 못하는 논리 오류다. (임시 사본으로 실험했고 원본은 수정하지 않음)

**Q5.**
- **빌드**: `ar rcs` 대신 `g++ -fPIC -shared MathUtils.cpp -o libmathutils.so`를 쓴다. 링크 명령(`-L. -lmathutils`)은 그대로이고, 같은 이름이면 `.so`가 우선 선택된다.
- **실행**: 그냥 실행하면 `cannot open shared object file`(exit 127)이 나므로 로더에 경로를 알려야 한다. 방법은 `LD_LIBRARY_PATH=. ./robot_sim`, 링크 때 `-Wl,-rpath,'$ORIGIN'`, 또는 `/usr/local/lib` 설치 후 `ldconfig`다.
- **결과 차이**: `nm`은 `U`, `ldd`에 `libmathutils.so`가 나타난다. 실행 파일은 112바이트 작아지는 대신 배포할 때 `.so`를 함께 둬야 한다.

> 스크린샷 ① 실행 직후 / ② 로봇을 구석(좌하단 또는 우상단 권장)으로 이동한 후 — 별도 첨부
