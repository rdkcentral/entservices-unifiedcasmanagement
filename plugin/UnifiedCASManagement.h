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

#ifndef UNIFIEDCASMANAGEMENT_H
#define UNIFIEDCASMANAGEMENT_H

#include "Module.h"
#include "MediaPlayer.h"
#include <interfaces/IUnifiedCASManagement.h>
#include <interfaces/json/JUnifiedCASManagement.h>
#include <interfaces/json/JsonData_UnifiedCASManagement.h>

namespace WPEFramework 
{

namespace Plugin 
{

class UnifiedCASManagement : public PluginHost::IPlugin, public PluginHost::JSONRPC 
{
    private:
        class Notification : public RPC::IRemoteConnection::INotification, public Exchange::IUnifiedCASManagement::INotification
        {
            private:
                Notification() = delete;
                Notification(const Notification&) = delete;
                Notification& operator=(const Notification&) = delete;
            public:
                explicit Notification(UnifiedCASManagement* parent)
                    : _parent(*parent)
                {
                    ASSERT(parent != nullptr);
                }

                virtual ~Notification()
                {
                }

                BEGIN_INTERFACE_MAP(Notification)
                    INTERFACE_ENTRY(Exchange::IUnifiedCASManagement::INotification)
                    INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
                END_INTERFACE_MAP

                void Activated(RPC::IRemoteConnection*) override
                {
                }

                void Deactivated(RPC::IRemoteConnection *connection) override
                {
                    _parent.Deactivated(connection);
                }

                void OnDataReceived(const string& payload, const Exchange::IUnifiedCASManagement::DataSource& source) override
                {
                    Exchange::JUnifiedCASManagement::Event::OnDataReceived(_parent, payload, source);
                }

            private:
                UnifiedCASManagement& _parent;
        };

    public:
        UnifiedCASManagement(const UnifiedCASManagement&) = delete;
        UnifiedCASManagement& operator=(const UnifiedCASManagement&) = delete;

        UnifiedCASManagement();
        virtual ~UnifiedCASManagement();

        BEGIN_INTERFACE_MAP(UnifiedCASManagement)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::IUnifiedCASManagement, _UnifiedCASManagement)
        END_INTERFACE_MAP

        //  IPlugin methods
        // ------------------------------------------------------------------------------------------
        const string Initialize(PluginHost::IShell* service) override;
        void Deinitialize(PluginHost::IShell* service) override;
        string Information() const override;

    public:
        // Public method to trigger event notifications
        void event_data(const string& payload, const Exchange::IUnifiedCASManagement::DataSource& source);

    private:
        void Deactivated(RPC::IRemoteConnection* connection);

    private:
        PluginHost::IShell* _service{};
        uint32_t _connectionId{};
        Exchange::IUnifiedCASManagement* _UnifiedCASManagement{};
        Core::Sink<Notification> _UnifiedCASManagementNotification;    

        
};
    
} // namespace Plugin

} // namespace WPEFramework
#endif /* UNIFIEDCASMANAGEMENT_H */

