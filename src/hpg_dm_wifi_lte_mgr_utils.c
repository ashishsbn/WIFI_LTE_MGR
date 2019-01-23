
#ifndef _WIFI_LTE_MGR_UTILS_C_
#define _WIFI_LTE_MGR_UTILS_C_

/** @file   wifi_lte_mgr_utils.c
  * @brief  Implementation of Harman gateway Wi-Fi to LTE Handoff Manager
  * @author Ashish Sharma <Ashish.Sharma@Harman.com>
  * @bug    No known bugs.
  */

/********************************
*     Included Header Files     *
********************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <artik_module.h>
#include <artik_loop.h>
#include <artik_wifi.h>
#include <artik_network.h>
#include <hpg_dm_wifi_lte_mgr.h>
#include <hpg_dm_utils.h>
#include <hpg_dm_wifi.h>
#include <hpg_dm_wifi_internal.h>
#include <hpg_debug.h>
#include <hpg_dm_lte.h>
#include <hpg_dm_lte_quectel.h>
#include <stdbool.h>
#include <errno.h>

static hpg_dm_modules_t module = LTE_WIFI_MGR;

/* Function: To execute a command and receive output in double pointer buffer */
int hpg_dm_get_output(char *cmd, char **buff) {

    if ( NULL == cmd ) {
        hpg_log(module, LOG_DEBUG,"Error: invalid arguments for command: %s, %s\n",
                cmd, strerror(errno));
        return errno;
    } else {
        char buffer[BUFSIZ];
        size_t byte_count = 0;
        FILE *ptr = NULL;
        bzero(buffer,BUFSIZ);

        if ((ptr = popen(cmd, "r")) == NULL) {
            hpg_log(module, LOG_DEBUG, "Error in popen:  %s\n", strerror(errno));
            (void) pclose(ptr);
            return errno;
        }
        /* Read one byte at a time, up to BUFSIZ - 1 bytes,
         * the last byte will be used for null termination.
         */
        byte_count = fread(buffer, 1, BUFSIZ - 1, ptr);
        /* Apply null termination so that read bytes can be treated as a string.
        */
        buffer[byte_count] = '\0';
        (void) pclose(ptr);
        *buff = (char *) malloc(strlen(buffer) +1);
        strncpy(*buff, buffer, strlen(buffer)+1);
        return HPG_HF_SUCCESS;
    }
}

/* Function: To add route table for wifi station mode */
int hpg_dm_route_add() {

    int ret = 0;
    char *buff = NULL;
    char *route_buff = NULL;
    char *remote_ip = NULL;
    char *route_cmd = NULL;

    /* fetch remote IP address of wlan0 and change default gateway */
    hpg_dm_get_output(WLAN_REMOTE_IP_ADDR, &buff);
    if(buff == '\0') {
        hpg_log(module, LOG_ERR,"Failed to get remote IP\n");
        return HPG_HF_FAILURE;
    }
    remote_ip = (char *) malloc(strlen(buff) +1);
    memset(remote_ip, 0, sizeof(buff) + 1);
    strncpy(remote_ip, buff, strlen(buff) +1);
    hpg_log(module, LOG_DEBUG,"Remote IP == %s\n", remote_ip);

    route_cmd = (char *) malloc(sizeof(char) * 48);
    memset(route_cmd, 0, sizeof(char) * 48);
    sprintf(route_cmd, "route add default gw %s", remote_ip);
    hpg_log(module, LOG_DEBUG,"Route change/add command: %s\n", route_cmd);

    if (remote_ip != NULL) {
        ret = hpg_dm_get_output(route_cmd, &route_buff);
        hpg_log(module, LOG_INFO,"Route changed successfully,output = %d\n", ret);
    } else
        hpg_log(module, LOG_INFO,"Remote IP Not available ...\n");

    if(NULL != route_cmd)
        free(route_cmd);
    if(NULL != remote_ip)
        free(remote_ip);
    if(NULL != route_buff)
        free(route_buff);
    if(NULL != buff)
        free(buff);

    return HPG_HF_SUCCESS;
}
#endif /*End of file*/
