from conan import ConanFile
from conan.tools.cmake import CMakeToolchain, CMakeDeps, CMake, cmake_layout


class AmbilinkConan(ConanFile):
    settings = 'os', 'compiler', 'build_type', 'arch'

    tool_requires = ['ninja/1.11.1', 'cmake/3.25.3']

    requires = [
        'nng/1.5.2',
        'spdlog/1.10.0',
        'fmt/8.1.1',
        'openblas/0.3.17',
        'fftw/3.3.9',
        'glm/cci.20220420',
    ]

    cmake_generator = 'Ninja Multi-Config'

    def imports(self):
        def copy_everything(dir):
            self.copy("*", dst=f"./third-party/{dir}", src=dir)

        copy_everything('include')
        copy_everything('lib')
        copy_everything('licenses')

    def layout(self):
        cmake_layout(self, build_folder='build',
                     generator=self.cmake_generator)

    def generate(self):
        tc = CMakeToolchain(self, self.cmake_generator)
        tc.generate()
        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
