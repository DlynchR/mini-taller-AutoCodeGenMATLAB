/* Arduino AVR Bareboard scheduler
*
* Copyright 2014-2026 The MathWorks, Inc.
*/

#include "Arduino.h"
#include "io_wrappers.h"
#if defined(ARDUINO_ARM_CORTEX_M0_PLUS)
#include "arduinoARM_M0plusScheduler.h"
#elif defined(ARDUINO_ARM_CORTEX_M3)
#include "arduinoARMScheduler.h"
#include "watchdog.h"
#else
#include "arduinoAVRScheduler.h"
#endif

#if (defined(ARDUINO_ARCH_RENESAS_UNO))
#include "MW_ArduinoUnoR4.h"
#endif

#if defined(ESP_PLATFORM)
#include "esp_wpa2.h" //wpa2 library for connections to Enterprise networks
#include "soc/rtc.h"
#if (ARDUINO_USB_CDC_ON_BOOT | ARDUINO_USB_MSC_ON_BOOT | ARDUINO_USB_DFU_ON_BOOT) && !ARDUINO_USB_MODE
#include "USB.h"
#if ARDUINO_USB_MSC_ON_BOOT
#include "FirmwareMSC.h"
#endif
#endif
TaskHandle_t loopTaskHandle = NULL;
#endif

#ifdef _RTT_NUMI2C_
#include "MW_arduinoI2C.h"
#endif

#if defined(ARDUINO_ARDUINO_NANO33BLE)
#include <USB/PluggableUSBSerial.h> // for PluggableUSBD
#include "MW_PWM_NANO33BLE.h"
#endif

#if defined(ARDUINO_NANO_RP2040_CONNECT)
#include <USB/PluggableUSBSerial.h>
#include "double_tap_usb_boot.cpp" // to force double tap detection
#endif

#if (defined(ARDUINO_RASPBERRY_PI_PICO_W) || defined(ARDUINO_RASPBERRY_PI_PICO))
#include "RP2040USB.h"
#include "pico_bootsel_via_double_reset.c" // to force double tap detection
#include <pico/stdlib.h>
// Weak empty variant initialization. May be redefined by variant files.
void initVariant() __attribute__((weak));
void initVariant() { }
#include "RP2040Support.h"

// Only instantiate in PIL/SIL builds to avoid double definition in non-PIL.
#if defined(MW_PIL_ARUDINOSERIAL) || defined(ARDUINO_SIL_PIL)
#include <stdbool.h>
#include "RP2040Support.h"

/* ============================================================================
 * FreeRTOS Status Flags - Bare-Metal PIL/SIL Configuration
 * ============================================================================
 * These symbols are declared in:
 *   CLI\data\packages\rp2040\hardware\rp2040\4.4.4\cores\rp2040\_freertos.h
 * and are referenced by CoreMutex.cpp, lock.cpp, wiring_private.cpp to
 * determine if FreeRTOS is active. For PIL/SIL builds, we run bare-metal
 * (no FreeRTOS), so both flags remain false.
 *
 * We define __isFreeRTOS and __freeRTOSinitted (coming from
 * CLI\data\packages\rp2040\hardware\rp2040\4.4.4\cores\rp2040\_freertos.h
 * and associated with _freertos.cpp) here in a .cpp file
 * (MW_ArduinoHWInit.cpp) rather than in a .c file (pil_main_arduino.c) to
 * avoid C/C++ incompatibility issues. 
 */

extern bool __isFreeRTOS = false;
volatile bool __freeRTOSinitted = false;

/* ============================================================================
 * Newlib Reentrancy Support
 * ============================================================================
 * Provides the global reentrancy structure pointer for newlib's thread-safe
 * operations. The __wrap___getreent function is called by newlib functions
 * that need per-thread data (e.g., errno, strtok state).
 * 
 * The linker flag -Wl,--wrap=__getreent redirects calls to __getreent to this
 * wrapper, allowing us to provide the correct _reent structure for bare-metal
 * PIL builds.
 */

struct _reent;
extern struct _reent *_impure_ptr;

#ifdef __cplusplus
extern "C"
#endif
struct _reent * __wrap___getreent(void)
{
    return _impure_ptr;
}

/* ============================================================================
 * Global RP2040 Hardware Interface Instance
 * ============================================================================
 * Used by MW_ArduinoRaspberryPiPicoWInit() and RP2040Support.h methods.
 * 
 * This instance is only defined in PIL/SIL builds. 
 */
RP2040 rp2040;
#endif

