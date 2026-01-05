#ifndef LIDAR_CONTROLLER_H
#define LIDAR_CONTROLLER_H

#include "lds_all_models.h"
#include <cmath>
#include <vector>
#include <Arduino.h>

// TO SEE:
// https://manuals.plus/pl/slamtec/a1m8-rplidar-a1-low-cost-360-degree-laser-range-scanner-manual

// TO DO:
// - ?

// For the time being we want to debug the stuff that we read from or send to lidar
//#define DEBUG_GPIO        ///< Uncomment if you want to see the gpio info in Serial
//#define DEBUG_SERIAL_OUT  ///< Uncomment if you want to see debug info about gpio output data.

/// @brief Contains class which operates the A1M8 Slamtec lidar.
///        Uses for that kaiaai library compatible with esp32.

/**
 * @brief Contains class which operates the A1M8 Slamtec lidar.
 *        Uses for that kaiaai library compatible with esp32.
 * Exemplary use:
 *   LidarController& lidar_controller = LidarController::getInstance();
 *   long timer = 0;
 *   bool all_scans_completed = false;
 *   void setup() {
 *   Serial.begin(lidar_controller.SERIAL_MONITOR_BAUD);
 *   while(!Serial){
 *     delay(10);
 *   }
 *   Serial.println("Serial - OK");
 *   lidar_controller.setPinout(2, 7, 6, 21);
 *   lidar_controller.init();
 *   Serial.println("LiDAR - OK");
 *   }
 *   void loop() {
 *   // Every 10 seconds perform 5 scans and set flag that scanning is complete
 *   if(millis() - timer > 10000){
 *       lidar_controller.scanNtimes(5);
 *       all_scans_completed = true;
 *       timer = millis();
 *   }
 *   // If scans has been completed, print them to Serial port and reset the flag.
 *   if(all_scans_completed == true){
 *       lidar_controller.printData(Serial); //pcSerial
 *       all_scans_completed = false;
 *   }
 * }
 */
namespace Lidar{
    
    /// @brief A controller class, which simplifies communication with lidar and performs most of the setup.
    ///        Scans are performed in a blocking way. Unless scan is completed, code does not exit the function.
    class LidarController{
    private:
        LDS* lidar; ///< Main lidar class, through which we communicate with lidar
        HardwareSerial LidarSerial = HardwareSerial(1); ///< Serial communication with lidar via UART 1 (Serial 1)

        // ----------- Lidar Pinout ----------- //
        uint8_t gpio_en = 2;  ///< Enable pin.
        uint8_t gpio_rx = 0;  ///< UART receive pin.
        uint8_t gpio_tx = 1;  ///< UART transmit pin.
        uint8_t gpio_pwm = 3; ///< PWM pin controlling LiDARs motor.

        // Lidar internal setup & scanning setup
        int desired_scans_num = 5;            ///< Specifies how many rotations will lidar make during a single scanning session.
        uint8_t every_nth_point = 20; ///< When scan is performed, only every n-th point is included, other are discarded.

        // Storage of measurement points
        std::vector<float> points;
        uint8_t point_counter = 0; ///< Counts how many points have been scanned since last point that has been saved. See #every_nth_point.
        uint8_t scan_counter = 0;  ///< Counts how many scans have been performed.

        // LiDARs orientation in space
        float inclination = 0.0f;  ///< Inclination in degrees of the LiDARs scanning plane to the absolute xy plane (plane on which platform is driving).

        // ----------- Constants ----------- //
        // Constants specify the communication protocol parameters 

        uint8_t hex_dump_pos = 0;  ///< Required by the serialReadCallback

    public:

        // ----------- Constants ----------- //
        // Constants specify the communication protocol parameters 

        const uint32_t SERIAL_MONITOR_BAUD = 115200;
        const uint32_t LIDAR_PWM_FREQ = 10000;
        const uint32_t LIDAR_PWM_BITS = 11;
        const uint32_t LIDAR_PWM_CHANNEL = 2;
        const uint32_t LIDAR_SERIAL_RX_BUF_LEN = 1024;
        const uint32_t HEX_DUMP_WIDTH = 16;

        private:

        // ----------- Constructor ----------- //

        /// @brief Contructor is private to enforce singleton pattern.
        LidarController() {} 

        /// @brief Copy contructor is disabled 
        LidarController(const LidarController &ctrl) = delete;

        /// @brief Copy assignemnt is disabled. 
        LidarController& operator=(LidarController &ctrl) = delete;

