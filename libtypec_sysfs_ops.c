/*
MIT License

Copyright (c) 2022 Rajaram Regupathy <rajaram.regupathy@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

*/
// SPDX-License-Identifier: MIT
/**
 * @file libtypec_sysfs_ops.c
 * @author Rajaram Regupathy <rajaram.regupathy@gmail.com>
 * @brief Functions for libtypec sysfs based operations
 */

#include "libtypec_ops.h"
#include <dirent.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>

#define MAX_PORT_STR 7		/* port%d with 7 bit numPorts */
#define MAX_PORT_MODE_STR 9 /* port%d with 7+2 bit numPorts */
#define OS_TYPE_CHROME 1

static int get_os_type(void)
{
	FILE *fp = fopen("/etc/os-release", "r");
	char buf[128], *p = NULL;

	if (fp)
	{
		while (fgets(buf, 128, fp))
		{
			p = strstr(buf, "chrome");

			if (p)
			{
				fclose(fp);
				return OS_TYPE_CHROME;
			}
		}
		fclose(fp);
	}

	return 0;
}
static unsigned long get_hex_dword_from_path(char *path)
{
	char buf[64];
	unsigned long dword;

	FILE *fp = fopen(path, "r");

	if (fp == NULL)
                return -1;
        else
	{
		if (fgets(buf, 64, fp) == NULL)
                {
		        fclose(fp);
			return -1;
                }
		dword = strtol(buf, NULL, 16);

		fclose(fp);
	}
	return dword;
}

static unsigned long get_dword_from_path(char *path)
{
	char buf[64];
	unsigned long dword=0;

	FILE *fp = fopen(path, "r");

	if (fp)
	{
		if (fgets(buf, 64, fp) == NULL)
                {
		        fclose(fp);
			return -1;
                }
		dword = strtoul(buf, NULL, 10);

		fclose(fp);
	}

	return dword;
}

unsigned char get_opr_mode(char *path)
{
	char buf[64];
	char *pEnd;
	short ret = OPR_MODE_RD_ONLY; /*Rd sink*/

	FILE *fp = fopen(path, "r");
	
	if (fp == NULL)
		return -1;

	if (fgets(buf, 64, fp) == NULL)
        {
		fclose(fp);
		return -1;
        }
	
	pEnd = strstr(buf, "source");

	if (pEnd != NULL)
	{

		pEnd = strstr(buf, "sink");

		if (pEnd != NULL)
			ret = OPR_MODE_DRP_ONLY; /*DRP*/
		else
			ret = OPR_MODE_RP_ONLY; /*Rp only*/
	}

	fclose(fp);

	return ret;
}

static short get_bcd_from_rev_file(char *path)
{
	char buf[10];
	short bcd = 0;

	FILE *fp = fopen(path, "r");

	if (fp)
	{
		if (fgets(buf, 10, fp) == NULL)
                {
		        fclose(fp);
			return -1;
                }
		bcd = ((buf[0] - '0') << 8) | (buf[2] - '0');

		fclose(fp);
	}
	return bcd;
}

static char get_pd_rev(char *path)
{
	char buf[10];
	char rev = 0;

	FILE *fp = fopen(path, "r");

	if (fp)
	{
		if (fgets(buf, 10, fp) == NULL)
                {
		        fclose(fp);
			return -1;
                }
		rev = ((buf[0] - '0') << 4) | (buf[2] - '0');

		fclose(fp);
	}
	return rev;
}

static int get_cable_plug_type(char *path)
{
	char buf[64];
	char *pEnd;
	short ret = PLUG_TYPE_OTH; /*not USB*/

	FILE *fp = fopen(path, "r");

	if (fp == NULL)
		return -1;
		
	if (fgets(buf, 64, fp) == NULL)
        {
		fclose(fp);
		return -1;
        }
	
	pEnd = strstr(buf, "type-c");

	if (pEnd == NULL)
	{

		pEnd = strstr(buf, "type-a");

		if (pEnd == NULL)
		{
			pEnd = strstr(buf, "type-b");

			if (pEnd == NULL)
			{
				ret = PLUG_TYPE_OTH;
			}
			else
			{
				ret = PLUG_TYPE_B;
			}
		}
		else
			ret = PLUG_TYPE_A;
	}
	else
		ret = PLUG_TYPE_C;

	fclose(fp);

	return ret;
}

