/*
* Wrappers to make I/O functions available with C linkage. This allows C++
* methods to be called from C code.
*
* Copyright 2011-2026 The MathWorks, Inc. */

#include <inttypes.h>
#include "Arduino.h"
#include "rtwtypes.h"

#if (defined(_RTT_TCPIP_EXTMODE_) && (_RTT_TCPIP_EXTMODE_ == 1)) || (defined(_RTT_UDP_) && (_RTT_UDP_ != 0)) || (defined(_RTT_TCP_) && (_RTT_TCP_ != 0) || defined(_RTT_THINGSPEAK_))
#include <Ethernet.h>
#include <EthernetUdp.h>
#include <SPI.h>
#endif

#include "NanoESP32PinRemap.h"

extern "C" {

    #if (defined(ESP_PLATFORM) && defined(BOARD_HAS_PIN_REMAP))
    // Required by Arduino ESP32 core v3.2.1 for boards with pin remapping
    // (e.g., Arduino Nano ESP32) where Arduino pins differ from GPIO numbers.
    // digitalPinToInterrupt macro calls this for pin validation.
    int8_t digitalPinToGPIONumber(uint8_t pin) {
        return convertPinToGPIO(pin);
    }
    #endif

}

#if defined(ARDUINO_ARM_CORTEX_M0_PLUS)
extern "C" void __libc_init_array(void);
#endif

extern "C" {
    void __cxa_pure_virtual(void);
}

