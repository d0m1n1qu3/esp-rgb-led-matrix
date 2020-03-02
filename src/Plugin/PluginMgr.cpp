/* MIT License
 *
 * Copyright (c) 2019 - 2020 Andreas Merkle <web@blue-andi.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*******************************************************************************
    DESCRIPTION
*******************************************************************************/
/**
 * @brief  Plugin manager
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "PluginMgr.h"
#include "DisplayMgr.h"
#include "MyWebServer.h"
#include "RestApi.h"
#include "Settings.h"

#include <Logging.h>
#include <ArduinoJson.h>

/******************************************************************************
 * Compiler Switches
 *****************************************************************************/

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and classes
 *****************************************************************************/

/******************************************************************************
 * Prototypes
 *****************************************************************************/

/******************************************************************************
 * Local Variables
 *****************************************************************************/

/* Initialize plugin manager */
PluginMgr   PluginMgr::m_instance;

/******************************************************************************
 * Public Methods
 *****************************************************************************/

void PluginMgr::registerPlugin(const String& name, IPluginMaintenance::CreateFunc createFunc)
{
    PluginRegEntry* entry = new PluginRegEntry();

    if (nullptr != entry)
    {
        entry->name         = name;
        entry->createFunc   = createFunc;

        if (false == m_registry.append(entry))
        {
            LOG_ERROR("Couldn't add %s to registry.", name.c_str());

            delete entry;
            entry = nullptr;
        }
        else
        {
            LOG_INFO("Plugin %s registered.", name.c_str());
        }
    }
    else
    {
        LOG_ERROR("Couldn't add %s to registry.", name.c_str());
    }

    return;
}

IPluginMaintenance* PluginMgr::install(const String& name, uint8_t slotId)
{
    return install(name, generateUID(), slotId);
}

bool PluginMgr::uninstall(IPluginMaintenance* plugin)
{
    bool status = false;

    if (nullptr != plugin)
    {
        if (false == m_plugins.find(plugin))
        {
            LOG_WARNING("Plugin 0x%X (%s) not found in list.", plugin, plugin->getName());
        }
        else
        {
            status = DisplayMgr::getInstance().uninstallPlugin(plugin);

            if (true == status)
            {
                plugin->unregisterWebInterface(MyWebServer::getInstance());
                m_plugins.removeSelected();
            }
        }
    }

    return status;
}

const char* PluginMgr::findFirst()
{
    const char* name = nullptr;

    if (true == m_registry.selectFirstElement())
    {
        name = (*m_registry.current())->name.c_str();
    }

    return name;
}

const char* PluginMgr::findNext()
{
    const char* name = nullptr;

    if (true == m_registry.next())
    {
        name = (*m_registry.current())->name.c_str();
    }

    return name;
}

String PluginMgr::getRestApiBaseUri(uint16_t uid)
{
    String  baseUri = RestApi::BASE_URI;
    baseUri += "/display";
    baseUri += "/uid/";
    baseUri += uid;

    return baseUri;
}

void PluginMgr::load()
{
    Settings& settings = Settings::getInstance();

    if (false == settings.open(true))
    {
        LOG_WARNING("Couldn't open filesystem.");
    }
    else
    {
        String                  installation    = settings.getPluginInstallation().getValue();
        const size_t            JSON_DOC_SIZE   = 512U;
        DynamicJsonDocument     jsonDoc(JSON_DOC_SIZE);
        DeserializationError    error           = deserializeJson(jsonDoc, installation);

        if (JSON_DOC_SIZE <= jsonDoc.memoryUsage())
        {
            LOG_WARNING("Max. JSON buffer size reached.");
        }

        settings.close();

        if (DeserializationError::Ok != error)
        {
            LOG_WARNING("JSON deserialization failed: %s", error.c_str());
        }
        else
        {
            JsonArray   jsonSlots   = jsonDoc["slots"].as<JsonArray>();
            uint8_t     slotId      = 0;

            for(JsonObject jsonSlot: jsonSlots)
            {
                String      name    = jsonSlot["name"];
                uint16_t    uid     = jsonSlot["uid"];

                if (false == name.isEmpty())
                {
                    IPluginMaintenance* plugin = install(name, uid, slotId);

                    if (nullptr == plugin)
                    {
                        LOG_WARNING("Couldn't install %s (uid %u) in slot %u.", name.c_str(), uid, slotId);
                    }
                    else
                    {
                        plugin->enable();
                    }
                }

                ++slotId;
                if (slotId <= DisplayMgr::MAX_SLOTS)
                {
                    break;
                }
            }
        }
    }
}

