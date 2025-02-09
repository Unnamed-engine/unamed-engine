#include "Entity.hpp"
#include "HushEngine.hpp"
#include "Scene.hpp"

#include <flecs.h>

struct RGBA
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
    std::uint8_t a;
};

struct MyComplexType
{
    std::string a;

    MyComplexType(int)
    {
        std::cout << "MyComplexType constructor" << std::endl;
    }

    ~MyComplexType()
    {
        std::cout << "MyComplexType destructor" << std::endl;
    }
};

struct Position
{
    float x;
    float y;
};

struct A
{
    int a = 234;
};

int main()
{
    Hush::Scene scene(nullptr);

    Hush::Entity entity = scene.CreateEntity();
    entity.EmplaceComponent<Position>(1.0f, 2.0f);

    Hush::Entity entity2 = scene.CreateEntity();
    entity2.EmplaceComponent<Position>(3.0f, 4.0f);
    entity2.AddComponent<A>();

    auto query = scene.CreateQuery<Position>();

    for (auto [positionArray] : query)
    {
        for (Position &position : positionArray)
        {
            std::cout << "Position: " << position.x << ", " << position.y << std::endl;
            // position.x += 1.0f;
        }
    }

    auto query2 = scene.CreateQuery<Position, A>();

    query2.Each([](Position &position, A &a) {
        std::cout << "Position: " << position.x << ", " << position.y << std::endl;
        std::cout << "A: " << a.a << std::endl;
    });

    scene.DestroyEntity(std::move(entity));

    // Hush::HushEngine engine;
    // engine.Run();
    // engine.Quit();
    // return 0;
}
