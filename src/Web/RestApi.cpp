/* MIT License
 *
 * Copyright (c) 2019 Andreas Merkle <web@blue-andi.de>
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
@brief  REST API
@author Andreas Merkle <web@blue-andi.de>

@section desc Description
@see RestApi.h

*******************************************************************************/

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "RestApi.h"
#include "Html.h"
#include <ArduinoJson.h>
#include "Settings.h"
#include "LedMatrix.h"
#include "DisplayMgr.h"
#include "Version.h"

#include <crypto/base64.h>

/******************************************************************************
 * Compiler Switches
 *****************************************************************************/

/******************************************************************************
 * Macros
 *****************************************************************************/

/******************************************************************************
 * Types and classes
 *****************************************************************************/

/**
 * REST request status code.
 */
typedef enum
{
    STATUS_CODE_OK = 0,     /**< Successful */
    STATUS_CODE_NOT_FOUND   /**< Requested URI not found. */

} StatusCode;

/******************************************************************************
 * Prototypes
 *****************************************************************************/

static bool toUInt8(const String& str, uint8_t& value);
static bool toUInt16(const String& str, uint16_t& value);
static void status(void);
static void slots(void);
static void slotText(void);
static void slotBitmap(void);
static void slotLamp(void);

/******************************************************************************
 * Local Variables
 *****************************************************************************/

/**
 * Web server
 */
static WebServer*   gWebServer  = NULL;

/******************************************************************************
 * Public Methods
 *****************************************************************************/

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

/******************************************************************************
 * External Functions
 *****************************************************************************/

void RestApi::init(WebServer& srv)
{
    String  path;

    gWebServer = &srv;

    gWebServer->on("/rest/api/v1/status", status);
    gWebServer->on("/rest/api/v1/display/slots", slots);
    gWebServer->on("/rest/api/v1/display/slot/{}/text", slotText);
    gWebServer->on("/rest/api/v1/display/slot/{}/bitmap", slotBitmap);
    gWebServer->on("/rest/api/v1/display/slot/{}/lamp/{}/state", slotLamp);
    
    return;
}

/******************************************************************************
 * Local Functions
 *****************************************************************************/

/**
 * Convert a string to uint8_t.
 * 
 * @param[in]   str     String
 * @param[out]  value   Converted value
 * 
 * @return If conversion fails, it will return false otherwise true.
 */
static bool toUInt8(const String& str, uint8_t& value)
{
    bool    success = false;
    int32_t tmp     = str.toInt();

    if ((0 <= tmp) ||
        (UINT8_MAX >= tmp))
    {
        value = static_cast<uint8_t>(tmp);
    }

    return success;
}

/**
 * Convert a string to uint16_t.
 * 
 * @param[in]   str     String
 * @param[out]  value   Converted value
 * 
 * @return If conversion fails, it will return false otherwise true.
 */
static bool toUInt16(const String& str, uint16_t& value)
{
    bool    success = false;
    int32_t tmp     = str.toInt();

    if ((0 <= tmp) ||
        (UINT16_MAX >= tmp))
    {
        value = static_cast<uint16_t>(tmp);
    }

    return success;
}

/**
 * Get status information.
 * GET /api/v1/status
 */