#endif

#if (defined(MW_ARDUINO_OV2640CSPIN)) || (defined(_RTT_SPI_SSPIN_)) || (defined(_RTT_DUE_SPI_SSPIN4_)) || (defined(_RTT_DUE_SPI_SSPIN10_)) || (defined(_RTT_DUE_SPI_SSPIN52_))
#include "MW_SPIwriteRead.h"
#endif

#if (defined(_RTT_RS485_INUSE_))
#include "ArduinoRS485.h"
#endif

#if (defined(_RTT_MODBUS_MASTER_)) || (defined(_RTT_MODBUS_SLAVE_))
#include "ArduinoModbus.h"
#endif

#ifdef NANO_MOTOR_CARRIER
#include <Wire.h>
#define PMIC_ADDRESS  0x6B
#define PMIC_REG00    0x00
#define PMIC_REG01    0x01
#define PMIC_REG02    0x02
#define PMIC_REG04    0x04
#define PMIC_REG05    0x05
void MW_EnableNanoCarrierBatteryCharging(void);
#endif

// Wifi init routines
#include <inttypes.h>
#if (defined(_RTT_WIFI_EXTMODE_) && (_RTT_WIFI_EXTMODE_ == 1)) || (defined(_RTT_WIFI_UDP_) && (_RTT_WIFI_UDP_ != 0)) || (defined(_RTT_WIFI_TCP_) && (_RTT_WIFI_TCP_ != 0))|| defined(_RTT_THINGSPEAK_WIFI_) || defined(_RTT_WEBSOCKET_PORT_) || defined(_RTT_MQTT_WIFI_) || defined(_RTT_HTTP_CLIENT_) || defined(OTA_SETUP) || defined(OTA_UPLOAD)
#if defined(ARDUINO_WIFI_LIB_101)
#include <WiFi101.h>
#elif defined(ARDUINO_WIFI_LIB_NINA)
#include <WiFiNINA.h>
#elif defined(ARDUINO_UNOR4_WIFI)
#include <WiFiS3.h>
#else
#include <WiFi.h>
#endif
#include <SPI.h>

/*#define that helps to stringify build flags
//Double evaluation is needed so that the double quotes can be derived out
//of the build flag and can be assigned to a character array */
#define RTT_StringifyBuildFlag(x) RTT_StringParamExpanded(x)
#define RTT_StringParamExpanded(x)  #x

#if (_RTT_DISABLE_Wifi_DHCP_ == 1)
IPAddress wifiLocalIpAddress(_RTT_WIFI_Local_IP1, _RTT_WIFI_Local_IP2, _RTT_WIFI_Local_IP3, _RTT_WIFI_Local_IP4);
#endif

#if (_RTT_DISABLE_Wifi_DHCP_ == 2)
IPAddress wifiLocalIpAddress(_RTT_WIFI_Local_IP1, _RTT_WIFI_Local_IP2, _RTT_WIFI_Local_IP3, _RTT_WIFI_Local_IP4);
IPAddress wifiGatewayAddress(_RTT_WIFI_Gateway_IP1, _RTT_WIFI_Gateway_IP2, _RTT_WIFI_Gateway_IP3, _RTT_WIFI_Gateway_IP4);
IPAddress wifiSubnetMask(_RTT_WIFI_Subnet_IP1, _RTT_WIFI_Subnet_IP2, _RTT_WIFI_Subnet_IP3, _RTT_WIFI_Subnet_IP4);
IPAddress wifiDNSAddress(_RTT_WIFI_DNS_IP1, _RTT_WIFI_DNS_IP2, _RTT_WIFI_DNS_IP3, _RTT_WIFI_DNS_IP4);
#endif

#if defined(ESP_PLATFORM) || defined(ARDUINO_UNOR4_WIFI)
IPAddress wifigateway(_RTT_WIFI_Local_IP1, _RTT_WIFI_Local_IP2, _RTT_WIFI_Local_IP3, uint8_t(1));
IPAddress subnet(uint8_t(255),uint8_t(255),uint8_t(255),uint8_t(0));
IPAddress dns1 = (uint32_t)0x00000000;
IPAddress dns2 = (uint32_t)0x00000000;
#endif

char ssid[] = RTT_StringifyBuildFlag(_RTT_WIFI_SSID);
int configureSuccess = WL_IDLE_STATUS;
int trialcount;

#ifdef _RTT_WIFI_WEP
char key[] = RTT_StringifyBuildFlag(_RTT_WIFI_KEY);
int keyIndex = _RTT_WIFI_KEY_INDEX;
#endif //_RTT_WIFI_WEP

