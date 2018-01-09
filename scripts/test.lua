local Foo = TestApi.Foo
local Bar = TestApi.Bar
local foo = Foo()
local bar = Bar()
local b = foo:make_bar(bar, 1, 2, 3, 4)

local TestEnum1 = TestApi.TestEnum1
foo:test_enum(TestEnum1.ONE)
