#include "MathUtils.h"
// 자기 헤더를 첫 줄에 include하면, 헤더의 선언과 아래 정의가 어긋날 때(시그니처 오타 등)
// 컴파일러가 바로 잡아 주고, 헤더가 다른 include 없이도 단독으로 컴파일되는지도 확인된다.

// 이 파일에는 main()을 넣지 않는다.
// 라이브러리는 다른 프로그램(test_math, robot_sim)에 링크되는 부품이므로,
// main이 있으면 사용하는 쪽의 main과 충돌해 "multiple definition of `main'" 링크 오류가 난다.

namespace mathutils
{

double degreeToRadian(double degree)
{
    // 반올림/절삭 없이 double 정밀도를 그대로 유지한다.
    return degree * PI / 180.0;
}

double radianToDegree(double radian)
{
    return radian * 180.0 / PI;
}

} // namespace mathutils