void PluginMgr::save()
{
    String              installation;
    uint8_t             slotId      = 0;
    Settings&           settings    = Settings::getInstance();
    const size_t        JSON_DOC_SIZE   = 512U;
    DynamicJsonDocument jsonDoc(JSON_DOC_SIZE);
    JsonArray           jsonSlots   = jsonDoc.createNestedArray("slots");

    for(slotId = 0; slotId < DisplayMgr::MAX_SLOTS; ++slotId)
    {
        IPluginMaintenance* plugin      = DisplayMgr::getInstance().getPluginInSlot(slotId);
        JsonObject          jsonSlot    = jsonSlots.createNestedObject();

        if (nullptr == plugin)
        {
            jsonSlot["name"]    = "";
            jsonSlot["uid"]     = 0;
        }
        else
        {
            jsonSlot["name"]    = plugin->getName();
            jsonSlot["uid"]     = plugin->getUID();
        }
    }

    if (false == settings.open(false))
    {
        LOG_WARNING("Couldn't open filesystem.");
    }
    else
    {
        if (JSON_DOC_SIZE <= jsonDoc.memoryUsage())
        {
            LOG_WARNING("Max. JSON buffer size reached.");
        }

        serializeJson(jsonDoc, installation);

        settings.getPluginInstallation().setValue(installation);
        settings.close();
    }
}

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

IPluginMaintenance* PluginMgr::install(const String& name, uint16_t uid, uint8_t slotId)
{
    IPluginMaintenance* plugin  = nullptr;
    PluginRegEntry*     entry   = nullptr;

    if (true == m_registry.selectFirstElement())
    {
        bool isFound = false;

        /* Find plugin in the registry */
        entry = *m_registry.current();

        while((false == isFound) && (nullptr != entry))
        {
            if (name == entry->name)
            {
                isFound = true;
            }
            else if (false == m_registry.next())
            {
                entry = nullptr;
            }
            else
            {
                entry = *m_registry.current();
            }
        }

        /* Plugin found? */
        if ((true == isFound) &&
            (nullptr != entry))
        {
            plugin = entry->createFunc(entry->name, uid);

            if (DisplayMgr::SLOT_ID_INVALID == slotId)
            {
                if (false == installToAutoSlot(plugin))
                {
                    delete plugin;
                    plugin = nullptr;
                }
            }
            else
            {
                if (false == installToSlot(plugin, slotId))
                {
                    delete plugin;
                    plugin = nullptr;
                }
            }
        }
    }

    return plugin;
}

bool PluginMgr::installToAutoSlot(IPluginMaintenance* plugin)
{
    bool status = false;

    if (nullptr != plugin)
    {
        if (DisplayMgr::SLOT_ID_INVALID == DisplayMgr::getInstance().installPlugin(plugin))
        {
            LOG_ERROR("Couldn't install plugin %s.", plugin->getName());
        }
        else
        {
            if (false == m_plugins.append(plugin))
            {
                LOG_ERROR("Couldn't append plugin %s.", plugin->getName());

                (void)DisplayMgr::getInstance().uninstallPlugin(plugin);
            }
            else
            {
                String baseUri = getRestApiBaseUri(plugin->getUID());

                plugin->registerWebInterface(MyWebServer::getInstance(), baseUri);

                status = true;
            }
        }
    }

    return status;
}

bool PluginMgr::installToSlot(IPluginMaintenance* plugin, uint8_t slotId)
{
    bool status = false;

    if (nullptr != plugin)
    {
        if (DisplayMgr::SLOT_ID_INVALID == DisplayMgr::getInstance().installPlugin(plugin, slotId))
        {
            LOG_ERROR("Couldn't install plugin %s to slot %u.", plugin->getName(), slotId);
        }
        else
        {
            if (false == m_plugins.append(plugin))
            {
                LOG_ERROR("Couldn't append plugin %s.", plugin->getName());

                (void)DisplayMgr::getInstance().uninstallPlugin(plugin);
            }
            else
            {
                String baseUri = getRestApiBaseUri(plugin->getUID());

                plugin->registerWebInterface(MyWebServer::getInstance(), baseUri);

                status = true;
            }
        }
    }

    return status;
}

uint16_t PluginMgr::generateUID()
{
    uint16_t    uid;
    bool        isFound;

    do
    {
        isFound = false;
        uid     = random(UINT16_MAX);

        /* Ensure that UID is really unique. */
        if (true == m_plugins.selectFirstElement())
        {
            IPluginMaintenance* plugin = *m_plugins.current();

            while((false == isFound) && (nullptr != plugin))
            {
                if (uid == plugin->getUID())
                {
                    isFound = true;
                }
                else if (false == m_plugins.next())
                {
                    plugin = nullptr;
                }
                else
                {
                    plugin = *m_plugins.current();
                }
            }
        }
    }
    while(true == isFound);

    return uid;
}

/******************************************************************************
 * External Functions
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/
