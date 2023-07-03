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
 * @file libtypec_ops.h
 * @author Rajaram Regupathy <rajaram.regupathy@gmail.com>
 * @brief Interface "ops" for abstracting backend typec framework
 *
 */

#ifndef LIBTYPEC_OPS_H
#define LIBTYPEC_OPS_H

#include "libtypec.h"

#define SYSFS_TYPEC_PATH "/sys/class/typec"
#define SYSFS_PSY_PATH "/sys/class/power_supply"

/**
 * @brief
 *
 */
extern const struct libtypec_os_backend libtypec_lnx_sysfs_backend;

struct libtypec_os_backend
{
    int (*init)(char **);

    int (*exit)(void);

    int (*get_capability_ops)(struct libtypec_capabiliy_data *cap_data);

    int (*get_conn_capability_ops)(int conn_num, struct libtypec_connector_cap_data *conn_cap_data);

    int (*get_alternate_modes)(int recipient, int conn_num, struct altmode_data *alt_mode_data);

    int (*get_cam_supported_ops)(int conn_num, char *cam_data);

    int (*get_current_cam_ops)(char *cur_cam_data);

    int (*get_pdos_ops)(int conn_num, int partner, int offset, int *num_pdo, int src_snk, int type, unsigned int *pdo_data);

    int (*get_cable_properties_ops)(int conn_num, struct libtypec_cable_property *cbl_prop_data);

    int (*get_connector_status_ops)(int conn_num, struct libtypec_connector_status *conn_sts);

    int (*get_pd_message_ops)(int recipient, int conn_num, int num_bytes, int resp_type, char *pd_msg_resp);

    int (*get_bb_status)(unsigned int *num_bb_instance);

    int (*get_bb_data)(int num_billboards,char* bb_data);
};

#endif /*LIBTYPEC_OPS_H*/
