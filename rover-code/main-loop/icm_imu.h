/// @file icm_imu.h
/// @brief File contains namespace which contains helper enums and structs such as:
///        Instruments, EulerAngles and Quat, which simplify interaction with an IMU.
///        It also provides the main class: IMU which simplifies interaction with the icm_20948 imu.

#include "ICM_20948.h"

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
        void printAngles(Stream &s){
            s.print("Yaw:");
            s.print(yaw);
            s.print(",");

            s.print("Pitch:");
            s.print(pitch);
            s.print(",");

            s.print("Roll:");
            s.print(roll);
            s.println("");
        }
    };

    /// @brief Represents a rotation quaternion.
    ///        To create a quaternion only q1, q2 and q3 are required. q0 will be calculated from them.
    ///        Values q1, q2 and q3 can be freely modified and its programmers responsibility to make sure 
    ///        they fulfill the rotation quaternion requirements (q0^2 + q1^2 + q2^2 + q3^2 == 1)
    struct Quat {
        float q0, q1, q2, q3 = 0; ///< Quaternion values.

        Quat(){

        }

        Quat(float _q1, float _q2, float _q3){
            q1 = _q1;
            q2 = _q2;
            q3 = _q3;
            q0 = sqrt(1 - (q1*q1) - (q2*q2) - (q3*q3));
        }

        /// @brief Updates the value of q0 based on values of q1, q2 and q3.
        ///        (q0^2 + q1^2 + q2^2 + q3^2 == 1)
        void updateQ0(){
            q0 = sqrt(max(0.0f, 1.0f - (q1*q1) - (q2*q2) - (q3*q3)));
        }
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
        
        ICM_20948_I2C imu; ///< IMU object instance.
        Stream& commStream; ///< Communication object wihch inherits from Stream class. Will be most likely Serial in your case.
        Quat orientationQuat{0, 0, 0}; ///< Last saved orientation quaternion.

    public:

        // ----------------- CONTRUCTOR & DESTRUCTOR ----------------- //

        /// @brief Constructs the IMu object.
        /// @param communicationStream Reference to the communication stream that will be used by the IMU.
        ///                            Must inherit from Stream class and implement its methods.
        IMU(Stream &communicationStream) : commStream(communicationStream) {

        }

        ~IMU() {}

        // ----------------- INITIALIZATION ----------------- //

        /// @brief Initilizes IMU. Communication is performed via I2C.
        ///        If useDMP parameter is false, then DMP processor is
        ///        disabled and only raw AGMT values are available.
        /// @param[in] useDMP If true, DMP processor is used to fileter data.
        ///               Uncomment (#define ICM_20948_USE_DMP) in ICM_20948_C.h to use DMP.
        /// @param[in] ad0_val Value of the 0'th bit in the I2C address of the IMU. 
        ///                    By default it should be 1. 
        /// @param[in] showDebug Specifies if debug info should be showed or not. 
        void init(bool useDMP = false, int ad0_val = 1, bool showDebug = false){

            if(showDebug)
            imu.enableDebugging(Serial);

            bool imuInitialized = false;

            while(!imuInitialized){
            imu.begin(Wire, ad0_val);

            commStream.println("Initilization of IMU returned: ");
            commStream.println(imu.statusString());
            if(imu.status != ICM_20948_Stat_Ok){
                commStream.println("IMU initialization failed. Trying again in 0.5s ...");
                delay(500);
            }
            else{
                imuInitialized = true;
            }
            }

            // Initially assume that initialization of DMP was successfull. 
            bool DMPsuccess = true; 

            // Enable the DMP. Can be overwritten.
            DMPsuccess &= (imu.initializeDMP() == ICM_20948_Stat_Ok);
            // INV_ICM20948_SENSOR_ROTATION_VECTOR (32-bit 9-axis quaternion + heading accuracy)

            // Enable the orientation sensor
            DMPsuccess &= (imu.enableDMPSensor(INV_ICM20948_SENSOR_ORIENTATION) == ICM_20948_Stat_Ok);

            // Enable the IMU to output different sensors data into FIFO at different rates.
            DMPsuccess &= (imu.setDMPODRrate(DMP_ODR_Reg_Quat9, 0) == ICM_20948_Stat_Ok);

            // Enable FIFO
            DMPsuccess &= (imu.enableFIFO() == ICM_20948_Stat_Ok);

            // Enable the DMP
            DMPsuccess &= (imu.enableDMP() == ICM_20948_Stat_Ok);

            // Reset DMP
            DMPsuccess &= (imu.resetDMP() == ICM_20948_Stat_Ok);

            // Reset FIFO
            DMPsuccess &= (imu.resetFIFO() == ICM_20948_Stat_Ok);

            // Check the success
            // If operation wasn't succesfull, enter forever loop as the imu has not initialized properly
            if(DMPsuccess){
            commStream.println("DMP - OK");
            }
            else{
            while (true){
                commStream.println("Setup of DMP failed, doing nothing forever ...");
                delay(500);
            }
            }
        }

        // ----------------- MEASUREMENTS ----------------- //

        /// @brief Refreshes the value stored in #orientationQuat with the newest data.
        ///        IMUs FIFO queue is emptied and the last (newest element) is saved to #orientationQuat.
        void refresh(){
            // Data from the queue will be stored in here
            icm_20948_DMP_data_t data;

            // Read all avaialable data from the queue iteratively and leave out only the most recent (last element)
            bool updated = false;
            while(imu.readDMPdataFromFIFO(&data) != ICM_20948_Stat_FIFONoDataAvail){
            if((imu.status == ICM_20948_Stat_Ok) || (imu.status == ICM_20948_Stat_FIFOMoreDataAvail)){
                // If the data is a quaternion, read it
                if((data.header & DMP_header_bitmap_Quat9) > 0){
                orientationQuat.q1 = ((float)data.Quat9.Data.Q1) / 1073741824.0;
                orientationQuat.q2 = ((float)data.Quat9.Data.Q2) / 1073741824.0;
                orientationQuat.q3 = ((float)data.Quat9.Data.Q3) / 1073741824.0;
                updated = true;
                }
            }
            }

            // If data has been updated, calculate the q0 from q1, q2 and q3
            if(updated)
            orientationQuat.updateQ0();
        }

        /// @brief Calculates the euler angles in 
        void getEulerAngles(EulerAngles& dest, bool refresh = true){
            // Refresh the data
            if (refresh){
                this->refresh();
            }

            // Convert the quaternion to Euler angle
            // and print the outcome to Serial out
            this->quat2Euler(dest, this->orientationQuat);
        }

        // ----------------- PRINTING ----------------- //

        /// @brief Prints the current raw readings from a specified instrument / group of instruments. 
        /// @param[in] type Type of the intrument or group of intruments. 
        void printAGMT(Instrument type){
            switch (type){
            case Instrument::ACCELEROMETER:
                commStream.print("Ax:");
                commStream.print(imu.accX());
                commStream.print(",");

                commStream.print("Ay:");
                commStream.print(imu.accY());
                commStream.print(",");

                commStream.print("Az:");
                commStream.print(imu.accZ());
                commStream.println("");
            break;
            case Instrument::GYROSCOPE:
                commStream.print("Gx:");
                commStream.print(imu.gyrX());
                commStream.print(",");

                commStream.print("Gy:");
                commStream.print(imu.gyrY());
                commStream.print(",");

                commStream.print("Gz:");
                commStream.print(imu.gyrZ());
                commStream.println("");
            break;
            case Instrument::MAGNETOMETER:
                commStream.print("Mx:");
                commStream.print(imu.magX());
                commStream.print(",");

                commStream.print("My:");
                commStream.print(imu.magY());
                commStream.print(",");

                commStream.print("Mz:");
                commStream.print(imu.magZ());
                commStream.println("");
            break;
            case Instrument::AGM:
                this->printAGMT(Instrument::ACCELEROMETER);
                this->printAGMT(Instrument::GYROSCOPE);
                this->printAGMT(Instrument::MAGNETOMETER);

            break;
            default:
                commStream.println("Unknown instrument");
            break;
            }
        }

        /// @brief Prints the Euler angles based on the gathered data.
        ///        If refresh == true, the #refresh is called, otherwise the current value of #orientationQuat is used.
        /// @param refresh Soecfies if data should be refreshed before printing. 
        void printEulerOrientation(bool refresh = true){
            EulerAngles outcome;
            this->getEulerAngles(outcome, refresh);

            outcome.printAngles(commStream);
        }

        // ----------------- UTILITY ----------------- //

        /// @brief Converts rotation quaternion data into EulerAngles structure.
        /// @param[out] dest Destination where Euler Angles are to be stored as Yaw, Pitch, Roll.
        /// @param[in] quaternion Quaternion based on wich the Euelr angles will be calculated. 
        void quat2Euler(EulerAngles &dest, Quat quaternion){

            // Define q0, .., q3 based on quaternion data so that its more readible
            float q0 = quaternion.q0;
            float q1 = quaternion.q1;
            float q2 = quaternion.q2;
            float q3 = quaternion.q3;

            // Compute the yaw
            dest.yaw = atan2(2.0*(q0*q3 + q1*q2), 1.0f - 2.0f * (q2*q2 + q3*q3));

            // Compute the pitch, avoid gimbal lock
            float sinp = 2.0*(q0*q2 - q1*q3);
            if (sinp >= 1.0)
            dest.pitch = 3.14 / 2.0;
            else
            dest.pitch = asin(sinp);

            // Compute roll
            dest.roll = atan2(2.0*(q0*q1 + q2*q3), 1.0f - q1*q1 + q2*q2);
        }

    };

};


