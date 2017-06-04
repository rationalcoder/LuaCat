#include <cstdio>
#include <lg.hpp>

using namespace std;

class Factory;

struct Foo
{
    void test()
    {
        printf("got here\n");
    }
};

struct Factory {};

enum class TestEnum1
{
    ONE,
    TWO,
    THREE,
};

enum class TestEnum2
{
    ONE,
    TWO,
    THREE,
};

int main()
{
    auto api = lg::make_api("TestApi", lg::make_id<0>());
    auto& types = api.set_types(lg::Class<Foo>("Foo"),
                                lg::Enum<TestEnum1>("TestEnum1"),
                                lg::Enum<TestEnum2>("TestEnum2"));
    auto& foo = types.at<Foo>();

    foo.set_constructor(lg::Constructor<int, int>());
    foo.add_methods(
        LG_METHOD("test", &Foo::test)
    );

    auto& testEnum = types.at<TestEnum1>();

    testEnum.add_values(
        lg::enum_value("ONE", TestEnum1::ONE),
        lg::enum_value("TWO", TestEnum1::TWO),
        lg::enum_value("THREE", TestEnum1::THREE)
    );

    lua_State* L = luaL_newstate();
    api.export_to(L);
    luaL_dostring(L, "local m = TestApi.Foo._methods;");
    if (lua_gettop(L)) printf("Error: %s\n", lua_tostring(L, -1));
    lua_close(L);

    return EXIT_SUCCESS;
}
