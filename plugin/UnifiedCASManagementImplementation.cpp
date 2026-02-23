/**
* If not stated otherwise in this file or this component's LICENSE
* file the following copyright and licenses apply:
*
* Copyright 2026 RDK Management
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

#include <stdlib.h>
#include <errno.h>
#include <string>
#include <iomanip>
#include <iostream>
#include <algorithm>

#include "UnifiedCASManagementImplementation.h"
#ifdef LMPLAYER_FOUND
#include "LibMediaPlayerImpl.h"
#endif
#include "UtilsLogging.h"

#ifdef ENABLE_DEBUG
#define DBGINFO(fmt, ...) LOGINFO(fmt, ##__VA_ARGS__)
#else
#define DBGINFO(fmt, ...)
#endif

namespace WPEFramework {
    namespace Plugin {

        SERVICE_REGISTRATION(UnifiedCASManagementImplementation, 1, 0);

    UnifiedCASManagementImplementation::UnifiedCASManagementImplementation()
        : _adminLock()
        , _notifications()
    {
        LOGINFO("UnifiedCASManagementImplementation Constructor");

#ifdef LMPLAYER_FOUND
        _player = std::make_shared<LibMediaPlayerImpl>(this);
#else
        _player = nullptr;
        LOGERR("NO VALID PLAYER AVAILABLE TO USE");
#endif
    }

    UnifiedCASManagementImplementation::~UnifiedCASManagementImplementation() 
    {
        LOGINFO("UnifiedCASManagementImplementation Destructor");
    }

    /* virtual */ Core::hresult UnifiedCASManagementImplementation::Manage(
        const string& mediaurl, 
        const TuneMode& mode, 
        const ManagementType& managementType, 
        const string& casinitdata, 
        const string& casocdmid,
        bool& success) /* override */
    {
        LOGINFO("Manage: mediaurl=%s, casocdmid=%s", mediaurl.c_str(), casocdmid.c_str());
        
        Core::hresult result = Core::ERROR_NONE;
        success = false;

        _adminLock.Lock();

        try {
            if (nullptr == _player) {
                LOGERR("NO VALID PLAYER AVAILABLE TO USE");
            }
            else if (mode != TuneMode::MODE_NONE) {
                LOGERR("mode must be MODE_NONE for CAS Management");
            }
            else if (managementType != ManagementType::MANAGE_FULL && managementType != ManagementType::MANAGE_NO_PSI && managementType != ManagementType::MANAGE_NO_TUNER) {
                LOGERR("managementType must be MANAGE_FULL, MANAGE_NO_PSI or MANAGE_NO_TUNER for CAS Management");
            }
            else if (casocdmid.empty()) {
                LOGERR("casocdmid is mandatory for CAS management session");
            }
            else {
                // Create JSON parameters for MediaPlayer 
                JsonObject jsonParams;
                jsonParams["mediaurl"] = mediaurl;
                jsonParams["mode"] = Core::EnumerateType<TuneMode>(mode).Data();
                jsonParams["manage"] = Core::EnumerateType<ManagementType>(managementType).Data();
                jsonParams["casinitdata"] = casinitdata;
                jsonParams["casocdmid"] = casocdmid;

                std::string openParams;
                jsonParams.ToString(openParams);
                LOGINFO("OpenData = %s", openParams.c_str());

                if (false == _player->openMediaPlayer(openParams, Core::EnumerateType<ManagementType>(managementType).Data())) {
                    LOGERR("Failed to open MediaPlayer");
                } else {
                    LOGINFO("Successfully opened MediaPlayer for CAS management");
                    success = true;  // Set output parameter
                }
            }
        } catch (const std::exception& e) {
            LOGERR("Exception in Manage: %s", e.what());
        }

        _adminLock.Unlock();

        return result;
    }

    /* virtual */ Core::hresult UnifiedCASManagementImplementation::Unmanage(bool& success) /* override */
    {
        LOGINFO("Unmanage: Destroying CAS management session");
        
        Core::hresult result = Core::ERROR_NONE;
        success = false;

        _adminLock.Lock();

        try {
            if (nullptr == _player) {
                LOGERR("NO VALID PLAYER AVAILABLE TO USE");
            }
            else {
                if (false == _player->closeMediaPlayer()) {
                    LOGERR("Failed to close MediaPlayer");
                    LOGWARN("Error in destroying CAS Management Session...");
                } else {
                    LOGINFO("Successful in destroying CAS Management Session...");
                    success = true;  // Set output parameter
                }
            }
        } catch (const std::exception& e) {
            LOGERR("Exception in Unmanage: %s", e.what());
        }

        _adminLock.Unlock();

        return result;
    }

    /* virtual */ Core::hresult UnifiedCASManagementImplementation::Send(const string& payload, 
                                                                        const DataSource& source,
                                                                        bool& success) /* override */
    {
        LOGINFO("Send: payload=%s", payload.c_str());
        
        Core::hresult result = Core::ERROR_NONE;
        success = false;

        _adminLock.Lock();

        try {
            if (nullptr == _player) {
                LOGERR("NO VALID PLAYER AVAILABLE TO USE");
            }
            else {
                // Create JSON parameters for MediaPlayer
                JsonObject jsonParams;
                jsonParams["payload"] = payload;
                jsonParams["source"] = Core::EnumerateType<DataSource>(source).Data();

                std::string data;
                jsonParams.ToString(data);
                LOGINFO("Send Data = %s", data.c_str());

                if (false == _player->requestCASData(data)) {
                    LOGERR("requestCASData failed");
                } else {
                    LOGINFO("UnifiedCASManagement send Data succeeded.. Calling Play");
                    success = true;  // Set output parameter
                }
            }
        } catch (const std::exception& e) {
            LOGERR("Exception in Send: %s", e.what());
        }

        _adminLock.Unlock();

        return result;
    }

    // Notification management
    /* virtual */ Core::hresult UnifiedCASManagementImplementation::Register(Exchange::IUnifiedCASManagement::INotification* notification) /* override */
    {
        LOGINFO("Register notification callback");
        
        Core::hresult result = Core::ERROR_GENERAL;

        ASSERT(nullptr != notification);

        _adminLock.Lock();

        auto it = std::find(_notifications.begin(), _notifications.end(), notification);
        if (it == _notifications.end()) {
            _notifications.push_back(notification);
            notification->AddRef();
            result = Core::ERROR_NONE;
            LOGINFO("Notification callback registered successfully");
        } else {
            result = Core::ERROR_ALREADY_CONNECTED;
            LOGWARN("Notification callback already registered");
        }

        _adminLock.Unlock();

        return result;
    }

    /* virtual */ Core::hresult UnifiedCASManagementImplementation::Unregister(Exchange::IUnifiedCASManagement::INotification* notification) /* override */
    {
        LOGINFO("Unregister notification callback");
        
        Core::hresult result = Core::ERROR_GENERAL;

        ASSERT(nullptr != notification);

        _adminLock.Lock();

        auto it = std::find(_notifications.begin(), _notifications.end(), notification);
        if (it != _notifications.end()) {
            (*it)->Release();
            _notifications.erase(it);
            result = Core::ERROR_NONE;
            LOGINFO("Notification callback unregistered successfully");
        } else {
            result = Core::ERROR_UNKNOWN_KEY;
            LOGWARN("Notification callback not found for unregistration");
        }

        _adminLock.Unlock();

        return result;
    }

    // Event notification method - called by MediaPlayer or internal events
    void UnifiedCASManagementImplementation::event_data(const string& payload, const DataSource& source)
    {
        LOGINFO("event_data: payload=%s", payload.c_str());

        // Create a local copy of notifications to avoid holding lock during callbacks
        // This prevents potential deadlocks if callbacks try to register/unregister
        std::list<Exchange::IUnifiedCASManagement::INotification*> notificationsCopy;
        
        _adminLock.Lock();
        notificationsCopy = _notifications;
        
        // AddRef all notifications while holding the lock to ensure they remain valid
        for (auto& notification : notificationsCopy) {
            if (notification != nullptr) {
                notification->AddRef();
            }
        }
        _adminLock.Unlock();

        // Notify all registered COM-RPC clients without holding the lock
        for (auto& notification : notificationsCopy) {
            if (notification != nullptr) {
                notification->OnDataReceived(payload, source);
                notification->Release();
            }
        }

        LOGINFO("Event dispatched to %zu registered clients", notificationsCopy.size());
    }

    } // namespace Plugin
} // namespace WPEFramework
