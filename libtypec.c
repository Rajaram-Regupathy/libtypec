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
 * @file libtypec.c
 * @author Rajaram Regupathy <rajaram.regupathy@gmail.com>
 * @brief Core functions for libtypec
 */
#include <linux/magic.h>
#include <linux/types.h>
#include <sys/statfs.h>
#include "libtypec.h"
#include "libtypec_ops.h"
#include <sys/utsname.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>

static int ops_method = -1;
static char ver_buf[64];
static struct utsname ker_uname;
static const struct libtypec_os_backend *cur_libtypec_os_backend;

#define OPS_METHOD_DBGFS 0
#define OPS_METHOD_SYSFS 1

/**
 * \mainpage libtypec 0.3.1 API Reference
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

char *get_kernel_verion(void)
{
    if (uname(&ker_uname) != 0)
        return 0;
    else
        return ker_uname.release;
}

char *get_os_name(void)
{
    FILE *fp = fopen("/etc/os-release", "r");
    static char buf[128];
    char *p = NULL;

    if (fp)
    {
	while (fgets(buf, sizeof(buf), fp))
        {
            char *ptr;

            /* Ensure buffer always has eos marker at end */
            buf[sizeof(buf) - 1] = '\0';
            /* Remove \n */
            for (ptr = buf; *ptr && *ptr != '\n'; ptr++)
                    ;
            *ptr = '\0';

            if (strncmp(buf, "ID=", 3) == 0) {
                p = buf + 3;
                break;
            }
        }
        fclose(fp);
    }

    return p;
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
    char *ops_str[] = {"debugfs","sysfs"};

    sprintf(ver_buf, "libtypec %d.%d", LIBTYPEC_MAJOR_VERSION, LIBTYPEC_MINOR_VERSION);

    session_info[LIBTYPEC_VERSION_INDEX] = ver_buf;
    session_info[LIBTYPEC_KERNEL_INDEX] = get_kernel_verion();
    session_info[LIBTYPEC_OS_INDEX] = get_os_name();

    /**
        debugfs provides direct access to UCSI command and response.
        Try opening debugfs before falling back to sysfs
    */
    ret = statfs(UCSI_DEBUGFS_PATH, &sb);

    if (ret == 0 && sb.f_type == DEBUGFS_MAGIC)
    {
            ops_method = OPS_METHOD_DBGFS;
            cur_libtypec_os_backend = &libtypec_lnx_dbgfs_backend;
	    if (!cur_libtypec_os_backend || !cur_libtypec_os_backend->init );
	    else
                ret = cur_libtypec_os_backend->init(session_info);
    }
    else
    {
        ret = statfs(SYSFS_TYPEC_PATH, &sb);

        if (ret == 0 && sb.f_type == SYSFS_MAGIC)
        {
            ops_method = OPS_METHOD_SYSFS;
            cur_libtypec_os_backend = &libtypec_lnx_sysfs_backend;
            if (!cur_libtypec_os_backend || !cur_libtypec_os_backend->init );
            else
            	ret = cur_libtypec_os_backend->init(session_info);
        }    
    }

    session_info[LIBTYPEC_OPS_INDEX] = ops_str[ops_method];

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
    if (!cur_libtypec_os_backend || !cur_libtypec_os_backend->exit )
        return -EIO;

    /* clear session info */

    return cur_libtypec_os_backend->exit();
}

/**
 * This function shall be used to get the platform policy capabilities
 *
 * \param  cap_data Data structure to hold platform capability
 *
 * \returns 0 on success
 */
int libtypec_get_capability(struct libtypec_capability_data *cap_data)
{
    if (!cur_libtypec_os_backend || !cur_libtypec_os_backend->get_capability_ops )
        return -EIO;

    return cur_libtypec_os_backend->get_capability_ops(cap_data);
}

/**
 * This function shall be used to get the capabilities of a connector
 *
 * \param  conn_num Indicates which connector's capability needs to be retrieved
 *
 * \param  conn_cap_data Data structure to hold connector capability
 *
 * \returns 0 on success
 */
int libtypec_get_conn_capability(int conn_num, struct libtypec_connector_cap_data *conn_cap_data)
{
    if (!cur_libtypec_os_backend || !cur_libtypec_os_backend->get_conn_capability_ops )
        return -EIO;

    return cur_libtypec_os_backend->get_conn_capability_ops(conn_num, conn_cap_data);
}

