//
// Created by lhumphreys on 16/08/2020.
//
#include <iostream>
#include <ClientUtils.h>

namespace {
    void UsageAndExit() {
        std::cout << "Load from file: req <portNum> <reqName> <msgFile>" << std::endl;
        std::cout << "Build Json msg: req <portNum> <reqName> -- [<name> <value] [<name2> <value2>] ... " << std::endl;
        exit(1);
    }
}

int main(int argc, const char*  argv[]) {
    const std::string nameValueMarker = "--";
    if (argc < 4) {
        UsageAndExit();
    } else if (argc == 4 && nameValueMarker == argv[3]) {
        const std::string port = argv[1];
        const std::string reqName = argv[2];
        const std::string msgFile = argv[3];

        const std::string response = ClientUtils::FileRequestLocal(port, reqName, msgFile);

        std::cout << response << std::endl;
    } else if (nameValueMarker == argv[3]) {
        const std::string port = argv[1];
        const std::string reqName = argv[2];
        std::map<std::string, std::string> values;
        for (int i = 4; (i+1) < argc; i+=2) {
            values[argv[i]] = argv[i+1];
        }
        const std::string response = ClientUtils::JSONRequestLocal(port, reqName, values);
        std::cout << response << std::endl;
    } else {
        UsageAndExit();
    }

    return 0;
}