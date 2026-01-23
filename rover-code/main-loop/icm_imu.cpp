/// @file icm_imu.cpp
/// @brief Provides definitions to elements from icm_imu.h file.

#include "icm_imu.h"

namespace ICM_IMU{

    // ----------------- STRUCT EulerAngles ----------------- //

    void EulerAngles::printAngles(Stream &s){
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

    // ----------------- STRUCT Quat ----------------- //

    Quat::Quat(){

    }

    Quat::Quat(float _q1, float _q2, float _q3){
        q1 = _q1;
        q2 = _q2;
        q3 = _q3;
        q0 = sqrt(1 - (q1*q1) - (q2*q2) - (q3*q3));
    }

    void Quat::updateQ0(){
        q0 = sqrt(max(0.0f, 1.0f - (q1*q1) - (q2*q2) - (q3*q3)));
    }

    // ----------------- CLASS IMU ----------------- //

    void BiasStore::update(){
        int32_t localSum = header;

        localSum += biasGyroX;
        localSum += biasGyroY;
        localSum += biasGyroZ;
        localSum += biasAccelX;
        localSum += biasAccelY;
        localSum += biasAccelZ;
        localSum += biasCPassX;
        localSum += biasCPassY;
        localSum += biasCPassZ;

        sum = localSum;
    }

    bool BiasStore::isValid(){
        int32_t newSum = header;

        if(newSum != 0x42)
            return false;

        newSum += biasGyroX;
        newSum += biasGyroY;
        newSum += biasGyroZ;
        newSum += biasAccelX;
        newSum += biasAccelY;
        newSum += biasAccelZ;
        newSum += biasCPassX;
        newSum += biasCPassY;
        newSum += biasCPassZ;

        return (newSum == sum);
    }

    void BiasStore::printBiases(Stream &s){
        s.print(F("Gyro X: "));
        s.print(biasGyroX);
        s.print(F(" Gyro Y: "));
        s.print(biasGyroY);
        s.print(F(" Gyro Z: "));
        s.println(biasGyroZ);
        s.print(F("Accel X: "));
        s.print(biasAccelX);
        s.print(F(" Accel Y: "));
        s.print(biasAccelY);
        s.print(F(" Accel Z: "));
        s.println(biasAccelZ);
        s.print(F("CPass X: "));
        s.print(biasCPassX);
        s.print(F(" CPass Y: "));
        s.print(biasCPassY);
        s.print(F(" CPass Z: "));
        s.println(biasCPassZ);
    }

    // ----------------- CONTRUCTOR & DESTRUCTOR ----------------- //

    IMU::IMU(Stream &communicationStream) : commStream(communicationStream) {

    }

    IMU::~IMU() {}

    // ----------------- INITIALIZATION ----------------- //

    void IMU::init(bool useDMP, int ad0_val, bool showDebug){

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

        delay(1000);

        // Read biases from the EEPROM
        this->readBiases();
    }

    // ----------------- CALIBRATION ----------------- //

    bool IMU::readBiases(){
        // Allocate 128 bytes for EEPROM storage
        if (!EEPROM.begin(128)){
            commStream.println("EEPROM failed, biases for IMU cannot be saved");
            return false;
        }

        BiasStore store;

        // Read the biases 
        EEPROM.get(0, store);

        // If biases are okay, store them on IMU
        if(store.isValid()){
            commStream.println("Bias data in EEPROM is valid. Restoring it...");
            bool success = true;

            success &= (imu.setBiasGyroX(store.biasGyroX) == ICM_20948_Stat_Ok);
            success &= (imu.setBiasGyroY(store.biasGyroY) == ICM_20948_Stat_Ok);
            success &= (imu.setBiasGyroZ(store.biasGyroZ) == ICM_20948_Stat_Ok);
            success &= (imu.setBiasAccelX(store.biasAccelX) == ICM_20948_Stat_Ok);
            success &= (imu.setBiasAccelY(store.biasAccelY) == ICM_20948_Stat_Ok);
            success &= (imu.setBiasAccelZ(store.biasAccelZ) == ICM_20948_Stat_Ok);
            success &= (imu.setBiasCPassX(store.biasCPassX) == ICM_20948_Stat_Ok);
            success &= (imu.setBiasCPassY(store.biasCPassY) == ICM_20948_Stat_Ok);
            success &= (imu.setBiasCPassZ(store.biasCPassZ) == ICM_20948_Stat_Ok);

            // Update of biases on imu succeeded, print them
            if(success){
                commStream.println("Biases restored");
                store.printBiases(commStream);
                return true;
            }
            else{
                commStream.println("Restoration of imu biases failed!");
            }
        }

        return false;
    }

    bool IMU::storeBiases(){
        commStream.println("Saving bias data");

        BiasStore store;

        // Read the biases from IMU
        bool success = (imu.getBiasGyroX(&store.biasGyroX) == ICM_20948_Stat_Ok);
        success &= (imu.getBiasGyroY(&store.biasGyroY) == ICM_20948_Stat_Ok);
        success &= (imu.getBiasGyroZ(&store.biasGyroZ) == ICM_20948_Stat_Ok);
        success &= (imu.getBiasAccelX(&store.biasAccelX) == ICM_20948_Stat_Ok);
        success &= (imu.getBiasAccelY(&store.biasAccelY) == ICM_20948_Stat_Ok);
        success &= (imu.getBiasAccelZ(&store.biasAccelZ) == ICM_20948_Stat_Ok);
        success &= (imu.getBiasCPassX(&store.biasCPassX) == ICM_20948_Stat_Ok);
        success &= (imu.getBiasCPassY(&store.biasCPassY) == ICM_20948_Stat_Ok);
        success &= (imu.getBiasCPassZ(&store.biasCPassZ) == ICM_20948_Stat_Ok);

        store.update();
        
        // If reading from IMU was a success, then put them in EEPROM
        if (success){
            EEPROM.put(0, store);
            EEPROM.commit();

            // Test if the saved biases are okay, if so print a message
            EEPROM.get(0, store);
            if(store.isValid()){
                commStream.println("Biases stored succesfully");
                store.printBiases(commStream);

                return true;
            }
            else{
                commStream.println("Bias store failed!");
            }
        }
        else{
            commStream.println("Bias read failed!");
        }

        return false;
    }

    void IMU::resetIMU(){
        commStream.println("Resetting the IMU biases. Learning from zero");
        
        // Reset the DMP and the FIFO
        imu.resetDMP();
        imu.resetFIFO();

        // Reset the bias values
        imu.setBiasGyroX(0);
        imu.setBiasGyroY(0);
        imu.setBiasGyroZ(0);

        imu.setBiasAccelX(0);
        imu.setBiasAccelY(0);
        imu.setBiasAccelZ(0);

        imu.setBiasCPassX(0);
        imu.setBiasCPassY(0);
        imu.setBiasCPassZ(0);
    }

    // ----------------- MEASUREMENTS ----------------- //

    void IMU::refresh(){
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

    void IMU::getEulerAngles(EulerAngles& dest, bool refresh){
        // Refresh the data
        if (refresh){
        this->refresh();
        }

        // Convert the quaternion to Euler angle
        // and print the outcome to Serial out
        this->quat2Euler(dest, this->orientationQuat);
    }

    // ----------------- PRINTING ----------------- //

    void IMU::printAGMT(Instrument type){
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

    void IMU::printEulerOrientation(bool refresh){
        EulerAngles outcome;
        this->getEulerAngles(outcome, refresh);

        outcome.printAngles(commStream);
    }

    // ----------------- UTILITY ----------------- //

    void IMU::quat2Euler(EulerAngles &dest, Quat quaternion){

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
        dest.roll = atan2(2.0*(q0*q1 + q2*q3), 1.0f - (q1*q1 + q2*q2));
    }
};