static int get_cable_type(char *path)
{
	char buf[64];
	char *pEnd;
	short ret = CABLE_TYPE_PASSIVE;

	FILE *fp = fopen(path, "r");

	if (fp == NULL)
		return -1;

	if (fgets(buf, 64, fp) == NULL)
        {
		fclose(fp);
		return -1;
        }
	
	pEnd = strstr(buf, "passive");

	if (pEnd == NULL)
	{
		pEnd = strstr(buf, "active");

		if (pEnd == NULL)
			ret = CABLE_TYPE_UNKNOWN;
		else
			ret = CABLE_TYPE_ACTIVE;
	}

	fclose(fp);

	return ret;
}

static int get_cable_mode_support(char *path)
{
	char buf[64];
	short ret;

	FILE *fp = fopen(path, "r");

	if (fp == NULL)
		return -1;

	if (fgets(buf, 64, fp) == NULL)
        {
		fclose(fp);
		return -1;
        }
	
	ret = (buf[0] - '0') ? 1 : 0;

	fclose(fp);

	return ret;
}
static unsigned int get_variable_supply_pdo(char *path, int src_snk)
{
	char path_str[512], port_content[1024];
	union libtypec_variable_supply_src var_src;
	unsigned int tmp;

	var_src.obj_var_sply.type = 1;
	
	snprintf(port_content, sizeof(port_content), "%s/%s", path, "maximum_voltage");
	tmp = get_dword_from_path(port_content);
	var_src.obj_var_sply.max_volt = tmp/50;

	snprintf(port_content, sizeof(port_content), "%s/%s", path, "minimum_voltage");
	tmp = get_dword_from_path(port_content);
	var_src.obj_var_sply.min_volt = tmp/50;

	if(src_snk)
	{
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "maximum_current");
		tmp = get_dword_from_path(port_content);
		var_src.obj_var_sply.max_cur = tmp/10;
	}
	else
	{
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "operational_current");
		tmp = get_dword_from_path(port_content);
		var_src.obj_var_sply.max_cur = tmp/10;		
	}
	return var_src.variable_supply;

}
static unsigned int get_battery_supply_pdo(char *path, int src_snk)
{
	char path_str[512], port_content[512 + 512];
	union libtypec_battery_supply_src bat_src;
	unsigned int tmp;

	bat_src.obj_bat_sply.type = 2;

	snprintf(port_content, sizeof(port_content), "%s/%s", path, "maximum_voltage");
	tmp = get_dword_from_path(port_content);
	bat_src.obj_bat_sply.max_volt = tmp/50;

	snprintf(port_content, sizeof(port_content), "%s/%s", path, "minimum_voltage");
	tmp = get_dword_from_path(port_content);
	bat_src.obj_bat_sply.min_volt = tmp/50;
	
	if(src_snk)
	{
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "maximum_power");
		tmp = get_dword_from_path(port_content);
		bat_src.obj_bat_sply.max_pwr = tmp/250;
	}
	else
	{
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "operational_power");
		tmp = get_dword_from_path(port_content);
		bat_src.obj_bat_sply.max_pwr = tmp/250;


	}
	return bat_src.battery_supply;

}
static unsigned int get_programmable_supply_pdo(char *path, int src_snk)
{
	char path_str[512], port_content[512 + 512];
	union libtypec_pps_src pps_src={0};
	unsigned int tmp;

	pps_src.obj_pps_sply.type = 3;
	if(src_snk)
	{
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "pps_power_limited");
		pps_src.obj_pps_sply.pwr_ltd = get_dword_from_path(port_content);
	}

	snprintf(port_content, sizeof(port_content), "%s/%s", path, "maximum_voltage");
	tmp = get_dword_from_path(port_content);
	pps_src.obj_pps_sply.max_volt = tmp/100;

	snprintf(port_content, sizeof(port_content), "%s/%s", path, "minimum_voltage");
	tmp = get_dword_from_path(port_content);
	pps_src.obj_pps_sply.min_volt = tmp/100;

	snprintf(port_content, sizeof(port_content), "%s/%s", path, "maximum_current");
	tmp = get_dword_from_path(port_content);
	pps_src.obj_pps_sply.max_cur = tmp/50;


	return pps_src.spr_pps_supply;

}
static unsigned int get_fixed_supply_pdo(char *path, int src_snk)
{
	char path_str[512], port_content[512 + 512];
	union libtypec_fixed_supply_src fxd_src;
	union libtypec_fixed_supply_snk fxd_snk;
	
	unsigned int tmp;

	if(src_snk)
	{
		fxd_src.obj_fixed_sply.type = 0;
		
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "dual_role_power");
		fxd_src.obj_fixed_sply.dual_pwr = get_dword_from_path(port_content);
		
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "usb_suspend_supported");
		fxd_src.obj_fixed_sply.usb_suspend = get_dword_from_path(port_content);
		
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "unconstrained_power");
		fxd_src.obj_fixed_sply.uncons_pwr = get_dword_from_path(path);
		
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "usb_communication_capable");
		fxd_src.obj_fixed_sply.usb_comm = get_dword_from_path(port_content);
		
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "dual_role_data");
		fxd_src.obj_fixed_sply.drd = get_dword_from_path(port_content);
		
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "unchunked_extended_messages_supported");
		fxd_src.obj_fixed_sply.unchunked = get_dword_from_path(port_content);
		
		fxd_src.obj_fixed_sply.epr = 0;
		
		fxd_src.obj_fixed_sply.peak_cur = 0;
		
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "voltage");
		tmp = get_dword_from_path(port_content);
		fxd_src.obj_fixed_sply.volt = tmp/50;
		
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "maximum_current");
		tmp = get_dword_from_path(port_content);
		fxd_src.obj_fixed_sply.max_cur = tmp/10;

		return fxd_src.fixed_supply;
	}
	else
	{
		fxd_snk.obj_fixed_supply.type = 0;
		
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "dual_role_power");
		fxd_snk.obj_fixed_supply.drp = get_dword_from_path(port_content);
		
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "unconstrained_power");
		fxd_snk.obj_fixed_supply.uncons_pwr = get_dword_from_path(path);
		
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "usb_communication_capable");
		fxd_snk.obj_fixed_supply.usb_comm_cap = get_dword_from_path(port_content);
		
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "dual_role_data");
		fxd_snk.obj_fixed_supply.drd = get_dword_from_path(port_content);
		
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "fast_role_swap_current");
		fxd_snk.obj_fixed_supply.fr_swp = get_dword_from_path(port_content);
		
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "voltage");
		tmp = get_dword_from_path(port_content);
		fxd_snk.obj_fixed_supply.volt = tmp/50;
		
		snprintf(port_content, sizeof(port_content), "%s/%s", path, "operational_current");
		tmp = get_dword_from_path(port_content);
		fxd_snk.obj_fixed_supply.opr_cur = tmp/10;

		return fxd_snk.fixed_supply;

	}

}
static int libtypec_sysfs_init(char **session_info)
{

	return 0;
}