extern "C" void Serial_begin(int port, long r, int c)
{
    static int serialInit=0;
    #if !(defined(ESP_PLATFORM))
    serialInit=0;   //Reset serialInit flag for all targets except ESP32
    #endif
    #if defined(ARDUINO_ARCH_SAM)
    UARTClass::UARTModes config = UARTClass::UARTModes(c);
    #else
    int config = c;
    #endif
    #if defined(ARDUINO_NUM_SERIAL_PORTS)
    switch(port) {
        case 0:
            if(serialInit & 1)return;
            serialInit |= 1;
            #if defined(SERIAL_MULTIPURPOSE_0)
                #if defined(ESP_PLATFORM)
                    Serial.begin(r,config,_RTT_RX_PIN_SERIAL0_,_RTT_TX_PIN_SERIAL0_);
                #elif defined(ARDUINO_ARCH_RP2040)
                    gpio_set_function(_RTT_RX_PIN_SERIAL0_, GPIO_FUNC_UART);
                    gpio_set_function(_RTT_TX_PIN_SERIAL0_, GPIO_FUNC_UART);
                    Serial.begin(r,config);
                #endif
            #elif (defined(ARDUINO_TEENSY40) || defined(ARDUINO_TEENSY41))
                Serial.begin(r);
            #elif (defined(ESP_PLATFORM) && defined(ARDUINO_USB_CDC_ON_BOOT) && (ARDUINO_USB_CDC_ON_BOOT == 1)) //Arduino Nano ESP32
                Serial.begin(r);
            #else
                Serial.begin(r,config);
            #endif
            break;
        case 1:
            if(serialInit & 2)return;
            serialInit |= 2;
            #if(ARDUINO_NUM_SERIAL_PORTS >= 2)            
            #if defined(SERIAL_MULTIPURPOSE_1)
                #if defined(ESP_PLATFORM)   
                    Serial1.begin(r,config,_RTT_RX_PIN_SERIAL1_,_RTT_TX_PIN_SERIAL1_);
                #elif defined(ARDUINO_ARCH_RP2040)
                    gpio_set_function(_RTT_RX_PIN_SERIAL1_, GPIO_FUNC_UART);
                    gpio_set_function(_RTT_TX_PIN_SERIAL1_, GPIO_FUNC_UART);
                    Serial1.begin(r,config);
                #endif
            #elif (defined(ARDUINO_TEENSY40) ||defined(ARDUINO_TEENSY41))
            Serial1.begin(r);
            #elif (defined(ARDUINO_NANO_ESP32) || defined(ARDUINO_ESP32S3_DEV)) //Arduino Nano ESP32 & Espressif ESP32-S3-Series support Serial1 (No pin multipurposing)
            Serial1.begin(r,config,_RTT_RX_PIN_SERIAL1_,_RTT_TX_PIN_SERIAL1_);     
            #else
            Serial1.begin(r,config);
            #endif           
            #endif
            break;
        case 2:
            if(serialInit & 4)return;
            serialInit |= 4;
            #if(ARDUINO_NUM_SERIAL_PORTS >= 3)            
            #if defined(SERIAL_MULTIPURPOSE_2)
                #if defined(ESP_PLATFORM)
                    Serial2.begin(r,config,_RTT_RX_PIN_SERIAL2_,_RTT_TX_PIN_SERIAL2_);
                #elif defined(ARDUINO_ARCH_RP2040)
                gpio_set_function(_RTT_RX_PIN_SERIAL2_, GPIO_FUNC_UART);
                gpio_set_function(_RTT_TX_PIN_SERIAL2_, GPIO_FUNC_UART);
                Serial2.begin(r,config);
                #endif
            #elif (defined(ARDUINO_TEENSY40) ||defined(ARDUINO_TEENSY41))
            Serial2.begin(r);
            #elif defined(ARDUINO_NANO_ESP32) //Arduino Nano ESP32 supports Serial2 (No pin multipurposing)
            Serial2.begin(r,config,_RTT_RX_PIN_SERIAL2_,_RTT_TX_PIN_SERIAL2_);
            #else
            Serial2.begin(r,config);
            #endif           
            #endif
            break;
        case 3:
            if(serialInit & 8)return;
            serialInit |= 8;
            #if(ARDUINO_NUM_SERIAL_PORTS >= 4)
            #if (defined(ARDUINO_TEENSY40) ||defined(ARDUINO_TEENSY41))
            Serial3.begin(r);
            #else
            Serial3.begin(r,config);
            #endif
            #endif

            break;
        case 4:
            if(serialInit & 16)return;
            serialInit |= 16;     
            #if(ARDUINO_NUM_SERIAL_PORTS >= 5)
            #if (defined(ARDUINO_TEENSY40) ||defined(ARDUINO_TEENSY41))
            Serial4.begin(r);
            #else
            Serial4.begin(r,config);
            #endif
            #endif
            break;
       case 5:
            if(serialInit & 32)return;
            serialInit |= 32;
            #if(ARDUINO_NUM_SERIAL_PORTS >= 6)
            #if (defined(ARDUINO_TEENSY40) ||defined(ARDUINO_TEENSY41))
            Serial5.begin(r);
            #else
            Serial5.begin(r,config);
            #endif
            #endif

            break;
       case 6:
            if(serialInit & 64)return;
            serialInit |= 64;
            #if(ARDUINO_NUM_SERIAL_PORTS >= 7)
            #if (defined(ARDUINO_TEENSY40) ||defined(ARDUINO_TEENSY41))
            Serial6.begin(r);
            #else
            Serial6.begin(r,config);
            #endif
            #endif

            break;
        case 7:
            if(serialInit & 128)return;
            serialInit |= 128;
            #if(ARDUINO_NUM_SERIAL_PORTS >= 8)
            #if (defined(ARDUINO_TEENSY40) ||defined(ARDUINO_TEENSY41))
            Serial7.begin(r);
            #else
            Serial7.begin(r,config);
            #endif
            #endif
      case 8:
            if(serialInit & 256)return;
            serialInit |= 256;
            #if(ARDUINO_NUM_SERIAL_PORTS >= 9)
            #if (defined(ARDUINO_TEENSY41))
            Serial8.begin(r);
            #else
            Serial8.begin(r,config);
            #endif
            #endif

            break;

            break;
    }
    #else
    if(serialInit & 1)return;
    serialInit |= 1;
    Serial.begin(r,SERIAL_8N1);
    #endif
}

extern "C" void Serial_read(int port, int showOutStatus, uint8_t *outData, int16_t *outStatus)
{
    int libFcnOutput;
    #if defined(ARDUINO_NUM_SERIAL_PORTS)
    switch(port) {
        case 0:
            libFcnOutput = Serial.read();
            break;
        case 1:
            #if(ARDUINO_NUM_SERIAL_PORTS >= 2)
            libFcnOutput = Serial1.read();
            #endif
            break;
        case 2:
            #if(ARDUINO_NUM_SERIAL_PORTS >= 3)
            libFcnOutput = Serial2.read();
            #endif
            break;
        case 3:
            #if(ARDUINO_NUM_SERIAL_PORTS >= 4)
            libFcnOutput = Serial3.read();
            #endif
            break;
        case 4:
            #if(ARDUINO_NUM_SERIAL_PORTS >= 5)
            libFcnOutput = Serial4.read();
            #endif
            break;
        case 5:
            #if(ARDUINO_NUM_SERIAL_PORTS >= 6)
            libFcnOutput = Serial5.read();
            #endif
            break;
        case 6:
            #if(ARDUINO_NUM_SERIAL_PORTS >= 7)
            libFcnOutput = Serial6.read();
            #endif
            break;
       case 7:
            #if(ARDUINO_NUM_SERIAL_PORTS >= 8)
            libFcnOutput = Serial7.read();
            #endif
            break;
       case 8:
            #if(ARDUINO_NUM_SERIAL_PORTS >= 9)
            libFcnOutput = Serial8.read();
            #endif
            break;
    }
    #else
    libFcnOutput = Serial.read();
    #endif
    *outData = (uint8_t) libFcnOutput;
    *outStatus = (libFcnOutput != -1);
}

