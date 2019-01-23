
#ifndef _WIFI_LTE_MGR_H_
#define _WIFI_LTE_MGR_H_

/** @file   hpg_dm_wifi_lte_mgr.h
  * @brief  This header file contains all functions prototypes for
  *         Wi-Fi to LTE Handoff Manager
  * @author Ashish Sharma <ashishmcacu@gmail.com>
  * @bug    No known bugs.
  */

#define WLAN_REMOTE_IP_ADDR         "cat /etc/resolv.conf | grep nameserver | awk '{print $NF}'"
#define ETC_RESOLV_CONF             "/etc/resolv.conf"
#define NAMESERVER_STD              "nameserver 8.8.8.8"
#define HPG_LTE_WIFI_MGR_LOGGING    "wifi_lte_mgr"

#define SCAN_BUFF	1000
#define HPG_HF_SUCCESS  0
#define HPG_HF_FAILURE  1

#define LOCAL_IP_ADDRESS	"127.0.0.1"
#define LTE_WIFI_IPC_PORT	5924
#define PORT 			0x1212    /* server port of sys_manager */
#define MAXDATASIZE 		100
#define BUFF_CHUNK 		32
#define STATE_BUFF		5

#define LTE_WIFI_MANAGER_TO_SYSMGR_HO_TRIGGERED 40

#define WIFI_STATE  "wifi"
#define LTE_STATE   "lte"

char wifi_lte_mgr_state[STATE_BUFF];

typedef enum {
    LTE_ON = 0,
    LTE_OFF,
    WIFI_ON,
    WIFI_OFF
}handoff_state;

/** @brief  This method gives command output buffer
 *  @param  string
 *  @param  string
 *  @return integer
 */
int hpg_dm_get_output(char *cmd, char **buff);

/** @brief  This method will add route for Wi-Fi
 *  @param  void
 *  @return integer
 */
int hpg_dm_route_add(void);

/** @brief  This method will use in getscan callback
 *  @param  integer
 *  @return void
 */
void getConnectResult(int status);

/** @brief  This method will turn on and off lte/wifi
 *  @param  enum
 *  @param  enum
 *  @return integer
 */
int hpg_dm_wifi_lte_mgr(handoff_state state);

/** @brief  This method will get hotspot info from DB
 *  @param  void
 *  @return integer
 */
int hpg_dm_get_hotspot_info(void);

/** @brief  This method will send handoff state
 *  @param  integer
 *  @return void
 */
void hpg_dm_lte_wifi_mgr_send_over_ipc(int fd);

/** @brief  This method will start ipc thread with sysmgr
 *  @param  void
 *  @return integer
 */
int hpg_dm_lte_wifi_mgr_ipc_thread(void);

#endif/*End of file*/
