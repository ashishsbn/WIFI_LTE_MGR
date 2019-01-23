
#ifndef _WIFI_LTE_MGR_C_
#define _WIFI_LTE_MGR_C_

/** @file   wifi_lte_mgr.c
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

char wifi_lte_mgr_state[STATE_BUFF];

int sockfd;

/* Structure variable to keep Wi-Fi client ssid and psk	*/
hpg_wifi_sta_connect_info sta_info;

/* Global variables for ssid, psk and rssi */
char *rcvd_ssid_g = NULL;
char *rcvd_psk_g = NULL;
int rcvd_rssi_g = 0;

/* Function: To scan wifi Network and store Rssi of
 * SSID input by user into Global Variable
 */
void getScanResult(hpg_wifi_scan_result *scan_result) {

    char buffer[SCAN_BUFF];
    char *rcvd_ssid = NULL;
    char *rcvd_rssi = NULL;
    char *temp_str = NULL;
    char *search_str = NULL;
    char *rssi_token = NULL;
    char *ssid_output = NULL;
    const char *substring = NULL;
    int rssi_output = 0;
    int rcvd_data_count = 0;
    size_t len = 0;

    hpg_log(module, LOG_DEBUG,"Final unique wifi network with higher signal count: %d\n",
		scan_result->length);
    hpg_log(module, LOG_INFO,"Global SSID & SSID length is :%s, %d\n",
    		rcvd_ssid_g, strlen(rcvd_ssid_g));

    for(int i =0; i < scan_result->length ;i++) {
        /* Counting the characters and storing in buffer using snprintf */
        rcvd_data_count = snprintf (buffer, sizeof(buffer),
			"\"%s\"ssid :  #%d \n", scan_result->result[i].Name,
			scan_result->result[i].signal_level);
	if (rcvd_data_count == 0)
	    hpg_log(module, LOG_DEBUG,"Recieved SSID count: %d\n", rcvd_data_count);

        /* Search the Buffer for ssid suffix and store in substring */
        substring = strstr(buffer, "ssid");
        if (substring) {
            len = substring - buffer;
            rcvd_ssid = malloc(len + 1);
            if (rcvd_ssid) {
                memcpy(rcvd_ssid, buffer, len);
                rcvd_ssid[len] = '\0';
                /* Cut blank spaces */
                ssid_output = strtok(rcvd_ssid, "\"\"" );
                /* Check if ssid received from the Network matches to saved ssid
		 */
                if ((temp_str = strstr(ssid_output, rcvd_ssid_g)) != NULL) {
                    /* buffer parsing for rssi */
                    search_str = "#";
                    rcvd_rssi = strtok(buffer, search_str);
                    rcvd_rssi = strtok(NULL, search_str);
                    hpg_log(module, LOG_INFO,"Found SSID: %s, rssi : %s\n",
				temp_str, rcvd_rssi);
                    rssi_token = strtok(rcvd_rssi, "\-");
                    rssi_output = atoi(rssi_token);
                    /* If rssi_output is not Null,
                     * Copy rssi received into Global Variable
                     */
                    if (rssi_output != 0) {
                        rcvd_rssi_g = rssi_output;
                    } else {
                        rcvd_rssi_g = 0;
                    }
                    hpg_log(module, LOG_DEBUG, "DATA = %-20s %s 0x%X %d \n",
				scan_result->result[i].bssid,
				scan_result->result[i].Name,
				scan_result->result[i].encryptionFlag,
				scan_result->result[i].signal_level);
                }
            }
	    if(NULL != rcvd_ssid)
        	free(rcvd_ssid);
        }
    }/*Ending loop*/
}

/* Function: callback for connection status */
void getConnectResult(int status) {

    hpg_log(module, LOG_DEBUG,"Connect response:%d\n", status);
}

