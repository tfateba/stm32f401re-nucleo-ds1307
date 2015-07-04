/*
* @file     main.c 
* @brief    Real Time Clock Calendar for NUCLEO STM32F401 with DS1307
* @author   Theodore ATEBA
* @version  1.0
* @date 21  June 2015
*/

//=============================================================================
// Include Files
//=============================================================================
    #include <stdlib.h>
    #include "ch.h"
    #include "hal.h"
    #include "dslib.h"

//=============================================================================
// Global variables, I2C TX and RX buffers, I2C and Serial Configurations
//=============================================================================

    // Declaration of a structure of calendar
    struct ds1307_t calendar;

//=============================================================================
// Functions
//=============================================================================


// RTC reader thread
static WORKING_AREA ( waRtcReadThread, 128 );
static msg_t RtcReadThread (void *arg)
{
    msg_t status = RDY_OK;
    systime_t timeOut = MS2ST ( 4 );

    (void)arg;
    chRegSetThreadName ( "RTC reader" );
    while ( TRUE )
    {
        calendar = getDs1307Date ( &status, &timeOut );
        chThdSleepMilliseconds( 800 );
    }
    return 0;
}

// RTC printer thread
static WORKING_AREA ( waRtcPrintThread, 128 );
static msg_t RtcPrintThread (void *arg)
{
    (void)arg;
    chRegSetThreadName ( "RTC printer" );
    while ( TRUE )
    {
        ds1307Print ( calendar );
        chThdSleepMilliseconds ( 1000 );
    }
    return 0;
}


/*
* @fn       int main (void)
* @brief    Application entry point.
*/
int main (void) 
{
    // System initializations.
    halInit ();
    chSysInit ();

    // Init the I2C interface 1
    ds1307InterfaceInit ();
        
    // init the serial Driver
    //serialDriver2Init ();
     sdStart(&SD2, NULL);
        
    print("\n\r Chibios Real Time Clock calendar with Nucleo and DS1307.\n\r");
    
    // Used when you whant to set the calendar and clock
    calendar.seconds    = 0;
    calendar.minutes    = 32;
    calendar.hours      = 23; 
    calendar.day        = 3;
    calendar.date       = 24;
    calendar.month      = 6;
    calendar.year       = 2015;
    //setDs1307Date( &status, &tmo, calendar);

    // Create the thread used to read the RTC DS1307
    chThdCreateStatic(waRtcReadThread, sizeof(waRtcReadThread), NORMALPRIO, 
    RtcReadThread, NULL);
    
    // Create the thread used to print the RTC data every seconds
    chThdCreateStatic(waRtcPrintThread, sizeof(waRtcPrintThread), NORMALPRIO, 
    RtcPrintThread, NULL);

    while (1)
    {
        chThdSleepMilliseconds (1);
    }
}