static int libtypec_sysfs_exit(void)
{
	return 0;
}

static int libtypec_sysfs_get_capability_ops(struct libtypec_capabiliy_data *cap_data)
{
	DIR *typec_path = opendir(SYSFS_TYPEC_PATH), *port_path;
	struct dirent *typec_entry, *port_entry;
	int num_ports = 0, num_alt_mode = 0;
	char path_str[512], port_content[512 + 64];

	if (!typec_path)
	{
		printf("opendir typec class failed, %s", SYSFS_TYPEC_PATH);
		return -1;
	}

	while ((typec_entry = readdir(typec_path)))
	{

		if (!(strncmp(typec_entry->d_name, "port", 4)) && (strlen(typec_entry->d_name) <= MAX_PORT_STR))
		{
			num_ports++;

			snprintf(path_str, sizeof(path_str), SYSFS_TYPEC_PATH "/%s", typec_entry->d_name);

			/*Scan the port capability*/
			port_path = opendir(path_str);

			while ((port_entry = readdir(port_path)))
			{

				if (!(strncmp(port_entry->d_name, "port", 4)) && (strlen(port_entry->d_name) <= MAX_PORT_MODE_STR))
				{
					num_alt_mode++;
				}
			}

			snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "usb_power_delivery_revision");

			cap_data->bcdPDVersion = get_bcd_from_rev_file(port_content);

			memset(port_content, 0, sizeof(port_content));

			snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "usb_typec_revision");

			cap_data->bcdTypeCVersion = get_bcd_from_rev_file(port_content);

			closedir(port_path);
		}
	}

	cap_data->bNumConnectors = num_ports;
	cap_data->bNumAltModes = num_alt_mode;

	closedir(typec_path);
	return 0;
}

