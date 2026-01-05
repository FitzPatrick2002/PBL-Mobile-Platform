#include "Core0Manager.h"

namespace Cores{
   TaskHandle_t task0Handle;        ///< Handle to the http handling which runs on core 0.
   QueueHandle_t manToHttpQ = NULL; ///< Handle to the queue with which Manager can send stuff to the http operator.
}