#include "memory_manager.h"

#include <pthread.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include "queue.h"

static MemoryConfigurationSector configuration_sector   = {};
static MemoryDataSector dataSectors[SectorsCount]       = {};
static queue s_page_cb                                  = {};
static pthread_t s_queue_monitor                        ;
static uint8_t is_queue_monitor_running                 = {0};
static uint8_t s_is_initialized                         = {0};


int _add_data(Sector sector, uint8_t * buffer, size_t size);
void _writeAsyncToFlashIfAnythingIsAvailable();
void *_queue_monitor_task(void *arg);

int memory_manager_init()
{
    for (uint8_t sector = IMU; sector < SectorsCount; sector++)
    {
        if (dataSectors[sector].write == NULL)
        {
            dataSectors[sector].write = &dataSectors[sector].buffers[0];
        }

        if (dataSectors[sector].read == NULL)
        {
            dataSectors[sector].read = &dataSectors[sector].buffers[1];
        }
    }

    s_is_initialized = 1;

    return 0;
}

int memory_manager_configure()
{
    if(!s_is_initialized)
    {
        return 1;
    }

    return 0;
}

int memory_manager_update(Data *_container)
{
    if(!s_is_initialized)
    {
        return 1;
    }

    if(_container == NULL)
    {
        return 1;
    }

    if (_container->inertial.updated)
    {
        memory_manager_add_imu_update(&_container->inertial.data);
        _container->inertial.updated = 0;
    }

    if (_container->pressure.updated)
    {
        memory_manager_add_pressure_update(&_container->pressure.data);
        _container->pressure.updated = 0;
    }

    if (_container->continuity.updated)
    {
        memory_manager_add_continuity_update(&_container->continuity.data);
        _container->continuity.updated = 0;
    }

    if (_container->event.updated)
    {
        memory_manager_add_flight_event_update(&_container->event.data);
        _container->event.updated = 0;
    }
    
    return 0;
}


int memory_manager_add_imu_update(IMUDataU *_container)
{
    return _add_data(IMU, _container->bytes, sizeof(PressureDataU));
}

int memory_manager_add_pressure_update(PressureDataU *_container)
{
    return _add_data(Pressure, _container->bytes, sizeof(PressureDataU));
}

int memory_manager_add_continuity_update(ContinuityU *_container)
{

    return _add_data(Cont, _container->bytes, sizeof(ContinuityU));
}

int memory_manager_add_flight_event_update(FlightEventU *_container)
{

    return _add_data(Event, _container->bytes, sizeof(FlightEventU));
}


int memory_manager_start()
{
    if(!s_is_initialized)
    {
        return 1;
    }

    if(pthread_create(&s_queue_monitor, NULL, _queue_monitor_task, NULL)) {

        printf("Error creating thread\n");
        return 1;
    }

    return 0;
}

int memory_manager_stop()
{
    is_queue_monitor_running = 0;
    return 0;
}


int _add_data(Sector sector, uint8_t * buffer, size_t size)
{
    if(!s_is_initialized)
    {
        return 1;
    }

    if (dataSectors[sector].write->info.bytesWritten >= 252 /*sizeof(IMUData) * 9*/)
    {
        MemoryBuffer * temp_read = dataSectors[sector].read;
        dataSectors[sector].read = dataSectors[sector].write;
        dataSectors[sector].write = temp_read;
        _writeAsyncToFlashIfAnythingIsAvailable();

        // safeguard check
        if(dataSectors[sector].write->info.bytesWritten >= 252)
        {
            printf("The page was not saved. Probably the monitor has stuck, or something went wrong. Overwriting the page...");
            dataSectors[sector].write->info.bytesWritten = 0;
        }
    }

    MemoryBuffer *currentBuffer = dataSectors[sector].write;
    uint8_t *currWriteBufferPosition = &currentBuffer->data[currentBuffer->info.bytesWritten];
    memcpy(currWriteBufferPosition, buffer, size);
    currentBuffer->info.bytesWritten += size;

    return 0;
}


void _writeAsyncToFlashIfAnythingIsAvailable()
{
    for (uint8_t sector = IMU; sector < SectorsCount; sector++)
    {
        if (dataSectors[sector].read->info.bytesWritten >= 252)
        {
            queue_item item;
            item.type = sector;
            memmove(item.data, dataSectors[sector].read->data, 256);
            dataSectors[sector].read->info.bytesWritten = 0;
            s_page_cb.push(item);
        }
    }
}

void *_queue_monitor_task(void *arg)
{
    is_queue_monitor_running = 1;

    while(is_queue_monitor_running)
    {
        queue_item item;
        while(s_page_cb.pop(item))
        {
            switch(item.type)
            {
                case IMU:
                    printf("Monitor: IMU was flushed!\n");
                    break;
                case Pressure:
                    printf("Monitor: Pressure was flushed!\n");
                    break;
                case Cont:
                    printf("Monitor: Cont was flushed!\n");
                    break;
                case Event:
                    printf("Monitor: Event was flushed!\n");
                    break;
                default:
                    printf("Monitor: Wrong type! Wtf?\n");
                    break;
            }
        }
    }

    return NULL;
}

