#ifndef ICM_IMU_FILE
#define ICM_IMU_FILE

/// @file icm_imu.h
/// @brief File contains namespace which contains helper enums and structs such as:
///        Instruments, EulerAngles and Quat, which simplify interaction with an IMU.
///        It also provides the main class: IMU which simplifies interaction with the icm_20948 imu.

#include "ICM_20948.h"
#include <EEPROM.h>

/// @brief Contains the necessary classes and datastructures to operate on icm_20948 imu with a bit of ease.
/**
 * Exemplary use of IMU:
#define I2C_SDA 11 ///< Pin on which SDA is defined.
#define I2C_SCL 12 ///< Pin on which SCL is defined.
#define AD0_VAL 1  ///< The last value of the I2C address.
ICM_IMU::IMU imu(Serial);
uint32_t timer = 0;
void setup() {
  // Initilize the Serial communication with pc
  Serial.begin(115200);
  while(!Serial)
    delay(10);
  Serial.println("SERIAL - OK");
  // Init I2C communication with SCL clock frequency = 400 kHz
  Wire.begin(I2C_SDA, I2C_SCL);
  Wire.setClock(400000);
  Serial.println("WIRE - OK");
  // Initilize the IMU
  imu.init(true, 1, true);
  Serial.println("IMU - OK");
}
void loop() {
  if(millis() - timer > 1000){
    imu.printEulerOrientation(true);
    timer = millis();
  }
}
 */
namespace ICM_IMU{
    
    /// @brief Enumerates isntruments and their combiinations available on the icm imu. 
    enum Instrument{
        ACCELEROMETER = 0,
        GYROSCOPE,
        MAGNETOMETER,
        TERMOMETER,
        AGM,
        AGMT
    };

    /// @brief Stores the info about Euler angles.
    struct EulerAngles{
        float yaw, pitch, roll = 0.0; ///< Euler angles. 


        /// @brief Prints the eueler angles to specified stream.
        /// @param s Instance of class which implements Stream interface.
        void printAngles(Stream &s);
    };

    /// @brief Represents a rotation quaternion.
    ///        To create a quaternion only q1, q2 and q3 are required. q0 will be calculated from them.
    ///        Values q1, q2 and q3 can be freely modified and its programmers responsibility to make sure 
    ///        they fulfill the rotation quaternion requirements (q0^2 + q1^2 + q2^2 + q3^2 == 1)
    struct Quat {
        float q0, q1, q2, q3 = 0; ///< Quaternion values.

        Quat();

        Quat(float _q1, float _q2, float _q3);

        /// @brief Updates the value of q0 based on values of q1, q2 and q3.
        ///        (q0^2 + q1^2 + q2^2 + q3^2 == 1)
        void updateQ0();
    };

    /// @brief Stores biases of gyro, acc and mag.
    struct BiasStore{
        int32_t header = 0x42;

        int32_t biasGyroX = 0;
        int32_t biasGyroY = 0;
        int32_t biasGyroZ = 0;

        int32_t biasAccelX = 0;
        int32_t biasAccelY = 0;
        int32_t biasAccelZ = 0;

        int32_t biasCPassX = 0;
        int32_t biasCPassY = 0;
        int32_t biasCPassZ = 0;

        int32_t sum = 0;

        /// @brief Updates the checksum of the bias store.
        void update();

        /// @brief Checks if the bias values are not stale.
        /// @returns true if the checksum is okay and false otherwise.
        bool isValid();

        /// @brief Prints values of biases to designated stream.
        void printBiases(Stream &s);
    };

    /// @brief Wrapper class which enables easier control over the ICM_20948_I2C object.
    ///        Allows to display raw AGMT readings and calculation of Euler angles.
    /// @note  Wire.h is needed and needs to be initilized on the I2C pins of the controller
    ///        before IMU is initialized.  
    /// @note InvenSense coordinate frame is used.
    ///       X-axis: Forward. 
    ///       Y-axis: Left.
    ///       Z-axis: Up.     
    /// @note Eueler angles are calculated assuming:
    ///       Yaw - Angle between the X-axis and magnetic north.
    ///       Pitch - Determines how much the sensor is "levelled". (If it is held parallel to the ground / perpendicular to the gravity vector).
    ///       Roll - Determines rotation about the X-axis. (Hopefully it stays 0 always for objects which drive on a ground with wheels).
    class IMU {
    private:
        
        ICM_20948_I2C imu;             ///< IMU object instance.
        Stream& commStream;            ///< Communication object wihch inherits from Stream class. Will be most likely Serial in your case.
        Quat orientationQuat{0, 0, 0}; ///< Last saved orientation quaternion.

    public:

        // ----------------- CONTRUCTOR & DESTRUCTOR ----------------- //

        /// @brief Constructs the IMu object.
        /// @param communicationStream Reference to the communication stream that will be used by the IMU.
        ///                            Must inherit from Stream class and implement its methods.
        IMU(Stream &communicationStream);

        ~IMU();

        // ----------------- INITIALIZATION ----------------- //

        /// @brief Initilizes IMU. Communication is performed via I2C.
        ///        If useDMP parameter is false, then DMP processor is
        ///        disabled and only raw AGMT values are available.
        /// @param[in] useDMP If true, DMP processor is used to fileter data.
        ///               Uncomment (#define ICM_20948_USE_DMP) in ICM_20948_C.h to use DMP.
        /// @param[in] ad0_val Value of the 0'th bit in the I2C address of the IMU. 
        ///                    By default it should be 1. 
        /// @param[in] showDebug Specifies if debug info should be showed or not. 
        void init(bool useDMP = false, int ad0_val = 1, bool showDebug = false);

        // ----------------- CALIBRATION ----------------- //

        /// @brief Reads biases from EEPROM memory and loads it into IMU.
        ///        If biases are valid then they are saved on IMU.
        /// @returns True if biases were read and saved succesfully and false otherwise.
        bool readBiases();

        /// @brief Reads biases from IMU and saves them in EEPROM under address == 0.
        ///        Use this to update the biases stored in EEPROM.
        /// @returns True uf biases were read from EEPROM and loaded into IMU succesfully, false otherwise.
        bool storeBiases();

        /// @brief Rests DMP and cleares FIFO, zeroes all biases.
        ///        Use it to re-enable fast learning mode of the IMU.
        void resetIMU();

        // ----------------- MEASUREMENTS ----------------- //

        /// @brief Refreshes the value stored in #orientationQuat with the newest data.
        ///        IMUs FIFO queue is emptied and the last (newest element) is saved to #orientationQuat.
        void refresh();
        /// @brief Calculates the euler angles in 
        void getEulerAngles(EulerAngles& dest, bool refresh = true);

        // ----------------- PRINTING ----------------- //

        /// @brief Prints the current raw readings from a specified instrument / group of instruments. 
        /// @param[in] type Type of the intrument or group of intruments. 
        void printAGMT(Instrument type);

        /// @brief Prints the Euler angles based on the gathered data.
        ///        If refresh == true, the #refresh is called, otherwise the current value of #orientationQuat is used.
        /// @param refresh Soecfies if data should be refreshed before printing. 
        void printEulerOrientation(bool refresh = true);

        // ----------------- UTILITY ----------------- //

        /// @brief Converts rotation quaternion data into EulerAngles structure.
        /// @param[out] dest Destination where Euler Angles are to be stored as Yaw, Pitch, Roll.
        /// @param[in] quaternion Quaternion based on wich the Euelr angles will be calculated. 
        void quat2Euler(EulerAngles &dest, Quat quaternion);

    };
};

#endif


