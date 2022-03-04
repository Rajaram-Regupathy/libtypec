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
 * @file libtypec.h
 * @author Rajaram Regupathy <rajaram.regupathy@gmail.com>
 * @brief 
 *      Public libtypec header file providing interfaces for USB Type-C 
 * Connector System Software.
 * 
 */

#ifndef LIBTYPEC_H
#define LIBTYPEC_H


struct libtypec_capabiliy_data
{
    unsigned int bmAttributes;
    unsigned bNumConnectors:7;
    unsigned reserved:1;
    unsigned bmOptionalFeatures:24;
    unsigned bNumAltModes:8;
    unsigned reserverd :8;
    unsigned bcdBCVersion:16;
    unsigned bcdPDVersion:16;
    unsigned bcdTypeCVersion:16;    
};

struct libtypec_connector_cap_data
{
    unsigned int opr_mode:8;
    unsigned provider:1;
    unsigned consumer:1;
    unsigned swap2dfp:1;
    unsigned swap2ufp:1;
    unsigned swap2src:1;
    unsigned swap2snk:1;
    unsigned reserved:1;
};

struct altmode_data {
	unsigned long svid;
	unsigned long vdo;
};

union libtypec_discovered_identity
{
    char buf_disc_id[24];
    struct discovered_identity {
        unsigned long cert_stat;
        unsigned long id_header;
        unsigned long product;
        unsigned long product_type_vdo1;
        unsigned long product_type_vdo2;
        unsigned long product_type_vdo3;  
    }disc_id; 
};

struct libtypec_connector_status {
	unsigned    sts_change:16;
	unsigned    pwr_op_mode:3;
    unsigned    connect_sts:1;
    unsigned    pwr_dir:1;
    unsigned    ptnr_flags:8;
    unsigned    ptnr_type:3;
    unsigned long rdo;
	unsigned    bat_chrg_cap_sts:2;
    unsigned    cap_ltd_reason:4;
    unsigned    bcdPDVer_op_mode:16;
    unsigned    reserved_1:10;
    unsigned    reserved_2;
};

struct libtypec_cable_property {
	unsigned short  speed_supported;
    unsigned char   current_capability;
    unsigned        vbus_support:1;
    unsigned char   cable_type;
    unsigned        directionality:1;
    unsigned char   plug_end_type;
    unsigned        mode_support:1;
    unsigned        reserved_1:2;
    unsigned        latency:4;
    unsigned        reserved_2:4;
};

#define LIBTYPEC_MAJOR_VERSION 0
#define LIBTYPEC_MINOR_VERSION 1

#define LIBTYPEC_VERSION_INDEX 0
#define LIBTYPEC_KERNEL_INDEX 1
#define LIBTYPEC_CLASS_INDEX 2
#define LIBTYPEC_INTF_INDEX 3
#define LIBTYPEC_SESSION_MAX_INDEX 4

#define OPR_MODE_RP_ONLY 0
#define OPR_MODE_RD_ONLY 1
#define OPR_MODE_DRP_ONLY 2

#define AM_CONNECTOR 0
#define AM_SOP 1
#define AM_SOP_PR 2
#define AM_SOP_DPR 3

#define PLUG_TYPE_A 0
#define PLUG_TYPE_B 1
#define PLUG_TYPE_C 2
#define PLUG_TYPE_OTH 3

#define CABLE_TYPE_PASSIVE 0
#define CABLE_TYPE_ACTIVE 1
#define CABLE_TYPE_UNKNOWN 2

#define GET_SINK_CAP_EXTENDED 0
#define GET_SOURCE_CAP_EXTENDED 1
#define GET_BATTERY_CAP 2
#define GET_BATTERY_STATUS 3
#define DISCOVER_ID_REQ 4


int libtypec_init(char **session_info);
int libtypec_exit(void);

/**
 * @brief 
 * 
 */
int libtypec_get_capability (struct libtypec_capabiliy_data *cap_data);
int libtypec_get_conn_capability (int conn_num, struct libtypec_connector_cap_data *conn_cap_data);
int libtypec_get_alternate_modes (int recipient, int conn_num, struct altmode_data *alt_mode_data);
int libtypec_get_cam_supported (int conn_num, char *cam_data);
int libtypec_get_current_cam (char *cur_cam_data);
int libtypec_get_pdos (int conn_num, int partner, int offset, int num_pdo, int src_snk, int type, char *pdo_data);
int libtypec_get_cable_properties (int conn_num, struct libtypec_cable_property *cbl_prop_data);
int libtypec_get_connector_status (int conn_num, struct libtypec_connector_status *conn_sts);
int libtypec_get_pd_message (int recipient, int conn_num, int num_bytes, int resp_type, char *pd_msg_resp);

#endif /*LIBTYPEC_H*/
