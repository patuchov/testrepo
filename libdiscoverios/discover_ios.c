#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "discover_android.h"
#include "discover_android_threads.h"

devicelist_t *GetConnectedDevices()
{
    int fd[2];

    if (pipe(fd) == -1)
    {
        printf("pipe() failed\n");
        return NULL;
    }

    pid_t iDevicePID = fork();

    if (iDevicePID == 0)
    {
        close(fd[0]);
        dup2(fd[1],1);
        dup2(fd[1],2);
        close(fd[1]);
        execl("/usr/bin/idevice_id", "idevice_id", "-l", NULL);
    }

    close(fd[0]);
    int status;

    waitpid(iDevicePID, &status, 0);

    if (WEXITSTATUS(status) != 0)
    {
        printf("Couldn't find any connected iOS device\n");
        return NULL;
    }

    char buffer[42];
    FILE *input = fdopen(fd[0], "r");
    int deviceCount = 0;
    char **udidList = (char *)calloc(100, sizeof(char *));

    while(fgets(buffer, 42, input))
    {
        if (deviceCount < 100)
        {
            udidList[deviceCount++] = strdup(buffer);
        }
        else
        {
            printf("Reached max amount of devices connected\n");
            break;
        }
    }

    devicelist_t *iosDeviceList = CreateDeviceList();

    if (iosDeviceList == NULL)
    {
        printf("failed to create iosdevicelist\n");
        return NULL;
    }

    for (int i = 0; i < deviceCount; i++)
    {
        iosprops_t *newProps = GetDeviceInfo(udidList[i]);
        if (AddElementToDeviceList(iosDeviceList, (void *)newProps) == false)
        {
            return NULL;
        }
    }

    return deviceList;
}