/* Function: To execute handoff state for lte and wifi */
int hpg_dm_wifi_lte_mgr(handoff_state state) {

    artik_error ret = S_OK;
    hpg_wifi_sta_info *data = NULL;

    volatile int sta_pid_buf = 0;
    volatile int lte_pid_buf = 0;
    int output = 0;

    switch(state) {

	case LTE_ON:
	    /* LTE ON funtionality */
            hpg_log(module, LOG_INFO,"Turning on : LTE\n");
            /* Get LTE process ID/state */
            lte_pid_buf = hpg_dm_get_lte_on_off_status();
            hpg_log(module, LOG_DEBUG,"State of LTE : %d\n", lte_pid_buf);
            if (lte_pid_buf == HPG_LTE_IFACE_DOWN_ALREADY ||
                lte_pid_buf == HPG_LTE_FAILURE) {
                output = hpg_dm_lte_on_off(true);
                hpg_log(module, LOG_INFO,"LTE turned on/off successfully = %s\n",
				(output == 0 ? "on" : "off"));
            }
	    break;

        case WIFI_ON:
            /* WIFI ON funtionality */
            hpg_log(module, LOG_INFO,"Starting WiFi STA Connect ...\n");
            memset(sta_info.ssid, 0, MAX_SSID_LENGTH);
            memset(sta_info.psk, 0, MAX_PSK_LENGTH);

            /* Get Wi-Fi Client info */
            data = hpg_dm_get_wifi_info();
            hpg_log(module, LOG_DEBUG,"Wi-Fi is Connected:%d\n", data->is_connected);
            /* If wifi connection returns false, then attempt to connect,
             * else print the ssid name
             */
            if(data->is_connected == false) {
                strncpy(sta_info.ssid, rcvd_ssid_g, strlen(rcvd_ssid_g));
                strncpy(sta_info.psk, rcvd_psk_g, strlen(rcvd_psk_g));
                hpg_log(module, LOG_INFO,"Connecting to Wi-Fi network with ssid:%s\n",
                        sta_info.ssid);
                hpg_log(module, LOG_DEBUG,"Connecting to Wi-Fi network with psk:%s\n",
                        sta_info.psk);
                /* Wi-Fi connect call */
                ret = hpg_dm_wifi_connect(&sta_info, &getConnectResult);
                /* Connect return & fetch IP Address received on wlan0 interface */
                hpg_log(module, LOG_INFO,"WiFi STA ON result: %d\nIP Address of STA: %s\n",
				ret, data->ip);
            /*Add route after STA ON*/
            ret = hpg_dm_route_add();
            if(!ret)
                hpg_log(module, LOG_ERR,"Route is not added for WiFi\n");
            } else
                hpg_log(module, LOG_INFO,"Already Connected, SSID is :%s\n",
			data->apName);
	    break;

        case LTE_OFF:
            /* LTE OFF funtionality */
            hpg_log(module, LOG_INFO,"Turning LTE off ...\n");
            /* Get LTE process ID/state */
            lte_pid_buf = hpg_dm_get_lte_on_off_status();
            hpg_log(module, LOG_DEBUG,"State of LTE : %d\n", lte_pid_buf);
            if (lte_pid_buf == HPG_LTE_IFACE_UP_ALREADY) {
                hpg_log(module, LOG_DEBUG,"Turning LTE : OFF ...\n");
                output = hpg_dm_lte_on_off (false);
                hpg_log(module, LOG_INFO,"LTE turned on/off successfully = %s\n",
				(output == 0 ? "off" : "on"));
            }
            rcvd_rssi_g = 0;
            break;

        case WIFI_OFF:
            /* WIFI OFF funtionality */
            hpg_log(module, LOG_INFO,"WiFi STA Disconnect ...\n");
            /* Get Wi-Fi Client info */
            data = hpg_dm_get_wifi_info();
            hpg_log(module, LOG_DEBUG,"WiFi is Connected:%d\n", data->is_connected);
            if(data->is_connected == true) {
                hpg_log(module, LOG_DEBUG,"WIFI STATION Mode is also executing"
			"along with lte,\t Stop STATION Mode \n");
                ret = hpg_dm_wifi_disconnect();
                hpg_log(module, LOG_INFO,"WiFi STA OFF result: %d\n", ret);
                ret = system(FLUSH_WLAN0);
                if(ret == -1)
                    hpg_log(module, LOG_DEBUG, "some error is occured in shell"
                            "command: %s\n", FLUSH_WLAN0);
            } else
                hpg_log(module, LOG_INFO,"WIFI STATION Mode was not executing\n");

            sleep(5);

            /* Keep wpa_supplicant active */
            sta_pid_buf =  hpg_dm_get_wpa_supplicant_pid ();
            hpg_log(module, LOG_DEBUG, "STA process ID : %d\n", sta_pid_buf);
            if (sta_pid_buf == 0) {
                output = hpg_dm_start_wpa_supplicant();
                hpg_log(module, LOG_DEBUG,"STARTED wpa_supplicant : %d\n",output);
            }
            break;

        default:
            hpg_log(module, LOG_DEBUG, "(%s): No Valid handoff state provided\n");
	}

    return HPG_HF_SUCCESS;
}

