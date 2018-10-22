#include <OpenConnections.h>
#include <OpenConnections.h>

void OpenConnectionsList::Add(SubscriptionHandler::RequestHandle hdl) {
    hdls.emplace_back(hdl);
}

void OpenConnectionsList::Publish(const std::string &msg) {
    if (hdls.size() > 0) {
        std::vector<SubscriptionHandler::RequestHandle> newHdls;
        newHdls.reserve(hdls.size());

        for (auto& hdl: hdls) {
            if (hdl->Ok()) {
                hdl->SendMessage(msg);
                newHdls.emplace_back(std::move(hdl));
            }
        }

        hdls = std::move(newHdls);
    }
}
