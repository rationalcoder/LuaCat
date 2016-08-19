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

int main()
{
    lua_State* L = luaL_newstate();
    luaL_openlibs(L);
    luaglue::LuaClass<Foo, Foo::Factory, int, int> foo("Foo");

    LG_ADD_METHOD(foo, "GetI1", &Foo::GetI1);
    LG_ADD_METHOD(foo, "GetI2", &Foo::GetI2);
    LG_ADD_METHOD(foo, "SayHello", &Foo::SayHello);
    LG_ADD_METHOD(foo, "DoStuff", &Foo::DoStuff);
    LG_ADD_METHOD(foo, "Procedure", &Foo::Procedure);

    foo.Register(L);

    luaL_dostring(L, " foo = Foo:aquire(1, 2)\n"
                     " print(\"One: \"..foo:GetI1() .. \" Two: \" ..foo:GetI2())\n"
                     " foo:SayHello()\n"
                     " foo:DoStuff(1, 2, 3.1, 4.2, true, 5)\n"
                     " foo:Procedure()\n"
                     " Foo.release(foo)");


    return EXIT_SUCCESS;
}
