/*
    Copyright (c) 2021-2022 by Rajaram Regupathy, rajaram.regupathy@gmail.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    (See the full license text in the LICENSES directory)
*/
// SPDX-License-Identifier: GPL-2.0-only
/**
 * @file libtypec.c
 * @author Rajaram Regupathy <rajaram.regupathy@gmail.com>
 * @brief Core functions for libtypec
 */
#include <linux/magic.h>
#include <linux/types.h>
#include <sys/statfs.h>
#include "libtypec_ops.h"
#include <sys/utsname.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h> 
#include <errno.h> 

static int sysfs_method = 0;
static char ver_buf[64];
static struct utsname ker_uname;
static const struct libtypec_os_backend *cur_libtypec_os_backend;

/**
 * \mainpage libtypec 0.1 API Reference
 *
 * \section intro Introduction
 * 
 * Existing USB-Type C and USB Power Delivery solutions, platform
 * designs vary because of USB PD Specification versions, Vendor 
 * specific host interface and division in the protocol stack/policy 
 * management across the software layers.  This necessitates tools and
 * applications that enable efficient port management and allows end 
 * users to root cause/debug system issues.
 * 
 * Existing TypeC class ABI is tied up between 1) Multiple Interface 
 * Protocols 2) Need for OS level PD policy for System level management 
 * against the static PD policy.
 * 
 * Multiple Interface Protocols :  
 * 
 * Adding to this complexity is the multiple specification versions and
 * platform designs there are different interface protocols ( PD Host 
 * Interface, UCSI,  CrOS EC, TCPC) that provide inconsistent information
 * to the TypeC Class ABI or hold additional design specific hooks. This 
 * necessitates an abstraction that provides a transparent interface to 
 * user space software. 
 * 
 * Static Capabilities: 
 * 
 * With policy management defined as implementation specific by USB-C and 
 * USB PD specifications, implementation of these policies are scattered
 * in different platform drivers or firmware modules. This necessitates a 
 * common interface across all designs that allows management of port 
 * policy.
 * 
 * “libtypec” is aimed to provide a generic interface abstracting all 
 * platform complexity for user space to develop tools for efficient USB-C
 * port management and efficient diagnostic and debugging tools to debug of
 * system issues around USB-C/USB PD topology.
 * 
 * The data structures and interface APIs would match closer to USB Type-C®
 * Connector System Software Interface (UCSI) Specification.
 *
*/


char * get_kernel_verion(void)
{
    if (uname(&ker_uname) != 0)
        return 0;
    else
        return ker_uname.release;
}


/**
 * This function initializes libtypec and must be called before
 * calling any other libtypec function.
 * 
 * The function is responsible for setting up the backend interface and 
 * also provides necessary platform session information
 *
 * \param Array of platform session strings 
 *
 * \returns 0 on success
 */
int libtypec_init(char **session_info)
{
    int ret;
    struct statfs sb;  

    sprintf(ver_buf,"libtypec %d.%d",LIBTYPEC_MAJOR_VERSION,LIBTYPEC_MINOR_VERSION);

    session_info[LIBTYPEC_VERSION_INDEX] = ver_buf;
    session_info[LIBTYPEC_KERNEL_INDEX] = get_kernel_verion();
 
    ret = statfs(SYSFS_TYPEC_PATH, &sb);

    if (ret == 0 && sb.f_type == SYSFS_MAGIC) {
        sysfs_method = 1;
        cur_libtypec_os_backend = &libtypec_lnx_sysfs_backend;
        ret = cur_libtypec_os_backend->init(session_info);
    }

    return ret;
}

/**
 * This function must be called before exiting libtypec session to perform
 * cleanup.
 * 
 * \param  
 *
 * \returns 0 on success
 */

int libtypec_exit(void)
{
    /* clear session info */

    cur_libtypec_os_backend->exit();
}

/**
 * This function shall be used to get the platform policy capabilities
 * 
 * \param  cap_data Data structure to hold platform capabilty 
 *
 * \returns 0 on success
 */
int libtypec_get_capability (struct libtypec_capabiliy_data  *cap_data)
{ 
    if(!cur_libtypec_os_backend)
        return -EIO;

    cur_libtypec_os_backend->get_capability_ops(cap_data);
}

/**
 * This function shall be used to get the capabilities of a connector
 * 
 * \param  conn_num Indicates which connector's capability needs to be retrived
 *
 * \param  conn_cap_data Data structure to hold connector capabilty
 * 
 * \returns 0 on success
 */
int libtypec_get_conn_capability (int conn_num, struct libtypec_connector_cap_data *conn_cap_data)
{
    if(!cur_libtypec_os_backend)
        return -EIO;

   cur_libtypec_os_backend->get_conn_capability_ops(conn_num,conn_cap_data);   
}

/**
 * This function shall be used to get the Alternate Modes that the Connector/
 * Cable/Attached Device is capable of supporting. 
 * 
 * \param  recipient Represents alternate mode to be retrieved from local
 * or SOP or SOP' or SOP"
 * 
 * \param  conn_num Indicates which connector's capability needs to be retrived
 * 
 * \returns 0 on success
 */
int libtypec_get_alternate_modes (int recipient, int conn_num, struct altmode_data *alt_mode_data)
{
    if(!cur_libtypec_os_backend)
        return -EIO;

    cur_libtypec_os_backend->get_alternate_modes(recipient,conn_num,alt_mode_data);
    
}
/**
 * This function shall be used to get the Cable Property of a connector 
 * 
 * \param  conn_num Indicates which connector's status needs to be retrived
 * 
 * \returns 0 on success
 */
int libtypec_get_cable_properties (int conn_num,struct libtypec_cable_property *cbl_prop_data)
{
    if(!cur_libtypec_os_backend)
        return -EIO;

    cur_libtypec_os_backend->get_cable_properties_ops(conn_num,cbl_prop_data);
    
}

/**
 * This function shall be used to get the Connector status 
 * 
 * \param  conn_num Indicates which connector's status needs to be retrived
 * 
 * \returns 0 on success
 */
int libtypec_get_connector_status (int conn_num,struct libtypec_connector_status *conn_sts)
{
    if(!cur_libtypec_os_backend)
        return -EIO;

    cur_libtypec_os_backend->get_connector_status_ops(conn_num,conn_sts);
    
}
