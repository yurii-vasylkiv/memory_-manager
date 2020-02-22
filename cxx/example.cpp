#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <stddef.h>
#include <zconf.h>

#include "memory_manager.h"

void *imu_data_generator(void *arg);
void *pressure_data_generator(void *arg);
void *continuity_data_generator(void *arg);
void *event_detector(void *arg);

pthread_t imu_background_task;
pthread_t press_background_task;
pthread_t cont_background_task;
pthread_t event_detector_background_task;


int main()
{
    Data data_ = {};

    if(pthread_create(&imu_background_task, NULL, imu_data_generator, &data_)) {

        fprintf(stderr, "Error creating thread\n");
        return 1;
    }

    if(pthread_create(&press_background_task, NULL, pressure_data_generator, &data_)) {

        fprintf(stderr, "Error creating thread\n");
        return 1;
    }

    if(pthread_create(&cont_background_task, NULL, continuity_data_generator, &data_)) {

        fprintf(stderr, "Error creating thread\n");
        return 1;
    }

    if(pthread_create(&event_detector_background_task, NULL, event_detector, &data_)) {

        fprintf(stderr, "Error creating thread\n");
        return 1;
    }

    if(memory_manager_init() != 0)
    {
        fprintf(stderr, "Error initializing memory_manager\n");
        return 1;
    }

    if(memory_manager_configure() != 0)
    {
        fprintf(stderr, "Error configuring memory_manager\n");
        return 1;
    }

    if(memory_manager_start() != 0)
    {
        fprintf(stderr, "Error starting memory_manager\n");
        return 1;
    }


    while(1)
    {
        if(memory_manager_update(&data_) != 0)
        {
            fprintf(stderr, "Could not update the memory_manager. Abort...\n");
            return 1;
        }
    }


    memory_manager_stop();


    if(pthread_join(imu_background_task, NULL)) {
        fprintf(stderr, "Error joining thread\n");
        return 2;
    }

    if(pthread_join(press_background_task, NULL)) {
        fprintf(stderr, "Error joining thread\n");
        return 2;
    }

    if(pthread_join(cont_background_task, NULL)) {
        fprintf(stderr, "Error joining thread\n");
        return 2;
    }

    if(pthread_join(event_detector_background_task, NULL)) {
        fprintf(stderr, "Error joining thread\n");
        return 2;
    }



    return 0;
}

void *imu_data_generator(void *arg)
{
    Data * temp = (Data*)arg;
    IMUData imu = {};
    imu.data.gyroscope[0] = 345;
    imu.data.gyroscope[1] = 346;
    imu.data.gyroscope[2] = 347;
    imu.data.accelerometer[0] = 355;
    imu.data.accelerometer[1] = 356;
    imu.data.accelerometer[2] = 357;
    imu.data.timestamp = time(NULL);

    temp->inertial = imu;
    usleep(500000);
    while(1)
    {
        temp->inertial.updated = 1;
        usleep(5); // 200Hz
    }
}
void *pressure_data_generator(void *arg)
{
    Data * temp = (Data*)arg;
    PressureData pressureData = {};
    pressureData.data.timestamp = time(NULL);
    pressureData.data.pressure = 762.8f;
    pressureData.data.temperature = -36.6f;
    temp->pressure = pressureData;
    while(1)
    {
        temp->pressure.updated = 1;
        usleep(5); // 1 Hz
    }
}
void *continuity_data_generator(void *arg)
{
    Data * temp = (Data*)arg;
    Continuity continuity = {};
    continuity.data.timestamp = time(NULL);
    continuity.data.status = Short;
    temp->continuity = continuity;

    while(1)
    {
        temp->continuity.updated = 1;
        usleep(5); // 0.1 Hz
    }
}
void *event_detector(void *arg)
{
    Data * temp = (Data*)arg;
    FlightEvent flightEvent = {};
    flightEvent.data.status = Apogee;
    temp->event = flightEvent;

    while(1)
    {
        temp->event.updated = 1;
        usleep(5); // 0.1 Hz
    }
}


