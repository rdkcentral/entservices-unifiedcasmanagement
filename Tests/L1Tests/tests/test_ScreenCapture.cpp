/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2024 RDK Management
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
**/

#include <gtest/gtest.h>
#include <iostream> 
#include <string>
#include <vector>
#include <cstdio>
#include "COMLinkMock.h"
#include "WrapsMock.h"
#include "IarmBusMock.h"
#include "WorkerPoolImplementation.h"
#include "RfcApiMock.h"

#include "ScreenCapture.h"
#include "ScreenCaptureImplementation.h"

#include "FactoriesImplementation.h"
#include "ServiceMock.h"
#include "DRMScreenCaptureMock.h"
#include "ThunderPortability.h"

using namespace WPEFramework;

using ::testing::NiceMock;

class ScreenCaptureTest : public ::testing::Test {
protected:
    Core::ProxyType<Plugin::ScreenCapture> plugin;
    Core::JSONRPC::Handler& handler;
    DECL_CORE_JSONRPC_CONX connection;
    Core::JSONRPC::Message message;
    string response;
    
    WrapsImplMock  *p_wrapsImplMock   = nullptr ;
    Core::ProxyType<Plugin::ScreenCaptureImplementation> ScreenCaptureImpl;
    NiceMock<COMLinkMock> comLinkMock;
    NiceMock<ServiceMock> service;
    PLUGINHOST_DISPATCHER* dispatcher;
    Core::ProxyType<WorkerPoolImplementation> workerPool;
    NiceMock<FactoriesImplementation> factoriesImplementation;

    ScreenCaptureTest()
        : plugin(Core::ProxyType<Plugin::ScreenCapture>::Create())
        , handler(*(plugin))
        , INIT_CONX(1, 0)
        , workerPool(Core::ProxyType<WorkerPoolImplementation>::Create(
            2, Core::Thread::DefaultStackSize(), 16))
    {
	p_wrapsImplMock  = new testing::NiceMock <WrapsImplMock>;
    	Wraps::setImpl(p_wrapsImplMock);
        
        ON_CALL(service, COMLink())
            .WillByDefault(::testing::Invoke(
                  [this]() {
                        TEST_LOG("Pass created comLinkMock: %p ", &comLinkMock);
                        return &comLinkMock;
                    }));

#ifdef USE_THUNDER_R4
        ON_CALL(comLinkMock, Instantiate(::testing::_, ::testing::_, ::testing::_))
			.WillByDefault(::testing::Invoke(
                  [&](const RPC::Object& object, const uint32_t waitTime, uint32_t& connectionId) {
                        ScreenCaptureImpl = Core::ProxyType<Plugin::ScreenCaptureImplementation>::Create();
                        TEST_LOG("Pass created ScreenCaptureImpl: %p &ScreenCaptureImpl: %p", ScreenCaptureImpl, &ScreenCaptureImpl);
                        return &ScreenCaptureImpl;
                    }));
#else
	  ON_CALL(comLinkMock, Instantiate(::testing::_, ::testing::_, ::testing::_, ::testing::_, ::testing::_))
	    .WillByDefault(::testing::Return(ScreenCaptureImpl));
#endif /*USE_THUNDER_R4 */

        PluginHost::IFactories::Assign(&factoriesImplementation);

        Core::IWorkerPool::Assign(&(*workerPool));
        workerPool->Run();

        dispatcher = static_cast<PLUGINHOST_DISPATCHER*>(
        plugin->QueryInterface(PLUGINHOST_DISPATCHER_ID));
        dispatcher->Activate(&service);

        EXPECT_EQ(string(""), plugin->Initialize(&service));	    
    }
    virtual ~ScreenCaptureTest() override
    {
        TEST_LOG("ScreenCaptureTest Destructor");

        plugin->Deinitialize(&service);

        dispatcher->Deactivate();
        dispatcher->Release();

        Core::IWorkerPool::Assign(nullptr);
        workerPool.Release();

        Wraps::setImpl(nullptr);
        if (p_wrapsImplMock != nullptr)
        {
            delete p_wrapsImplMock;
            p_wrapsImplMock = nullptr;
        }

        PluginHost::IFactories::Assign(nullptr);
    }
};

