/*
* @file main.c
* 
* @brief Real Time Clock Calendar for NUCLEO STM32F401 with DS1307
*
* author Theodore ATEBA
*
* @version  1.0
*
* @date 21 June 2015
*
*/

//=============================================================================
// Include Files
//=============================================================================
    #include "ch.h"
    #include "hal.h"
    #include <stdlib.h>

//=============================================================================
// Global variables, I2C TX and RX buffers, I2C and Serial Configurations
//=============================================================================
    #define DS1307_RX_DEPTH 7
    #define DS1307_TX_DEPTH 8

    #define DS1307_ADDRESS 0x68

    #define DS1307_SECONDS_REG	0x00

    #define SUCCES      0
    #define FAILLURE    1

    static uint8_t rxbuf[DS1307_RX_DEPTH];
    static uint8_t txbuf[DS1307_TX_DEPTH];

    static i2cflags_t errors = 0;

    static char seconds;
    static char minutes;
    static char hours;
    static char day;
    static char date;
    static char month;
    static uint16_t year;

    // Serial Driver configuration
    static SerialConfig serialDriverConfig;

    // I2C interface #1
    static const I2CConfig i2cConf = {
        OPMODE_I2C,
        400000,
        FAST_DUTY_CYCLE_2,
    };

//=============================================================================
// Functions
//=============================================================================

/*
* @fn          static uint8_t bcd2Dec(uint8_t val)
* @brief       Convert BCD to Decimal
* @param[in]   val value to convert from BCD to Decimal
* @return      res Converted value
*/
static uint8_t bcd2Dec(uint8_t val) 
{
    uint8_t res = ((val/16*10) + (val % 16));
    return res;
}

/*
* @fn           static uint8_t dec2Bcd(uint8_t val)
* @brief        Convert Decimal to BCD
* @param[in]    val value to convert.
* @return       res Converted value
*/
static uint8_t dec2Bcd(uint8_t val)
{
    uint8_t res = ((val/10*16) + (val%10));
    return res;
}

/*
*@fn           static void print(char *p)
*@brief        Function used to send data to the Serial Driver 2
*@param[in]    *p pointer of data to print
*/
static void print(char *p)
{
    while(*p)
        sdPut(&SD2, *p++);
}

/*
*@fn        static void printn(int16_t number)
*@brief     Function used to print integer with Serial Driver 2
*@param[in] number integer to print
*/
static void printn(int16_t number)
{
    char buf[16], *p;

    if(!number)
        sdPut(&SD2, '0');
    else
    {
        p = buf;
        while(number)
            *p++ = (number % 10) + '0', number /= 10;
        while(p > buf)
            sdPut(&SD2, *--p);
    }
}

/*
* @fn       static void i2cInit(void)
* @brief   Configure the I2C Interface 1 and start the Interface
*/
void ds1307InterfaceInit(void)
{
    i2cStart(&I2CD1, &i2cConf);
    palSetPadMode(GPIOB, 8, PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN); // SCL
    palSetPadMode(GPIOB, 9, PAL_MODE_ALTERNATE(4) | PAL_STM32_OTYPE_OPENDRAIN); // SDA
}

/*
* @fn       static void sdInit(void)
* @brief    Serial Driver Initialisation and start the interface
*/
void serialDriver2Init(void)
{
    // Configure the Baud Rate of the serial driver at 115200
    serialDriverConfig.speed = 115200;

    // Activate the Serial Driver 2
    sdStart(&SD2, &serialDriverConfig);
}

/*
*
*
*
*
*
*
*
*
*
*/
void setDs1307Date(msg_t *status, systime_t *tmo,
                    uint8_t seconds,
                    uint8_t minutes,
                    uint8_t hours,
                    uint8_t day,
                    uint8_t date,
                    uint8_t month,
                    uint16_t year)
{
    txbuf[0] = DS1307_SECONDS_REG;
    txbuf[1] = dec2Bcd(seconds);
    txbuf[2] = dec2Bcd(minutes);
    txbuf[3] = dec2Bcd(hours);
    txbuf[4] = dec2Bcd(day);
    txbuf[5] = dec2Bcd(date);
    txbuf[6] = dec2Bcd(month);
    txbuf[7] = dec2Bcd(year - 2000);

    i2cAcquireBus(&I2CD1);
    *status = i2cMasterTransmitTimeout(&I2CD1, DS1307_ADDRESS, txbuf, DS1307_TX_DEPTH, rxbuf, 0, *tmo);
    i2cReleaseBus(&I2CD1);

    if ( *status != MSG_OK )
        print ("\n Error when setting the DS1307 date over the I2C bus\n");
    else
        print ("\n DS1307 was setting succefuly.\n");
}

/*
* @fn void getDs1307Date(void)
* @brief    Get Clock and Calendar
*/
static uint8_t getDs1307Date(msg_t *status, systime_t *tmo)
{
    txbuf[0] = DS1307_SECONDS_REG; // Register address of the Seconds
    i2cAcquireBus(&I2CD1);
    *status = i2cMasterTransmitTimeout(&I2CD1, DS1307_ADDRESS, txbuf, 1, rxbuf, 7, *tmo);
    i2cReleaseBus(&I2CD1);

    if (*status != MSG_OK)
    {
        errors = i2cGetErrors(&I2CD1);
        print("\n I2C transmission error!\n\r");

        return FAILLURE;
    }
    else
    {    
	    seconds = bcd2Dec ( rxbuf[0] & 0x7F );
	    minutes = bcd2Dec ( rxbuf[1] & 0x3F );
	    hours	= bcd2Dec ( rxbuf[2] );
	    day		= bcd2Dec ( rxbuf[3] );
	    date	= bcd2Dec ( rxbuf[4] );
	    month	= bcd2Dec ( rxbuf[5] );
	    year	= bcd2Dec ( rxbuf[6] ) + 2000;
        
        printn (year);
        print ("/");
        printn (month);
        print ("/");
        printn (date);
        print (" ");
        
        printn (hours);
        print (":");
        printn (minutes);
        print (":");
        printn (seconds);
        print ("\n\r");
    }
    return SUCCES;
}

/*
* @fn       int main(void)
* @brief    Application entry point.
*/
int main(void) 
{
    msg_t status = MSG_OK;
    systime_t tmo = MS2ST(4);

    // System initializations.
    halInit();
    chSysInit();

    // Init the I2C interface 1
    ds1307InterfaceInit();
        
    // init the serial Driver
    serialDriver2Init();
        
    print("\n\r Chibios Real Time Clock calendar with Nucleo and DS1307.\n\r");
    
    // Used when you whant to set the calendar and clock
    // status tmo seconds minutes hours day date month year
    //setDs1307Date(&status, &tmo, 0, 22, 20, 7, 21, 6, 2015); // 0, seconds, minutes, hours

    while(1)
    {
        chThdSleepMilliseconds(500);

        // Read  and print RTC Clock and Calendar
        getDs1307Date(&status, &tmo);
    }
}