extern "C" void Serial_write(int port, uint8_t * c, size_t s)
{
    #if defined(ARDUINO_NUM_SERIAL_PORTS)
    switch(port) {
        case 0:
            Serial.write(c, s);
            break;
        case 1:
            #if(ARDUINO_NUM_SERIAL_PORTS >= 2)
            Serial1.write(c, s);
            #endif
            break;
        case 2:
            #if(ARDUINO_NUM_SERIAL_PORTS >= 3)
            Serial2.write(c, s);
            #endif
            break;
        case 3:
            #if(ARDUINO_NUM_SERIAL_PORTS >= 4)
            Serial3.write(c, s);
            #endif
            break;
        case 4:
            #if(ARDUINO_NUM_SERIAL_PORTS >= 5)
            Serial4.write(c, s);
            #endif
            break;
        case 5:
            #if(ARDUINO_NUM_SERIAL_PORTS >= 6)
            Serial5.write(c, s);
            #endif
            break;
        case 6:
            #if(ARDUINO_NUM_SERIAL_PORTS >= 7)
            Serial6.write(c, s);
            #endif
            break;
        case 7:
            #if(ARDUINO_NUM_SERIAL_PORTS >= 8)
            Serial7.write(c, s);
            #endif
            break;
        case 8:
            #if(ARDUINO_NUM_SERIAL_PORTS >= 9)
            Serial8.write(c, s);
            #endif
            break;
    }
    #else
    Serial.write(c, s);
    #endif
}

#if _RTT_UDP_ != 0
EthernetUDP Udp[_RTT_UDP_];
IPAddress localIpAddress(_RTT_Local_IP1, _RTT_Local_IP2, _RTT_Local_IP3, _RTT_Local_IP4);
byte mac[] = { _RTT_Local_MAC1, _RTT_Local_MAC2, _RTT_Local_MAC3, _RTT_Local_MAC4, _RTT_Local_MAC5, _RTT_Local_MAC6 };
unsigned int localport[_RTT_UDP_] = {_RTT_UDP_LOCAL_PORT_ORDERED};
int configureSuccess;
int trialcount=0;
extern "C" uint8_t MW_EthernetAndUDPBegin(uint8_t udpindex, uint32_t localPort)
{
    static int only_one_ethernet_begin;
    if (only_one_ethernet_begin==0)
    {
        #if (defined(_RTT_DISABLE_Ethernet_DHCP_) && (_RTT_DISABLE_Ethernet_DHCP_ !=0))
        Ethernet.begin(mac, localIpAddress);
        configureSuccess = 1;
        #else
        configureSuccess = Ethernet.begin(mac);
        #endif
        only_one_ethernet_begin++;

        for(trialcount=0;trialcount<5;trialcount++)
        {
            if(Serial)
            {
                break;
            }else
            {
                delay(1000);
            }
        }
        // Depending on if the IP address assignment using DHCP has been successful or not,
        // send the appropriate information back to the host.
        // If External mode over TCP/IP or Wifi, do not print this Info. Information of IP address will be printed as
        // a part of rtiostream_open
        #if (!(defined(_RTT_TCPIP_EXTMODE_) && (_RTT_TCPIP_EXTMODE_ == 1)))
        if (configureSuccess==1)
        {
            if(Ethernet.localIP()[0] == 0 || Ethernet.localIP()[0] == 255)
            {
                /*Tried to assign Static IP, failed due to one of the following reasons :
                * 1. Ethernet shield is not connected properly with Arduino.
                * 2. Ethernet shield is not connected to a network.
                **/
                Serial.println("<<<IP address: Failed to configure.>>>");
            }
            else
            {
                // If the Configuration is successful, relay back the assigned IP address.
                Serial.print("<<<IP address: ");
                for (byte thisByte = 0; thisByte < 4; thisByte++)
                {
                    // Print the value of each byte of the IP address:
                    Serial.print(Ethernet.localIP()[thisByte], DEC);
                    if (thisByte<3)
                    {
                        Serial.print(".");
                    }
                }
                Serial.println(">>>");
            }
        }
        else
        {
            // If the Configuration failed,relay back the error message.
            Serial.println("<<<IP address: Failed to configure.>>>");
        }
        Serial.flush();
        #endif
    }
    return (Udp[udpindex].begin(localport[udpindex]));
}