class ScreenCaptureDRMTest : public ScreenCaptureTest {
protected:
    NiceMock<DRMScreenCaptureApiImplMock> drmScreenCaptureApiImplMock;
    RfcApiImplMock    *p_rfcApiImplMock  = nullptr;

    ScreenCaptureDRMTest()
        : ScreenCaptureTest()
    {
        DRMScreenCaptureApi::getInstance().impl = &drmScreenCaptureApiImplMock;
        p_rfcApiImplMock = new NiceMock<RfcApiImplMock>;
        RfcApi::setImpl(p_rfcApiImplMock);
    }
    virtual ~ScreenCaptureDRMTest() override
    {
        RfcApi::setImpl(nullptr);
        if (p_rfcApiImplMock != nullptr) {
            delete p_rfcApiImplMock;
            p_rfcApiImplMock = nullptr;
        }
        DRMScreenCaptureApi::getInstance().impl = nullptr;
    }
};


TEST_F(ScreenCaptureTest, RegisteredMethods)
{
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("uploadScreenCapture")));
    EXPECT_EQ(Core::ERROR_NONE, handler.Exists(_T("sendScreenshot")));
}

TEST_F(ScreenCaptureDRMTest, Upload)
{   
    DRMScreenCapture drmHandle = {0, 1280, 720, 5120, 32};
    uint8_t* buffer = (uint8_t*) malloc(5120 * 720);
    memset(buffer, 0xff, 5120 * 720);

    Core::Event uploadComplete(false, true);

    EXPECT_CALL(drmScreenCaptureApiImplMock, Init())
        .Times(1)
        .WillOnce(
            ::testing::Return(&drmHandle));

    ON_CALL(drmScreenCaptureApiImplMock, GetScreenInfo(::testing::_))
        .WillByDefault(
            ::testing::Return(true));

    ON_CALL(drmScreenCaptureApiImplMock, ScreenCapture(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(
            ::testing::Invoke(
            [&](DRMScreenCapture* handle, uint8_t* output, uint32_t size) {
                memcpy(output, buffer, size);
                return true;
            }));

    EXPECT_CALL(drmScreenCaptureApiImplMock, Destroy(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(true));

    
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));

                EXPECT_EQ(text, string(_T(
                	"{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.ScreenCapture.uploadComplete\",\"params\":{\"status\":true,\"message\":\"Success\",\"call_guid\":\"\"}}"
                )));

                uploadComplete.SetEvent();

                return Core::ERROR_NONE;
            }));

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    ASSERT_TRUE(sockfd != -1);
    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_port = htons(11111);
    ASSERT_FALSE(bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0);
    ASSERT_FALSE(listen(sockfd, 10) < 0);

    std::thread thread = std::thread([&]() {
        auto addrlen = sizeof(sockaddr);
        const int connection = accept(sockfd, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);
        ASSERT_FALSE(connection < 0);
        char buffer[2048] = { 0 };
        ssize_t bytesRead = read(connection, buffer, 2048);
        ASSERT_TRUE(bytesRead > 0);

        std::string reqHeader(buffer, bytesRead);
        EXPECT_TRUE(std::string::npos != reqHeader.find("Content-Type: image/png"));

        std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        ssize_t bytesSent = send(connection, response.c_str(), response.size(), 0);

        close(connection);
        ASSERT_TRUE(bytesSent > 0);
    });

    EVENT_SUBSCRIBE(0, _T("uploadComplete"), _T("org.rdk.ScreenCapture"), message);

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("uploadScreenCapture"), _T("{\"url\":\"http://127.0.0.1:11111\"}"), response));
    EXPECT_EQ(response, _T("{\"success\":true}"));

    EXPECT_EQ(Core::ERROR_NONE, uploadComplete.Lock());

    EVENT_UNSUBSCRIBE(0, _T("uploadComplete"), _T("org.rdk.ScreenCapture"), message);

    free(buffer);
    close(sockfd);
    thread.join();
}

