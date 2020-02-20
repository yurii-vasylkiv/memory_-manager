#ifndef MEMORY_MANAGER_MEMORY_MANAGER_H
#define MEMORY_MANAGER_MEMORY_MANAGER_H


#include <inttypes.h>

typedef union
{
    struct{
        uint32_t timestamp;
        int32_t accelerometer[3];
        int32_t gyroscope[3];
    };
    uint8_t bytes[sizeof(uint32_t) + 3 * sizeof(int32_t) + 3 * sizeof(int32_t)];
} IMUDataU;

typedef struct
{
    uint8_t updated;
    IMUDataU data;
} IMUData;


typedef union
{
    struct {
        uint32_t timestamp;
        float pressure;
        float temperature;
    };
    uint8_t bytes[sizeof(uint32_t) + sizeof(float) + sizeof(float)];
} PressureDataU;

typedef struct
{
    uint8_t updated;
    PressureDataU data;
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
} ContinuityU;

typedef struct
{
    uint8_t updated;
    ContinuityU data;
} Continuity;

typedef union
{
    struct {
        uint32_t timestamp;
        FlightEventStatus status;
    };
    uint8_t bytes[sizeof(uint32_t) + sizeof(uint8_t)];
}FlightEventU;


typedef struct
{
    uint8_t updated;
    FlightEventU data;
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

typedef struct
{
    const MemorySectorInfo info;
    MemoryBuffer buffers[2];
    MemoryBuffer *read;
    MemoryBuffer *write;
} MemoryDataSector;

typedef struct
{
    MemorySectorInfo info;
    MemoryBuffer buffers[2];
} MemoryConfigurationSector;


typedef enum { IMU, Pressure, Cont, Event, SectorsCount} Sector;

#ifdef __cplusplus
extern "C" {
#endif

int memory_manager_init();
int memory_manager_configure();
int memory_manager_update(Data *_container);
int memory_manager_add_imu_update(IMUDataU *_container);
int memory_manager_add_pressure_update(PressureDataU *_container);
int memory_manager_add_continuity_update(ContinuityU *_container);
int memory_manager_add_flight_event_update(FlightEventU *_container);
int memory_manager_start();
int memory_manager_stop();

// all of your legacy C code here
#ifdef __cplusplus
}
#endif

#endif //MEMORY_MANAGER_MEMORY_MANAGER_H
