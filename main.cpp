#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <cstdio>
#include "lg.hpp"

using namespace std;

class Factory;

struct Foo
{
    void test()
    {
        printf("got here\n");
    }
};

struct FooFactory
{
    static Foo* make()
    {
        Foo* p = new Foo();
        printf("returning %p\n", p);
        return p;
    }

    static void free(Foo* p)
    {
        printf("deleting %p\n", p);
        delete p;
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
    auto api = lg::make_api("TestApi", lg::id<0>());
    auto& types = api.set_types(lg::Class<Foo, FooFactory>("Foo"),
                                lg::Enum<TestEnum1>("TestEnum1"),
                                lg::Enum<TestEnum2>("TestEnum2"));
    auto& foo = types.at<Foo>();

    foo.set_constructor(lg::Constructor<>());
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
    luaL_openlibs(L);
    api.export_to(L);
    luaL_dostring(L, "local Foo = TestApi.Foo; local foo = Foo(); foo:test();");
    if (lua_gettop(L)) printf("Error: %s\n", lua_tostring(L, -1));
    lua_close(L);

    return EXIT_SUCCESS;
}