TEST_F(ScreenCaptureDRMTest, SendScreenshot)
{   
    DRMScreenCapture drmHandle = {0, 1280, 720, 5120, 32};
    uint8_t* buffer = (uint8_t*) malloc(5120 * 720);
    memset(buffer, 0xff, 5120 * 720);

    Core::Event uploadComplete(false, true);

    EXPECT_CALL(drmScreenCaptureApiImplMock, Init())
        .Times(1)
        .WillOnce(
            ::testing::Return(&drmHandle));

    ON_CALL(drmScreenCaptureApiImplMock, GetScreenInfo(::testing::_))
        .WillByDefault(
            ::testing::Return(true));

    ON_CALL(drmScreenCaptureApiImplMock, ScreenCapture(::testing::_, ::testing::_, ::testing::_))
        .WillByDefault(
            ::testing::Invoke(
            [&](DRMScreenCapture* handle, uint8_t* output, uint32_t size) {
                memcpy(output, buffer, size);
                return true;
            }));

    EXPECT_CALL(drmScreenCaptureApiImplMock, Destroy(::testing::_))
        .Times(1)
        .WillOnce(::testing::Return(true));

    
    EXPECT_CALL(service, Submit(::testing::_, ::testing::_))
        .Times(1)
        .WillOnce(::testing::Invoke(
            [&](const uint32_t, const Core::ProxyType<Core::JSON::IElement>& json) {
                string text;
                EXPECT_TRUE(json->ToString(text));

                EXPECT_EQ(text, string(_T(
                    "{\"jsonrpc\":\"2.0\",\"method\":\"org.rdk.ScreenCapture.uploadComplete\",\"params\":{\"status\":true,\"message\":\"Success\",\"call_guid\":\"test-guid-123\"}}"
                )));

                uploadComplete.SetEvent();

                return Core::ERROR_NONE;
            }));

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        free(buffer);
        FAIL() << "Socket creation failed";
        return;
    }
    sockaddr_in sockaddr;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_port = htons(11112);
    if (bind(sockfd, (struct sockaddr*)&sockaddr, sizeof(sockaddr)) < 0) {
        close(sockfd);
        free(buffer);
        FAIL() << "Socket bind failed";
        return;
    }
    if (listen(sockfd, 10) < 0) {
        close(sockfd);
        free(buffer);
        FAIL() << "Socket listen failed";
        return;
    }

    std::thread thread = std::thread([&]() {
        auto addrlen = sizeof(sockaddr);
        const int connection = accept(sockfd, (struct sockaddr*)&sockaddr, (socklen_t*)&addrlen);
        if (connection < 0) {
			close(sockfd);
            return;
        }
        char buffer[2048] = { 0 };
        ssize_t bytesRead = read(connection, buffer, 2048);
        if (bytesRead <= 0) {
            close(connection);
			close(sockfd);
            return;
        }

        std::string reqHeader(buffer, bytesRead);
        EXPECT_TRUE(std::string::npos != reqHeader.find("Content-Type: image/png"));

        std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
        ssize_t bytesSent = send(connection, response.c_str(), response.size(), 0);

        close(connection);
        if (bytesSent <= 0) {
			close(sockfd);
            return;
        }
    });

    EVENT_SUBSCRIBE(0, _T("uploadComplete"), _T("org.rdk.ScreenCapture"), message);

    // Mock RFC responses for enable and URL
    ON_CALL(*p_rfcApiImplMock, getRFCParameter(::testing::_, ::testing::StrEq("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ScreenCapture.Enable"), ::testing::_))
        .WillByDefault(::testing::Invoke(
            [](const char*, const char*, RFC_ParamData_t* param) {
                strcpy(param->value, "true");
                return WDMP_SUCCESS;
            }));

    ON_CALL(*p_rfcApiImplMock, getRFCParameter(::testing::_, ::testing::StrEq("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ScreenCapture.URL"), ::testing::_))
        .WillByDefault(::testing::Invoke(
            [](const char*, const char*, RFC_ParamData_t* param) {
                strcpy(param->value, "http://127.0.0.1:11112");
                return WDMP_SUCCESS;
            }));

    EXPECT_EQ(Core::ERROR_NONE, handler.Invoke(connection, _T("sendScreenshot"), _T("{\"callGUID\":\"test-guid-123\"}"), response));
    EXPECT_EQ(response, _T("{\"success\":true}"));

    EXPECT_EQ(Core::ERROR_NONE, uploadComplete.Lock());

    EVENT_UNSUBSCRIBE(0, _T("uploadComplete"), _T("org.rdk.ScreenCapture"), message);

    // Cleanup
    if (buffer) {
        free(buffer);
    }
    if (thread.joinable()) {
        thread.join();
    }
    close(sockfd);
}

