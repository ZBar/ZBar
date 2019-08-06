from conans import ConanFile, CMake, tools


class ZBarConan(ConanFile):
    name = 'zbar'
    lib_version = '0.10.0'
    revision = '1'
    version = '{}-{}'.format(lib_version, revision)
    settings = 'os', 'compiler', 'build_type', 'arch'
    description = 'ZBar QR Code Reader'
    url = 'git@github.com:Pix4D/ZBar.git'
    license = 'LGPL LICENSE'
    generators = 'cmake'
    exports = ['Findzbar.cmake']
    exports_sources = [
            'zbar/*',
            'include/*',
            'CMakeLists.txt',
            ] 
    options = {
            'shared' : [True, False],
            }
    default_options = 'shared=False'

    def build(self):
        cmake = CMake(self)
        cmake.definitions['BUILD_SHARED_LIBS'] = 'ON' if self.options.shared else 'OFF'

        cmake.configure(build_folder='build')
        cmake.build(target='install')

    def package_info(self):
        self.cpp_info.libs = ['zbar']

        if self.settings.os == 'Linux' and not self.options.shared:
            self.cpp_info.libs.append('pthread')

