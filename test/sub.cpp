#include "gtest/gtest.h"

#include "WorkerThread.h"
#include "ReqServer.h"
#include "io_thread.h"
#include <PipePublisher.h>
#include <stream_client.h>
#include <OpenConnections.h>

const size_t serverPort = 1250;
const std::string serverUri = "ws://localhost:1250";

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class TestHandler: public SubscriptionHandler
{
public:
    static constexpr const char* SUB_TYPE = "SUB_Echo_Test";

    void OnRequest(RequestHandle hdl) override {
        hdl->SendMessage(hdl->RequestMessasge());
        hdls.Add(std::move(hdl));
    }

    void Publish(const std::string& message) {
        hdls.Publish(message);
    }

    static std::shared_ptr<TestHandler> New() {
        return std::make_unique<TestHandler>();
    }

    OpenConnectionsList hdls;
};

class ClientPublisher: public StreamClient {
public:
    ClientPublisher(std::string url, std::string subName, std::string body)
        : StreamClient(std::move(url), std::move(subName), std::move(body)) {}

    void OnMessage(const std::string& msg) override {
        publisher.Publish(msg);
    }

    std::shared_ptr<PipeSubscriber<std::string>> NewClient() {
        return publisher.NewClient(1000);
    }

    void Start() {
        Run();
    }

    using StreamClient::Stop;
private:
    PipePublisher<std::string> publisher;
};

class SubTest: public ::testing::Test {
public:
    SubTest()
       : clientPub(serverUri, TestHandler::SUB_TYPE, "Hello World!")
    {
        handler = TestHandler::New();
        messages = clientPub.NewClient();
    }
    void SetUp() {

        server.AddHandler(TestHandler::SUB_TYPE, handler);
        // Request server's main loop is blocking, start up on a slave thread...
        serverThread.PostTask([&] () -> void {
            server.HandleRequests(serverPort);
        });
        serverThread.Start();

        // wait for the server to spin up...
        server.WaitUntilRunning();

        messages = clientPub.NewClient();
        clientThread.PostTask([&] () -> void {
            clientPub.Start();
        });
        clientThread.Start();
        clientPub.WaitUntilRunning();

        std::string message;
        ASSERT_TRUE(messages->WaitForMessage(message));

        ASSERT_EQ(message, "Hello World!");
    }

    void TearDown () {
        clientPub.Stop();
        clientThread.DoTask([=] () -> void {
        });
    }
protected:
    std::shared_ptr<TestHandler> handler;
    WorkerThread           serverThread;
    RequestServer          server;
    WorkerThread           clientThread;
    ClientPublisher        clientPub;

    std::shared_ptr<PipeSubscriber<std::string>> messages;
};

TEST_F(SubTest, NewMessage) {
    const std::string message = "New Message to send";
    handler->Publish(message);

    std::string got;
    messages->WaitForMessage(got);
    ASSERT_EQ(message, got);
}

TEST_F(SubTest, AbortedClient) {
    clientPub.Stop();
    const std::string message = "New Message to send";
    handler->Publish(message);

    std::string got;
    ASSERT_FALSE(messages->GetNextMessage(got));
}

class SubTestSecondClient: public SubTest {
public:
    SubTestSecondClient()
       : SubTest()
       , clientPub2(serverUri, TestHandler::SUB_TYPE, logonMsg)
    {}

    void SetUp() override {
        SubTest::SetUp();

        messages2 = clientPub2.NewClient();
        clientThread2.PostTask([&] () -> void {
            clientPub2.Start();
        });
        clientThread2.Start();
        clientPub2.WaitUntilRunning();

        std::string got;
        messages2->WaitForMessage(got);
        ASSERT_EQ(logonMsg, got);

        // But don't publish to the original...
        ASSERT_FALSE(messages->GetNextMessage(got));

    }

    void TearDown() override {
        SubTest::TearDown();
        clientPub2.Stop();
        clientThread2.DoTask([=] () -> void {
        });
    }
protected:
    const std::string      logonMsg;
    WorkerThread           clientThread2;
    ClientPublisher        clientPub2;
    std::shared_ptr<PipeSubscriber<std::string>> messages2;
};

TEST_F(SubTestSecondClient, NewMessage) {
    const std::string message = "New Message to send";
    handler->Publish(message);

    std::string got;
    ASSERT_TRUE(messages->WaitForMessage(got));
    ASSERT_EQ(message, got);

    ASSERT_TRUE(messages2->WaitForMessage(got));
    ASSERT_EQ(message, got);
}

TEST_F(SubTestSecondClient, AbortedClient) {
    // Kill only the first client
    clientPub.Stop();

    const std::string message = "New Message to send";
    handler->Publish(message);

    std::string got;
    ASSERT_TRUE(messages2->WaitForMessage(got));
    ASSERT_EQ(message, got);

    ASSERT_FALSE(messages->GetNextMessage(got));
}

class SubTestThirdClient: public SubTestSecondClient {
public:
    SubTestThirdClient()
            : SubTestSecondClient()
            , clientPub3(serverUri, TestHandler::SUB_TYPE, logonMsg3)
    {}

    void SetUp() override {
        SubTestSecondClient::SetUp();

        messages3 = clientPub3.NewClient();
        clientThread3.PostTask([&] () -> void {
            clientPub3.Start();
        });
        clientThread3.Start();
        clientPub3.WaitUntilRunning();

        std::string got;
        messages3->WaitForMessage(got);
        ASSERT_EQ(logonMsg, got);

        // But don't publish to the original...
        ASSERT_FALSE(messages->GetNextMessage(got));

    }

    void TearDown() override {
        SubTestSecondClient::TearDown();
        clientPub3.Stop();
        clientThread3.DoTask([=] () -> void {
        });
    }
protected:
    const std::string      logonMsg3;
    WorkerThread           clientThread3;
    ClientPublisher        clientPub3;
    std::shared_ptr<PipeSubscriber<std::string>> messages3;
};

TEST_F(SubTestThirdClient, NewMessage) {
    const std::string message = "New Message to send";
    handler->Publish(message);

    std::string got;
    ASSERT_TRUE(messages->WaitForMessage(got));
    ASSERT_EQ(message, got);

    ASSERT_TRUE(messages2->WaitForMessage(got));
    ASSERT_EQ(message, got);

    ASSERT_TRUE(messages3->WaitForMessage(got));
    ASSERT_EQ(message, got);
}

TEST_F(SubTestThirdClient, AbortedClient) {
    clientPub2.Stop();
    const std::string message = "New Message to send";
    handler->Publish(message);

    std::string got;
    ASSERT_TRUE(messages->WaitForMessage(got));
    ASSERT_EQ(message, got);

    ASSERT_TRUE(messages3->WaitForMessage(got));
    ASSERT_EQ(message, got);

    ASSERT_FALSE(messages2->GetNextMessage(got));

    const std::string message2 = "Post publish message";
    handler->Publish(message2);
    ASSERT_TRUE(messages->WaitForMessage(got));
    ASSERT_EQ(message2, got);

    ASSERT_TRUE(messages3->WaitForMessage(got));
    ASSERT_EQ(message2, got);

    ASSERT_FALSE(messages2->GetNextMessage(got));
}
