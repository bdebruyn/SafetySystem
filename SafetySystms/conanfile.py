#!/usr/bin/python
from Azul3DConan import Azul3DConan

class SEIConan(Azul3DConan):
    name = "SEI"
    version = "1.0.0"

    def requirements(self):
        # WARNNING! DO NOT REMOVE! Beginning of Protected Section
        self.preRequirements(self)
        # WARNNING! DO NOT REMOVE! End of Protected Section

        # self.repos.requires("Framework/0.0.1@local/stable")

        if not self.options.sdk:
            pass
        else:
            pass

    def package(self):
        # WARNNING! DO NOT REMOVE! Beginning of Protected Section
        self.prePackage()
        # WARNNING! DO NOT REMOVE! End of Protected Section

        # self.build_folder = self.getBuildFolder()
        # self.copy("*", dst="bin", src=self.build_folder + "/bin", keep_path=False)
        # self.copy("*", dst="lib", src=self.build_folder + "/lib", keep_path=False)
        # self.copy("*.h", dst="include", src="src/PrinterNetworkFramework/include", keep_path=True)

    def package_info(self):
        pass
        # self.cpp_info.include = ["include"]
        # self.cpp_info.libs = [
        #         "PrinterNetworkFramework",
        #         "PrinterNetworkFrameworkMocks"
        #         ]

