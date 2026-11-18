// [실습 3] 2D 로봇 클래스 - 구현
#include "Robot.h"

#include <algorithm> // std::clamp, std::max
#include <cmath>     // std::sqrt, std::cos, std::sin, std::fmod

#include "MathUtils.h" // 방향 지시선 계산에만 쓰므로 .cpp에서만 include한다.

namespace
{
// 튜닝 상수: 익명 네임스페이스 = 이 .cpp 파일 안에서만 보인다(내부 링크).
// 헤더에 두면 main.cpp까지 보이고, 값을 바꿀 때마다 Robot.h를 include한 파일이 전부 재컴파일된다.
constexpr float kBatteryPerPixel = 0.05f;  // 100 px 이동 시 5 % 소모
constexpr float kHeatPerPixel    = 0.015f; // 100 px 이동 시 1.5 ℃ 상승
constexpr float kCoolPerSec      = 6.0f;   // 정지 중 초당 6 ℃ 냉각
constexpr float kMaxTemperature  = 85.0f;
constexpr float kAmbientTemp     = 25.0f;
constexpr float kDefaultSpeed    = 180.0f;

// 지시서의 초기값/범위를 이름 붙은 상수로 둔다 (매직 넘버 방지).
constexpr float kMaxBattery      = 100.0f;
constexpr float kMaxSpeed        = 1000.0f;
constexpr float kInitialHeading  = 270.0f; // 위쪽
constexpr float kHeadingLineLen  = Robot::kRadius + 14.f;

const sf::Color kOutlineColor(20, 20, 20);
const sf::Color kHeadingColor(255, 235, 120);
const sf::Color kBatteryHigh(80, 200, 120);  // > 50 %
const sf::Color kBatteryMid(230, 190, 60);   // 20 ~ 50 %
const sf::Color kBatteryLow(220, 80, 80);    // <= 20 %
} // namespace

// ---------------------------------------------------------------- 생성 / 초기화

Robot::Robot(float startX, float startY)
    : m_startPosition(startX, startY)
    , m_position(startX, startY)
    , m_battery(kMaxBattery)
    , m_temperature(kAmbientTemp)
    , m_speed(kDefaultSpeed)
    , m_heading(kInitialHeading)
    , m_movedThisFrame(false)
    , m_body(kRadius)
{
    // 시작 좌표가 경계 밖으로 주어져도 필드 안에서 시작하게 한다.
    clampToField();
    m_startPosition = m_position;

    // 원점을 원의 중심으로: setPosition이 "중심 좌표"가 되어 m_position과 1:1로 대응한다.
    m_body.setOrigin({kRadius, kRadius});
    m_body.setOutlineThickness(2.f);
    m_body.setOutlineColor(kOutlineColor);
    m_body.setPosition(m_position);
    m_body.setFillColor(batteryColor());
}

void Robot::reset()
{
    m_position = m_startPosition;
    m_battery = kMaxBattery;
    m_temperature = kAmbientTemp;
    m_speed = kDefaultSpeed;
    m_heading = kInitialHeading;
    m_movedThisFrame = false;

    // 다음 update()를 기다리지 않고 바로 화면에 반영되도록 도형도 동기화한다.
    m_body.setPosition(m_position);
    m_body.setFillColor(batteryColor());
}

// ---------------------------------------------------------------- 이동

void Robot::moveForward(float dt)  { applyMove(0.f, -1.f, 270.f, dt); }
void Robot::moveBackward(float dt) { applyMove(0.f, 1.f, 90.f, dt); }
void Robot::moveLeft(float dt)     { applyMove(-1.f, 0.f, 180.f, dt); }
void Robot::moveRight(float dt)    { applyMove(1.f, 0.f, 0.f, dt); }

