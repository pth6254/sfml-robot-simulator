#!/usr/bin/env bash
# [실습 4] Makefile 없이 수동 빌드 명령을 순서대로 실행하는 스크립트
#   set -e : 명령이 하나라도 실패하면 즉시 중단 (실패한 채로 다음 단계로 넘어가지 않게)
#   set -u : 정의하지 않은 변수를 쓰면 오류 (오타 방지)
#   set -o pipefail : 파이프 중간 명령의 실패도 실패로 처리
set -euo pipefail

# 어느 디렉토리에서 실행하든 스크립트가 있는 디렉토리를 기준으로 빌드한다.
cd "$(dirname "$0")"

CXX=g++
CXXFLAGS=(-std=c++17 -Wall -Wextra -I.)
LIBS=(-L. -lmathutils -lsfml-graphics -lsfml-window -lsfml-system)

echo "[1/4] 정적 라이브러리 생성: MathUtils.cpp -> MathUtils.o -> libmathutils.a"
"$CXX" "${CXXFLAGS[@]}" -c MathUtils.cpp -o MathUtils.o
rm -f libmathutils.a                 # 예전 아카이브에 남은 오브젝트가 섞이지 않게 새로 만든다
ar rcs libmathutils.a MathUtils.o

echo "[2/4] 오브젝트 파일 컴파일: main.cpp, Robot.cpp"
"$CXX" "${CXXFLAGS[@]}" -c main.cpp -o main.o
"$CXX" "${CXXFLAGS[@]}" -c Robot.cpp -o Robot.o

echo "[3/4] 링크: [오브젝트] -> [내 라이브러리] -> [SFML] 순서"
"$CXX" main.o Robot.o "${LIBS[@]}" -o robot_sim

echo "[4/4] 완료"
ls -lh robot_sim libmathutils.a