#ifdef _RTT_WIFI_WPA
char wpapass[] = RTT_StringifyBuildFlag(_RTT_WIFI_WPA_PASSWORD);
#endif //_RTT_WIFI_WPA

#ifdef _RTT_WIFI_WPA2_ENTERPRISE
char wpa2username[] = RTT_StringifyBuildFlag(_RTT_WIFI_WPA2_USERNAME);
char wpa2password[] = RTT_StringifyBuildFlag(_RTT_WIFI_WPA2_PASSWORD);
#endif //_RTT_WIFI_WPA2_ENTERPRISE
#endif //(defined(_RTT_THINGSPEAK_WIFI_) || (_RTT_WIFI_TCP_ !=0) || defined(_RTT_WEBSOCKET_PORT_))

#include "MW_ArduinoHWInit.h"
#ifdef _MW_DASHBOARD_BLOCK_PRESENT_
#include "MW_graphicPrimitive.h"
#endif

#if defined(OTA_SETUP) || defined(OTA_UPLOAD)
#include "WiFiServer.h"
#include "WiFiClient.h"
#include "WiFiUdp.h"
#include "ArduinoOTA.h"
char OTA_Device_Name[] = RTT_StringifyBuildFlag(_RTT_OTA_DEVICE_NAME);
char OTA_Device_Pass[] = RTT_StringifyBuildFlag(_RTT_OTA_DEVICE_PASS);
#endif

void MW_OTA_Setup(void) // initialize OTA handlers
{
  #if defined(OTA_SETUP) || defined(OTA_UPLOAD)
  ArduinoOTA.begin(WiFi.localIP(), OTA_Device_Name, OTA_Device_Pass, InternalStorage);
  #endif
}

void MW_OTA_Poll(void) // check for OTA updates
{
  #if defined(OTA_SETUP) || defined(OTA_UPLOAD)
  ArduinoOTA.poll(); 
  #endif
}

void MW_Modbus_Slave_Poll(void)
{
    #if defined(_RTT_MODBUS_SLAVE_)
    ModbusRTUServer.poll();
    #endif
}

void MW_ArduinoNano33BLESenseInit(void)
{
    #if (defined(ARDUINO_ARDUINO_NANO33BLE))
    initVariant();
    #if defined(SERIAL_CDC)
    PluggableUSBD().begin();
    _SerialUSB.begin(115200);
    #endif
    #if defined(_RTT_PWM_NANO33BLE_INUSE)
    MW_NANO33BLE_PWM_Setup();
    #endif
    #endif
}

void MW_ArduinoNanoRP2040Init(void)
{
    #if defined(ARDUINO_NANO_RP2040_CONNECT)
	init();
	initVariant();
    #if defined(SERIAL_CDC)
    PluggableUSBD().begin();
    _SerialUSB.begin(115200);
    #endif
    #endif
}

void UNOR4_Init(void) 
{   
     #if(defined(ARDUINO_ARCH_RENESAS_UNO))
    // Disable stack pointer protection
    // TODO: the best thing would be keeping SPMON active but changing
    // R_MPU_SPMON->SP[0].EA = __stack_top; // on every call to malloc()
    // When stack and heap would collide, we could signal the NMI with mbed style leds patterns
    #if defined(__ARM_ARCH_8M_MAIN__) || defined(__ARM_ARCH_8M_BASE__)
        __set_MSPLIM(0);
        #if FSP_PRIV_TZ_USE_SECURE_REGS
            unsecure_registers();
        #endif
    #else
        R_MPU_SPMON->SP[0].CTL = 0;
    #endif

   __disable_irq();
   irq_vector_table = (volatile uint32_t *)APPLICATION_VECTOR_TABLE_ADDRESS_RAM;
   size_t _i;
   for (_i=0; _i<BSP_CORTEX_VECTOR_TABLE_ENTRIES; _i++) {
      *(irq_vector_table + _i) = (uint32_t)__VECTOR_TABLE[_i];
   }
   for (_i=0; _i<BSP_ICU_VECTOR_MAX_ENTRIES; _i++) {
      *(irq_vector_table + _i +BSP_CORTEX_VECTOR_TABLE_ENTRIES) = (uint32_t)g_vector_table[_i];
   }

   SCB->VTOR = (uint32_t)irq_vector_table;

   __DSB();
   __enable_irq();

    #ifdef BACKTRACE_SUPPORT
       // "install" stacktrace print over Hardfault handler
       *(irq_vector_table + 3) = (uint32_t)Stacktrace_Handler;
       cm_backtrace_init(stringify(PROJECT_NAME), "RA", __DATE__ "\0");
    #endif

    _init();
     initVariant();
    analogReference();

   #ifndef NO_USB
       __USBStart();
       Serial.begin(115200);
   #endif
   startAgt();
   #endif
}

