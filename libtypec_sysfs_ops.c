/**
 * @file libtypec_sysfs_ops.c
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2021-09-30
 * 
 * @copyright Copyright (c) 2021
 * 
 */

#include "libtypec_ops.h"
#include <dirent.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h> 
#include <sys/stat.h>


#define MAX_PORT_STR 7 /* port%d with 7 bit numPorts */
#define MAX_PORT_MODE_STR 9 /* port%d with 7+2 bit numPorts */




static unsigned long get_svid_from_path(char * path)
{
	char buf[64];
	unsigned short svid;

	FILE *fp = fopen(path,"r");

	fgets(buf,64,fp);

	svid = strtol(buf, NULL, 16);;

	fclose(fp);

	return svid;

}

static unsigned long get_vdo_from_path(char * path)
{
	char buf[64];
	unsigned int vdo;

	FILE *fp = fopen(path,"r");

	fgets(buf,64,fp);

	vdo = strtol(buf, NULL, 16);

	fclose(fp);

	return vdo;

}

unsigned char get_opr_mode(char * path)
{
	char buf[64];
	char * pEnd;
	short ret = OPR_MODE_RD_ONLY; /*Rd sink*/

	FILE *fp = fopen(path,"r");

	fgets(buf,64,fp);
	
	pEnd = strstr(buf,"source");
 
 	if (pEnd != NULL) {

		pEnd = strstr(buf,"sink");
 		
		if (pEnd != NULL) 
				ret = OPR_MODE_DRP_ONLY; /*DRP*/
		else
				ret = OPR_MODE_RP_ONLY; /*Rp only*/
	}

	fclose(fp);

	return ret;
}

static short get_bcd_from_rev_file(char * path)
{
	char buf[10];
	short bcd;

	FILE *fp = fopen(path,"r");

	fgets(buf,10,fp);

	bcd = ((buf[0] -'0') <<8) | (buf[2]-'0');

	fclose(fp);

	return bcd;
}

static int libtypec_sysfs_init (char **session_info)
{

	return 0;

}

static int libtypec_sysfs_exit (void)
{


}

static int libtypec_sysfs_get_capability_ops (struct libtypec_capabiliy_data  *cap_data)
{
    DIR *typec_path = opendir(SYSFS_TYPEC_PATH), *port_path;
	struct dirent *typec_entry, *port_entry;
	int num_ports = 0,num_alt_mode = 0;
	char path_str[512], port_content[512];

	if (!typec_path) {
		printf("opendir typec class failed, %s",SYSFS_TYPEC_PATH);
		return -1;
	}

	while ((typec_entry = readdir(typec_path))) {

		if( !(strncmp(typec_entry->d_name, "port", 4)) && (strlen(typec_entry->d_name) <= MAX_PORT_STR))
		{
			num_ports++;

			sprintf(path_str, SYSFS_TYPEC_PATH "/%s",typec_entry->d_name);

			/*Scan the port capability*/
			port_path = opendir(path_str);

			while ((port_entry = readdir(port_path))) {

				if( !(strncmp(port_entry->d_name, "port", 4)) && (strlen(port_entry->d_name) <= MAX_PORT_MODE_STR))
				{
					num_alt_mode++;
				}
				
			}

			sprintf(port_content,"%s/%s",path_str ,"usb_power_delivery_revision");

			cap_data->bcdPDVersion = get_bcd_from_rev_file(port_content);
			
			memset(port_content,0,512);

			sprintf(port_content,"%s/%s",path_str ,"usb_typec_revision");

			cap_data->bcdTypeCVersion = get_bcd_from_rev_file(port_content);

			closedir(port_path);

		}
	}

    cap_data->bNumConnectors = num_ports;
    cap_data->bNumAltModes = num_alt_mode;

	closedir(typec_path);

}