static void status(void)
{
    String                  content;
    StaticJsonDocument<200> jsonDoc;
    uint32_t                httpStatusCode  = Html::STATUS_CODE_OK;

    if (NULL == gWebServer)
    {
        return;
    }

    if (HTTP_GET != gWebServer->method())
    {
        JsonObject errorObj = jsonDoc.createNestedObject("error");

        /* Prepare response */
        jsonDoc["status"]   = STATUS_CODE_NOT_FOUND;
        errorObj["msg"]     = "HTTP method not supported.";
        httpStatusCode      = Html::STATUS_CODE_NOT_FOUND;
    }
    else
    {
        String      ssid;
        JsonObject  dataObj         = jsonDoc.createNestedObject("data");
        JsonObject  hwObj           = dataObj.createNestedObject("hardware");
        JsonObject  swObj           = dataObj.createNestedObject("software");
        JsonObject  internalRamObj  = swObj.createNestedObject("internalRam");
        JsonObject  wifiObj         = dataObj.createNestedObject("wifi");

        if (true == Settings::getInstance().open(true))
        {
            ssid = Settings::getInstance().getWifiSSID();
            Settings::getInstance().close();
        }

        /* Prepare response */
        jsonDoc["status"]       = STATUS_CODE_OK;

        hwObj["chipRev"]        = ESP.getChipRevision();
        hwObj["cpuFreqMhz"]     = ESP.getCpuFreqMHz();

        swObj["version"]        = Version::SOFTWARE;
        swObj["espSdkVersion"]  = ESP.getSdkVersion();

        internalRamObj["heapSize"]      = ESP.getHeapSize();
        internalRamObj["availableHeap"] = ESP.getFreeHeap();

        wifiObj["ssid"]         = ssid;

        httpStatusCode          = Html::STATUS_CODE_OK;
    }

    serializeJsonPretty(jsonDoc, content);
    gWebServer->send(httpStatusCode, "application/json", content);

    return;
}

/**
 * Get number of slots
 * GET /api/v1/display/slots
 */
static void slots(void)
{
    String                  content;
    StaticJsonDocument<200> jsonDoc;
    uint32_t                httpStatusCode  = Html::STATUS_CODE_OK;

    if (NULL == gWebServer)
    {
        return;
    }

    if (HTTP_GET != gWebServer->method())
    {
        JsonObject errorObj = jsonDoc.createNestedObject("error");

        /* Prepare response */
        jsonDoc["status"]   = STATUS_CODE_NOT_FOUND;
        errorObj["msg"]     = "HTTP method not supported.";
        httpStatusCode      = Html::STATUS_CODE_NOT_FOUND;
    }
    else
    {
        JsonObject  dataObj = jsonDoc.createNestedObject("data");

        dataObj["slots"] = DisplayMgr::getInstance().MAX_SLOTS;

        /* Prepare response */
        jsonDoc["status"]   = STATUS_CODE_OK;
        httpStatusCode      = Html::STATUS_CODE_OK;
    }

    serializeJsonPretty(jsonDoc, content);
    gWebServer->send(httpStatusCode, "application/json", content);

    return;
}

/**
 * Set text of a slot.
 * POST /display/slot/<slot-id>/text?show=<text>
 */
static void slotText(void)
{
    String                  content;
    StaticJsonDocument<200> jsonDoc;
    uint32_t                httpStatusCode  = Html::STATUS_CODE_OK;
    
    if (NULL == gWebServer)
    {
        return;
    }

    if (HTTP_POST != gWebServer->method())
    {
        JsonObject errorObj = jsonDoc.createNestedObject("error");

        /* Prepare response */
        jsonDoc["status"]   = STATUS_CODE_NOT_FOUND;
        errorObj["msg"]     = "HTTP method not supported.";
        httpStatusCode      = Html::STATUS_CODE_NOT_FOUND;
    }
    else
    {
        uint8_t slotId = DisplayMgr::getInstance().MAX_SLOTS;
        
        /* Slot id invalid? */
        if ((false == toUInt8(gWebServer->pathArg(0), slotId)) ||
            (DisplayMgr::getInstance().MAX_SLOTS <= slotId))
        {
            JsonObject errorObj = jsonDoc.createNestedObject("error");

            /* Prepare response */
            jsonDoc["status"]   = STATUS_CODE_NOT_FOUND;
            errorObj["msg"]     = "Slot id not supported.";
            httpStatusCode      = Html::STATUS_CODE_NOT_FOUND;
        }
        /* "show" argument missing? */
        else if (false == gWebServer->hasArg("show"))
        {
            JsonObject errorObj = jsonDoc.createNestedObject("error");

            /* Prepare response */
            jsonDoc["status"]   = STATUS_CODE_NOT_FOUND;
            errorObj["msg"]     = "Show is missing.";
            httpStatusCode      = Html::STATUS_CODE_NOT_FOUND;
        }
        else
        {
            String text = gWebServer->arg("show");

            DisplayMgr::getInstance().setText(slotId, text);

            (void)jsonDoc.createNestedObject("data");

            /* Prepare response */
            jsonDoc["status"]   = STATUS_CODE_OK;
            httpStatusCode      = Html::STATUS_CODE_OK;
        }
    }

    serializeJsonPretty(jsonDoc, content);
    gWebServer->send(httpStatusCode, "application/json", content);

    return;
}