extern "C" int MW_UDPFinalWrite(uint8_t udpindex, uint8_t data, uint8_t remoteIPOctect1,uint8_t remoteIPOctect2,uint8_t remoteIPOctect3,uint8_t remoteIPOctect4, uint32_t remoteport)
{
    IPAddress remoteIpAddress(remoteIPOctect1,remoteIPOctect2, remoteIPOctect3, remoteIPOctect4);
    Udp[udpindex].beginPacket(remoteIpAddress, remoteport);
    Udp[udpindex].write(data);
    return(Udp[udpindex].endPacket());
}

extern "C" void MW_UDPFinalRead(uint8_t udpindex, uint8_t *data, int32_t *outStatus)
{
    int packetSize = Udp[udpindex].parsePacket();
    if(packetSize)
    {
        int libFcnOutput;
        libFcnOutput = Udp[udpindex].read();
        *data = (uint8_t) libFcnOutput;
        *outStatus = (libFcnOutput != -1);
    }

    else
    {
        //When the client is not connected the status flag is accordingly toggled.
        *outStatus = 0;
    }
}
#endif

#if defined(_RTT_THINGSPEAK_) || (_RTT_TCP_ != 0)
IPAddress localIpAddress(_RTT_Local_IP1, _RTT_Local_IP2, _RTT_Local_IP3, _RTT_Local_IP4);
byte mac[] = { _RTT_Local_MAC1, _RTT_Local_MAC2, _RTT_Local_MAC3, _RTT_Local_MAC4, _RTT_Local_MAC5, _RTT_Local_MAC6 };
int configureSuccess;
int trialcount;
#if _RTT_TCP_ != 0
EthernetServer server[_RTT_TCP_] = {_RTT_TCP_SERVER_PORT_ORDERED};
#endif
extern "C" void MW_EthernetAndTCPServerBegin(uint8_t tcpindex, uint32_t serverport)
{
    static int only_one_tcp_server_and_ethernet_begin = 0;

    if (only_one_tcp_server_and_ethernet_begin==0)
    {
        #if (defined(_RTT_DISABLE_Ethernet_DHCP_) && (_RTT_DISABLE_Ethernet_DHCP_ !=0))
        Ethernet.begin(mac, localIpAddress);
        configureSuccess = 1;
        #else
        configureSuccess = Ethernet.begin(mac);
        #endif
        only_one_tcp_server_and_ethernet_begin++;

        for(trialcount=0;trialcount<5;trialcount++)
        {
            if(Serial)
            {
                break;
            }else
            {
                delay(1000);
            }
        }

        // Depending on if the IP address assignment using DHCP has been successful or not,
        // send the appropriate information back to the host.
        // If External mode over TCP/IP or Wifi, do not print this Info. Information of IP address will be printed as
        // a part of rtiostream_open
        #if (!(defined(_RTT_TCPIP_EXTMODE_) && (_RTT_TCPIP_EXTMODE_ == 1)))
        if (configureSuccess==1)
        {
            if(Ethernet.localIP()[0] == 0 || Ethernet.localIP()[0] == 255)
            {
                /*Tried to assign Static IP, failed due to one of the following reasons :
                * 1. Ethernet shield is not connected properly with Arduino.
                * 2. Ethernet shield is not connected to a network.
                **/
                Serial.println("<<<IP address: Failed to configure.>>>");
            }
            else
            {
                // If the Configuration is successful, relay back the assigned IP address.
                Serial.print("<<<IP address: ");
                for (byte thisByte = 0; thisByte < 4; thisByte++)
                {
                    // Print the value of each byte of the IP address:
                    Serial.print(Ethernet.localIP()[thisByte], DEC);
                    if (thisByte<3)
                    {
                        Serial.print(".");
                    }
                }
                Serial.println(">>>");
            }
        }
        else
        {
            // If the Configuration failed,relay back the error message.
            Serial.println("<<<IP address: Failed to configure.>>>");
        }
        Serial.flush();
        #endif

        #if _RTT_TCP_ != 0
        // Begin all servers based on the number of different TCP ports.
        for (int i = 0;i < _RTT_TCP_;i++)
        {
            server[i].begin();
        }
        #endif
        only_one_tcp_server_and_ethernet_begin = 1;
    }
}
#endif
#if _RTT_TCP_ != 0
extern "C" void MW_TCPFinalread(uint8_t tcpindex, uint8_t *data, uint32_t serverport, int32_t *outStatus)
{
    int libFcnOutput;
    EthernetClient client = server[tcpindex].available();
    if (int(client) == true) {
        libFcnOutput = client.read();
        *data = (uint8_t) libFcnOutput;
        *outStatus = (libFcnOutput != -1);

    }
    else
    {
        //When the client is not connected the status flag is accordingly toggled.
        *outStatus = 0;
    }

}
extern "C" void MW_TCPFinalWrite(uint8_t tcpindex, uint8_t data, uint32_t serverport)
{
    server[tcpindex].write(data);
}
#endif


