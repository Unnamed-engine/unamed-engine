#include "Entity.hpp"
#include "HushEngine.hpp"
#include "Scene.hpp"

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

int main()
{
    Hush::Scene scene(nullptr);

    Hush::Entity entity = scene.CreateEntity();

    entity.EmplaceComponent<MyComplexType>(1);

    scene.DestroyEntity(std::move(entity));

    // Hush::HushEngine engine;
    // engine.Run();
    // engine.Quit();
    // return 0;
}
