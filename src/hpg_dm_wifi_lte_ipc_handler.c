
#ifndef _WIFI_LTE_IPC_HANDLER_C_
#define _WIFI_LTE_IPC_HANDLER_C_

/** @file   hpg_dm_wifi_lte_ipc_handler.c
  * @brief  Implementation of ipc handler for Wi-Fi to LTE Handoff Manager
  * @author Ashish Sharma <ashishmcacu@gmail.com>
  * @bug    No known bugs.
  */

/********************************
*     Included Header Files     *
********************************/

#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <fcntl.h>      /* for opening socket */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     /* for closing socket */
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdbool.h>
#include <pthread.h>

#include <hpg_debug.h>
#include <hpg_dm_wifi_lte_mgr.h>

pthread_t g_thread_ipc;

static hpg_dm_modules_t module = LTE_WIFI_MGR;

extern int sockfd;

typedef struct event_t {
    int  event_id;
    char data[BUFF_CHUNK];
} event_t;

/* Function: function for sending handoff state to sysmgr */
void hpg_dm_lte_wifi_mgr_send_over_ipc(int fd) {

    event_t reply_event;

    memset(&reply_event, 0, sizeof(reply_event));
    hpg_log(module, LOG_DEBUG, "Handoff State : %s\n", wifi_lte_mgr_state);

    if(wifi_lte_mgr_state[0] != '\0') {
        reply_event.event_id = LTE_WIFI_MANAGER_TO_SYSMGR_HO_TRIGGERED;
        strncpy(reply_event.data, wifi_lte_mgr_state, strlen(wifi_lte_mgr_state)+1);
        send(fd, &reply_event, sizeof(event_t), 0);
        hpg_log(module, LOG_DEBUG,"%s:%d LTE_WIFI Status updated successfully.\n",
		__FILE__, __LINE__);
        memset(wifi_lte_mgr_state, 0, sizeof(wifi_lte_mgr_state));
    }
}

/* Function: function for starting socket communication with sysmgr server */
static void *hpg_dm_lte_wifi_mgr_ipc_handler() {

    int    numbytes = 0;
    int    ret = 0;
    struct sockaddr_in their_addr; /* connector's address information */
    struct sockaddr_in sa_loc;
    int    option_value;
    event_t event;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
	perror("socket");
	hpg_log(module, LOG_ERR, "%s:%d, Socket fd open failed\n",
		__FILE__, __LINE__);
	exit(1);
    }

    /* Local port */
    memset(&sa_loc, 0, sizeof(struct sockaddr_in));
    sa_loc.sin_family      = AF_INET;
    sa_loc.sin_port        = htons(LTE_WIFI_IPC_PORT);
    sa_loc.sin_addr.s_addr = inet_addr(LOCAL_IP_ADDRESS);

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option_value, sizeof(option_value));

    ret = bind(sockfd, (struct sockaddr *)&sa_loc, sizeof(struct sockaddr));
    if (ret == -1) {
	hpg_log(module, LOG_ERR, "Bind failed\n");
	exit(1);
    }

    their_addr.sin_family      = AF_INET;      /* host byte order */
    their_addr.sin_port        = htons(PORT);    /* short, network byte order */
    their_addr.sin_addr.s_addr = inet_addr(LOCAL_IP_ADDRESS);
    bzero(&(their_addr.sin_zero), 8);     /* zero the rest of the struct */

    if (connect(sockfd, (struct sockaddr *)&their_addr, \
		sizeof(struct sockaddr)) == -1) {
	perror("connect");
	hpg_log(module, LOG_ERR, "%s:%d, Socket connect failed\n",
		__FILE__, __LINE__);
	exit(1);
    }
    while (1) {
	memset(&event,0,sizeof(event_t));
	if ((numbytes=recv(sockfd, (char *)&event, sizeof(event_t), 0)) == -1) {
	    perror("recv");
	    hpg_log(module, LOG_ERR, "%s:%d, Recieved failed from server\n",
		__FILE__, __LINE__);
	    exit(1);
	}
    }
    close(sockfd);
}

/* Function: function for starting thread */
int hpg_dm_lte_wifi_mgr_ipc_thread() {

    if( pthread_create(&g_thread_ipc, NULL, &hpg_dm_lte_wifi_mgr_ipc_handler, NULL) != 0) {
        hpg_log(module, LOG_DEBUG,"(%s:%d) pthread_create failed :%s\n",
                __FUNCTION__, __LINE__, strerror( errno ));
    }
    return 0;
}
#endif
