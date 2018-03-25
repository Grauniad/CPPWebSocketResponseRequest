//
// Created by lhumphreys on 31/03/18.
//

#ifndef CPPWEBSOCKETRESPONSEREQUEST_IMESSAGECONSUMER_H
#define CPPWEBSOCKETRESPONSEREQUEST_IMESSAGECONSUMER_H

#include <string>

class IOneShotConnectionConsumer {
public:

    /*
    * Invoked on the IOService thread to notify the consumer that
    * the connection is now open, and ready to send messages.
    *
    * @return The initial payload to send.
    */
    virtual const std::string& onOpen () = 0;

    /*
     * Invoked on the IOService thread to notify the consumer that a
     * new message has been received.
     */
    virtual void onMessage (std::string) = 0;

    /*
     * Invoked on the IOService thread to notify the consumer that
     * the session has been terminated early. No further callbacks
     * will be triggered.
     */
    virtual void onTerminate () = 0;

    /*
     * Invoked on the IOService thread to notify the consumer that
     * the session has been completed normally. No further callbacks
     * will be triggered..
     *
     * NOTE: This does *not* mean a message has been received. If some
     *       kind of in flight disconnect occurred, this will
     *       result in an onComplete being triggered without
     *       a onMessage
     *
     * No further callbacks will be received.
     */
    virtual void onComplete () = 0;

    /*
     * Invoked on the IOService thread to notify the consumer that
     * the request has been abandoned due to it being invalid
     *
     * No further callbacks will be received.
     */
    virtual void onInvalidRequest (std::string errMsg) = 0;
};
#endif //CPPWEBSOCKETRESPONSEREQUEST_IMESSAGECONSUMER_H