/* Function: function to get hotspot info and saved to global variable */
int hpg_dm_get_hotspot_info() {

    int ret = 0;
    struct sysmgr_config *config;

    /* Create and update DB for Test */
    //hpg_dm_utils_create_db(SYSMGR_SQLITE_DB);
    config = calloc(sizeof(sysmgr_config),1);
    #if 0
    config->state = 0;
    strncpy(config->ssid,"Alks",strlen("Alks")+1);
    strncpy(config->psk,"alok1234",strlen("alok1234")+1);

    hpg_dm_utils_sqlite_write(config);
    #endif
    hpg_dm_utils_sqlite_read(&config);
    hpg_log(module, LOG_DEBUG,"Data from DB, SSID : '%s', PSK : '%s', State : ",
		config->ssid, config->psk, config->state);

    if(config->ssid[0] != '\0' && config->psk[0] != '\0') {
        /* Allocate memory for sta_ssid and sta_psk	*/
        rcvd_ssid_g =(char *) malloc(strlen(config->ssid) +1);
        rcvd_psk_g = (char *) malloc(strlen(config->psk)  +1);

        /* Copy user input ssid & psk into Global Variable */
        strncpy(rcvd_ssid_g, config->ssid, strlen(config->ssid) +1);
        strncpy(rcvd_psk_g, config->psk, strlen(config->psk) +1);
        hpg_log(module, LOG_DEBUG,"Global variable, SSID: '%s', PSK: '%s'\n",
			rcvd_ssid_g, rcvd_psk_g);
    } else if(config->ssid[0] != '\0') {
        /* Allocate memory for sta_ssid for open network */
        rcvd_ssid_g =(char *) malloc(strlen(config->ssid) +1);

        /* Copy user input ssid into Global Variable */
        strncpy(rcvd_ssid_g, config->ssid, strlen(config->ssid) +1);
        hpg_log(module, LOG_DEBUG,"Global variable, SSID: '%s'\n",
			rcvd_ssid_g);
    } else
	ret = HPG_HF_FAILURE;

    free(config);

    return ret;
}

/*Function: This will initialize logging LTE_WIFI_MGR module*/
void hpg_dm_lte_wifi_mgr_initialize_debug (void) {
    hpg_log_t log_level;

    hpg_init_logging(module, HPG_LTE_WIFI_MGR_LOGGING);
    log_level = LOG_DEBUG;
    hpg_set_log_level(log_level);
}

