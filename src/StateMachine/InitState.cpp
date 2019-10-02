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
 * @brief  System state: Init
 * @author Andreas Merkle <web@blue-andi.de>
 */

/******************************************************************************
 * Includes
 *****************************************************************************/
#include "InitState.h"

#include <Arduino.h>

#include "Board.h"
#include "ButtonDrv.h"
#include "LedMatrix.h"
#include "DisplayMgr.h"
#include "Version.h"
#include "AmbientLightSensor.h"

#include "APState.h"
#include "ConnectingState.h"

#include <Logging.h>

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

/* Set short wait time for showing a system message in ms. */
const uint32_t  InitState::SYS_MSG_WAIT_TIME_SHORT  = 250u;

/* Set serial interface baudrate. */
const uint32_t  InitState::SERIAL_BAUDRATE          = 115200u;

/* Initialization state instance */
InitState       InitState::m_instance;

/******************************************************************************
 * Public Methods
 *****************************************************************************/

void InitState::entry(StateMachine& sm)
{
    uint8_t index = 0u;

    /* Initialize hardware */
    Board::init();

    /* Setup serial interface */
    Serial.begin(SERIAL_BAUDRATE);

    /* Initialize logging, which uses the serial interface as sink. */
    Logging::getInstance().init(&Serial);
    Logging::getInstance().setLogLevel(Logging::LOGLEVEL_INFO);

    /* Initialize drivers */
    ButtonDrv::getInstance().init();

    /* Start LED matrix */
    LedMatrix::getInstance().begin();

    /* Initialize display manager */
    DisplayMgr::getInstance().init();

    /* Initialize display layouts */
    for(index = 0u; index < DisplayMgr::MAX_SLOTS; ++index)
    {
        DisplayMgr::getInstance().setLayout(index, DisplayMgr::LAYOUT_ID_2);
    }

    /* Show some interesting boot information */    
    showBootInfo();

    /* Enable the automatic display brightness adjustment according to the
     * ambient light.
     */
    if (false == DisplayMgr::getInstance().enableAutoBrightnessAdjustment(true))
    {
        LOG_WARNING("Failed to enable autom. brigthness adjustment.");
    }

    return;
}

void InitState::process(StateMachine& sm)
{
    /* Does the user request for setting up an wifi access point?
     * Because we just initialized the button driver, wait until
     * the button state has a reliable value.
     */
    if (true == ButtonDrv::getInstance().isUpdated())
    {
        /* Setup a wifi access point? */
        if (ButtonDrv::STATE_PRESSED == ButtonDrv::getInstance().getState())
        {
            sm.setState(APState::getInstance());
        }
        /* Connect to a remote wifi network. */
        else
        {
            sm.setState(ConnectingState::getInstance());
        }
    }

    return;
}

void InitState::exit(StateMachine& sm)
{
    /* Nothing to do. */
    return;
}

/******************************************************************************
 * Protected Methods
 *****************************************************************************/

/******************************************************************************
 * Private Methods
 *****************************************************************************/

/**
 * Show boot information on the serial interface.
 */
void InitState::showBootInfo(void)
{
    /* Show information via serial interface */    
    LOG_INFO("Booting ...");
    
    LOG_INFO(String("SW version: ") + Version::SOFTWARE);
    DisplayMgr::getInstance().showSysMsg(Version::SOFTWARE);

    LOG_INFO(String("ESP32 chip rev.: ") + ESP.getChipRevision());
    LOG_INFO(String("ESP32 SDK version: ") + ESP.getSdkVersion());

    LOG_INFO(String("Ambient light sensor detected: ") + AmbientLightSensor::getInstance().isSensorAvailable());

    /* User shall be able to read it on the display. But it shall be really a short delay. */
    DisplayMgr::getInstance().delay(SYS_MSG_WAIT_TIME_SHORT);

    return;
}

/******************************************************************************
 * External Functions
 *****************************************************************************/

/******************************************************************************
 * Local Functions
 *****************************************************************************/