TEST_F(ScreenCaptureDRMTest, SendScreenshot_EmptyCallGUID)
{
    // Test with empty callGUID - should fail immediately
    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("sendScreenshot"), _T("{}"), response));
}

TEST_F(ScreenCaptureDRMTest, SendScreenshot_RFCEnableKeyFailure)
{
    // Mock RFC Enable key retrieval failure
    EXPECT_CALL(*p_rfcApiImplMock, getRFCParameter(::testing::_, ::testing::StrEq("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ScreenCapture.Enable"), ::testing::_))
        .WillOnce(::testing::Return(WDMP_FAILURE));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("sendScreenshot"), _T("{\"callGUID\":\"test-guid-fail\"}"), response));
}

TEST_F(ScreenCaptureDRMTest, SendScreenshot_RFCDisabled)
{
    // Mock RFC with ScreenCapture disabled (Enable = false)
    EXPECT_CALL(*p_rfcApiImplMock, getRFCParameter(::testing::_, ::testing::StrEq("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ScreenCapture.Enable"), ::testing::_))
        .WillOnce(::testing::Invoke(
            [](const char*, const char*, RFC_ParamData_t* param) {
                strcpy(param->value, "false");
                return WDMP_SUCCESS;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("sendScreenshot"), _T("{\"callGUID\":\"test-guid-disabled\"}"), response));
}

TEST_F(ScreenCaptureDRMTest, SendScreenshot_RFCDisabledCaseInsensitive)
{
    // Test case-insensitive handling of "FALSE"
    EXPECT_CALL(*p_rfcApiImplMock, getRFCParameter(::testing::_, ::testing::StrEq("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ScreenCapture.Enable"), ::testing::_))
        .WillOnce(::testing::Invoke(
            [](const char*, const char*, RFC_ParamData_t* param) {
                strcpy(param->value, "FALSE");
                return WDMP_SUCCESS;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("sendScreenshot"), _T("{\"callGUID\":\"test-guid-disabled-caps\"}"), response));
}

TEST_F(ScreenCaptureDRMTest, SendScreenshot_RFCURLKeyFailure)
{
    // Mock successful Enable but failed URL retrieval
    EXPECT_CALL(*p_rfcApiImplMock, getRFCParameter(::testing::_, ::testing::StrEq("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ScreenCapture.Enable"), ::testing::_))
        .WillOnce(::testing::Invoke(
            [](const char*, const char*, RFC_ParamData_t* param) {
                strcpy(param->value, "true");
                return WDMP_SUCCESS;
            }));

    EXPECT_CALL(*p_rfcApiImplMock, getRFCParameter(::testing::_, ::testing::StrEq("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ScreenCapture.URL"), ::testing::_))
        .WillOnce(::testing::Return(WDMP_FAILURE));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("sendScreenshot"), _T("{\"callGUID\":\"test-guid-no-url\"}"), response));
}

TEST_F(ScreenCaptureDRMTest, SendScreenshot_EmptyURL)
{
    // Mock successful Enable but empty URL value
    EXPECT_CALL(*p_rfcApiImplMock, getRFCParameter(::testing::_, ::testing::StrEq("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ScreenCapture.Enable"), ::testing::_))
        .WillOnce(::testing::Invoke(
            [](const char*, const char*, RFC_ParamData_t* param) {
                strcpy(param->value, "true");
                return WDMP_SUCCESS;
            }));

    EXPECT_CALL(*p_rfcApiImplMock, getRFCParameter(::testing::_, ::testing::StrEq("Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.ScreenCapture.URL"), ::testing::_))
        .WillOnce(::testing::Invoke(
            [](const char*, const char*, RFC_ParamData_t* param) {
                strcpy(param->value, "");
                return WDMP_SUCCESS;
            }));

    EXPECT_EQ(Core::ERROR_GENERAL, handler.Invoke(connection, _T("sendScreenshot"), _T("{\"callGUID\":\"test-guid-empty-url\"}"), response));
}
