#ifndef PLATFORM_DEFS
#define PLATFORM_DEFS


//#define DEBUG_ODOMETRY_PUTTY ///< If platform is connected to pc with putty software, odometry data can be printed to serial out and saved by the putty software. 
//#define DEBUG_RECEIVE_MSG ///< When message is received via ESP NOW it is logged through UART. Comment this out to disable
//#define DEBUG_SAVE_MSG ///< When received message is processed (Manager state is updated) in Manager::listenToMessage()

#define LEFT_ENCODER 17  ///< Odometry encoder on left wheel.
#define RIGHT_ENCODER 10 ///< Odmetry encoder on right wheel.
#define HCSR_STOP 1     ///< ISR pin for stopping the platform when obstacle was detected by ultrasonic sensors. 

// Lidar connection pins
#define LIDAR_EN 2
#define LIDAR_RX 7
#define LIDAR_TX 6
#define LIDAR_PWM 21

// Wifi setup
#define SSID "TELPOL-19886"
#define WIFI_PASSWORD "38j8gze9sh"
#define SERVER_NAME "http://192.168.21.17:9000"

// Server running on PC endpoints
#define SERVER_POST_ENDPOINT "/receive_post"

// Platform server endpoints
#define PLATFORM_COMMAND_ENDPOINT "/steering"
#define PLATFORM_SCAN_REQ_ENDPOINT "/lidar"

// I2C setup
#define I2C_SDA 11
#define I2C_SCL 12

// Multiprocessing queue setup
#define MAN_TO_HTTP_CAPACITY 50
#define MAN_TO_HTTP_MESSAGE_SIZE 2*sizeof(float)

// Configuration of L298N 
#define L_IN1 3 //5;   // kierunek silnik A
#define L_IN2 4 //6;   // kierunek silnik A
#define L_ENA 13 //18; // PWM silnik A

#define R_IN1 14 //7;  // kierunek silnik B
#define R_IN2 9 //8;   // kierunek silnik B
#define R_ENB 8 //21;  // PWM silnik B

#endif