/**
 * Set bitmap of a slot.
 * POST /display/slot/<slot-id>/bitmap?width=<width-in-pixel>&height=<height-in-pixel>&data=<data-uint16_t>
 */
static void slotBitmap(void)
{
    String                  content;
    StaticJsonDocument<200> jsonDoc;
    uint32_t                httpStatusCode  = Html::STATUS_CODE_OK;

    if (NULL == gWebServer)
    {
        return;
    }

    if (HTTP_POST != gWebServer->method())
    {
        JsonObject errorObj = jsonDoc.createNestedObject("error");

        /* Prepare response */
        jsonDoc["status"]   = STATUS_CODE_NOT_FOUND;
        errorObj["msg"]     = "HTTP method not supported.";
        httpStatusCode      = Html::STATUS_CODE_NOT_FOUND;
    }
    else
    {
        uint8_t     slotId  = DisplayMgr::getInstance().MAX_SLOTS;
        uint16_t    width   = 0u;
        uint16_t    height  = 0u;

        /* Slot id invalid? */
        if ((false == toUInt8(gWebServer->pathArg(0), slotId)) ||
            (DisplayMgr::getInstance().MAX_SLOTS <= slotId))
        {
            JsonObject errorObj = jsonDoc.createNestedObject("error");

            /* Prepare response */
            jsonDoc["status"]   = STATUS_CODE_NOT_FOUND;
            errorObj["msg"]     = "Slot id not supported.";
            httpStatusCode      = Html::STATUS_CODE_NOT_FOUND;
        }
        /* "width" argument missing? */
        else if (false == gWebServer->hasArg("width"))
        {
            JsonObject errorObj = jsonDoc.createNestedObject("error");

            /* Prepare response */
            jsonDoc["status"]   = STATUS_CODE_NOT_FOUND;
            errorObj["msg"]     = "Width is missing.";
            httpStatusCode      = Html::STATUS_CODE_NOT_FOUND;
        }
        /* "height" argument missing? */
        else if (false == gWebServer->hasArg("height"))
        {
            JsonObject errorObj = jsonDoc.createNestedObject("error");

            /* Prepare response */
            jsonDoc["status"]   = STATUS_CODE_NOT_FOUND;
            errorObj["msg"]     = "Height is missing.";
            httpStatusCode      = Html::STATUS_CODE_NOT_FOUND;
        }
        /* "data" argument missing? */
        else if (false == gWebServer->hasArg("data"))
        {
            JsonObject errorObj = jsonDoc.createNestedObject("error");

            /* Prepare response */
            jsonDoc["status"]   = STATUS_CODE_NOT_FOUND;
            errorObj["msg"]     = "Data is missing.";
            httpStatusCode      = Html::STATUS_CODE_NOT_FOUND;
        }
        /* Invalid width? */
        else if (false == toUInt16(gWebServer->arg("width"), width))
        {
            JsonObject errorObj = jsonDoc.createNestedObject("error");

            /* Prepare response */
            jsonDoc["status"]   = STATUS_CODE_NOT_FOUND;
            errorObj["msg"]     = "Invalid width.";
            httpStatusCode      = Html::STATUS_CODE_NOT_FOUND;
        }
        /* Invalid height? */
        else if (false == toUInt16(gWebServer->arg("height"), width))
        {
            JsonObject errorObj = jsonDoc.createNestedObject("error");

            /* Prepare response */
            jsonDoc["status"]   = STATUS_CODE_NOT_FOUND;
            errorObj["msg"]     = "Invalid height.";
            httpStatusCode      = Html::STATUS_CODE_NOT_FOUND;
        }
        else
        {
            String          dataBase64Str       = gWebServer->arg("data");
            size_t          dataBase64ArraySize = dataBase64Str.length();
            const uint8_t*  dataBase64Array     = reinterpret_cast<const uint8_t*>(dataBase64Str.c_str());
            size_t          bitmapSize          = 0;
            uint16_t*       bitmap              = reinterpret_cast<uint16_t*>(base64_decode(dataBase64Array, dataBase64ArraySize, &bitmapSize));

            DisplayMgr::getInstance().setBitmap(slotId, bitmap, width, height);
            delete bitmap;

            (void)jsonDoc.createNestedObject("data");

            /* Prepare response */
            jsonDoc["status"]   = STATUS_CODE_OK;
            httpStatusCode      = Html::STATUS_CODE_OK;
        }
    }

    serializeJsonPretty(jsonDoc, content);
    gWebServer->send(httpStatusCode, "application/json", content);

    return;
}