    public:

        // ----------- Destructor ----------- //

        ~LidarController() {}

        /// @brief When called for the first time, creates the LidarController object and returns a reference to it.
        /// @return Returns a reference to the static LidarController object.
        static LidarController& getInstance(){
            static LidarController lidarControllerInstance;
            return lidarControllerInstance;
        }

        // ----------- Initilization ----------- //

        /// @brief Initilizes the lidar, defines callbacks and sets them
        void init(){
            // Create instance of a used lidar
            lidar = new LDS_RPLIDAR_A1();

            // Set the callbacks 
            this->lidar->setScanPointCallback(LidarController::getInstance().staticScanPointCallback);
            this->lidar->setPacketCallback(LidarController::getInstance().staticPacketCallback);
            this->lidar->setSerialWriteCallback(LidarController::getInstance().staticSerialWriteCallback);
            this->lidar->setSerialReadCallback(LidarController::getInstance().staticSerialReadCallback);

            this->lidar->setMotorPinCallback(LidarController::staticMotorPinCallback);
            this->lidar->setInfoCallback(LidarController::getInstance().staticInfoCallback);
            this->lidar->setErrorCallback(LidarController::getInstance().staticErrorCallback);

            // Wait for everything to initialize 
            // and start serial communication through the SoftwareSerial object.
            delay(200);
            this->LidarSerial.begin(lidar->getSerialBaudRate(), SERIAL_8N1, gpio_tx, gpio_rx);
            delay(200);

            this->lidar->init();
        }

        // ----------- Operation ----------- //

        /// @brief   Starts the lidar
        /// @returns The result of the startup contains information if it was successfull.
        ///          Refer to the LDS__result_t enum
        LDS::result_t start(){
            return this->lidar->start();
        }

        /// @brief Stops the lidar.
        ///        Resets the #point_counter and #scan_counter.
        void stop(){
            this->lidar->stop();
            this->resetPointCounter();
            this->resetScanCounter();
        }

        /// @brief Invokes the lidars loop.
        ///        Measurements are gathered and stored in ?.
        void loop(){
            this->lidar->loop();
        }

        /// @brief Sets the point counter to zero.
        void resetPointCounter(){
            this->point_counter = 0;
        }

        /// @brief Resets the scans counter to 0.
        void resetScanCounter(){
            this->scan_counter = 0;
        }

        /// @brief Performs N rotations of the lidar and saves the data.
        /// @param[in] n How many times lidar should rotate.
        /// @param[in] inclination LiDARs inclination to the xy plane.
        void scanNtimes(uint8_t n = -1){
            // Prepare the lidar
            this->stop();
            delay(5000);

            // Start the lidar and print outcome
            LDS::result_t start_result = this->start();

            Serial.print("scanNtimes() result: ");
            Serial.println(lidar->resultCodeToString(start_result));

            // If the number of scans has been set globally, perform #desired_scans_num number of scans.
            // If not, perform the number specified locally.
            int loopBound = (n != -1 ? n : this->desired_scans_num);
            while(this->scan_counter < n){
                this->lidar->loop();
            }

            this->stop();

            delay(5000);
        }

        /// @brief Clears the storage vector #points
        void clearPoints(){
            this->points.clear();
        }

        // ----------- Communication ----------- //

        /// @brief Copies the data to desired target.
        ///         We can specify if the storage vector will be cleared afterwards.
        /// @param[out] target Target storage to which all of the points will be copied.
        ///                    Must be of type std::vector<Vector3dPolar>.
        /// @param[in] clearPoints Specifies if we want to clear the #points vector after copying.
        ///                        Defaults to false.
        void copyData(std::vector<float> &target, bool clearPoints = false){
            // Copy points from the original location 
            target = points;

            // If specified, clear the points vector
            if (clearPoints == true){
                this->clearPoints();
            }
        }

        /// @brief Accesses the #points vector. 
        /// @return Reference to the points vector.
        std::vector<float>& accessData(){
            return this->points;
        }

