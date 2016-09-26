#include <iostream>
#include "lg_class.hpp"

using namespace std;

class Foo
{
public:

struct Factory
{
    static Foo* Create(int i1, int i2)
    {
        cout << "Creating Foo with " << i1 << ", " << i2 << endl;
        return new Foo(i1, i2);
    }

    static void Destroy(Foo* f)
    {
        cout << "Destroying Foo" << endl;
        delete f;
    }
};


public:
    Foo(int i1, int i2)
        : i1_(i1), i2_(i2)
    {
        cout << "Ctor Foo" << endl;
    }

    ~Foo()
    {
        cout << "Dtor Foo" << endl;
    }


    int GetI1() { return i1_; }
    int GetI2() { return i2_; }
    void SayHello()
    {
        cout << "Hello from Foo!!" << endl;
    }

    void DoStuff(int one, int two, double three, float four, bool cond, uint64_t i)
    {
        boolalpha(cout);
        cout << one << " " << two << " " << three << " " << four;
        cout << " " << cond << " " << i << endl;
    }

    void Procedure()
    {
        cout << "Procedure called in Foo" << endl;
    }

private:
    int i1_;
    int i2_;
};

void ExportAPI(lua_State* L)
{
    luaglue::LuaClass<Foo, Foo::Factory, int, int> foo("Foo");

    luaglue::MethodSet fooMethods =
    {
        {"GetI1", LG_METHOD(&Foo::GetI1)},
        {"GetI2", LG_METHOD(&Foo::GetI2)},
        {"SayHello", LG_METHOD(&Foo::SayHello)},
        {"DoStuff", LG_METHOD(&Foo::DoStuff)},
        {"Procedure", LG_METHOD(&Foo::Procedure)},
    };

    foo.AddMethods(fooMethods);
    foo.Export(L);
}

int main()
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    ExportAPI(L);

    luaL_dostring(L, " foo = Foo:aquire(1, 2)\n"
                     " foo2 = Foo:aquire(3, 4)\n"
                     " --print(\"One: \"..foo:GetI1() .. \" Two: \" ..foo:GetI2())\n"
                     " --print(\"One: \"..foo2:GetI1() .. \" Two: \" ..foo2:GetI2())\n"
                     " --foo:SayHello()\n"
                     " --foo2:SayHello()\n"
                     " foo:DoStuff(1, 2, 3.1, 4.2, true, 5)\n"
                     " foo2:DoStuff(1, 2, 3.5, 4.2, true, 5)\n"
                     " --foo:Procedure()\n"
                     " --foo2:Procedure()\n"
                     " Foo.release(foo)"
                     " Foo.release(foo2)");

    lua_close(L);

    return EXIT_SUCCESS;
}