static int libtypec_sysfs_get_conn_capability_ops (int conn_num, struct libtypec_connector_cap_data *conn_cap_data)
{
    struct stat sb;
	struct dirent *port_entry;
	char path_str[512], port_content[512];

	sprintf(path_str, SYSFS_TYPEC_PATH "/port%d",conn_num);

 	if (lstat(path_str, &sb) == -1) {
		printf("Incorrect connector number : failed to open, %s",path_str);
		return -1;
    }

	sprintf(port_content,"%s/%s",path_str ,"power_role");

	conn_cap_data->opr_mode = get_opr_mode(port_content);

	if(conn_cap_data->opr_mode == OPR_MODE_DRP_ONLY) {
		conn_cap_data->provider = 1;
		conn_cap_data->consumer = 1;
	} 
	else if (conn_cap_data->opr_mode == OPR_MODE_RD_ONLY)
		conn_cap_data->consumer = 1;
	else
		conn_cap_data->provider = 1;
	
}

static int libtypec_sysfs_get_alternate_modes (int recipient, int conn_num, struct altmode_data *alt_mode_data)
{
    struct stat sb;
	int num_alt_mode = 0;
	char path_str[512], port_content[512];

	sprintf(path_str, SYSFS_TYPEC_PATH "/port%d",conn_num);

 	if (lstat(path_str, &sb) == -1) {
		printf("Incorrect connector number : failed to open, %s",path_str);
		return -1;
    }

	if (recipient == AM_CONNECTOR){

		do {

				sprintf(path_str, SYSFS_TYPEC_PATH "/port%d/port%d.%d",conn_num,conn_num,num_alt_mode);

				if (lstat(path_str, &sb) == -1) 
					break;

				sprintf(port_content,"%s/%s",path_str ,"svid");

				alt_mode_data[num_alt_mode].svid = get_svid_from_path(port_content);

				memset(port_content,0,512);

				sprintf(port_content,"%s/%s",path_str ,"vdo");

				alt_mode_data[num_alt_mode].vdo = get_vdo_from_path(port_content);

				memset(port_content,0,512);
	
				memset(path_str,0,512);

				num_alt_mode++;

							
		}while(1);

	} else if (recipient == AM_SOP){

 		do {

				sprintf(path_str, SYSFS_TYPEC_PATH "/port%d/port%d-partner/port%d-partner.%d",conn_num,conn_num,conn_num,num_alt_mode);

				if (lstat(path_str, &sb) == -1) 
					break;

				sprintf(port_content,"%s/%s",path_str ,"svid");

				alt_mode_data[num_alt_mode].svid = get_svid_from_path(port_content);

				memset(port_content,0,512);

				sprintf(port_content,"%s/%s",path_str ,"vdo");

				alt_mode_data[num_alt_mode].vdo = get_vdo_from_path(port_content);

				memset(port_content,0,512);
	
				memset(path_str,0,512);

				num_alt_mode++;
							
		}while(1);

	} else {

	}

	return num_alt_mode;

}

static int libtypec_sysfs_get_cable_properties_ops (int conn_num,char *cbl_prop_data)
{

}

static int libtypec_sysfs_get_connector_status_ops (int conn_num,struct libtypec_connector_status *conn_sts)
{
	struct stat sb;
	struct dirent *port_entry;
	char path_str[512], port_content[512];

	sprintf(path_str, SYSFS_TYPEC_PATH "/port%d",conn_num);

 	if (lstat(path_str, &sb) == -1) {
		printf("Incorrect connector number : failed to open, %s",path_str);
		return -1;
    	}

	sprintf(path_str, SYSFS_TYPEC_PATH "/port%d/port%d-partner",conn_num,conn_num);

	conn_sts->connect_sts = (lstat(path_str, &sb) == -1) ? 0:1;

	return 0;

}


const struct libtypec_os_backend libtypec_lnx_sysfs_backend = {
    .init = libtypec_sysfs_init,
    .exit = libtypec_sysfs_exit,
    .get_capability_ops = libtypec_sysfs_get_capability_ops,
    .get_conn_capability_ops = libtypec_sysfs_get_conn_capability_ops,
    .get_alternate_modes = libtypec_sysfs_get_alternate_modes,
    .get_cam_supported_ops = NULL,
    .get_current_cam_ops = NULL,
    .get_pdos_ops =  NULL,
    .get_cable_properties_ops = libtypec_sysfs_get_cable_properties_ops,
    .get_connector_status_ops = libtypec_sysfs_get_connector_status_ops,
};
