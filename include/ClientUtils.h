//
// Created by lhumphreys on 16/08/2020.
//

#ifndef CPPWEBSOCKETRESPONSEREQUEST_CLIENTUTILS_H
#define CPPWEBSOCKETRESPONSEREQUEST_CLIENTUTILS_H

#include <string>
#include <map>

namespace ClientUtils {
    std::string FileRequestLocal(const std::string& port,
                                 const std::string& reqName,
                                 const std::string& fname);

    std::string JSONRequestLocal(const std::string& port,
                                 const std::string& reqName,
                                 const std::map<std::string, std::string>& values);
}

#endif //CPPWEBSOCKETRESPONSEREQUEST_CLIENTUTILS_H