static int libtypec_sysfs_get_conn_capability_ops(int conn_num, struct libtypec_connector_cap_data *conn_cap_data)
{
	struct stat sb;
	struct dirent *port_entry;
	char path_str[512], port_content[512 + 64];

	snprintf(path_str, sizeof(path_str), SYSFS_TYPEC_PATH "/port%d", conn_num);

	if (lstat(path_str, &sb) == -1)
	{
		printf("Incorrect connector number : failed to open, %s", path_str);
		return -1;
	}

	snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "power_role");

	conn_cap_data->opr_mode = get_opr_mode(port_content);

	if (conn_cap_data->opr_mode == OPR_MODE_DRP_ONLY)
	{
		conn_cap_data->provider = 1;
		conn_cap_data->consumer = 1;
	}
	else if (conn_cap_data->opr_mode == OPR_MODE_RD_ONLY)
		conn_cap_data->consumer = 1;
	else
		conn_cap_data->provider = 1;

	if (get_os_type() == OS_TYPE_CHROME)
	{
		snprintf(port_content, sizeof(port_content), "%s/port%d-partner/%s", path_str, conn_num, "usb_power_delivery_revision");

		conn_cap_data->partner_rev = get_pd_rev(port_content);

		snprintf(port_content, sizeof(port_content), "%s/port%d-cable/%s", path_str, conn_num, "usb_power_delivery_revision");

		conn_cap_data->cable_rev = get_pd_rev(port_content);
	}
	return 0;
}

static int libtypec_sysfs_get_alternate_modes(int recipient, int conn_num, struct altmode_data *alt_mode_data)
{
	struct stat sb;
	int num_alt_mode = 0;
	char path_str[512], port_content[512 + 64];

	snprintf(path_str, sizeof(path_str), SYSFS_TYPEC_PATH "/port%d", conn_num);

	if (lstat(path_str, &sb) == -1)
	{
		printf("Incorrect connector number : failed to open, %s", path_str);
		return -1;
	}

	if (recipient == AM_CONNECTOR)
	{

		do
		{

			snprintf(path_str, sizeof(path_str), SYSFS_TYPEC_PATH "/port%d/port%d.%d", conn_num, conn_num, num_alt_mode);

			if (lstat(path_str, &sb) == -1)
				break;

			snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "svid");

			alt_mode_data[num_alt_mode].svid = get_hex_dword_from_path(port_content);

			memset(port_content, 0, sizeof(port_content));

			snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "vdo");

			alt_mode_data[num_alt_mode].vdo = get_hex_dword_from_path(port_content);

			memset(port_content, 0, sizeof(port_content));

			memset(path_str, 0, sizeof(path_str));

			num_alt_mode++;

		} while (1);
	}
	else if (recipient == AM_SOP)
	{

		do
		{

			snprintf(path_str, sizeof(path_str), SYSFS_TYPEC_PATH "/port%d/port%d-partner/port%d-partner.%d", conn_num, conn_num, conn_num, num_alt_mode);

			if (lstat(path_str, &sb) == -1)
				break;

			snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "svid");

			alt_mode_data[num_alt_mode].svid = get_hex_dword_from_path(port_content);

			memset(port_content, 0, sizeof(port_content));

			snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "vdo");

			alt_mode_data[num_alt_mode].vdo = get_hex_dword_from_path(port_content);

			memset(port_content, 0, sizeof(port_content));

			memset(path_str, 0, sizeof(path_str));

			num_alt_mode++;

		} while (1);
	}
	else if (recipient == AM_SOP_PR)
	{

		do
		{

			snprintf(path_str, sizeof(path_str), SYSFS_TYPEC_PATH "/port%d-cable/port%d-plug0/port%d-plug0.%d", conn_num, conn_num, conn_num, num_alt_mode);

			if (lstat(path_str, &sb) == -1)
				break;

			snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "svid");

			alt_mode_data[num_alt_mode].svid = get_hex_dword_from_path(port_content);

			memset(port_content, 0, sizeof(port_content));

			snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "vdo");

			alt_mode_data[num_alt_mode].vdo = get_hex_dword_from_path(port_content);

			memset(port_content, 0, sizeof(port_content));

			memset(path_str, 0, sizeof(path_str));

			num_alt_mode++;

		} while (1);
	}
	else
	{
	}

	return num_alt_mode;
}

static int libtypec_sysfs_get_cable_properties_ops(int conn_num, struct libtypec_cable_property *cbl_prop_data)
{
	struct stat sb;
	struct dirent *port_entry;
	char path_str[512], port_content[512 + 64];

	snprintf(path_str, sizeof(path_str), SYSFS_TYPEC_PATH "/port%d-cable", conn_num);

	/* No cable identified or connector number is incorrect */
	if (lstat(path_str, &sb) == -1)
		return -1;

	snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "plug_type");

	cbl_prop_data->plug_end_type = get_cable_plug_type(port_content);

	memset(port_content, 0, sizeof(port_content));

	snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "type");

	cbl_prop_data->cable_type = get_cable_type(port_content);

	memset(port_content, 0, sizeof(port_content));

	snprintf(port_content, sizeof(port_content), SYSFS_TYPEC_PATH "/port%d-plug0/%s", conn_num, "number_of_alternate_modes");

	cbl_prop_data->mode_support = get_cable_mode_support(port_content);

	return 0;
}

