/*
 * ReqFileList.cpp
 *
 *  Created on: 29 Oct 2015
 *      Author: lhumphreys
 */

#include "ReqFileList.h"
#include <OSTools.h>

ReqFileList::ReqFileList() {
}

std::string ReqFileList::OnRequest(const char* req) {
    static std::string error;
    request.Clear();
    reply.Clear();

    if ( !request.Parse(req,error) ) {
        throw InvalidRequestException{0,error};
    }

    if ( request.Get<pattern>() == "")
    {
        throw InvalidRequestException{0,"No pattern provided!"};
    }

    reply.Get<files>() = OS::Glob(request.Get<pattern>());

    if ( request.Get<prefix>() != "")
    {
        for (std::string& file: reply.Get<files>()) {
            file = request.Get<prefix>()  + file;
        }
    }

    return reply.GetJSONString();
}

ReqFileList::~ReqFileList() {
}

