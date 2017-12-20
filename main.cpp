#include <cstdio>
#include <cstdlib>
#include <lc/lc.hpp>

using namespace std;

class Factory;

struct Bar
{
    int one = 1;
    int two = 2;
};

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

struct Foo
{
    Bar* make_bar(Bar* p, int, int, int, int)
    {
        printf("%s\n", __PRETTY_FUNCTION__);
        return lc::HeapFactory<Bar>::make();
    }

    void test_enum(TestEnum1)
    {
        printf("Worked\n");
    }
};

int main()
{
    auto api = lc::make_api("TestApi");
    auto& types = api.set_types(lc::Class<Foo>("Foo"),
                                lc::Class<Bar>("Bar"),
                                lc::Enum<TestEnum1>("TestEnum1"),
                                lc::Enum<TestEnum2>("TestEnum2"));
    auto& foo = types.at<Foo>();
    foo.set_constructor(lc::Constructor<>());
    foo.add_methods(
        LC_METHOD("make_bar", &Foo::make_bar),
        LC_METHOD("test_enum", &Foo::test_enum)
    );

    auto& bar = types.at<Bar>();
    bar.set_constructor(lc::Constructor<>());

    auto& testEnum = types.at<TestEnum1>();
    testEnum.add_values(
        lc::enum_value("ONE", TestEnum1::ONE),
        lc::enum_value("TWO", TestEnum1::TWO),
        lc::enum_value("THREE", TestEnum1::THREE)
    );

    auto& testEnum2 = types.at<TestEnum2>();
    testEnum2.add_values(
        lc::enum_value("ONE", TestEnum2::ONE),
        lc::enum_value("TWO", TestEnum2::TWO),
        lc::enum_value("THREE", TestEnum2::THREE)
    );

    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    api.export_to(L);

    luaL_dofile(L, "scripts/test.lua");
    if (lua_gettop(L)) printf("Error: %s\n", lua_tostring(L, -1));
    lua_close(L);

    return EXIT_SUCCESS;
}
