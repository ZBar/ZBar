from conans import ConanFile, CMake
import os

class ZBarTestConan(ConanFile):
    settings = 'os', 'compiler', 'build_type', 'arch'
    generators = 'cmake'

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def test(self):
        qr_code_sample_path = '../../sample_matrix.bin'

        print(os.curdir)
        self.run(os.path.join(os.curdir, 'bin', 'testApp') + ' ' + qr_code_sample_path)