#if defined(_ROTH_LEONARDO_)
extern "C" void MW_usbattach()
{
    USBDevice.attach();
}
#elif defined(ARDUINO_VIRTUAL_COM_PORT)
extern "C" void MW_usbattach()
{
    __libc_init_array();
    USBDevice.init();
    USBDevice.attach();

}
#endif

extern "C" void MW_pinModeExternalInterrupt(uint8_t pinNumber)
{
    pinMode(pinNumber, INPUT_PULLUP);
}

extern "C" void MW_delay_in_milliseconds(uint32_T period)
{
    uint32_T start_time = 0;
    start_time = millis();
    while(millis()- start_time <= period)
    {
        1;
    }
}

extern "C" void MW_delay_in_microseconds(uint32_T period)
{
    uint32_T start_time = 0;
    start_time = micros();
    while(micros()- start_time <= period)
    {
        1;
    }
}

extern "C" uint32_T MW_getCurrentTime_in_milliseconds()
{
   return (uint32_T)millis();
}


// #if defined(ARDUINO_ARDUINO_NANO33BLE)
// #include "Arduino.h"
// #include "pins_arduino.h"
// #include "mbed/drivers/AnalogIn.h"
// #include "mbed.h"
//
// extern uint8_t nano33BLE_dutycycle[21];
// extern uint16_t nano33BLE_period[21];
//
//
// events::EventQueue queue(32 * EVENTS_EVENT_SIZE);
// rtos::Thread t;
// mbed::Ticker tick;
//
// static int write_resolution = 8;
// static int read_resolution = 10;
//
// extern "C" void MW_NANO33BLE_analogWrite_1(pin_size_t pin, int val, uint16_t period)
// {
//   if (pin >= PINS_COUNT) {
//     return;
//   }
//   float percent = (float)val/(float)((1 << write_resolution)-1);
//   mbed::PwmOut* pwm = digitalPinToPwm(pin);
//   if (pwm == NULL) {
//     pwm = new mbed::PwmOut(digitalPinToPinName(pin));
//     digitalPinToPwm(pin) = pwm;
//     pwm->period_ms(period);
//   }
//   pwm->write(percent);
// }
//
// extern "C" void MW_NANO33BLE_PWM_Handler()
// {
//     MW_NANO33BLE_analogWrite_1(5, nano33BLE_dutycycle[5], nano33BLE_period[5]);
// }
//
// extern "C" void MW_NANO33BLE_PWM_Setup()
// {
//   t.start(callback(&queue, &events::EventQueue::dispatch_forever));
//   tick.attach(queue.event(MW_NANO33BLE_PWM_Handler),_RTT_PWM_PIN5_CST );
// }
//
//
//
//
// extern "C" void MW_NANO33BLE_analogWrite(PinName pin, int val, uint16_t period)
// {
// //   pin_size_t idx = PinNameToIndex(pin);
// //   if (idx != NOT_A_PIN) {
// //     MW_NANO33BLE_analogWrite_1(idx, 50, period);
// //   } else {
// //     mbed::PwmOut* pwm = new mbed::PwmOut(pin);
// //     pwm->period_ms(2);
// //     float percent = (float)50/(float)((1 << write_resolution)-1);
// //     pwm->write(percent);
// //   }
// }
//
// #endif