/**
 * This function shall be used to get the Alternate Modes that the Connector/
 * Cable/Attached Device is capable of supporting.
 *
 * \param  recipient Represents alternate mode to be retrieved from local
 * or SOP or SOP' or SOP"
 *
 * \param  conn_num Indicates which connector's capability needs to be retrivied
 *
 * \returns number of alternate modes on success
 */
int libtypec_get_alternate_modes(int recipient, int conn_num, struct altmode_data *alt_mode_data)
{
    if (!cur_libtypec_os_backend || !cur_libtypec_os_backend->get_alternate_modes )
        return -EIO;

    return cur_libtypec_os_backend->get_alternate_modes(recipient, conn_num, alt_mode_data);
}
/**
 * This function shall be used to get the Cable Property of a connector
 *
 * \param  conn_num Indicates which connector's status needs to be retrieved
 *
 * \returns 0 on success
 */
int libtypec_get_cable_properties(int conn_num, struct libtypec_cable_property *cbl_prop_data)
{
    if (!cur_libtypec_os_backend || !cur_libtypec_os_backend->get_cable_properties_ops )
        return -EIO;

    return cur_libtypec_os_backend->get_cable_properties_ops(conn_num, cbl_prop_data);
}

/**
 * This function shall be used to get the Connector status
 *
 * \param  conn_num Indicates which connector's status needs to be retrieved
 *
 * \returns 0 on success
 */
int libtypec_get_connector_status(int conn_num, struct libtypec_connector_status *conn_sts)
{
    if (!cur_libtypec_os_backend || !cur_libtypec_os_backend->get_connector_status_ops )
        return -EIO;

    return cur_libtypec_os_backend->get_connector_status_ops(conn_num, conn_sts);
}

/**
 * This function shall be used to get the USB PD response messages from
 *
 * \param  recipient Represents PD response message to be retrieved from local
 * or SOP or SOP' or SOP"
 *
 * \param  conn_num Indicates which connector's PD message needs to be retrieved
 *
 * \param pd_msg_resp
 * \returns 0 on success
 */

int libtypec_get_pd_message(int recipient, int conn_num, int num_bytes, int resp_type, char *pd_msg_resp)
{
    if (!cur_libtypec_os_backend || !cur_libtypec_os_backend->get_pd_message_ops )
        return -EIO;

    return cur_libtypec_os_backend->get_pd_message_ops(recipient, conn_num, num_bytes, resp_type, pd_msg_resp);
}

/**
 * This function shall be used to get PDOs from local and partner Policy Managers
 *
 * \param  conn_num Represents connector to be queried
 *
 * \param  partner Set to TRUE to retrieve partner PDOs
 *
 * \param  offset Index from which PDO needs to be retrieved 
 *
 * \param  num_pdo Represents number of PDOs to be retrieved
 *
 * \param  src_snk Set to TRUE to retrieve Source PDOs
 *
 * \param  type Represents type of Source PDOs requested
 *
 * \param  pdo_data Holds PDO data retrieved 
 * 
 * \returns PDO retrieved on success
 */
int libtypec_get_pdos (int conn_num, int partner, int offset, int *num_pdo, int src_snk, int type, unsigned int *pdo_data)
{
    if (!cur_libtypec_os_backend || !cur_libtypec_os_backend->get_pdos_ops )
        return -EIO;

    return cur_libtypec_os_backend->get_pdos_ops(conn_num,  partner, offset,  num_pdo,  src_snk, type, pdo_data);

}
/**
 * This function shall be used to retrive number of billboard interfaces in the system
 *
 * \param  num_bb_instance Reference passed to retrive number of billboard interfaces. If 0 then 
 * no billboard interface and >0 indicates number of BB devices enumerated in the system.
 *
 * \returns 0 on success
 */
int libtypec_get_bb_status(unsigned int *num_bb_instance)
{

    if (!cur_libtypec_os_backend || !cur_libtypec_os_backend->get_bb_status )
        return -EIO;

    return cur_libtypec_os_backend->get_bb_status(num_bb_instance);

}

/**
 * This function shall be used to retrive Billboard Capability Descriptor of BB device instances
 * in the system. When multiple BB devices are in the system instance shall indicate the instance
 * index for data to be retrived
 *
 * \param  bb_instance index of the BB instance
 * \param  bb_data BB capability descriptor of the device instance
 *
 * \returns 0 on success
 */
int libtypec_get_bb_data(int bb_instance,char* bb_data)
{

    if (!cur_libtypec_os_backend || !cur_libtypec_os_backend->get_bb_data )
        return -EIO;

    return cur_libtypec_os_backend->get_bb_data(bb_instance,bb_data);

}
