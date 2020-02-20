#include <stdio.h>
#include <stdint-gcc.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#include <stdint.h>
#include <stddef.h>

#define DESERIALIZE(type, size)                             \
    static inline type to_##type(uint8_t* src){             \
        union {                                             \
            type value;                                     \
            char bytes[size];                               \
        } u;                                                \
        memcpy(u.bytes, src, size);                         \
        return u.value;                                     \
    }                                                       \

#define SERIALIZE(type, size)                                   \
    static inline void from_##type(type value, uint8_t* buf){   \
        union {                                                 \
            type value;                                         \
            char bytes[size];                                   \
        } u;                                                    \
        u.value = value;                                        \
        memcpy(buf, u.bytes, size);                             \
    }                                                           \


SERIALIZE(double, 4)
SERIALIZE(float, 4)
SERIALIZE(uint32_t, 4)
SERIALIZE(int32_t, 4)
SERIALIZE(int8_t, 1)


DESERIALIZE(double, 4)
DESERIALIZE(float,4)
DESERIALIZE(uint32_t,4)
DESERIALIZE(int32_t,4)
DESERIALIZE(uint8_t,1)

typedef union
{
    uint8_t updated;
    struct{
        uint32_t timestamp;
        int32_t accelerometer[3];
        int32_t gyroscope[3];
    };
    uint8_t bytes[sizeof(uint32_t) + 3 * sizeof(int32_t) + 3 * sizeof(int32_t)];
} IMUData;

typedef union
{
    uint8_t updated;
    struct {
        uint32_t timestamp;
        float pressure;
        float temperature;
    };
    uint8_t bytes[sizeof(uint32_t) + sizeof(float) + sizeof(float)];
} PressureData;

typedef enum
{
    Open = 0 , Short = 1
} ContinuityStatus;

typedef enum
{
    Start = 0, Launch = 1, Apogee = 2, PostApogee = 3, Landed = 4
}FlightEventStatus;


typedef union
{
    uint8_t updated;
    struct {
        uint32_t timestamp;
        ContinuityStatus status;
    };
    uint8_t bytes[sizeof(uint32_t) + sizeof(uint8_t)];
} Continuity;

typedef union
{
    uint8_t updated;
    struct {
        uint32_t timestamp;
        FlightEventStatus status;
    };
    uint8_t bytes[sizeof(uint32_t) + sizeof(uint8_t)];
}FlightEvent;

typedef struct
{
    uint32_t timestamp;
    IMUData inertial;
    PressureData pressure;
    Continuity continuity;
    FlightEvent event;
}Data;

typedef struct
{
    uint16_t startAddress;
    uint16_t endAddress;
    uint16_t bytesWritten;
} BufferInfo;

typedef struct
{
    uint16_t size;
    BufferInfo info;
} MemorySectorInfo;


typedef struct
{
    BufferInfo info;
    uint8_t data[256];
} MemoryBuffer;

#define DATA_SECTOR_PAGES 5
typedef struct
{
    const MemorySectorInfo info;
    MemoryBuffer buffers[DATA_SECTOR_PAGES];
    int currPage;
} MemoryDataSector;

#define CONFIGURATION_SECTOR_PAGES 2
typedef struct
{
    MemorySectorInfo info;
    MemoryBuffer buffers[CONFIGURATION_SECTOR_PAGES];
} MemoryConfigurationSector;


typedef struct
{
    MemoryConfigurationSector configuration_sector;
    MemoryDataSector dataSectors[4];
} MemoryLayoutData;

typedef enum { IMU, Pressure, Cont, Event} Sector;


typedef struct Queue{

    MemoryBuffer data;
    Sector sector;
    struct Queue* previous;

}*node_ptr;


node_ptr front = NULL;
node_ptr rear = NULL;

bool is_empty(){

    return (front == NULL);
}

void display_front(){

    if(is_empty()){

        printf("\nThe queue is empty!\n");
        return;
    }

    printf("\n[%d]\n", front->data);
}

