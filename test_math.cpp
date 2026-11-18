// [실습 2] libmathutils.a 단위 테스트
// 라이브러리와 "별도의 프로그램"에서 링크해 쓸 수 있는지 확인하는 것이 목적이다.
#include "MathUtils.h"

#include <cmath>
#include <iomanip>
#include <iostream>

namespace
{

// 부동소수점은 == 로 비교하면 반올림 오차 때문에 실패할 수 있어 허용 오차를 둔다.
constexpr double kEpsilon = 1e-9;

int g_failures = 0;

void checkNear(const char* name, double actual, double expected, double eps = kEpsilon)
{
    const double err = std::fabs(actual - expected);
    const bool ok = err <= eps;
    if (!ok)
        ++g_failures;

    std::cout << (ok ? "[PASS] " : "[FAIL] ") << std::left << std::setw(32) << name
              << " got=" << std::setprecision(15) << actual
              << " expected=" << expected
              << " err=" << std::scientific << std::setprecision(2) << err
              << std::defaultfloat << '\n';
}

// 0은 곱셈만 하므로 정확히 0이 나와야 한다 → 허용 오차 없이 == 로 검사한다.
void checkExactZero(const char* name, double actual)
{
    const bool ok = (actual == 0.0);
    if (!ok)
        ++g_failures;
    std::cout << (ok ? "[PASS] " : "[FAIL] ") << std::left << std::setw(32) << name
              << " got=" << actual << " expected=0 (exact)\n";
}

} // namespace

int main()
{
    using mathutils::PI;
    using mathutils::degreeToRadian;
    using mathutils::radianToDegree;

    std::cout << "=== 1. degreeToRadian ===\n";
    checkNear("degreeToRadian(180) ~ PI", degreeToRadian(180.0), PI);
    checkNear("degreeToRadian(90)  ~ PI/2", degreeToRadian(90.0), PI / 2.0);
    checkExactZero("degreeToRadian(0)  == 0", degreeToRadian(0.0));

    std::cout << "\n=== 2. radianToDegree ===\n";
    checkNear("radianToDegree(PI)   ~ 180", radianToDegree(PI), 180.0);
    checkNear("radianToDegree(PI/2) ~ 90", radianToDegree(PI / 2.0), 90.0);
    checkExactZero("radianToDegree(0)    == 0", radianToDegree(0.0));

    std::cout << "\n=== 3. round trip (deg -> rad -> deg), |error| <= 1e-9 ===\n";
    const double samples[] = {0.0, 30.0, 45.0, 90.0, 180.0, 270.0, 360.0, -45.0, 123.456};

    std::cout << std::right << std::fixed << std::setprecision(6)
              << std::setw(12) << "degree"
              << std::setw(12) << "radian"
              << std::setw(12) << "back"
              << std::setw(14) << "error"
              << std::setw(8) << "result" << '\n'
              << std::string(58, '-') << '\n';

    for (const double deg : samples)
    {
        const double rad = degreeToRadian(deg);
        const double back = radianToDegree(rad);
        const double err = std::fabs(back - deg);
        const bool ok = err <= kEpsilon;
        if (!ok)
            ++g_failures;

        std::cout << std::setw(12) << deg
                  << std::setw(12) << rad
                  << std::setw(12) << back
                  // 오차는 1e-14 수준이라 fixed(6)로는 전부 0.000000으로 보인다 → 지수 표기로 출력
                  << std::setw(14) << std::scientific << std::setprecision(2) << err
                  << std::fixed << std::setprecision(6)
                  << std::setw(8) << (ok ? "OK" : "FAIL") << '\n';
    }

    std::cout << '\n';
    if (g_failures == 0)
    {
        std::cout << "ALL TESTS PASSED\n";
        return 0;
    }
    std::cout << g_failures << " TEST(S) FAILED\n";
    return 1;
}
