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
 
#include "LinearPlaybackControl.h"
#include <interfaces/IConfiguration.h>

#define API_VERSION_NUMBER_MAJOR 1
#define API_VERSION_NUMBER_MINOR 0
#define API_VERSION_NUMBER_PATCH 0

namespace WPEFramework
{
    namespace
    {
        static Plugin::Metadata<Plugin::LinearPlaybackControl> metadata(
            // Version (Major, Minor, Patch)
            API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH,
            // Preconditions
            {},
            // Terminations
            {},
            // Controls
            {});
    }

    namespace Plugin
    {
        /*
         * Register LinearPlaybackControl module as wpeframework plugin
         */
        SERVICE_REGISTRATION(LinearPlaybackControl, API_VERSION_NUMBER_MAJOR, API_VERSION_NUMBER_MINOR, API_VERSION_NUMBER_PATCH);

        LinearPlaybackControl::LinearPlaybackControl()
            : _service(nullptr)
            , _connectionId(0)
            , _linearPlaybackControl(nullptr)
            , _linearPlaybackControlNotification(this)
        {
            syslog(LOG_INFO, "LinearPlaybackControl Constructor");
        }

        LinearPlaybackControl::~LinearPlaybackControl()
        {
            syslog(LOG_INFO, "LinearPlaybackControl Destructor");
        }

        const string LinearPlaybackControl::Initialize(PluginHost::IShell* service)
        {
            string message = "";

            ASSERT(nullptr != service);
            ASSERT(nullptr == _service);
            ASSERT(nullptr == _linearPlaybackControl);
            ASSERT(0 == _connectionId);

            syslog(LOG_INFO, "LinearPlaybackControl::Initialize: PID=%u", getpid());

            _service = service;
            _service->AddRef();
            _service->Register(&_linearPlaybackControlNotification);
            _linearPlaybackControl = _service->Root<Exchange::ILinearPlaybackControl>(_connectionId, 5000, _T("LinearPlaybackControlImplementation"));

            if (nullptr != _linearPlaybackControl)
            {
                auto configConnection = _linearPlaybackControl->QueryInterface<Exchange::IConfiguration>();
                if (configConnection != nullptr)
                {
                    configConnection->Configure(service);
                    configConnection->Release();
                }
                // Register for notifications
                _linearPlaybackControl->Register(&_linearPlaybackControlNotification);
                // Invoking Plugin API register to wpeframework
                Exchange::JLinearPlaybackControl::Register(*this, _linearPlaybackControl);
            }
            else
            {
                syslog(LOG_ERR, "LinearPlaybackControl::Initialize: Failed to initialise LinearPlaybackControl plugin");
                message = _T("LinearPlaybackControl plugin could not be initialised");
            }

            if (0 != message.length())
            {
                Deinitialize(service);
            }

            return message;
        }

        void LinearPlaybackControl::Deinitialize(PluginHost::IShell* service)
        {
            ASSERT(_service == service);

            syslog(LOG_INFO, "LinearPlaybackControl::Deinitialize");

            // Make sure the Activated and Deactivated are no longer called before we start cleaning up..
            if (_service != nullptr)
            {
                _service->Unregister(&_linearPlaybackControlNotification);
            }
            if (nullptr != _linearPlaybackControl)
            {
                _linearPlaybackControl->Unregister(&_linearPlaybackControlNotification);
                Exchange::JLinearPlaybackControl::Unregister(*this);

                // Stop processing:
                RPC::IRemoteConnection* connection = service->RemoteConnection(_connectionId);
                VARIABLE_IS_NOT_USED uint32_t result = _linearPlaybackControl->Release();

                _linearPlaybackControl = nullptr;

                // It should have been the last reference we are releasing,
                // so it should endup in a DESTRUCTION_SUCCEEDED, if not we
                // are leaking...
                ASSERT(result == Core::ERROR_DESTRUCTION_SUCCEEDED);

                // If this was running in a (container) process...
                if (nullptr != connection)
                {
                    // Lets trigger the cleanup sequence for
                    // out-of-process code. Which will guard
                    // that unwilling processes, get shot if
                    // not stopped friendly :-)
                    connection->Terminate();
                    connection->Release();
                }
            }

            _connectionId = 0;

            if (_service != nullptr)
            {
                _service->Release();
                _service = nullptr;
            }
            syslog(LOG_INFO, "LinearPlaybackControl de-initialised");
        }

        string LinearPlaybackControl::Information() const
        {
            return ("This LinearPlaybackControl Plugin facilitates linear playback control functionality");
        }

        void LinearPlaybackControl::Deactivated(RPC::IRemoteConnection* connection)
        {
            if (connection->Id() == _connectionId)
            {
                ASSERT(nullptr != _service);
                Core::IWorkerPool::Instance().Submit(PluginHost::IShell::Job::Create(_service, PluginHost::IShell::DEACTIVATED, PluginHost::IShell::FAILURE));
            }
        }
    } // namespace Plugin
} // namespace WPEFramework
