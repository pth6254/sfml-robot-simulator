// [실습 3] 2D 로봇 클래스 - 헤더 (선언 + 단순 getter만)
//
// 좌표계 규칙
//   - SFML 원점 = 창 좌상단, +x = 오른쪽, +y = 아래쪽 (수학 좌표계와 y축 방향이 반대)
//   - 각도(m_heading, 도 단위): 0° = 오른쪽(+x), 90° = 아래(+y), 180° = 왼쪽, 270° = 위(-y)
//     → y축이 아래를 향하므로 각도는 화면에서 "시계 방향"으로 증가한다.
//
// 책임 범위: 로봇의 상태(위치/배터리/온도/속도/방향)와 이동 규칙만 담당한다.
//            창 생성, 이벤트 처리, 키 입력 해석은 main이 담당한다.
#ifndef ROBOT_H
#define ROBOT_H

// MathUtils.h는 여기서 include하지 않는다.
// 헤더의 선언(public 인터페이스)에는 mathutils가 전혀 등장하지 않고 Robot.cpp 구현에서만 쓰기 때문이다.
// 여기 넣으면 Robot.h를 include하는 모든 파일(main.cpp 등)이 불필요하게 MathUtils.h에 의존하게 된다.
#include <SFML/Graphics.hpp>

class Robot
{
public:
    // 클래스 내부 static constexpr: C++17부터 암묵적으로 inline이라 .cpp에 따로 정의할 필요가 없다.
    static constexpr float kRadius      = 20.f;
    static constexpr float kFieldWidth  = 800.f;
    static constexpr float kFieldHeight = 600.f;

    // explicit: Robot r = 100.f; 같은 float → Robot 암묵적 변환을 막는다.
    explicit Robot(float startX = kFieldWidth / 2.f, float startY = kFieldHeight / 2.f);

    // 모든 상태를 생성 시점의 초기값으로 되돌린다.
    void reset();

    // 이동 (dt: 직전 프레임으로부터 경과한 초). 모두 private applyMove로 위임한다.
    void moveForward(float dt);  // 위    (-y), heading 270°
    void moveBackward(float dt); // 아래  (+y), heading  90°
    void moveLeft(float dt);     // 왼쪽  (-x), heading 180°
    void moveRight(float dt);    // 오른쪽(+x), heading   0°

    // 매 프레임 1회 호출: 정지 중이면 냉각하고, 화면용 도형(m_body)을 상태와 동기화한다.
    void update(float dt);

    // 그리기만 위임받는다. 상태를 바꾸지 않으므로 const.
    void draw(sf::RenderTarget& target) const;

    // 조회 (HUD 표시용) - 단순 getter라 헤더 인라인 허용
    float getX() const { return m_position.x; }
    float getY() const { return m_position.y; }
    float getBattery() const { return m_battery; }
    float getTemperature() const { return m_temperature; }
    float getSpeed() const { return m_speed; }
    float getHeading() const { return m_heading; }
    bool isAlive() const;

    // 설정 - 모두 유효 범위로 클램프한다(방어적 프로그래밍)
    void setPosition(float x, float y);  // 필드 경계 안으로
    void setSpeed(float pxPerSec);       // 0 ~ 1000 px/s
    void setBattery(float percent);      // 0 ~ 100 %
    void setTemperature(float celsius);  // 25 ~ 85 ℃

private:
    void applyMove(float dirX, float dirY, float headingDeg, float dt);
    void clampToField();
    void consumeBattery(float amount);
    void heatUp(float amount);
    sf::Color batteryColor() const;

    // 멤버 선언 순서 = 초기화 순서. 생성자 초기화 리스트도 같은 순서로 써야 -Wreorder 경고가 없다.
    sf::Vector2f m_startPosition;  // reset()이 돌아갈 생성 시점 위치
    sf::Vector2f m_position;       // 로봇 중심 좌표 (px)
    float m_battery;               // 0 ~ 100 %
    float m_temperature;           // 25 ~ 85 ℃
    float m_speed;                 // px/s
    float m_heading;               // 0 ~ 360°, 도 단위 float (sf::Angle 아님: 변환을 mathutils로 하기 위해)
    bool m_movedThisFrame;         // 이번 프레임에 실제로 이동했는가 (정지 중에만 냉각하기 위한 플래그)
    sf::CircleShape m_body;        // 화면 표시용 도형
};

#endif // ROBOT_H
