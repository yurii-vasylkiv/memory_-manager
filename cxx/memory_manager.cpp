#include "memory_manager.h"

#include <pthread.h>
#include <stdio.h>
#include <memory.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "queue.h"

static const char * MAGIC                               = "e10e720-6fb8-4fc0-8807-6e2201ac6e0d";
#define CONFIGURATION_SECTOR_BASE_ADDRESS               0
#define CONFIGURATION_SECTOR_OFFSET                     CONFIGURATION_SECTOR_BASE_ADDRESS + sizeof(ConfigurationU)
#define DATA_SECTOR_SIZE                                (1024 /* KB */ * 1024 /* MB */ * 10)
#define CONFIGURATION_AUTOSAVE_INTERVAL                 200
#define PAGE_SIZE                                       256

static ConfigurationU configurations                    = {};
static MemoryConfigurationSector confSector             = {};
static MemoryDataSector dataSectors[SectorsCount]       = {};
static queue s_page_cb                                  = {};
static pthread_t s_queue_monitor                        ;
static uint8_t is_queue_monitor_running                 = {0};
static uint8_t s_is_initialized                         = {0};
static size_t s_configurations_autosave_interval_tick   = {0};


static int _add_data(Sector sector, uint8_t * buffer, size_t size);
static void _write_async_to_flash_if_anything_is_available();
static void *_queue_monitor_task(void *arg);
static int _update_configurations();
static int _read_configurations_from_flash(ConfigurationU * conf_container);
static int _read_page_from_flash(int address, MemoryBuffer * page_container);


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

    if(_read_configurations_from_flash(&configurations) != 0)
    {
        return 1;
    }

    if(memcmp(configurations.magic, MAGIC, 36) == 0) // magic sequences match)
    {
        // Configuration is correct to read
        for (size_t sector = IMU; sector < SectorsCount; sector++)
        {
            dataSectors[sector].info = configurations.data_sectors[sector];
        }
    }
    else
    {
        // configuration magic-number is missing

        // fresh start
        for (size_t sector = IMU; sector < SectorsCount; sector++)
        {
            dataSectors[sector].info.size           = DATA_SECTOR_SIZE;
            dataSectors[sector].info.startAddress   = CONFIGURATION_SECTOR_OFFSET + DATA_SECTOR_SIZE * sector;
            dataSectors[sector].info.endAddress     = dataSectors[sector].info.startAddress + dataSectors[sector].info.size;
            dataSectors[sector].info.bytesWritten   = 0;

            configurations.data_sectors[sector]     = dataSectors[sector].info;
        }

        // storing the magic number
        memcpy(configurations.magic, MAGIC, 36);
    }

    return 0;
}

int memory_manager_get_configurations(ConfigurationU * pConfigs)
{
    // validation
    if(memcmp(configurations.magic, MAGIC, 36) == 0) // magic sequences match)
    {
        pConfigs = &configurations;
        return 0;
    }

    pConfigs = NULL;
    return 1;
}

int memory_manager_update_sensors_ground_data(GroundDataU * _data)
{
    if(_data == NULL)
    {
        return 1;
    }

    configurations.ground_altitude = _data->altitude;
    configurations.ground_pressure = _data->pressure;

    _update_configurations();
    return 0;
}


int _read_configurations_from_flash(ConfigurationU * conf_container)
{
    assert(sizeof(ConfigurationU) < PAGE_SIZE);

    if(conf_container == NULL)
    {
        return 1;
    }

    int bytesLeftToWrite = sizeof(ConfigurationU);
    int configuration_pages = trunc(bytesLeftToWrite / PAGE_SIZE);
    MemoryBuffer page;

    if(configuration_pages == 0)
    {
        page = {};
        if(_read_page_from_flash(CONFIGURATION_SECTOR_BASE_ADDRESS, &page) !=0)
        {
            return 1;
        }

        memmove(conf_container->bytes, page.data, bytesLeftToWrite);
        return 0;
    }

    int bytesWritten = 0;
    while (bytesLeftToWrite > PAGE_SIZE)
    {
        page = {};
        if(_read_page_from_flash(CONFIGURATION_SECTOR_BASE_ADDRESS, &page) !=0)
        {
            return 1;
        }

        memmove(&conf_container->bytes[bytesWritten], page.data, PAGE_SIZE);
        bytesLeftToWrite -= PAGE_SIZE;
        bytesWritten += PAGE_SIZE;
    }


    page = {};
    if(_read_page_from_flash(CONFIGURATION_SECTOR_BASE_ADDRESS, &page) !=0)
    {
        return 1;
    }

    memmove(conf_container->bytes, page.data, bytesLeftToWrite);
    return 0;
}


int _read_page_from_flash(int address, MemoryBuffer * page_container)
{
    if(page_container == NULL)
    {
        return 1;
    }

    // simulate the memory read and the configuration sector is empty;
    memset(page_container->data, 0, PAGE_SIZE);
    return 0;
}


int _update_configurations()
{
    assert(sizeof(ConfigurationU) < PAGE_SIZE);

    queue_item   item {};
    item.type = Conf;
    memmove(item.data, configurations.bytes, sizeof(ConfigurationU));
    s_page_cb.push(item);
    return 0;


//    for (int i = 0; i < CONFIGURATION_DATA_SECTOR_BUFFER_COUNT; ++i)
//    {
//        confSector.buffers[i]
//    }
//
//
//    MemoryConfigurationSector confS {};
//    int bytesToWrite = sizeof(ConfigurationU);
//    int bytesWritten = 0;
//    while (bytesToWrite > PAGE_SIZE)
//    {
//        queue_item item {};
//        item.type = Conf;
//        memmove(item.data, configurations.bytes, PAGE_SIZE);
//        s_page_cb.push(item);
//        memcpy(confS.buffers[confS.current_buffer_index].data, &configurations.bytes[bytesWritten], PAGE_SIZE);
//        bytesToWrite -= PAGE_SIZE;
//        bytesWritten += PAGE_SIZE;
//        confS.current_buffer_index++;
//    }
//
//    memmove(confS.buffers[confS.current_buffer_index]., &configurations.bytes[bytesWritten], bytesToWrite);
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
        _write_async_to_flash_if_anything_is_available();

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


    s_configurations_autosave_interval_tick++;
    if(s_configurations_autosave_interval_tick >= CONFIGURATION_AUTOSAVE_INTERVAL)
    {
        // it is time to update the configurations
        _update_configurations();
        // reset the timer
        s_configurations_autosave_interval_tick = 0;
    }

    return 0;
}


void _write_async_to_flash_if_anything_is_available()
{
    // free the read page
    for (size_t sector = IMU; sector < SectorsCount; sector++)
    {
        if (dataSectors[sector].read->info.bytesWritten >= 252)
        {
            queue_item item {};
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
        queue_item item {};
        while(s_page_cb.pop(item))
        {
            switch(item.type)
            {
                case IMU:
//                    printf("Monitor: IMU was flushed!\n");
                    break;
                case Pressure:
//                    printf("Monitor: Pressure was flushed!\n");
                    break;
                case Cont:
//                    printf("Monitor: Cont was flushed!\n");
                    break;
                case Event:
//                    printf("Monitor: Event was flushed!\n");
                    break;
                case Conf:
                    printf("Monitor: Configuration was flushed!\n");
                    break;
                default:
//                    printf("Monitor: Wrong type! Wtf?\n");
                    break;
            }
        }
    }

    return NULL;
}



