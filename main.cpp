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

struct Foo
{
    void test(Bar* p, int, int, int, int)
    {
        printf("Got here: %d, %d\n", p->one, p->two);
    }
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


int main()
{
    auto api = lc::make_api("TestApi");
    auto& types = api.set_types(lc::Class<Foo, lc::HeapFactory<Foo>>("Foo"),
                                lc::Class<Bar, lc::HeapFactory<Bar>>("Bar"),
                                lc::Enum<TestEnum1>("TestEnum1"),
                                lc::Enum<TestEnum2>("TestEnum2"));
    auto& foo = types.at<Foo>();
    foo.set_constructor(lc::Constructor<>());
    foo.add_methods(
        LC_METHOD("test", &Foo::test)
    );

    auto& bar = types.at<Bar>();
    bar.set_constructor(lc::Constructor<>());

    auto& testEnum = types.at<TestEnum1>();

    testEnum.add_values(
        lc::enum_value("ONE", TestEnum1::ONE),
        lc::enum_value("TWO", TestEnum1::TWO),
        lc::enum_value("THREE", TestEnum1::THREE)
    );

    lua_State* L = luaL_newstate();
    api.export_to(L);
    luaL_dofile(L, "scripts/test.lua");
    if (lua_gettop(L)) printf("Error: %s\n", lua_tostring(L, -1));
    lua_close(L);

    return EXIT_SUCCESS;
}