void MW_ArduinoRaspberryPiPicoWInit(void)
{
    #if (defined(ARDUINO_RASPBERRY_PI_PICO_W) || defined(ARDUINO_RASPBERRY_PI_PICO))

    #if F_CPU != 125000000
        set_sys_clock_khz(F_CPU / 1000, true);
    #endif
        
    rp2040.begin();
    initVariant();
    rp2040.enableDoubleResetBootloader();

    #ifndef NO_USB
    #ifdef USE_TINYUSB
    TinyUSB_Device_Init(0);

    #else
    __USBStart();

    #ifndef DISABLE_USB_SERIAL
    // Enable serial port for reset/upload always
    Serial.begin(115200);
    #endif
    #endif
    #endif

    #endif
}

void MW_Arduino_Init(void)
{
    //Hardware specific init for Arduino nano 33 ble sense
    #if (defined(ARDUINO_ARDUINO_NANO33BLE))
    MW_ArduinoNano33BLESenseInit();
    #endif
    #if defined(ARDUINO_NANO_RP2040_CONNECT)
    MW_ArduinoNanoRP2040Init();
    #endif
    #if (defined(ARDUINO_RASPBERRY_PI_PICO_W) || defined(ARDUINO_RASPBERRY_PI_PICO))
    MW_ArduinoRaspberryPiPicoWInit();
    #endif
    //Disable watchdog for Arduino Due.
    #if (defined(_ROTH_DUE_))
    watchdogDisable();
    #endif

    #if (defined(ESP_PLATFORM))
    // Configure CPU frequency (matches main.cpp)
    #ifdef F_XTAL_MHZ
    #if !CONFIG_IDF_TARGET_ESP32S2  // ESP32-S2 does not support rtc_clk_xtal_freq_update
    rtc_clk_xtal_freq_update((rtc_xtal_freq_t)F_XTAL_MHZ);
    rtc_clk_cpu_freq_set_xtal();
    #endif
    #endif
    #ifdef F_CPU
    setCpuFrequencyMhz(F_CPU / 1000000);
    #endif
    //TO-DO: Revisit when enhancement g4207628 is taken up.    
    Serial.begin(_RTT_EXTMODE_BAUDRATE_);
    #if ARDUINO_USB_MSC_ON_BOOT && !ARDUINO_USB_MODE
    MSC_Update.begin();
    #endif
    #if ARDUINO_USB_DFU_ON_BOOT && !ARDUINO_USB_MODE
    USB.enableDFU();
    #endif
    #if ARDUINO_USB_ON_BOOT && !ARDUINO_USB_MODE
    USB.begin();
    #endif
    initArduino();
    #endif

    //Hardware specific init for Arduino UNO R4 WiFi
    #if(defined(ARDUINO_ARCH_RENESAS_UNO))
    UNOR4_Init();
    #endif

    /*
     * Initialize Serial Peripheral
     */
    // Always Initialize Serial0
    #if (defined(ESP_PLATFORM))
    #else
    Serial_begin(0, _RTT_BAUDRATE_SERIAL0_, _RTT_CONFIG_SERIAL0_);
    #endif

    /* Overrun Detection Initialization */
    #ifdef _RTT_OVERRUN_DIGITAL_PIN_
    pinMode(_RTT_OVERRUN_DIGITAL_PIN_, OUTPUT);

    /* Init the overrun detection pin to zero or else the Pin is always in a
    * high/tristate giving the user a wrong indication of overrun conditions */
    digitalWrite(_RTT_OVERRUN_DIGITAL_PIN_, LOW);
    #endif

    /*
    * Initialize Analog Reference Voltage
    * Default - Due
    * Default, Internal 1.1, Internal 2.56, External -  Mega 2560, Mega ADK
    * Default, Internal 0.6, Internal 1.2, Internal 2.4 -  BLE Sense
    * Default, Internal, External - Other Arduino targets
    */
    #if (defined(_ROTH_MEGA2560_))
    #if (defined(_RTT_ANALOG_REF_) && (_RTT_ANALOG_REF_ == 1))
    analogReference(INTERNAL1V1);
    #elif (defined(_RTT_ANALOG_REF_) && (_RTT_ANALOG_REF_ == 2))
    analogReference(INTERNAL2V56);
    #elif (defined(_RTT_ANALOG_REF_) && (_RTT_ANALOG_REF_ == 3))
    analogReference(EXTERNAL);
    #endif
    #elif defined(ARDUINO_ARM_CORTEX_M0_PLUS)
    #if (defined(_RTT_ANALOG_REF_) && (_RTT_ANALOG_REF_ == 1))
    analogReference(AR_INTERNAL1V0);
    #elif (defined(_RTT_ANALOG_REF_) && (_RTT_ANALOG_REF_ == 2))
    analogReference(AR_INTERNAL1V65);
    #elif (defined(_RTT_ANALOG_REF_) && (_RTT_ANALOG_REF_ == 3))
    analogReference(AR_INTERNAL2V23);
    #elif (defined(_RTT_ANALOG_REF_) && (_RTT_ANALOG_REF_ == 4))
    analogReference(AR_EXTERNAL);
    #endif
    #elif defined(ARDUINO_ARDUINO_NANO33BLE)
    #if (defined(_RTT_ANALOG_REF_) && (_RTT_ANALOG_REF_ == 1))
    analogReference(AR_VDD);
    #elif (defined(_RTT_ANALOG_REF_) && (_RTT_ANALOG_REF_ == 2))
    analogReference(AR_INTERNAL);
    #elif (defined(_RTT_ANALOG_REF_) && (_RTT_ANALOG_REF_ == 3))
    analogReference(AR_INTERNAL1V2);
    #elif (defined(_RTT_ANALOG_REF_) && (_RTT_ANALOG_REF_ == 4))
    analogReference(AR_INTERNAL2V4);
    #endif
    #elif defined(ARDUINO_UNOR4_WIFI)
    #if (defined(_RTT_ANALOG_REF_) && (_RTT_ANALOG_REF_ == 1))
    analogReference(AR_DEFAULT);
    #elif (defined(_RTT_ANALOG_REF_) && (_RTT_ANALOG_REF_ == 2))
    analogReference(AR_INTERNAL);
    #endif
    
    #else
    #if (defined(_RTT_ANALOG_REF_) && (_RTT_ANALOG_REF_ == 1))
    analogReference(INTERNAL);
    #elif (defined(_RTT_ANALOG_REF_) && (_RTT_ANALOG_REF_ == 2))
    analogReference(EXTERNAL);
    #endif
    #endif

    /*
    * Initialize SPI bus
    */
    //_RTT_SPI_SSPIN_ definition is applicable for all boards excluding Due
    #if (defined(_RTT_SPI_SSPIN_)) || (defined(MW_ARDUINO_OV2640CSPIN))
    MW_SPIwriteReadSetup();
    #endif

    //Initialize Modbus
    int modStatus;
    #if defined(_RTT_MODBUS_MASTER_)
    modStatus = ModbusRTUClient.begin(_RTT_RS485_BAUD_, _RTT_RS485_CONFIG_);
    ModbusRTUClient.setTimeout(_RTT_MODBUS_MASTERTIMEOUT_/1000);
    #endif

    #if defined(_RTT_MODBUS_SLAVE_)
    modStatus = ModbusRTUServer.begin(_RTT_MODBUS_SLAVEID_, _RTT_RS485_BAUD_, _RTT_RS485_CONFIG_);
    #if defined(_RTT_MODBUS_CONFIGCOIL_)
    ModbusRTUServer.configureCoils(_RTT_MODBUS_COILADDR_,_RTT_MODBUS_COILNUM_);
    #endif
    #if defined(_RTT_MODBUS_CONFIGINPUT_)
    ModbusRTUServer.configureDiscreteInputs(_RTT_MODBUS_INPUPTADDR_,_RTT_MODBUS_INPUTNUM_);
    #endif
    #if defined(_RTT_MODBUS_CONFIGHOLDINGREG_)
    ModbusRTUServer.configureHoldingRegisters(_RTT_MODBUS_HOLDINGREGADDR_,_RTT_MODBUS_HOLDINGREGNUM_);
    #endif
    #if defined(_RTT_MODBUS_CONFIGINPUTREG_)
    ModbusRTUServer.configureInputRegisters(_RTT_MODBUS_INPUTREGADDR_,_RTT_MODBUS_INPUTREGNUM_);
    #endif
    #endif
    /*
    * Initialize wifi
    */
    #if (defined(_RTT_WIFI_UDP_) && (_RTT_WIFI_UDP_ != 0)) || (defined(_RTT_WIFI_TCP_) && (_RTT_WIFI_TCP_ != 0))|| defined(_RTT_THINGSPEAK_WIFI_) || defined(_RTT_WEBSOCKET_PORT_) || defined(_RTT_MQTT_WIFI_) || defined(_RTT_HTTP_CLIENT_) || defined(OTA_SETUP) || defined(OTA_UPLOAD)
    /* Begin WiFi*/
    while(configureSuccess!=WL_CONNECTED)
    {
        #if (defined(_RTT_DISABLE_Wifi_DHCP_) && (_RTT_DISABLE_Wifi_DHCP_ !=0))
            #if defined(ESP_PLATFORM)
                #if (_RTT_DISABLE_Wifi_DHCP_ == 1)
                WiFi.config(wifiLocalIpAddress,wifigateway, subnet, dns1, dns2);
                #else
                WiFi.config(wifiLocalIpAddress, wifiGatewayAddress, wifiSubnetMask, wifiDNSAddress);
                #endif
            #elif (_RTT_DISABLE_Wifi_DHCP_ == 1)
            WiFi.config(wifiLocalIpAddress);
            #else
            WiFi.config(wifiLocalIpAddress, wifiDNSAddress, wifiGatewayAddress, wifiSubnetMask);
            #endif
        #endif

        #ifdef _RTT_WIFI_WEP
        configureSuccess=WiFi.begin(ssid,keyIndex,key);
        #endif //_RTT_WIFI_WEP

        #ifdef _RTT_WIFI_WPA
        configureSuccess=WiFi.begin(ssid,wpapass);
        #endif //_RTT_WIFI_WPA

        #ifdef _RTT_WIFI_WPA2_ENTERPRISE
        #if defined(ESP_PLATFORM) 
        WiFi.disconnect(true);  //disconnect form wifi to set new wifi connection
        WiFi.mode(WIFI_STA); //init wifi mode
        configureSuccess = WiFi.begin(ssid, WPA2_AUTH_PEAP, wpa2username, wpa2username, wpa2password); //connect to wifi
        
        #elif defined(ARDUINO_UNOR4_WIFI)
        WiFi.disconnect();  //disconnect form wifi to set new wifi connection
        int extmodewifistatus = WiFi.begin(ssid,wpa2password); //connect to wifi
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
        }
        extmodewifistatus = WL_CONNECTED;

        #elif defined(ARDUINO_WIFI_LIB_NINA)
        configureSuccess=WiFi.beginEnterprise(ssid,wpa2username,wpa2password);
        #endif
        #endif //_RTT_WIFI_WPA2_ENTERPRISE

        #ifdef _RTT_WIFI_NONE
        configureSuccess=WiFi.begin(ssid);
        #endif //_RTT_WIFI_NONE

        #if (defined(ESP_PLATFORM)) || defined(ARDUINO_UNOR4_WIFI)
        while (WiFi.status() != WL_CONNECTED) {
            delay(500);
        }
        configureSuccess=WL_CONNECTED;
        #else
        //Wait for 10 seconds for Wifi connection to happen
        unsigned long startMillis  = millis();
        while(millis()- startMillis < 1000)
        {
        };
        #endif // ESP_PLATFORM
    }

    #if (!defined(OTA_UPLOAD)) // Don't need serial open for OTA upload
    /* Wait for Serial0 to be up*/
    for(trialcount=0;trialcount<5;trialcount++)
    {
        if(Serial)
        {
            break;
        }else
        {
            unsigned long startMillis  = millis();
            while(millis()- startMillis < 1000)
            {
            };
        }
    }
    
    #if (!(defined(_RTT_WIFI_EXTMODE_) && (_RTT_WIFI_EXTMODE_ == 1)))
    if (configureSuccess==WL_CONNECTED)
    {
        // If the Configuration is successful, relay back the assigned IP address.
        #if (defined(ESP_PLATFORM)) || defined(ARDUINO_UNOR4_WIFI)
        Serial_begin(0, _RTT_BAUDRATE_SERIAL0_, _RTT_CONFIG_SERIAL0_);
        #endif
        IPAddress ip = WiFi.localIP();
        //Adding a precautionary check using a Serial command so as to print the IP address so that it is available at least 6 times
        //and the chances of it not being assigned to Target are minimised.
        int ipSendcounter = 0; // Initialize counter
        while ((Serial.read()!= 0xaa)  && ipSendcounter < ( _RTT_WIFI_TIMEOUT * 2)){
         ipSendcounter++; // Increment counter at the end of each loop iteration
            delay(500);
        }
    
        Serial.print("<<< IP address :");
        Serial.print(ip);
        Serial.println(" >>>");
    
    }
    else
    {
        // If the Configuration failed,relay back the error message.
        Serial.println("<<< IP address :Failed to configure. >>>");
        Serial.print(_RTT_WIFI_TIMEOUT);
    }
    #endif //(!(defined(_RTT_WIFI_EXTMODE_) && (_RTT_WIFI_EXTMODE_ == 1)))
    #endif //(!defined(OTA_UPLOAD))

    #endif /*Initialize wifi*/

    // Enable OTA handler
    #if defined(OTA_SETUP) || defined(OTA_UPLOAD)
    MW_OTA_Setup();
    #endif

    #ifdef NANO_MOTOR_CARRIER
    MW_EnableNanoCarrierBatteryCharging();
    #endif
    #ifdef _MW_DASHBOARD_BLOCK_PRESENT_
    MW_initScreen();
    #endif

    #if defined(ARDUINO_ARCH_SAMD) && defined(MW_SAMD_EVSYS_ENABLE)
    PM->APBCMASK.reg |= PM_APBCMASK_EVSYS;                                              // Switch on the event system peripheral
    #if defined(MW_SAMD_EVSYS_CH0)
    EVSYS->USER.reg = EVSYS_USER_CHANNEL(1) |                                           // Attach the event user (receiver) to channel 0 (n + 1)
        EVSYS_USER_USER(EVSYS_CH0_USER_EVENT);                            // Set the event user (receiver) as EVSYS_CH1_USER_EVENT
    EVSYS->CHANNEL.reg = EVSYS_CHANNEL_EDGSEL_NO_EVT_OUTPUT |                           // No event edge detection
        EVSYS_CHANNEL_PATH_ASYNCHRONOUS |                              // Set event path as asynchronous
        EVSYS_CHANNEL_EVGEN(EVSYS_CH0_GEN_EVENT) |                     // Set event generator (sender) as EVSYS_CH1_GEN_EVENT
        EVSYS_CHANNEL_CHANNEL(0);                                      // Attach the generator (sender) to channel 0
    EVSYS_CH0_USER->EVCTRL.reg |= EVSYS_CH0_USER_START_CMD;
    EVSYS_CH0_GEN->EVCTRL.reg |= EVSYS_CH0_GEN_START_CMD;
    #endif
    #if defined(MW_SAMD_EVSYS_CH1)
    EVSYS->USER.reg = EVSYS_USER_CHANNEL(2) |                                           // Attach the event user (receiver) to channel 1 (n + 1)
        EVSYS_USER_USER(EVSYS_CH1_USER_EVENT);                            // Set the event user (receiver) as EVSYS_CH2_USER_EVENT
    EVSYS->CHANNEL.reg = EVSYS_CHANNEL_EDGSEL_NO_EVT_OUTPUT |                           // No event edge detection
        EVSYS_CHANNEL_PATH_ASYNCHRONOUS |                              // Set event path as asynchronous
        EVSYS_CHANNEL_EVGEN(EVSYS_CH1_GEN_EVENT) |                     // Set event generator (sender) as EVSYS_CH2_GEN_EVENT
        EVSYS_CHANNEL_CHANNEL(1);                                      // Attach the generator (sender) to channel 1
    EVSYS_CH1_USER->EVCTRL.reg |= EVSYS_CH1_USER_START_CMD;
    EVSYS_CH1_GEN->EVCTRL.reg |= EVSYS_CH1_GEN_START_CMD;
    #endif
    #if defined(MW_SAMD_EVSYS_CH2)
    EVSYS->USER.reg = EVSYS_USER_CHANNEL(3) |                                           // Attach the event user (receiver) to channel 2 (n + 1)
        EVSYS_USER_USER(EVSYS_CH2_USER_EVENT);                            // Set the event user (receiver) as EVSYS_CH3_USER_EVENT
    EVSYS->CHANNEL.reg = EVSYS_CHANNEL_EDGSEL_NO_EVT_OUTPUT |                           // No event edge detection
        EVSYS_CHANNEL_PATH_ASYNCHRONOUS |                              // Set event path as asynchronous
        EVSYS_CHANNEL_EVGEN(EVSYS_CH2_GEN_EVENT) |                     // Set event generator (sender) as EVSYS_CH3_GEN_EVENT
        EVSYS_CHANNEL_CHANNEL(2);                                      // Attach the generator (sender) to channel 2
    EVSYS_CH2_USER->EVCTRL.reg |= EVSYS_CH2_USER_START_CMD;
    EVSYS_CH2_GEN->EVCTRL.reg |= EVSYS_CH2_GEN_START_CMD;
    #endif
    #if defined(MW_SAMD_EVSYS_CH3)
    EVSYS->USER.reg = EVSYS_USER_CHANNEL(4) |                                           // Attach the event user (receiver) to channel 3 (n + 1)
        EVSYS_USER_USER(EVSYS_CH3_USER_EVENT);                            // Set the event user (receiver) as EVSYS_CH4_USER_EVENT
    EVSYS->CHANNEL.reg = EVSYS_CHANNEL_EDGSEL_NO_EVT_OUTPUT |                           // No event edge detection
        EVSYS_CHANNEL_PATH_ASYNCHRONOUS |                              // Set event path as asynchronous
        EVSYS_CHANNEL_EVGEN(EVSYS_CH3_GEN_EVENT) |                     // Set event generator (sender) as EVSYS_CH4_GEN_EVENT
        EVSYS_CHANNEL_CHANNEL(3);                                      // Attach the generator (sender) to channel 3
    EVSYS_CH3_USER->EVCTRL.reg |= EVSYS_CH3_USER_START_CMD;
    EVSYS_CH3_GEN->EVCTRL.reg |= EVSYS_CH3_GEN_START_CMD;
    #endif
    #endif
}
/* Function: MW_Arduino_Terminate ================================================
* For Arduino MKR/Leonardo and its variants, the Virtual COM port is handled
* by the controller. In case the code running on the target exits main,
* the COM port cannot be accessed until a hard reset is performed.
* To over come this issue, a while loop is added to make sure that
* upon getting a stop command from external mode, the code running on
* the target stops but the code will not exit the main.
* This will ensure that the COM port is accessible even after the
* external mode has been stopped. */
void MW_Arduino_Terminate(void){
    
    #if(defined(ARDUINO_UNOR4_MINIMA) || defined(NANO_ESP32))
    while(1){};
    #endif

    #if defined(ARDUINO_VIRTUAL_COM_PORT) || (defined(ARDUINO_ARDUINO_NANO33BLE) && (!defined(USE_MBEDRTOS))) || defined(__IMXRT1062__) || (defined(ARDUINO_NANO_RP2040_CONNECT) && (!defined(USE_MBEDRTOS))) || defined(ARDUINO_RASPBERRY_PI_PICO_W) || defined(ARDUINO_RASPBERRY_PI_PICO)
    disable_rt_OneStep();
    while(1){};     /* To continue virtual COM port functionality */
    #endif    
}