        /// @brief Prints data via chosen Stream.
        /// @param s Stream to which data will be transmitted.
        /// @param clearPoints Specifies if the storage vector #points should be cleared after printing.
        ///                    By default its set to false.
        void printData(Stream &s, bool clearPoints = false){  
            s.println("Format: radius [mm] | phi [deg] | theta [deg]");

            // Print the gathered data
            for(int i = 0; i < this->points.size(); i += 3){
                s.print("Point: ");
                s.print(this->points[i]);
                s.print(" | ");
                s.print(this->points[i + 1]);
                s.print(" | ");
                s.println(this->points[i + 2]);
            }

            /*
            for(Vector3dPolar v : this->points){
                s.print("Point: ");
                s.print(v.radius);
                s.print(" | ");
                s.print(v.phi);
                s.print(" | ");
                s.println(v.theta);
            }
            */

            if (clearPoints == true){
                this->clearPoints();
            }
        }

        // ----------- Getters & Setters ----------- //

        /// @brief Sets the pinout for the lidar.
        ///        Handled pins: enable, rx - UART, tx - UART, pwm - motor pin.
        /// @warning Method must be called before #init(). Should not be used more than once.
        /// @param en Digital pin enable.
        /// @param rx Receive pin for UART transmission.
        /// @param tx Transmit pin for UART transmission.
        /// @param pwm PWM pin cor motor control.
        void setPinout(uint8_t en, uint8_t rx, uint8_t tx, uint8_t pwm){
            gpio_en = en;
            gpio_rx = rx;
            gpio_tx = tx;
            gpio_pwm = pwm;
        }

        /// @brief Specifies how many points will be skipped during scanning.
        ///        Only the n + 1'st point will be saved for every n + 1 scanned points. 
        /// @param n Number of points to skip.
        void setEveryNthPoint(uint8_t n){
            every_nth_point = n;
        }

        /// @brief Specifies the number of rotations, that LiDAR will perform during a scanning session.
        ///        Setter for #desired_scans_num.
        /// @param n New number of rotations, must be positive and within some reasonable bounds (like < 200)
        void setDesiredScansNumber(int n){
            if(n > 0){
                this->desired_scans_num = n;
            }
        }

        /// @brief Changes the used HardwareSerial object.
        ///        Does not validate if such number is valid.
        /// @param num Id of the HardwareSerial port.
        void setHardwareSerial(uint8_t num){
            LidarSerial = HardwareSerial(num);
        }

        /// @brief Sets the LiDARs inclination in degrees to the global xy plane.
        /// @param inc New inclination to the xy plane.
        void setInclination(float inc){
            this->inclination = inc;
        }

        private:

        // ----------- Storage ----------- //

        /// @brief Saves the scan point into the #points vector.
        /// @param distance_mm Distance measured, expressed in mm.
        /// @param angle_deg Angle of measurement in the xy plane, expressed in degrees.
        /// @param theta_deg Physical inclination of LiDAR to the xy plane.
        ///                  Normally it should be the #inclination parameter.
        void saveScanPoint(float distance_mm, float phi_deg, float theta_deg){
            points.push_back(distance_mm);
            points.push_back(phi_deg);
            points.push_back(theta_deg);
        }

        // ----------- Callbacks ----------- //

        // Callbacks specify the callback functions for the LDS lidar
        // Each callback has a static wrapper that is used in the init() method to set the LDS lidar callbacks

        /// @brief Callabck routine for the lidar scanPointCallback
        ///        Saves points into #points vector. Skips avery n-th pointed, specified by #every_nth_point
        ///        Assumes that LiDARs inclination is given by #inclination.
        void scanPointCallback(float angle_deg, float distance_mm, float quality, bool scan_completed){
            if(scan_completed)
                scan_counter++;

            if(point_counter % every_nth_point == 0){
                saveScanPoint(distance_mm, angle_deg, inclination);
                point_counter = 0;
            }

            point_counter++;
        }

        /// @brief Static wrapper of @scanPointCallback()
        static void staticScanPointCallback(float angle_deg, float distance_mm, float quality, bool scan_completed){
            LidarController::getInstance().scanPointCallback(angle_deg, distance_mm, quality, scan_completed);
        }

        void packetCallback(uint8_t *packet, uint16_t length, bool scan_completed){
            #ifdef DEBUG_PACKETS
            Serial.println();
            Serial.print("Packet callback, length=");
            Serial.print(length);
            Serial.print(", scan_completed=");
            Serial.println(scan_completed);
            #endif
            
            return;
        }

        static void staticPacketCallback(uint8_t *packet, uint16_t length, bool scan_completed){
            LidarController::getInstance().packetCallback(packet, length, scan_completed);
        }

        size_t serialWriteCallback(const uint8_t * buffer, size_t length) {
            #ifdef DEBUG_SERIAL_OUT
            Serial.println();
            Serial.print('>');
            printBytesAsHex(buffer, length);
            Serial.println();
            #endif
            
            return LidarSerial.write(buffer, length);
        }

