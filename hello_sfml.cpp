// [실습 1] SFML 3.x 동작 확인 예제
// 목적: 소스에서 빌드해 /usr/local에 설치한 SFML 3이 컴파일/링크/실행까지 되는지 확인한다.
#include <SFML/Graphics.hpp>

#include <optional>

int main()
{
    // SFML 3: VideoMode는 (폭, 높이) 두 정수가 아니라 Vector2u 하나를 받는다.
    sf::RenderWindow window(sf::VideoMode({800u, 600u}), "Hello SFML");
    window.setFramerateLimit(60); // 회전 속도가 PC 성능에 따라 달라지지 않게 60fps로 제한

    sf::CircleShape circle(80.f);
    circle.setFillColor(sf::Color(90, 160, 240));
    circle.setOutlineThickness(3.f);
    circle.setOutlineColor(sf::Color::White);
    // 원점을 도형 중심으로 옮겨야 setPosition이 "중심 좌표"가 되고, 회전도 중심 기준이 된다.
    circle.setOrigin({80.f, 80.f});
    circle.setPosition({400.f, 300.f});

    while (window.isOpen())
    {
        // SFML 3: pollEvent()는 std::optional<sf::Event>를 반환한다. 이벤트가 없으면 빈 optional.
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
            }
        }

        // SFML 3: 각도는 float가 아니라 sf::Angle 타입이다.
        circle.rotate(sf::degrees(1.f));

        window.clear(sf::Color(25, 28, 34));
        window.draw(circle);
        window.display();
    }

    return 0;
}
