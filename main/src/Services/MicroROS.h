#ifndef PERSE_ROVER_MICROROS_H
#define PERSE_ROVER_MICROROS_H

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

class MicroROS {
public:
    MicroROS();
    ~MicroROS();

    void begin();
    void stop();

private:
    TaskHandle_t taskHandle = nullptr;
    static void microRosTask(void* arg);
};

#endif //PERSE_ROVER_MICROROS_H
