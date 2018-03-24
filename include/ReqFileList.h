/*
 * ReqFileList.h
 *
 *  Created on: 29 Oct 2015
 *      Author: lhumphreys
 */

#ifndef DEV_TOOLS_CPP_LIBRARIES_LIBWEBSOCKETS_REQFILELIST_H_
#define DEV_TOOLS_CPP_LIBRARIES_LIBWEBSOCKETS_REQFILELIST_H_

#include "ReqServer.h"
#include <SimpleJSON.h>

/*
 * Request the list of files matching a glob pattern.
 *
 * REQUEST:
 *    {
 *         pattern: "*.out",
 *         prefix: ""
 *    }
 *
 * REPLY:
 *    {
 *         files: [
 *            "file1.out",
 *            "file2.out"
 *         ]
 *    }
 *
 *
 * WARNING: The use of this query has obvious security issues, however it is
 *           assumed the user already has access to the PC, and that this code
 *           is not running on a web server...
 */
class ReqFileList: public RequestReplyHandler {
public:
    ReqFileList();

    virtual std::string OnRequest(const char* req);

    virtual ~ReqFileList();
private:
    NewStringField(pattern);
    NewStringField(prefix);
    NewStringArrayField(files);
    typedef SimpleParsedJSON<pattern,prefix> Request;
    typedef SimpleParsedJSON<files> Reply;

    Request request;
    Reply   reply;
};

#endif /* DEV_TOOLS_CPP_LIBRARIES_LIBWEBSOCKETS_REQFILELIST_H_ */
