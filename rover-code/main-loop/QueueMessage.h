/// @file QueueMessage.h
/// @brief Contains the struct which is used to carry data from the core 1 process to core 0 process.

#ifndef QUEUE_MESSAGE_H
#define QUEUE_MESSAGE_H

/// @brief Stores a single element put into the interprocess queue (core1 -> core0).
///        Struct is used to transfer odometry and collision status data.
struct QueueMessage{
  float x, y = 0.0;       ///< Platform position.
  bool collision = false; ///< HCSR data (collision status).
  float angle = 0.0f;     ///< Direction of driving.
};

#endif