void Robot::applyMove(float dirX, float dirY, float headingDeg, float dt)
{
    // 1) 방어: 배터리가 없거나 시간이 흐르지 않았으면 아무것도 하지 않는다.
    if (!isAlive() || dt <= 0.f)
        return;

    // 2) 방향 벡터 정규화: (1, 1) 같은 대각선 입력이 와도 길이 1로 만들어 √2배 빨라지지 않게 한다.
    const float length = std::sqrt(dirX * dirX + dirY * dirY);
    if (length <= 0.f)
        return; // (0, 0)은 방향이 없으므로 이동 없음 (0으로 나누기 방지)
    dirX /= length;
    dirY /= length;

    // 3) 진행 방향 갱신: 0 ~ 360 범위로 정규화한다 (음수나 360 이상이 들어와도 안전하게).
    m_heading = std::fmod(headingDeg, 360.f);
    if (m_heading < 0.f)
        m_heading += 360.f;

    // 4) 프레임 독립 이동: 거리 = 속도(px/s) × 경과 시간(s).
    //    60fps든 30fps든 1초 동안 이동하는 거리는 m_speed px로 같다.
    const float distance = m_speed * dt;

    // 5) 좌표 갱신 → 경계 클램프
    m_position.x += dirX * distance;
    m_position.y += dirY * distance;
    clampToField();

    // 6) 이동한 만큼 배터리 소모, 발열
    consumeBattery(kBatteryPerPixel * distance);
    heatUp(kHeatPerPixel * distance);

    // 7) [지시서 대비 추가] 이번 프레임은 "이동 중"이었다고 표시 → update()에서 냉각하지 않는다.
    //    지시서 수치로는 발열(180 px/s × 0.015 = 2.7 ℃/s)보다 냉각(6 ℃/s)이 커서,
    //    매 프레임 무조건 냉각하면 온도가 25 ℃에서 전혀 오르지 않는다.
    m_movedThisFrame = true;
}

// ---------------------------------------------------------------- 갱신 / 렌더링

void Robot::update(float dt)
{
    // 정지 중(이번 프레임에 이동 없음)일 때만 냉각한다. 주변 온도(25 ℃) 아래로는 내려가지 않는다.
    // dt > 0 조건: 음수 dt가 들어와 온도가 오히려 오르는 것을 막는다.
    if (!m_movedThisFrame && dt > 0.f && m_temperature > kAmbientTemp)
        m_temperature = std::max(kAmbientTemp, m_temperature - kCoolPerSec * dt);

    // 다음 프레임을 위해 플래그를 내린다. (main 루프 순서: move…() → update() → draw())
    m_movedThisFrame = false;

    // 상태(m_position, m_battery)를 화면용 도형에 동기화한다.
    m_body.setPosition(m_position);
    m_body.setFillColor(batteryColor());
}

void Robot::draw(sf::RenderTarget& target) const
{
    target.draw(m_body);

    // 방향 지시선: 직접 만든 정적 라이브러리(libmathutils.a)로 도 → 라디안 변환.
    // (sf::degrees()/sf::Angle을 쓰지 않는 것이 이 실습의 요구사항)
    const double rad = mathutils::degreeToRadian(static_cast<double>(m_heading));

    // 좌표계가 +y 아래이므로 별도 부호 보정 없이 cos/sin을 그대로 쓰면
    // 270° → (0, -1) = 화면 위쪽, 90° → (0, +1) = 화면 아래쪽이 된다.
    const sf::Vector2f tip{
        m_position.x + kHeadingLineLen * static_cast<float>(std::cos(rad)),
        m_position.y + kHeadingLineLen * static_cast<float>(std::sin(rad))};

    // SFML 3: sf::Vertex는 집합체(aggregate) → {위치, 색}으로 초기화 (texCoords는 기본값)
    const sf::Vertex line[] = {
        sf::Vertex{m_position, kHeadingColor},
        sf::Vertex{tip, kHeadingColor}};
    target.draw(line, 2, sf::PrimitiveType::Lines);
}

// ---------------------------------------------------------------- 조회 / 설정

bool Robot::isAlive() const
{
    return m_battery > 0.f;
}

void Robot::setPosition(float x, float y)
{
    m_position = {x, y};
    clampToField();
    m_body.setPosition(m_position);
}

void Robot::setSpeed(float pxPerSec)
{
    m_speed = std::clamp(pxPerSec, 0.f, kMaxSpeed);
}

void Robot::setBattery(float percent)
{
    m_battery = std::clamp(percent, 0.f, kMaxBattery);
    m_body.setFillColor(batteryColor());
}

void Robot::setTemperature(float celsius)
{
    m_temperature = std::clamp(celsius, kAmbientTemp, kMaxTemperature);
}

// ---------------------------------------------------------------- 내부 헬퍼

void Robot::clampToField()
{
    // 중심 좌표를 반지름만큼 안쪽으로 제한해야 몸체가 창 밖으로 잘리지 않는다.
    m_position.x = std::clamp(m_position.x, kRadius, kFieldWidth - kRadius);
    m_position.y = std::clamp(m_position.y, kRadius, kFieldHeight - kRadius);
}

void Robot::consumeBattery(float amount)
{
    m_battery = std::clamp(m_battery - amount, 0.f, kMaxBattery);
}

void Robot::heatUp(float amount)
{
    m_temperature = std::clamp(m_temperature + amount, kAmbientTemp, kMaxTemperature);
}

sf::Color Robot::batteryColor() const
{
    if (m_battery > 50.f)
        return kBatteryHigh;
    if (m_battery > 20.f)
        return kBatteryMid;
    return kBatteryLow;
}