void display_rear(){

    if(is_empty()){

        printf("\nThe queue is empty!\n");
        return;
    }

    printf("\n[%d]\n", rear->data);
}

bool enqueue(MemoryBuffer value){

    node_ptr item = (node_ptr) malloc(sizeof(struct Queue));

    if (item == NULL)
        return false;

    item->data = value;
    item->previous = NULL;

    if(rear == NULL)
        front = rear = item;
    else{
        rear->previous = item;
        rear = item;
    }

    return true;
}

bool dequeue(){

    if(is_empty()){

        printf("\nThe queue is empty!\n");
        return false;
    }

    node_ptr temp = front;
    front = front->previous;
    free(temp);

    return true;
}

void display(){


    if(is_empty()){

        printf("\nThe queue is empty!\n");
        return;
    }

    node_ptr temp = front;

    printf("\n[front -> ");

    while(temp != NULL){
        printf("[%d]", temp->data);
        temp = temp->previous;
    }

    printf(" <- rear]\n");

}

bool clear(){

    if(is_empty()){

        printf("\nThe queue is empty!\n");
        return false;
    }

    node_ptr current = front;
    node_ptr previous = NULL;

    while(current != NULL){

        previous = current->previous;
        free(current);
        current = previous;
    }

    front = NULL;

    return is_empty();
}






void add_data(Data * data);

MemoryLayoutData sectors = {};
struct Queue queues[4] = {};

int main() {
    printf("Hello, World!\n");

    Data data = {};
    IMUData imu = {};
    imu.gyroscope[0] = 345;
    imu.gyroscope[1] = 346;
    imu.gyroscope[2] = 347;
    imu.accelerometer[0] = 355;
    imu.accelerometer[1] = 356;
    imu.accelerometer[2] = 357;
    imu.timestamp = time(NULL);
    imu.updated = true;

    PressureData pressureData = {};
    pressureData.timestamp = time(NULL);
    pressureData.pressure = 762.8f;
    pressureData.temperature = -36.6f;
    pressureData.updated = true;

    Continuity continuity = {};
    continuity.timestamp = time(NULL);
    continuity.status = Short;
    continuity.updated = true;

    FlightEvent flightEvent = {};

    data.inertial = imu;
    data.pressure = pressureData;
    data.continuity = continuity;
    data.event = flightEvent;

    add_data(&data);
    return 0;
}



