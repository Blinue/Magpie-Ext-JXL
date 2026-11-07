include(./host-msvc.profile)

[settings]
compiler=clang
compiler.version=20
compiler.cppstd=gnu17

lcms/*:compiler=msvc
lcms/*:compiler.version=195
lcms/*:compiler.cppstd=17
lcms/*:compiler.runtime=static

[conf]
tools.cmake.cmaketoolchain:generator=Visual Studio 18