/**
 * Set lamp state of a slot.
 * POST /display/slot/<slot-id>/lamp/<lamp-id>/state?set=<on/off>
 */
static void slotLamp(void)
{
    String                  content;
    StaticJsonDocument<200> jsonDoc;
    uint32_t                httpStatusCode  = Html::STATUS_CODE_OK;

    if (NULL == gWebServer)
    {
        return;
    }

    if (HTTP_POST != gWebServer->method())
    {
        JsonObject errorObj = jsonDoc.createNestedObject("error");

        /* Prepare response */
        jsonDoc["status"]   = STATUS_CODE_NOT_FOUND;
        errorObj["msg"]     = "HTTP method not supported.";
        httpStatusCode      = Html::STATUS_CODE_NOT_FOUND;
    }
    else
    {
        uint8_t slotId = DisplayMgr::getInstance().MAX_SLOTS;
        uint8_t lampId = 0u;

        /* Slot id invalid? */
        if ((false == toUInt8(gWebServer->pathArg(0u), slotId)) ||
            (DisplayMgr::getInstance().MAX_SLOTS <= slotId))
        {
            JsonObject errorObj = jsonDoc.createNestedObject("error");

            /* Prepare response */
            jsonDoc["status"]   = STATUS_CODE_NOT_FOUND;
            errorObj["msg"]     = "Slot id not supported.";
            httpStatusCode      = Html::STATUS_CODE_NOT_FOUND;
        }
        /* Lamp id invalid? */
        else if (false == toUInt8(gWebServer->pathArg(1u), lampId))
        {
            JsonObject errorObj = jsonDoc.createNestedObject("error");

            /* Prepare response */
            jsonDoc["status"]   = STATUS_CODE_NOT_FOUND;
            errorObj["msg"]     = "Lamp id not supported.";
            httpStatusCode      = Html::STATUS_CODE_NOT_FOUND;
        }
        else if ((0 != gWebServer->arg(0).compareTo("set")) ||
                 ((0 != gWebServer->arg(1).compareTo("off")) &&
                  (0 != gWebServer->arg(1).compareTo("on"))))
        {
            JsonObject errorObj = jsonDoc.createNestedObject("error");

            /* Prepare response */
            jsonDoc["status"]   = STATUS_CODE_NOT_FOUND;
            errorObj["msg"]     = "Command not supported.";
            httpStatusCode      = Html::STATUS_CODE_NOT_FOUND;
        }
        else
        {
            bool lampState = false;

            if (0 == gWebServer->arg(1).compareTo("on"))
            {
                lampState = true;
            }
            
            DisplayMgr::getInstance().setLamp(slotId, lampId, lampState);

            (void)jsonDoc.createNestedObject("data");

            /* Prepare response */
            jsonDoc["status"]   = STATUS_CODE_OK;
            httpStatusCode      = Html::STATUS_CODE_OK;
        }
    }

    serializeJsonPretty(jsonDoc, content);
    gWebServer->send(httpStatusCode, "application/json", content);

    return;
}