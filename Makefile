# [실습 4] 2D Robot Simulator 빌드 스크립트
#
# 의존 관계:
#   MathUtils.cpp -> MathUtils.o -> libmathutils.a --+
#   Robot.cpp     -> Robot.o ------------------------+--> robot_sim (+ SFML 3 .so)
#   main.cpp      -> main.o -------------------------+

CXX      := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -I.
LDFLAGS  := -L.
LDLIBS   := -lmathutils -lsfml-graphics -lsfml-window -lsfml-system

TARGET   := robot_sim
OBJS     := main.o Robot.o
LIB      := libmathutils.a
TEST     := test_math

# 파일이 아니라 "동작"의 이름인 타겟. 같은 이름의 파일이 생겨도 항상 실행되게 한다.
.PHONY: all test run clean

# 첫 번째 타겟 = 기본 타겟 (make만 입력하면 실행)
all: $(TARGET)

# 링크: 오브젝트를 먼저, 라이브러리는 반드시 그 뒤에.
# 링커는 왼쪽에서 오른쪽으로 한 번만 훑으며, 앞서 나온 미해결 심볼만 뒤의 라이브러리에서 채운다.
# $(LIB)를 전제 조건에 넣어야 라이브러리가 갱신됐을 때 다시 링크된다.
$(TARGET): $(OBJS) $(LIB)
	$(CXX) $(OBJS) $(LDFLAGS) $(LDLIBS) -o $@

# 정적 라이브러리: $@ = 타겟 이름(libmathutils.a), $^ = 전제 조건 전체(MathUtils.o)
$(LIB): MathUtils.o
	ar rcs $@ $^

# 패턴 규칙: 아무 X.cpp -> X.o. $< = 첫 번째 전제 조건(X.cpp)
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 헤더 의존성: 헤더가 바뀌면 그 헤더를 include한 .o만 다시 컴파일된다.
# (패턴 규칙의 레시피는 그대로 쓰고, 전제 조건만 추가하는 규칙)
main.o:      main.cpp      Robot.h
Robot.o:     Robot.cpp     Robot.h MathUtils.h
MathUtils.o: MathUtils.cpp MathUtils.h

# 단위 테스트: 라이브러리를 별도 프로그램에 링크해서 실행
$(TEST): test_math.cpp MathUtils.h $(LIB)
	$(CXX) $(CXXFLAGS) test_math.cpp $(LDFLAGS) -lmathutils -o $@

test: $(TEST)
	./$(TEST)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(TEST) $(OBJS) MathUtils.o $(LIB)
