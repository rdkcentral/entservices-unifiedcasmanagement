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

#ifndef UNIFIEDCASMANAGEMENTIMPLEMENTATION_H
#define UNIFIEDCASMANAGEMENTIMPLEMENTATION_H

#include "Module.h"
#include "MediaPlayer.h"

#include <com/com.h>
#include <core/core.h>
#include <plugins/plugins.h>
#include <interfaces/Ids.h>
#include <interfaces/IUnifiedCASManagement.h>
#include "tracing/Logging.h"

namespace WPEFramework {
    namespace Plugin {
        class UnifiedCASManagementImplementation : public Exchange::IUnifiedCASManagement {
            public:
                // do not allow this plugin to be copied !!
                UnifiedCASManagementImplementation();
                ~UnifiedCASManagementImplementation() override;

                // do not allow this plugin to be copied !!
                UnifiedCASManagementImplementation(const UnifiedCASManagementImplementation&) = delete;
                UnifiedCASManagementImplementation& operator=(const UnifiedCASManagementImplementation&) = delete;

                BEGIN_INTERFACE_MAP(UnifiedCASManagementImplementation)
                    INTERFACE_ENTRY(Exchange::IUnifiedCASManagement)
                END_INTERFACE_MAP

            public:
                virtual Core::hresult Register(Exchange::IUnifiedCASManagement::INotification *notification) override;
                virtual Core::hresult Unregister(Exchange::IUnifiedCASManagement::INotification *notification) override;

                //Begin methods
                Core::hresult Manage(const string& mediaurl, 
                                   const TuneMode& mode, 
                                   const ManagementType& managementType, 
                                   const string& casinitdata, 
                                   const string& casocdmid,
                                   bool& success) override;
                Core::hresult Unmanage(bool& success) override;
                Core::hresult Send(const string& payload, const DataSource& source, bool& success) override;
                //End methods

                // Event notification method - fires data events to registered COM-RPC clients
                void event_data(const string& payload, const DataSource& source);

#ifdef UNIT_TESTING
                // Test helper method to inject mock MediaPlayer
                void SetMediaPlayer(std::shared_ptr<MediaPlayer> player) {
                    _player = player;
                }
#endif

            private:
                mutable Core::CriticalSection _adminLock;
                std::list<Exchange::IUnifiedCASManagement::INotification*> _notifications;
                std::shared_ptr<MediaPlayer> _player;
};

} // namespace Plugin
} // namespace WPEFramework

#endif // UNIFIEDCASMANAGEMENTIMPLEMENTATION_H