/* Function: To initialize daemon of wifi and lte handoff */
int main(int argc, char *argv[]) {

    int output = 0;
    int state = 1;
    int mode = 0;
    int rssi = 0;
    int ret = -1;
    char current_lte_mgr_state[STATE_BUFF];

    /* Initialize Logging Library */
    hpg_dm_wifi_initialize_debug();
    hpg_dm_utils_initialize_debug();
    hpg_dm_lte_initialize_debug();
    hpg_dm_lte_wifi_mgr_initialize_debug();

    hpg_dm_lte_wifi_mgr_ipc_thread();

    /* Setting Dual Mode, It will turn on AP mode and bring up STA interface */
    mode = hpg_dm_wifi_current_mode();
    if(DUALMode != mode) {
        output = hpg_dm_wifi_set_mode("DUAL");
        hpg_log(module, LOG_DEBUG,"DUAL Mode set successfully, Return = %d\n", output);
    } else
        hpg_log(module, LOG_DEBUG,"Already in DUAL Mode : mode=%d\n", mode);

    while(1) {
        ret = hpg_dm_get_lte_rssi(&rssi);
        if( ret == 0) {
            hpg_log(module, LOG_DEBUG,"Able to get LTE RSSI : %d\n", rssi);
            break;
        }
	strncpy(wifi_lte_mgr_state, WIFI_STATE, sizeof(WIFI_STATE));

	if(wifi_lte_mgr_state[0] != '\0' && strncmp(current_lte_mgr_state, \
		wifi_lte_mgr_state, sizeof(wifi_lte_mgr_state)) ) {
	    strncpy(current_lte_mgr_state, wifi_lte_mgr_state, \
		sizeof(wifi_lte_mgr_state));
	    hpg_dm_lte_wifi_mgr_send_over_ipc(sockfd);
	}
	sleep(5);
    }

    /*		LTE TO WIFI HANDOFF Code Start		*/
    /*  Fetch wlan0 RSSI in a while loop and Start and stop LTE
     *  based on received wlan0 RSSI value.
     *  The received RSSI Values are like 0 , -30 dBm, -45 dBm etc &
     *  only integer part like 0, 30 or 45 etc is considered.
     *  The lower the RSSI, more stable is the connectivity
     *******************************************************************/

    while(1) {
        /* Scan Wi-Fi Network based on SSID input by the user and store rssi of
         * that network in decision variable As soon as the ssid matches with
         * user input ssid, Its rssi is fetched and put into Global variable to
         * be used as a switch decision variable.
         */
	if(state) {
	    state = hpg_dm_get_hotspot_info();
	    rcvd_rssi_g = 0;
	}
        if(!state) {
            hpg_dm_wifi_scan(&getScanResult);
            hpg_log(module, LOG_INFO,"Wi-Fi -Fi scan rssi = %d\n", rcvd_rssi_g);
	}

        /* State machine on the basis of RSSI Value of user entered SSID */
        switch (rcvd_rssi_g) {

            case 0:
		/* Received wlan0 rssi is zero, This may be the case when wifi
		 * network is not available or User specified wifi SSID is not
		 * in range, In this case, the first step would be to turn LTE
		 * on if LTE rssi is available and in suitable range
		 */
		hpg_dm_wifi_lte_mgr(LTE_ON);
		hpg_dm_wifi_lte_mgr(WIFI_OFF);
		strncpy(wifi_lte_mgr_state, LTE_STATE, sizeof(LTE_STATE));
                break;

            case 25 ... 75:
		/* Received wlan0 rssi is in suitable range to make a connection
		 * The values received are between -25 dBm to -75 dBm
		 */
		hpg_dm_wifi_lte_mgr(LTE_OFF);
		hpg_dm_wifi_lte_mgr(WIFI_ON);
		strncpy(wifi_lte_mgr_state, WIFI_STATE, sizeof(WIFI_STATE));
                break;

            case 76 ... 100:
		/* Received wlan0 rssi is available but not suitable to connect
		 * so keep LTE turned along with Wi-Fi
		 */
		hpg_dm_wifi_lte_mgr(LTE_ON);
		hpg_dm_wifi_lte_mgr(WIFI_ON);
		strncpy(wifi_lte_mgr_state, WIFI_STATE, sizeof(WIFI_STATE));
                break;

            default:
		/* Rssi is available but not in a range suitable to make
		 * connection, so keep LTE turned ON
		 */
		hpg_dm_wifi_lte_mgr(LTE_ON);
		hpg_dm_wifi_lte_mgr(WIFI_OFF);
		strncpy(wifi_lte_mgr_state, LTE_STATE, sizeof(LTE_STATE));
        }
	if(wifi_lte_mgr_state[0] != '\0' && strncmp(current_lte_mgr_state, \
		wifi_lte_mgr_state, sizeof(wifi_lte_mgr_state)) ) {
	    strncpy(current_lte_mgr_state, wifi_lte_mgr_state, \
		sizeof(wifi_lte_mgr_state));
	    hpg_dm_lte_wifi_mgr_send_over_ipc(sockfd);
	}
    }
    exit(0);
}
#endif /*End of file*/