#ifdef NANO_MOTOR_CARRIER
void MW_EnableNanoCarrierBatteryCharging(void) {
    Wire.begin();
    Wire.beginTransmission(PMIC_ADDRESS);
    Wire.write(PMIC_REG00);
    Wire.write(0x06); // min sys voltage 3.88V + max input current 2.0 A
    Wire.endTransmission();
    Wire.beginTransmission(PMIC_ADDRESS);
    Wire.write(PMIC_REG01);
    Wire.write(0x1B); // Charge Battery + Minimum System Voltage 3.5V
    Wire.endTransmission();
    Wire.beginTransmission(PMIC_ADDRESS);
    Wire.write(PMIC_REG02);
    Wire.write(0x00); // Charge current  512 mA
    Wire.endTransmission();
    Wire.beginTransmission(PMIC_ADDRESS);
    Wire.write(PMIC_REG04);
    Wire.write(0x9E); // Charge Voltage Limit 4.128V
    Wire.endTransmission();
    Wire.beginTransmission(PMIC_ADDRESS);
    Wire.write(PMIC_REG05);
    Wire.write(0x8A); // Enable Battery Charge termination + disable watchdog
    Wire.endTransmission();
}
#endif


// Overrun detection function for ESP32
#if defined(ESP_PLATFORM) && defined(_RTT_OVERRUN_DIGITAL_PIN_)
void onOverrunDetect(int id)
{
    digitalWrite(_RTT_OVERRUN_DIGITAL_PIN_, HIGH);
}
#endif
// LocalWords:  AVR Bareboard ADK SPI RTT SSPIN Modbus MKR