void add_data(Data * data) {
    if (data->inertial.updated) {
        int currentPageIndex = sectors.dataSectors[IMU].currPage;
        if (sectors.dataSectors[IMU].buffers[currentPageIndex].info.bytesWritten == 256) {
            sectors.dataSectors[IMU].currPage++;
            currentPageIndex++;
        }

        MemoryBuffer imuCurrentPage = sectors.dataSectors[IMU].buffers[currentPageIndex];
        uint8_t *currWritePagePointer = &imuCurrentPage.data[imuCurrentPage.info.bytesWritten];
        memcpy(currWritePagePointer, data->inertial.bytes, sizeof(IMUData));
        imuCurrentPage.info.bytesWritten += sizeof(IMUData);
        data->inertial.updated = false;

        printf("timestamp: %d, acc[0]= %d, acc[1]= %d, acc[2]= %d, gyro[0]= %d \n",
               to_int32_t(&imuCurrentPage.data[0]), to_int32_t(&imuCurrentPage.data[4]),
               to_int32_t(&imuCurrentPage.data[8]), to_int32_t(&imuCurrentPage.data[12]),
               to_int32_t(&imuCurrentPage.data[16]));
    }

    if (data->pressure.updated) {
        int currentPageIndex = sectors.dataSectors[Pressure].currPage;
        if (sectors.dataSectors[Pressure].buffers[currentPageIndex].info.bytesWritten == 256) {
            sectors.dataSectors[Pressure].currPage++;
            currentPageIndex++;
        }

        MemoryBuffer pressCurrentPage = sectors.dataSectors[Pressure].buffers[currentPageIndex];
        uint8_t *currWritePagePointer = &pressCurrentPage.data[pressCurrentPage.info.bytesWritten];
        memcpy(currWritePagePointer, data->pressure.bytes, sizeof(PressureData));
        pressCurrentPage.info.bytesWritten += sizeof(PressureData);
        data->pressure.updated = false;


        printf("timestamp: %d, pressure[0]= %f, temperature[1]= %f \n",
               to_int32_t(&pressCurrentPage.data[0]), to_float(&pressCurrentPage.data[4]),
               to_float(&pressCurrentPage.data[8]));
    }

    if (data->continuity.updated) {
        int currentPageIndex = sectors.dataSectors[Cont].currPage;
        if (sectors.dataSectors[Cont].buffers[currentPageIndex].info.bytesWritten == 256) {
            sectors.dataSectors[Cont].currPage++;
            currentPageIndex++;
        }

        MemoryBuffer contCurrentPage = sectors.dataSectors[Cont].buffers[currentPageIndex];
        uint8_t *currWritePagePointer = &contCurrentPage.data[contCurrentPage.info.bytesWritten];
        memcpy(currWritePagePointer, data->continuity.bytes, sizeof(Continuity));
        contCurrentPage.info.bytesWritten += sizeof(Continuity);
        data->continuity.updated = false;

        printf("timestamp: %d, continuity[0]= %d \n",
               to_int32_t(&contCurrentPage.data[0]), to_uint8_t(&contCurrentPage.data[4]));
    }

    if (data->event.updated) {
        int currentPageIndex = sectors.dataSectors[Event].currPage;
        if (sectors.dataSectors[Event].buffers[currentPageIndex].info.bytesWritten == 256) {
            sectors.dataSectors[Event].currPage++;
            currentPageIndex++;
        }

        MemoryBuffer eventCurrentPage = sectors.dataSectors[Event].buffers[currentPageIndex];
        uint8_t *currWritePagePointer = &eventCurrentPage.data[eventCurrentPage.info.bytesWritten];
        memcpy(currWritePagePointer, data->event.bytes, sizeof(FlightEvent));
        eventCurrentPage.info.bytesWritten += sizeof(FlightEvent);
        data->event.updated = false;

        printf("timestamp: %d, event[0]= %d \n",
               to_int32_t(&eventCurrentPage.data[0]), to_uint8_t(&eventCurrentPage.data[4]));
    }

}

//        uint32_t timestamp = to_int32_t(&data.inertial.bytes[0]);

//


//        sectors.dataSectors[IMU].buffers[0]->data[] = data.inertial.timestamp && data.inertial.accelerometer && data.inertial.gyroscope && data.inertial.magnetometer ;
//        if(sectors.dataSectors[IMU].buffers[0]->info.bytesWritten == 256)
//        {
//            // dataSectors[i].pages[IMU].isFull = true;
//        }
//    }


//    if(data.pressure.updated)
//    {
//        sectors.dataSectors[Pressure].buffers[0]->data[sectors.dataSectors[Pressure].buffers[0]->info.bytesWritten] = data.pressure.timestamp && data.pressure.pressure && data.pressure.temperature;
//        if(sectors.dataSectors[Pressure].buffers[0]->info.bytesWritten == 255)
//        {
//            // dataSectors[i].pages[Pres].isFull = true;
//        }
//    }
//
//    if(data.continuity.updated)
//    {
//        sectors.dataSectors[Cont].buffers[0]->data[sectors.dataSectors[Cont].buffers[0]->info.bytesWritten] = data.continuity.status;
//        if(sectors.dataSectors[Cont].buffers[0]->info.bytesWritten == 255)
//        {
//            // dataSectors[i].pages[Cont].isFull = true;
//        }
//    }
//
//    if(data.event.updated)
//    {
//        sectors.dataSectors[Event].buffers[0]->data[sectors.dataSectors[Event].buffers[0]->info.bytesWritten] = data.event.status;
//        if(sectors.dataSectors[Event].buffers[0]->info.bytesWritten == 255)
//        {
//            // dataSectors[i].pages[IMU].isFull = true;
//        }
//    }






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






