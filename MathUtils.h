// [실습 2] 각도 변환 수학 라이브러리 - 헤더 (선언만)
// 이 헤더는 라이브러리의 "사용 설명서"다. 함수 본체는 MathUtils.cpp → libmathutils.a에 들어 있다.
#ifndef MATHUTILS_H
#define MATHUTILS_H

namespace mathutils
{

// constexpr 변수는 암묵적으로 내부 링크(internal linkage)를 가지므로,
// 이 헤더를 여러 .cpp가 include해도 "multiple definition" 링크 오류가 나지 않는다.
constexpr double PI = 3.14159265358979323846;

// 도(degree) → 라디안(radian): degree × π / 180
double degreeToRadian(double degree);

// 라디안(radian) → 도(degree): radian × 180 / π
double radianToDegree(double radian);

} // namespace mathutils

#endif // MATHUTILS_H
