[settings]
os=Windows
compiler=msvc
compiler.cppstd=17
compiler.runtime=static
compiler.version=195

[tool_requires]
cmake/[>=4.4 <4.5]

[conf]
tools.info.package_id:confs=["tools.build:cxxflags", "tools.build:defines", "user.magpie:msbuild_version"]
