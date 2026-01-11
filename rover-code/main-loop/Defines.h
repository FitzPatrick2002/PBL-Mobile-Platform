/// @file Defines.h
/// @brief Contains defines of the whole project.
///        Mostly defines specify pinout values and interprocess queue parameters.

#include "QueueMessage.h"

#ifndef PLATFORM_DEFS_H
#define PLATFORM_DEFS_H 

// ----------------Debugs ---------------- //
// Uncomment those to enable debugging messages of some certain piece of software

//#define DEBUG_ODOMETRY_PUTTY ///< If platform is connected to pc with putty software, odometry data can be printed to serial out and saved by the putty software. 
//#define DEBUG_RECEIVE_MSG    ///< When message is received via ESP NOW it is logged through UART. Comment this out to disable
//#define DEBUG_SAVE_MSG       ///< When received message is processed (Manager state is updated) in Manager::listenToMessage()

// ----------------Wheel Encoders ---------------- //

#define LEFT_ENCODER 17  ///< Odometry encoder on left wheel.
#define RIGHT_ENCODER 10 ///< Odmetry encoder on right wheel.
#define HCSR_STOP 1      ///< ISR pin for stopping the platform when obstacle was detected by ultrasonic sensors. 

// ---------------- LiDAR ---------------- //

#define LIDAR_EN 2   ///< Enable pin LiDAR.
#define LIDAR_RX 7   ///< LiDARs receive pin of UART.
#define LIDAR_TX 6   ///< LiDARS transmission pin of UART.
#define LIDAR_PWM 21 ///< LiDARS PWM control pin, which steers the engine which rotates LiDAR.

// ---------------- I2C  ---------------- //

#define I2C_SDA 11   ///< I2C Serial Data (SDA) pin.
#define I2C_SCL 12   ///< I2C Serial Clock (SCL) pin.

// ---------------- Interprocess Queues ---------------- //

// Multiprocessing queue setup
#define MAN_TO_HTTP_CAPACITY 50                       ///< Capacity of the queue from core 1 to core 0. Contains 50 elements whose size is specified by #MAN_TO_HTTP_MESSAGE_SIZE.
#define MAN_TO_HTTP_MESSAGE_SIZE sizeof(QueueMessage) ///< Specifies the size of a single element stored in the interprocess queue (core 1 -> core 0 communication).

// ---------------- L298N Configuration ---------------- //

#define L_IN1 3  ///< Direction of rotation of engine A.  //5;   // kierunek silnik A
#define L_IN2 4  ///< Direction of rotation of engine A.  //6;   // kierunek silnik A
#define L_ENA 13 ///< PWM of engine A. PWM value on the pin determines the rotation speed. //18; // PWM silnik A

#define R_IN1 14 ///< Direction of rotation of engine B.  //7;  // kierunek silnik B
#define R_IN2 9  ///< Direction of rotation of engine B.  //8;   // kierunek silnik B
#define R_ENB 8  ///< PWM of engine B. PWM value on the pin determines the rotation speed. //21; // PWM silnik B

// ---------------- Engines / Joystick ---------------- //

#define JOYSTICK_DEADZONE 20 ///< Specifies deadzone for joystic when steering the engines.

// ---------------- Deprecated Stuff ---------------- //
// Defines replaced by TheSetuper class

// Wifi setup
//#define MY_WIFI_SSID  "TELPOL-19886"
//#define WIFI_PASSWORD "38j8gze9sh"
//#define SERVER_NAME   "http://192.168.21.17:9000"

// Server running on PC endpoints
//#define SERVER_POST_ENDPOINT "/receive_post"

// Platform server endpoints
//#define PLATFORM_COMMAND_ENDPOINT "/steering"
//#define PLATFORM_SCAN_REQ_ENDPOINT "/lidar"

#endif