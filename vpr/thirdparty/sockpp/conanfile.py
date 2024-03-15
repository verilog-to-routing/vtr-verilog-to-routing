from conans import ConanFile, CMake, tools

class Sockpp(ConanFile):
    name = "sockpp"
    description = """Modern C++ socket library."""
    license = "BSD-3-Clause License"
    author = "fpagliughi"
    settings = "os", "compiler", "build_type", "arch"
    exports = "CMakeLists.txt", "src/*", "include/*", "doc/*", "tests/*", "examples/*", "Doxyfile", "version.h.in"
    options = {
        "shared" : [True, False, None],
        "examples" : [True, False, None],
        "tests" : [True, False, None],
        "docs" : [True, False, None]
    }
    # If specified None the default values from CMakeLists will be used
    default_options = {
        "shared" : None,
        "examples" : None,
        "tests" : None,
        "docs" : None
    }

    def set_version(self):
        git = tools.Git(folder=self.recipe_folder)
        self.version = git.get_branch()

    def configure_cmake(self):
        cmake = CMake(self)
        # TODO: This might be removed from CMakeLists and BUILD_SHARED_LIBS might be used instead https://docs.conan.io/en/latest/reference/build_helpers/cmake.html#definitions
        if self.options.shared != None:
            if self.options.shared:
                cmake.definitions["SOCKPP_BUILD_SHARED"] = "ON"
                cmake.definitions["SOCKPP_BUILD_STATIC"] = "OFF"
            else:
                cmake.definitions["SOCKPP_BUILD_SHARED"] = "OFF"
                cmake.definitions["SOCKPP_BUILD_STATIC"] = "ON"

        if self.options.examples != None:
            cmake.definitions["SOCKPP_BUILD_EXAMPLES"] = "ON" if self.options.examples else "OFF"

        if self.options.tests != None:
            cmake.definitions["SOCKPP_BUILD_TESTS"] = "ON" if self.options.tests else "OFF"

        if self.options.docs != None:
            cmake.definitions["SOCKPP_BUILD_DOCUMENTATION"] = "ON" if self.options.docs else "OFF"

        cmake.configure()
        return cmake

    def build(self):
        cmake = self.configure_cmake()
        cmake.build()

    def package(self):
        cmake = self.configure_cmake()
        cmake.install()

    def package_info(self):
        self.cpp_info.includedirs = ["include"]
        self.cpp_info.libdirs = ["lib"]
        if self.settings.os == "Windows":
            self.cpp_info.libs = ["sockpp-static"]
            self.cpp_info.system_libs = ["ws2_32"]
        if self.settings.os == "Linux":
            self.cpp_info.libs = ["sockpp"]
