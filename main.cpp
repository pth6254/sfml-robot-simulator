// [실습 4] 2D 로봇 시뮬레이터 - 진입점 + 메인 루프
// 역할 분담: main은 창/이벤트/키 입력/화면 구성을 담당하고,
//            로봇의 상태와 이동 규칙은 Robot 클래스에 맡긴다.
#include "Robot.h"

#include <SFML/Graphics.hpp>

#include <algorithm> // std::min
#include <iomanip>   // std::fixed, std::setprecision
#include <iostream>  // std::cerr
#include <optional>  // std::optional (SFML 3 pollEvent 반환형)
#include <sstream>   // std::ostringstream

namespace
{
// 상수: 익명 네임스페이스 → main.cpp 안에서만 보인다.
constexpr unsigned int kWindowWidth  = 800u;
constexpr unsigned int kWindowHeight = 600u;
constexpr unsigned int kFrameRate    = 60u;
constexpr float kMaxDt       = 0.1f; // 창 드래그 등으로 프레임이 멈췄다 재개될 때 로봇이 순간이동하지 않게
constexpr float kGridSpacing = 40.f;

// 창 크기와 Robot의 필드 크기가 어긋나면 경계 클램프가 화면과 맞지 않는다 → 컴파일 시점에 검사.
static_assert(static_cast<float>(kWindowWidth) == Robot::kFieldWidth, "window width != Robot field width");
static_assert(static_cast<float>(kWindowHeight) == Robot::kFieldHeight, "window height != Robot field height");

const char* const kFontPath = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
const char* const kHelpText = "WASD / Arrows : move    R : reset    Esc : quit";

const sf::Color kBackgroundColor(25, 28, 34);
const sf::Color kGridColor(255, 255, 255, 18);
const sf::Color kPanelFill(0, 0, 0, 150);
const sf::Color kPanelOutline(255, 255, 255, 60);
const sf::Color kHelpColor(200, 200, 200, 180);

// 격자는 변하지 않으므로 루프 밖에서 1회만 만든다. 매 프레임 만들면 정점 수십 개를 계속 재할당하게 된다.
// Lines 타입은 정점 2개가 선 1개다.
sf::VertexArray makeGrid(float width, float height, float spacing, sf::Color color)
{
    sf::VertexArray grid(sf::PrimitiveType::Lines);
    for (float x = 0.f; x <= width; x += spacing)
    {
        grid.append(sf::Vertex{{x, 0.f}, color});
        grid.append(sf::Vertex{{x, height}, color});
    }
    for (float y = 0.f; y <= height; y += spacing)
    {
        grid.append(sf::Vertex{{0.f, y}, color});
        grid.append(sf::Vertex{{width, y}, color});
    }
    return grid;
}

// HUD: 좌상단 반투명 패널 + 상태 4줄, 우하단 조작 안내.
// SFML 3의 sf::Text는 폰트 없이는 만들 수 없으므로(기본 생성자 없음),
// 폰트 로드에 성공했을 때만 이 함수를 호출하고 Text는 함수 안에서 만든다.
void drawHud(sf::RenderTarget& target, const sf::Font& font, const Robot& robot)
{
    sf::RectangleShape panel({272.f, 116.f});
    panel.setPosition({12.f, 12.f});
    panel.setFillColor(kPanelFill);
    panel.setOutlineThickness(1.f);
    panel.setOutlineColor(kPanelOutline);
    target.draw(panel);

    // HUD는 영문 라벨만 쓴다 (DejaVu Sans에는 한글 글리프가 없어 □로 보인다).
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1)
        << "Pos      : (" << robot.getX() << ", " << robot.getY() << ")\n"
        << "Battery  : " << robot.getBattery() << " %\n"
        << "Temp     : " << robot.getTemperature() << " C\n"
        << "Speed    : " << robot.getSpeed() << " px/s";

    sf::Text status(font, oss.str(), 16);
    status.setFillColor(sf::Color::White);
    status.setLineSpacing(1.35f);
    status.setPosition({24.f, 22.f});
    target.draw(status);

    // 우하단 정렬: 글자 폭을 측정해서 오른쪽 여백 20px을 맞춘다. (SFML 3: rect.width → rect.size.x)
    sf::Text help(font, kHelpText, 14);
    help.setFillColor(kHelpColor);
    const sf::FloatRect bounds = help.getLocalBounds();
    help.setPosition({static_cast<float>(kWindowWidth) - bounds.size.x - 20.f,
                      static_cast<float>(kWindowHeight) - 30.f});
    target.draw(help);
}

// 두 키 중 하나라도 눌려 있으면 true (W/A/S/D와 방향키 동시 지원)
bool isEitherPressed(sf::Keyboard::Key a, sf::Keyboard::Key b)
{
    return sf::Keyboard::isKeyPressed(a) || sf::Keyboard::isKeyPressed(b);
}
} // namespace

int main()
{
    // R1. 창 생성
    sf::RenderWindow window(sf::VideoMode({kWindowWidth, kWindowHeight}), "2D Robot Simulator");
    window.setFramerateLimit(kFrameRate);

    Robot robot;

    // R7. 폰트: 실패해도 시뮬레이터는 계속 돌리고 HUD만 생략한다.
    //     sf::Font font("경로") 생성자는 실패 시 예외를 던지므로, 반환값으로 판단하는 openFromFile을 쓴다.
    sf::Font font;
    const bool hasFont = font.openFromFile(kFontPath);
    if (!hasFont)
        std::cerr << "[warning] failed to load font: " << kFontPath << " (HUD disabled)\n";

    const sf::VertexArray grid = makeGrid(static_cast<float>(kWindowWidth), static_cast<float>(kWindowHeight),
                                          kGridSpacing, kGridColor);

    sf::Clock clock;

    while (window.isOpen())
    {
        // R2. 이벤트: 한 번 일어나는 일(창 닫기, 키를 "누른 순간")
        while (const std::optional event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
            else if (const auto* key = event->getIf<sf::Event::KeyPressed>())
            {
                if (key->code == sf::Keyboard::Key::Escape)
                    window.close();
                else if (key->code == sf::Keyboard::Key::R)
                    robot.reset();
            }
        }
        if (!window.isOpen())
            break; // 닫힌 창에 그리지 않도록 이번 프레임의 나머지를 건너뛴다.

        // R4. 프레임 간 경과 시간 (상한 0.1초)
        const float dt = std::min(clock.restart().asSeconds(), kMaxDt);

        // R3. 실시간 입력: "누르고 있는 동안" 계속 움직여야 하므로 이벤트가 아니라 현재 키 상태를 조회한다.
        using Key = sf::Keyboard::Key;
        if (isEitherPressed(Key::W, Key::Up))
            robot.moveForward(dt);
        if (isEitherPressed(Key::S, Key::Down))
            robot.moveBackward(dt);
        if (isEitherPressed(Key::A, Key::Left))
            robot.moveLeft(dt);
        if (isEitherPressed(Key::D, Key::Right))
            robot.moveRight(dt);

        robot.update(dt);

        // R6. 렌더링 순서: 배경 → 격자 → 로봇 → HUD (나중에 그린 것이 위에 보인다)
        window.clear(kBackgroundColor);
        window.draw(grid);
        robot.draw(window);
        if (hasFont)
            drawHud(window, font, robot);
        window.display();
    }

    return 0;
}