/*


MemoryManager
{
pages_queue_CONFIG;
pages_queue_IMU;
pages_queue_PRESS;
pages_queue_CONT;
pages_queue_EVENTS;


autoSaveConfigEvery;
configurationSector;
dataSectors;
background_task_monitor;
isRunning;

timer;


initialize()
{
    initConfigurationSector();
    initializeDataSectors();
    background_task_monitor(startQueueMonitor);
}

configure(userConfiguration)
        {
                if(!writeDataToSeparateBlocks)
                {
                    dataSectors = is size 1
                    pages_queues = is size 1
                }
                else
                {
                    dataSectors; = is size 5
                    pages_queues = is size 5
                }

                if(useTheFastestSamplingRate)
                {
                    keep adding duplicates to the slower sectors using the sampling rate of the fastest sensors
                }

                this.autoSaveConfigEvery = autoSaveConfigEvery

        }

initConfigurationSector()
{
    for(i to pages N) // operation_duration * N
    {
        page = flashReadPage() // blocking
        configurationSector.add(page)
    }
}

initializeDataSectors()
{
    dataSectors[IMU] = configurationSector.dataSectorsInfo[IMU]
    dataSectors[PRESS] = configurationSector.dataSectorsInfo[PRESS]
    ....
    dataSectors[EVENTS] = configurationSector.dataSectorsInfo[EVENTS]

    // all the pointers for data sectors (stars, ends, curr) collected
}


add_data(data)
        {

                if(data[IMU].updated)
                {
                    dataSectors[IMU].pages[currentPageIndex][dataSectors[IMU].pages[currentIndex].bytes_written] = data[IMU];
                    if(dataSectors[i].pages[IMU].written_bytes == 255)
                    {
                        dataSectors[i].pages[IMU].isFull = true;
                    }
                }


                if(data[PRESS].updated)
                {
                    dataSectors[PRESS].pages[currentPageIndex][dataSectors[PRESS].pages[currentIndex].bytes_written] = data[PRESS];
                    if(dataSectors[i].pages[PRESS].written_bytes == 255)
                    {
                        dataSectors[i].pages[PRESS].isFull = true;
                    }
                }


                ..........



        }


isAnythingFull()
{
    timer++;

    if(timer >= autoSaveConfigsEvery)
    {
        dataSectors[i].pages[CONFIG].isFull = true;
    }

    for(i = 0 to dataSectors)
    {
        for (j = 0 to dataSectors[i].pages_buffer_to_alternate_in_between_list)
        {
            if(dataSectors[i].pages[j].isFull())
                return true;
        }
    }

    return false;
}


write_async()
{
    for(i = 0 to dataSectors[IMU])
    {
        if(dataSectors[IMU].pages[i].isFull())
        {
            pages_queueIMU.add(dataSectors[IMU].pages[i]);
        }

        ......

    }
}


stopQueueMonitor()
{
    isRunning = false;
}

startQueueMonitor()
{
    while(isRunning)
    {
        while(!pages_queue_IMU.poll(page))
        {
            flash_write(IMU, page);
        }

        while(!pages_queue_PRESS.poll(page))
        {
            flash_write(PRESS, page);
        }
        .....


        while(!pages_queue_CONFIG.poll(page))
        {
            flash_write(CONFIG, page);
        }
    }
}


flash_write(SECTOR_NAME, page)
{
dataSectors[SECTOR_NAME].
spi.send(configurationSector.dataSectorsInfo[SECTOR_NAME].bytes_written, page, 255);
}


getConfigurations()
{
    return configurationSector_as_nice_struct;
}

}














StateMachineController
{
functions = {Events.PRELAUNCH, onPrelauch
        .................
}

initialize()
{
    functions

}
tick(event)
        {
                functions[event]();
        }


onPrelanuch(){}
onLaunch(){}
onPreApogee(){}
onApogee(){}
}





IMUsensor
{
queue;
mainTask;
int init()
{
    STM32Configurations..... GPIO and so on
    mainTask = start;
    return 0;
}

start()
{
    while(isRunning)
    {
        while(____imu_retrieve_if_available(imu_data))
        {
            queue.add(imu_data);
        }
    }


}

delay(){}


bool getData(container*)
{
    container = queue.poll();
    return true;
}

}



...... all the other sensors


data
{
tuple IMU = {updated, bytes{timestamp, int[3]}};
tuple PRESS = {updated, bytes{timestamp, int[2]}};
....
}


STATIC data;

retrieveDataFromSensors()
{
    if(IMU_get(data.IMU.bytes))
    {
        data.IMU.updated = true;
    }

    .......

    return data;
}




main loop()
{
    IMU_init();
    PRESS_init();
    ......
    MemoryManager.init();
    MemoryManager.configure(UserConfigs);

    StateMachineController.init();

    // do all the checks before we start to makes sure everythings returns 0;
    // if > 0 or < 0 then match with the error message from a specific module

    while(isRunning)
    {
        data = retrieveDataFromSensors();

        if(MemoryManager.isAnythingFull())
        {
            MemoryManager.write_async();
        }

        MemoryManager.add_data(data);

        event = EventDetector.feed(data);

        StateMachineController.call(event);
    }


}

 */