        static size_t staticSerialWriteCallback(const uint8_t * buffer, size_t length) {
            return LidarController::getInstance().serialWriteCallback(buffer, length);
        }

        int serialReadCallback() {
            #ifdef DEBUG_SERIAL_IN
            int ch = LidarSerial.read();
            if (ch != -1) {
            if (hex_dump_pos++ % HEX_DUMP_WIDTH == 0)
                Serial.println();
            printByteAsHex(ch);
            }
            return ch;
            #else
            return LidarSerial.read();
            #endif
        }

        static int staticSerialReadCallback(){
            return LidarController::getInstance().serialReadCallback();
        }

        void infoCallback(LDS::info_t code, String info) {
            Serial.print("LiDAR info ");
            Serial.print(lidar->infoCodeToString(code));
            Serial.print(": ");
            Serial.println(info);
        }

        static void staticInfoCallback(LDS::info_t code, String info){
            LidarController::getInstance().infoCallback(code, info);
        }

        void errorCallback(LDS::result_t code, String aux_info) {
            Serial.print("LiDAR error ");
            Serial.print(lidar->resultCodeToString(code));
            Serial.print(": ");
            Serial.println(aux_info);
        }

        static void staticErrorCallback(LDS::result_t code, String aux_info){
            LidarController::getInstance().errorCallback(code, aux_info);
        }

        void motorPinCallback(float value, LDS::lds_pin_t lidar_pin){

            // If the lidar pin is set to motor_enable set 'int pin' as the gpio_pin and otherwise as gpio_pwm pin
            int pin = (lidar_pin == LDS::LDS_MOTOR_EN_PIN) ? this->gpio_en : this->gpio_pwm;

            #ifdef DEBUG_GPIO
            Serial.print("GPIO ");
            Serial.print(pin);
            Serial.print(' ');
            Serial.print(lidar->pinIDToString(lidar_pin));
            Serial.print(" mode set to ");
            Serial.println(lidar->pinStateToString((LDS::lds_pin_state_t) int(value)));
            #endif

            // If value lesser then DIR_INPUT there are 3 options:
            // value == DIR_INPUT, DIR_OUTPUT_CONST, DIR_OUTPUT_PWM
            if(value <= (float)LDS::DIR_INPUT){

            // Configure pin direction
            if(value == (float)LDS::DIR_OUTPUT_PWM){

                // Here would be an if statement, for ESP_IDF_VERSION_MAJOR < 5, we have to resort to ledcSetup
                if(!ledcAttachChannel(pin, this->LIDAR_PWM_FREQ, this->LIDAR_PWM_BITS, this->LIDAR_PWM_CHANNEL)){
                Serial.println("motorPinCallback() ledcAttachChannel() error");
                }
                
            }
            else{
                pinMode(pin, (value == (float)LDS::DIR_INPUT) ? INPUT : OUTPUT);
            }

            return;
            }

            if(value < (float)LDS::VALUE_PWM){
            // Constant output
            // If the value is set to high, write HIGH, if not, write LOW
            digitalWrite(pin, (value == (float)LDS::VALUE_HIGH) ? HIGH : LOW);
            }
            else{
            #ifdef DEBUG_GPIO
                Serial.print("PWM value set to: ");
                Serial.println(value);
            #endif 

            // Invert PWM duty cycle
            #ifdef INVERT_PWM_PIN
                value = 1 - value;
            #endif

            int pwm_value = ((1 << this->LIDAR_PWM_BITS) - 1) * value;

            ledcWriteChannel(this->LIDAR_PWM_CHANNEL, pwm_value);

            #ifdef DEBUG_GPIO
                Serial.print(' ');
                Serial.println(pwm_value);
            #endif
            }
        }

        static void staticMotorPinCallback(float value, LDS::lds_pin_t lidar_pin){
            LidarController::getInstance().motorPinCallback(value, lidar_pin);
        }

        // ----------- Formatting hex in Serial ----------- //

        void printByteAsHex(uint8_t b) {
            if (b < 16)
                Serial.print('0');
            Serial.print(b, HEX);
            Serial.print(' ');
        }

        void printBytesAsHex(const uint8_t* buffer, uint16_t length) {
            if (length == 0)
            return;

            for (uint16_t i = 0; i < length; i++) {
            printByteAsHex(buffer[i]);
            }
        }
    };
}

#endif
