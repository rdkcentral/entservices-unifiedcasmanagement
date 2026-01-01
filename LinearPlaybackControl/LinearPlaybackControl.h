/*
 * If not stated otherwise in this file or this component's LICENSE file the
 * following copyright and licenses apply:
 *
 * Copyright 2025 RDK Management
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
 
#pragma once

#include "Module.h"
#include <interfaces/json/JsonData_LinearPlaybackControl.h>
#include <interfaces/json/JLinearPlaybackControl.h>
#include <interfaces/ILinearPlaybackControl.h>
#include "tracing/Logging.h"

namespace WPEFramework
{
    namespace Plugin
    {
        class LinearPlaybackControl : public PluginHost::IPlugin, public PluginHost::JSONRPC
        {
        private:
            class Notification : public RPC::IRemoteConnection::INotification,
                                 public Exchange::ILinearPlaybackControl::INotification
            {
            private:
                Notification() = delete;
                Notification(const Notification&) = delete;
                Notification& operator=(const Notification&) = delete;

            public:
                explicit Notification(LinearPlaybackControl* parent)
                    : _parent(*parent)
                {
                    ASSERT(parent != nullptr);
                }

                virtual ~Notification()
                {
                }

                BEGIN_INTERFACE_MAP(Notification)
                INTERFACE_ENTRY(Exchange::ILinearPlaybackControl::INotification)
                INTERFACE_ENTRY(RPC::IRemoteConnection::INotification)
                END_INTERFACE_MAP

                void Activated(RPC::IRemoteConnection*) override
                {
                    syslog(LOG_INFO, "LinearPlaybackControl Notification Activated");
                }

                void Deactivated(RPC::IRemoteConnection* connection) override
                {
                    syslog(LOG_INFO, "LinearPlaybackControl Notification Deactivated");
                    _parent.Deactivated(connection);
                }

                void Speedchanged(const int16_t speed, const uint8_t muxId) override
                {
                    syslog(LOG_INFO, "Speedchanged: speed=%d, muxId=%d", speed, muxId);
                    Exchange::JLinearPlaybackControl::Event::Speedchanged(_parent, speed, muxId);
                }

            private:
                LinearPlaybackControl& _parent;
            };

        public:
            // We do not allow this plugin to be copied !!
            LinearPlaybackControl(const LinearPlaybackControl&) = delete;
            LinearPlaybackControl& operator=(const LinearPlaybackControl&) = delete;

            LinearPlaybackControl();
            virtual ~LinearPlaybackControl();

            BEGIN_INTERFACE_MAP(LinearPlaybackControl)
            INTERFACE_ENTRY(PluginHost::IPlugin)
            INTERFACE_ENTRY(PluginHost::IDispatcher)
            INTERFACE_AGGREGATE(Exchange::ILinearPlaybackControl, _linearPlaybackControl)
            END_INTERFACE_MAP

            //  IPlugin methods
            // -------------------------------------------------------------------------------------------------------
            const string Initialize(PluginHost::IShell* service) override;
            void Deinitialize(PluginHost::IShell* service) override;
            string Information() const override;

        private:
            void Deactivated(RPC::IRemoteConnection* connection);

        private:
            PluginHost::IShell* _service{};
            uint32_t _connectionId{};
            Exchange::ILinearPlaybackControl* _linearPlaybackControl{};
            Core::Sink<Notification> _linearPlaybackControlNotification;
        };

    } // namespace Plugin
} // namespace WPEFramework
