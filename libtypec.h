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

#include <stdint.h>

struct libtypec_capabiliy_data
{
    unsigned int bmAttributes;
    unsigned bNumConnectors : 7;
    unsigned reserved1 : 1;
    unsigned bmOptionalFeatures : 24;
    unsigned bNumAltModes : 8;
    unsigned reserved2 : 8;
    unsigned bcdBCVersion : 16;
    unsigned bcdPDVersion : 16;
    unsigned bcdTypeCVersion : 16;
};

struct libtypec_connector_cap_data
{
    unsigned int opr_mode : 8;
    unsigned provider : 1;
    unsigned consumer : 1;
    unsigned swap2dfp : 1;
    unsigned swap2ufp : 1;
    unsigned swap2src : 1;
    unsigned swap2snk : 1;
    unsigned reserved : 2;
    unsigned reservedmsb : 12;
    unsigned partner_rev : 8;
    unsigned cable_rev : 8;
};

struct altmode_data
{
    uint32_t svid;
    uint32_t vdo;
};

union libtypec_discovered_identity
{
    char buf_disc_id[24];
    struct discovered_identity
    {
        uint32_t cert_stat;
        uint32_t id_header;
        uint32_t product;
        uint32_t product_type_vdo1;
        uint32_t product_type_vdo2;
        uint32_t product_type_vdo3;
    } disc_id;
};

struct libtypec_connector_status
{
    unsigned sts_change : 16;
    unsigned pwr_op_mode : 3;
    unsigned connect_sts : 1;
    unsigned pwr_dir : 1;
    unsigned ptnr_flags : 8;
    unsigned ptnr_type : 3;
    unsigned long rdo;
    unsigned bat_chrg_cap_sts : 2;
    unsigned cap_ltd_reason : 4;
    unsigned bcdPDVer_op_mode : 16;
    unsigned reserved_1 : 10;
    unsigned reserved_2;
};

struct libtypec_cable_property
{
    unsigned short speed_supported;
    unsigned char current_capability;
    unsigned vbus_support : 1;
    unsigned char cable_type;
    unsigned directionality : 1;
    unsigned char plug_end_type;
    unsigned mode_support : 1;
    unsigned reserved_1 : 2;
    unsigned latency : 4;
    unsigned reserved_2 : 4;
};

union libtypec_fixed_supply_src
{
    unsigned fixed_supply;
    struct fixed_supply_bits
    {
        unsigned max_cur:10;
        unsigned volt:10;
        unsigned peak_cur:2;
        unsigned rsvd:1;
        unsigned epr:1;
        unsigned unchunked:1;
        unsigned drd:1;
        unsigned usb_comm:1;
        unsigned uncons_pwr:1;
        unsigned usb_suspend:1;
        unsigned dual_pwr:1;
        unsigned type:2;
    }obj_fixed_sply;
    
};

union libtypec_variable_supply_src
{
    unsigned int variable_supply;
    struct variable_supply_bits
    {
        unsigned max_cur:10;
        unsigned min_volt:10;
        unsigned max_volt:10;
        unsigned type:2;
        
    }obj_var_sply;
    
};

union libtypec_battery_supply_src
{
    unsigned int battery_supply;
    struct battery_supply_bits
    {
        unsigned max_pwr:10;
        unsigned min_volt:10;
        unsigned max_volt:10;
        unsigned type:2;
    }obj_bat_sply;
    
};

union libtypec_pps_src
{
    unsigned int spr_pps_supply;
    struct pps_supply_bits
    {
        unsigned max_cur:7;
        unsigned rsvd1:1;
        unsigned min_volt:8;
        unsigned rsvd2:1;
        unsigned max_volt:8;
        unsigned rsvd3:2;
        unsigned pwr_ltd:1;
        unsigned pps_type:2;
        unsigned type:2;
    }obj_pps_sply;
    
};

union libtypec_fixed_supply_snk
{
    unsigned int fixed_supply;
    struct fixed_sply_bits
    {
        unsigned opr_cur:10;
        unsigned volt:10;
        unsigned rsvd:3;
        unsigned fr_swp:2;
        unsigned drd:1;
        unsigned usb_comm_cap:1;
        unsigned uncons_pwr:1;
        unsigned higher_caps:1;
        unsigned drp:1;
        unsigned type:2;
    }obj_fixed_supply;
    
};

union libtypec_variable_sply_sink
{
    unsigned int var_sply_snk;
    struct var_supply_bits
    {
        unsigned opr_cur:10;
        unsigned min_volt:10;
        unsigned max_volt:10;
        unsigned type:2;
    }obj_var_sply;
    
};

union libtypec_battery_sply_sink
{
    unsigned int battery_supply;
    struct battery_sply_bits
    {
        unsigned opr_pwr:10;
        unsigned min_volt:10;
        unsigned max_volt:10;
        unsigned type:2;
    }obj_bat_sply;
    
};

union libtypec_pps_sink
{
    unsigned int spr_pps;
    struct spr_pps_bits
    {
        unsigned max_cur:7;
        unsigned rsvd1:1;
        unsigned min_volt:8;
        unsigned rsvd2:1;
        unsigned max_volt:8;
        unsigned rsvd3:3;
        unsigned pps_type:2;
        unsigned type:2;
    }obj_spr_pps;
    
};


#define LIBTYPEC_MAJOR_VERSION 0
#define LIBTYPEC_MINOR_VERSION 3

#define LIBTYPEC_VERSION_INDEX 0
#define LIBTYPEC_KERNEL_INDEX 1
#define LIBTYPEC_OS_INDEX 2
#define LIBTYPEC_INTF_INDEX 3
#define LIBTYPEC_OPS_INDEX 4
#define LIBTYPEC_SESSION_MAX_INDEX 5

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

#define POWER_OP_MODE_PD 3
#define POWER_OP_MODE_TC_1_5 4
#define POWER_OP_MODE_TC_3 5

#define PDO_FIXED 0
#define PDO_BATTERY 1
#define PDO_VARIABLE 2
#define PDO_AUGMENTED 3

int libtypec_init(char **session_info);
int libtypec_exit(void);

/**
 * @brief
 *
 */
int libtypec_get_capability(struct libtypec_capabiliy_data *cap_data);
int libtypec_get_conn_capability(int conn_num, struct libtypec_connector_cap_data *conn_cap_data);
int libtypec_get_alternate_modes(int recipient, int conn_num, struct altmode_data *alt_mode_data);
int libtypec_get_cam_supported(int conn_num, char *cam_data);
int libtypec_get_current_cam(char *cur_cam_data);
int libtypec_get_pdos(int conn_num, int partner, int offset, int *num_pdo, int src_snk, int type, unsigned int *pdo_data);
int libtypec_get_cable_properties(int conn_num, struct libtypec_cable_property *cbl_prop_data);
int libtypec_get_connector_status(int conn_num, struct libtypec_connector_status *conn_sts);
int libtypec_get_pd_message(int recipient, int conn_num, int num_bytes, int resp_type, char *pd_msg_resp);

int libtypec_get_bb_status(unsigned int *num_bb_instance);
int libtypec_get_bb_data(int num_billboards,char* bb_data);

#endif /*LIBTYPEC_H*/
