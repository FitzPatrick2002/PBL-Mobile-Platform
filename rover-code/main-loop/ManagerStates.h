#ifndef MANAGER_STATES_H
#define MANAGER_STATES_H

/// @brief Defines possible states of operation for the rover
enum ManagerState{
  STANDBY = 0,         ///< Rover is not performing any actions that could break communication.
  MOVING = 1,          ///< Rover is changing position using engines.
  SCANNING = 2,        ///< Rover is gathering environmental data, can't move now.
  UPLOADING = 3,       ///< Rover is uploading data to PC.
  STATUS_UPDATE = 4,   ///< Another state for uploading to controller.
  IMU_CALIBRATION = 5, ///< Rover is basically in idle state but imu is calibration mode.
  PLATFORM_SETUP = 6      ///< PLatform setup mode. User is able to setup some global proerties via UART commands.
  //UPLOADING_TO_PC = 5 ///< Platform is sending data to PC.
};

#endif