static int libtypec_sysfs_get_connector_status_ops(int conn_num, struct libtypec_connector_status *conn_sts)
{
	struct stat sb;
	struct dirent *port_entry;
	char path_str[512], port_content[512 + 64];
	int ret;

	snprintf(path_str, sizeof(path_str), SYSFS_TYPEC_PATH "/port%d", conn_num);

	if (lstat(path_str, &sb) == -1)
	{
		printf("Incorrect connector number : failed to open, %s\n", path_str);
		return -1;
	}

	snprintf(path_str, sizeof(path_str), SYSFS_TYPEC_PATH "/port%d/port%d-partner", conn_num, conn_num);

	conn_sts->connect_sts = (lstat(path_str, &sb) == -1) ? 0 : 1;

	snprintf(path_str, sizeof(path_str), SYSFS_PSY_PATH "/ucsi-source-psy-USBC000:00%d", conn_num + 1);

	if (lstat(path_str, &sb) == -1)
	{
		printf("Non UCSI based Type-C connector Class - PSY not supported\n:%s\n", path_str);
		return 0;
	}
	else
	{
		snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "online");

		ret = get_hex_dword_from_path(port_content);

		if (ret)
		{
			unsigned long cur, volt, op_mw, max_mw;

			snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "current_now");

			cur = get_dword_from_path(port_content) / 1000;

			snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "voltage_now");

			volt = get_dword_from_path(port_content) / 1000;

			op_mw = (cur * volt) / (250 * 1000);

			snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "current_max");

			cur = get_dword_from_path(port_content) / 1000;

			snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "voltage_max");

			volt = get_dword_from_path(port_content) / 1000;

			max_mw = (cur * volt) / (250 * 1000);

			conn_sts->rdo = ((op_mw << 10)) | (max_mw)&0x3FF;
		}
	}
	return 0;
}

static int libtypec_sysfs_get_discovered_identity_ops(int recipient, int conn_num, char *pd_resp_data)
{
	struct stat sb;
	char path_str[512], port_content[512 + 64];
	union libtypec_discovered_identity *id = (void *)pd_resp_data;

	snprintf(path_str, sizeof(path_str), SYSFS_TYPEC_PATH "/port%d", conn_num);

	if (lstat(path_str, &sb) == -1)
	{
		printf("Incorrect connector number : failed to open, %s", path_str);
		return -1;
	}

	if (recipient == AM_SOP)
	{

		snprintf(path_str, sizeof(path_str), SYSFS_TYPEC_PATH "/port%d-partner/identity", conn_num);

		if (lstat(path_str, &sb) == -1)
			return -1;

		snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "cert_stat");

		id->disc_id.cert_stat = get_hex_dword_from_path(port_content);

		memset(port_content, 0, sizeof(port_content));

		snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "id_header");

		id->disc_id.id_header = get_hex_dword_from_path(port_content);

		memset(port_content, 0, sizeof(port_content));

		snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "product");

		id->disc_id.product = get_hex_dword_from_path(port_content);

		memset(port_content, 0, sizeof(port_content));

		snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "product_type_vdo1");

		id->disc_id.product_type_vdo1 = get_hex_dword_from_path(port_content);

		memset(port_content, 0, sizeof(port_content));

		snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "product_type_vdo2");

		id->disc_id.product_type_vdo2 = get_hex_dword_from_path(port_content);

		memset(port_content, 0, sizeof(port_content));

		snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "product_type_vdo3");

		id->disc_id.product_type_vdo3 = get_hex_dword_from_path(port_content);
	}
	else if (recipient == AM_SOP_PR)
	{

		snprintf(path_str, sizeof(path_str), SYSFS_TYPEC_PATH "/port%d-cable/identity", conn_num);

		if (lstat(path_str, &sb) == -1)
			return -1;

		snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "cert_stat");

		id->disc_id.cert_stat = get_hex_dword_from_path(port_content);

		memset(port_content, 0, sizeof(port_content));

		snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "id_header");

		id->disc_id.id_header = get_hex_dword_from_path(port_content);

		memset(port_content, 0, sizeof(port_content));

		snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "product");

		id->disc_id.product = get_hex_dword_from_path(port_content);

		memset(port_content, 0, sizeof(port_content));

		snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "product_type_vdo1");

		id->disc_id.product_type_vdo1 = get_hex_dword_from_path(port_content);

		memset(port_content, 0, sizeof(port_content));

		snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "product_type_vdo2");

		id->disc_id.product_type_vdo2 = get_hex_dword_from_path(port_content);

		memset(port_content, 0, sizeof(port_content));

		snprintf(port_content, sizeof(port_content), "%s/%s", path_str, "product_type_vdo3");

		id->disc_id.product_type_vdo3 = get_hex_dword_from_path(port_content);
	}
	return 0;
}

