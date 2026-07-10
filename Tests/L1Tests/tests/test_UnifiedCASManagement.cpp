/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2022 RDK Management
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
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "UnifiedCASManagement.h"
#include "UnifiedCASManagementImplementation.h"
#include "MediaPlayer.h"

#include "ServiceMock.h"
#include "COMLinkMock.h"

using namespace WPEFramework;
using namespace WPEFramework::Plugin;
using namespace WPEFramework::Exchange;
using ::testing::_;
using ::testing::NiceMock;
using ::testing::Return;
using namespace testing;

class MockMediaPlayer : public MediaPlayer {
public:
    MockMediaPlayer() : MediaPlayer(nullptr) {}

    MOCK_METHOD(bool, openMediaPlayer, (std::string&, const std::string&), (override));
    MOCK_METHOD(bool, closeMediaPlayer, (), (override));
    MOCK_METHOD(bool, requestCASData, (std::string&), (override));
};


class UnifiedCASManagementImplTestable : public UnifiedCASManagementImplementation {
public:
    void AddRef() const override {}
    uint32_t Release() const override { return 0; }

    void SetMockPlayer(std::shared_ptr<MediaPlayer> player){
        SetMediaPlayer(player);
    }
    
    std::string lastPayload;
    IUnifiedCASManagement::DataSource lastSource;

    void EmitTestEvent(const std::string& payload, const IUnifiedCASManagement::DataSource& source) {
        lastPayload = payload;
        lastSource = source;
        event_data(payload, source);
    }
};

class UnifiedCASManagementTest : public ::testing::Test {
protected:
    UnifiedCASManagementImplTestable* plugin;
    NiceMock<MockMediaPlayer>* mockPlayer;
    NiceMock<ServiceMock>* mockService;

    void SetUp() override {
        plugin = new UnifiedCASManagementImplTestable();
        mockService = new NiceMock<ServiceMock>();
        mockPlayer = new NiceMock<MockMediaPlayer>();
    }

    void TearDown() override {
        delete mockPlayer;
        delete mockService;
        delete plugin;
    }
};



TEST_F(UnifiedCASManagementTest, InstanceShouldBeCreated) {
    ASSERT_NE(plugin, nullptr);
}


TEST_F(UnifiedCASManagementTest, Unmanage_ShouldSucceed_WhenPlayerClosesSuccessfully) {
    auto mock = std::make_shared<NiceMock<MockMediaPlayer>>();
    plugin->SetMockPlayer(mock);

    EXPECT_CALL(*mock, closeMediaPlayer())
        .WillOnce(Return(true));

    bool success = false;
    Core::hresult result = plugin->Unmanage(success);
    
    EXPECT_EQ(result, Core::ERROR_NONE);
    EXPECT_TRUE(success);
}

TEST_F(UnifiedCASManagementTest, Unmanage_ShouldFail_WhenPlayerFailsToClose) {
    auto mock = std::make_shared<NiceMock<MockMediaPlayer>>();
    plugin->SetMockPlayer(mock);

    EXPECT_CALL(*mock, closeMediaPlayer())
        .WillOnce(Return(false));

    bool success = false;
    Core::hresult result = plugin->Unmanage(success);
    
    EXPECT_EQ(result, Core::ERROR_NONE);
    EXPECT_FALSE(success);
}

TEST_F(UnifiedCASManagementTest, Unmanage_NoValidPlayer) {
    auto mock = nullptr;
    plugin->SetMockPlayer(mock);

    bool success = false;
    Core::hresult result = plugin->Unmanage(success);
    
    EXPECT_EQ(result, Core::ERROR_NONE);
    EXPECT_FALSE(success);
}



TEST_F(UnifiedCASManagementTest, Manage_WithValidParams_ShouldSucceed) {
    auto mock = std::make_shared<NiceMock<MockMediaPlayer>>();
    plugin->SetMockPlayer(mock);

    EXPECT_CALL(*mock, openMediaPlayer(_, "MANAGE_FULL")).WillOnce(Return(true));

    bool success = false;
    Core::hresult result = plugin->Manage(
        "http://test.stream",
        IUnifiedCASManagement::TuneMode::MODE_NONE,
        IUnifiedCASManagement::ManagementType::MANAGE_FULL,
        "initData",
        "cas123",
        success
    );
    
    EXPECT_EQ(result, Core::ERROR_NONE);
    EXPECT_TRUE(success);
}

TEST_F(UnifiedCASManagementTest, Send_RequestCASDataFails_ShouldReturnError) {
    auto mock = std::make_shared<NiceMock<MockMediaPlayer>>();
    plugin->SetMockPlayer(mock);

    EXPECT_CALL(*mock, requestCASData(_))
        .WillOnce(Return(false));

    bool success = false;
    Core::hresult result = plugin->Send(
        "test_payload",
        IUnifiedCASManagement::DataSource::PRIVATE,
        success
    );
    
    EXPECT_EQ(result, Core::ERROR_NONE);
    EXPECT_FALSE(success);
}


TEST_F(UnifiedCASManagementTest, InterfaceMapTest_IUnifiedCASManagement) {
    Exchange::IUnifiedCASManagement* iface = dynamic_cast<Exchange::IUnifiedCASManagement*>(plugin);
    ASSERT_NE(iface, nullptr);
}

TEST_F(UnifiedCASManagementTest, EventData_EmitsCorrectValues)
{
    std::string payload = "testPayload";
    IUnifiedCASManagement::DataSource source = IUnifiedCASManagement::DataSource::PUBLIC;

    plugin->EmitTestEvent(payload, source);

    EXPECT_EQ(plugin->lastPayload, payload);
    EXPECT_EQ(plugin->lastSource, source);
}


class MediaPlayerTest : public ::testing::Test {
protected:
    void* dummyCasMgmt = reinterpret_cast<void*>(0x1234); // Mock pointer
    MediaPlayer* mediaPlayer;

    void SetUp() override {
        mediaPlayer = new MediaPlayer(dummyCasMgmt);
    }

    void TearDown() override {
        delete mediaPlayer;
    }
};

TEST_F(MediaPlayerTest, OpenMediaPlayerReturnsTrue) {
    std::string params = "init_params";
    std::string sessionType = "test_session";
    EXPECT_TRUE(mediaPlayer->openMediaPlayer(params, sessionType));
}

TEST_F(MediaPlayerTest, CloseMediaPlayerReturnsTrue) {
    EXPECT_TRUE(mediaPlayer->closeMediaPlayer());
}

TEST_F(MediaPlayerTest, RequestCASDataReturnsTrue) {
    std::string data = "get_data_command";
    EXPECT_TRUE(mediaPlayer->requestCASData(data));
}

extern "C" {
    extern const char* MODULE_NAME;
}

TEST(ModuleTest, ModuleNameDeclarationIsAccessible) {
    ASSERT_STREQ(MODULE_NAME, "UnifiedCasManagement");
}