static int libtypec_sysfs_get_pd_message_ops(int recipient, int conn_num, int num_bytes, int resp_type, char *pd_msg_resp)
{
	if (resp_type == DISCOVER_ID_REQ)
	{
		return libtypec_sysfs_get_discovered_identity_ops(recipient, conn_num, pd_msg_resp);
	}

	return 0;
}

static int libtypec_sysfs_get_pdos_ops(int conn_num, int partner, int offset, int *num_pdo, int src_snk, int type, unsigned int *pdo_data)
{
	struct stat sb;
	int num_pdos_read = 0;
	char path_str[512], port_content[512 + 256];
	DIR *typec_path , *port_path;
	struct dirent *typec_entry, *port_entry;

	snprintf(path_str, sizeof(path_str), SYSFS_TYPEC_PATH "/port%d", conn_num);

	if (lstat(path_str, &sb) == -1)
	{
		printf("Incorrect connector number : failed to open, %s", path_str);
		return -1;
	}

	if (partner == 0)
	{
		if(src_snk)
			snprintf(path_str, sizeof(path_str), SYSFS_TYPEC_PATH "/port%d/usb_power_delivery/source-capabilities", conn_num);
		else
			snprintf(path_str, sizeof(path_str), SYSFS_TYPEC_PATH "/port%d/usb_power_delivery/sink-capabilities", conn_num);
	}
	else
	{
		if(src_snk)
			snprintf(path_str, sizeof(path_str), SYSFS_TYPEC_PATH "/port%d-partner/usb_power_delivery/source-capabilities", conn_num);
		else
			snprintf(path_str, sizeof(path_str), SYSFS_TYPEC_PATH "/port%d-partner/usb_power_delivery/sink-capabilities", conn_num);		
	}


	typec_path = opendir(path_str);
        
        	if (typec_path == NULL)
                		return -1;

	while ((typec_entry = readdir(typec_path)))
	{
		memset(port_content, 0, sizeof(port_content));
		snprintf(port_content, sizeof(port_content), "%s/%s", path_str,typec_entry->d_name);

		if(strstr(typec_entry->d_name, "fixed"))
		{
			pdo_data[num_pdos_read++] = get_fixed_supply_pdo(port_content,src_snk);

		}
		else if(strstr(typec_entry->d_name, "variable"))
		{
			pdo_data[num_pdos_read++] = get_variable_supply_pdo(port_content,src_snk);

		}
		else if(strstr(typec_entry->d_name, "battery"))
		{
			pdo_data[num_pdos_read++] = get_battery_supply_pdo(port_content,src_snk);

		}
		else if(strstr(typec_entry->d_name, "programmable"))
		{
			pdo_data[num_pdos_read++] = get_programmable_supply_pdo(port_content,src_snk);

		}
		
		
	}

        closedir(typec_path);
	
	*num_pdo = num_pdos_read;

	return num_pdos_read;

}

const struct libtypec_os_backend libtypec_lnx_sysfs_backend = {
	.init = libtypec_sysfs_init,
	.exit = libtypec_sysfs_exit,
	.get_capability_ops = libtypec_sysfs_get_capability_ops,
	.get_conn_capability_ops = libtypec_sysfs_get_conn_capability_ops,
	.get_alternate_modes = libtypec_sysfs_get_alternate_modes,
	.get_cam_supported_ops = NULL,
	.get_current_cam_ops = NULL,
	.get_pdos_ops = libtypec_sysfs_get_pdos_ops,
	.get_cable_properties_ops = libtypec_sysfs_get_cable_properties_ops,
	.get_connector_status_ops = libtypec_sysfs_get_connector_status_ops,
	.get_pd_message_ops = libtypec_sysfs_get_pd_message_ops,
};
