/*
    Copyright (c) 2021-2022 by Rajaram Regupathy, rajaram.regupathy@gmail.com

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
    details.

    (See the full license text in the LICENSES directory)
*/
// SPDX-License-Identifier: GPL-2.0-only
/**
 * @file lstypec.c
 * @author Rajaram Regupathy <rajaram.regupathy@gmail.com>
 * @brief Implements listing of typec port and port partner details
 *
 */

#include <stdio.h>
#include "../libtypec.h"
#include "names.h"
#include <stdlib.h>

#define LSTYPEC_MAJOR_VERSION 0
#define LSTYPEC_MINOR_VERSION 1

struct libtypec_capabiliy_data get_cap_data;
struct libtypec_connector_cap_data conn_data;
struct libtypec_connector_status conn_sts;
struct libtypec_cable_property cable_prop;
union libtypec_discovered_identity id;

struct altmode_data am_data[64];
char *session_info[LIBTYPEC_SESSION_MAX_INDEX];

#define LSTYPEC_ERROR 1
#define LSTYPEC_INFO 2

int verbose = 1;

union id_header
{
    unsigned long id_hdr;
    struct
    {
        unsigned usb_vendor_id : 16;
        unsigned reserved : 5;
        unsigned connector_type : 2;
        unsigned prd_type_dfp : 3;
        unsigned modal_operation : 1;
        unsigned product_type : 3;
        unsigned usb_capable_device : 1;
        unsigned usb_capable_host : 1;
    } id_hdr_pd3_1_v1_3;

    struct
    {
        unsigned usb_vendor_id : 16;
        unsigned reserved : 10;
        unsigned modal_operation : 1;
        unsigned product_type : 3;
        unsigned usb_capable_device : 1;
        unsigned usb_capable_host : 1;
    } id_hdr_pd2_v1_3;
};

union passive_vdo_1
{

    unsigned long psv_vdo_1;

    struct
    {
        unsigned usb_signalling : 3;
        unsigned reserved3 : 2;
        unsigned vbus_current_cap : 2;
        unsigned reserved7 : 2;
        unsigned vbus_max_volt : 2;
        unsigned termination_type : 2;
        unsigned latency : 4;
        unsigned epr_mode : 1;
        unsigned plug_type : 2;
        unsigned reserved20 : 1;
        unsigned vdo_version : 3;
        unsigned fw_ver : 4;
        unsigned hw_ver : 4;
    } psv_vdo1_pd3_1_v1_3;

    struct
    {
        unsigned usb_signalling : 3;
        unsigned reserved3 : 1;
        unsigned vbus_thru_cbl : 1;
        unsigned vbus_handling : 2;
        unsigned dir_support : 4;
        unsigned termination_type : 2;
        unsigned latency : 4;
        unsigned reserved17 : 1;
        unsigned plug_type : 2;
        unsigned reserved20 : 4;
        unsigned fw_ver : 4;
        unsigned hw_ver : 4;
    } psv_vdo1_pd2_v1_3;
};

void print_session_info()
{
    printf("lstypec %d.%d Session Info\n", LSTYPEC_MAJOR_VERSION, LSTYPEC_MINOR_VERSION);

    printf("\tUsing %s\n", session_info[LIBTYPEC_VERSION_INDEX]);

    printf("\t%s with Kernel %s\n", session_info[LIBTYPEC_OS_INDEX], session_info[LIBTYPEC_KERNEL_INDEX]);
}

void print_ppm_capability(struct libtypec_capabiliy_data ppm_data)
{
    printf("\nUSB-C Platform Policy Manager Capability\n");

    printf("\tNumber of Connectors:\t\t%d\n", ppm_data.bNumConnectors);

    printf("\tNumber of Alternate Modes:\t%d\n", ppm_data.bNumAltModes);

    printf("\tUSB Power Delivery Revision:\t%d.%d\n", (ppm_data.bcdPDVersion >> 8) & 0XFF, (ppm_data.bcdPDVersion) & 0XFF);

    printf("\tUSB Type-C Revision:\t\t%d.%d\n", (ppm_data.bcdTypeCVersion >> 8) & 0XFF, (ppm_data.bcdTypeCVersion) & 0XFF);
}

void print_conn_capability(struct libtypec_connector_cap_data conn_data)
{
    char *opr_mode_str[] = {"Source", "Sink", "DRP", "Analog Audio", "Debug Accessory", "USB2", "USB3", "Alternate Mode"};

    printf("\tOperation Mode:\t\t%s\n", opr_mode_str[conn_data.opr_mode]);
}

void print_cable_prop(struct libtypec_cable_property cable_prop, int conn_num)
{
    char *cable_type[] = {"Passive", "Active", "Unknown"};
    char *cable_plug_type[] = {"USB Type A", "USB Type B", "USB Type C", "Non-USB Type", "Unknown"};

    printf("\tCable Property in Port %d:\n", conn_num);

    printf("\t\tCable Type\t:\t%s\n", cable_type[cable_prop.cable_type]);

    printf("\t\tCable Plug Type\t:\t%s\n", cable_plug_type[cable_prop.plug_end_type]);
}

void print_alternate_mode_data(int recepient, int num_mode, struct altmode_data *am_data)
{

    if (recepient == AM_CONNECTOR)
    {

        for (int i = 0; i < num_mode; i++)
        {

            printf("\tLocal Modes %d:\n", i);

            printf("\t\tSVID\t:\t0x%04lx\n", am_data[i].svid);

            printf("\t\tVDO\t:\t0x%04lx\n", am_data[i].vdo);
        }
    }

    if (recepient == AM_SOP)
    {

        for (int i = 0; i < num_mode; i++)
        {

            printf("\tPartner Modes %d:\n", i);

            printf("\t\tSVID\t:\t0x%04lx\n", am_data[i].svid);

            printf("\t\tVDO\t:\t0x%04lx\n", am_data[i].vdo);
        }
    }

    if (recepient == AM_SOP_PR)
    {

        for (int i = 0; i < num_mode; i++)
        {

            printf("\tCable Plug Modes %d:\n", i);

            printf("\t\tSVID\t:\t0x%04lx\n", am_data[i].svid);

            printf("\t\tVDO\t:\t0x%04lx\n", am_data[i].vdo);
        }
    }
}

void print_identity_data(int recepient, union libtypec_discovered_identity id)
{
    char *conn_type[] = {"Legacy systems", "Undefined", "USB Type-C Receptacle", "USB Type-C Plug"};
    char *dfp_type[] = {"Not a DFP/Legacy Device", "PDUSB Hub", "PDUSB Host", "Power Brick", "Alternate Mode Adaptor", "Reserved"};
    char *ufp_type[] = {"Not a UFP", "PDUSB Hub", "PDUSB Peripheral", "PSD", "Reserved", "Alternate Mode Adapter", "VConn Powered Device", "Reserved"};
    char *cbl_type[] = {"Not a Cable Plug/VPD", "Reserved", "Reserved", "Passive Cable", "Active Cable", "Reserved"};
    char *latency[] = {"<10ns (~1m)", "10ns to 20ns (~2m)", "20ns to 30ns (~3m)",
                       "30ns to 40ns (~4m)", "40ns to 50ns (~5m)", "50ns to 60ns (~6m)",
                       "60ns to 70ns (~7m)", "> 70ns (>~7m)", "Reserved"};
    char *vbus_max_volt[] = {"20V", "Deprecated", "Deprecated", "50V"};
    char *vbus_cur_handling[] = {"Reserved", "3A", "5A", "Reserved"};
    char *usb_speed_v3[] = {"USB 2.0", "USB 3.2 Gen1", "USB 3.2/USB4 Gen 2", "USB4 Gen3"};
    char *usb_speed_v2[] = {"USB 2.0", "USB 3.1 Gen1", "USB 3.1 Gen1 Gen 2", "Reserved"};
    char *plug_type_v2[] = {"USB Type-A", "USB Type-B", "USB Type-C", "Captive"};
    char vendor_id[128];

    if (recepient == AM_SOP)
    {
        printf("\tPartner Identity :\n");

        printf("\t\tCertificate\t:\t0x%04lx\n", id.disc_id.cert_stat);

        printf("\t\tID Header\t:\t0x%04lx\n", id.disc_id.id_header);

        if (verbose)
        {
            union id_header id_hdr_val;
            id_hdr_val.id_hdr = id.disc_id.id_header;

            get_vendor_string(vendor_id, sizeof(vendor_id), id_hdr_val.id_hdr_pd3_1_v1_3.usb_vendor_id);

            printf("\t\t\tVendor ID\t\t:\t0x%04x (%s)\n", id_hdr_val.id_hdr_pd3_1_v1_3.usb_vendor_id, vendor_id);

            printf("\t\t\tConnector Type\t\t:\t%d(%s)\n", id_hdr_val.id_hdr_pd3_1_v1_3.connector_type, conn_type[id_hdr_val.id_hdr_pd3_1_v1_3.connector_type]);

            printf("\t\t\tDFP Product Type\t:\t%d(%s)\n", id_hdr_val.id_hdr_pd3_1_v1_3.prd_type_dfp, dfp_type[id_hdr_val.id_hdr_pd3_1_v1_3.prd_type_dfp]);

            printf("\t\t\tModal Operation\t\t:\t%s\n", id_hdr_val.id_hdr_pd3_1_v1_3.modal_operation ? "Yes" : "No");

            printf("\t\t\tUFP Product Type\t:\t%d(%s)\n", id_hdr_val.id_hdr_pd3_1_v1_3.product_type, ufp_type[id_hdr_val.id_hdr_pd3_1_v1_3.product_type]);

            printf("\t\t\tUSB Device Capability\t:\t%s\n", id_hdr_val.id_hdr_pd3_1_v1_3.usb_capable_device ? "Yes" : "No");

            printf("\t\t\tUSB Host Capability\t:\t%s\n", id_hdr_val.id_hdr_pd3_1_v1_3.usb_capable_host ? "Yes" : "No");
        }

        printf("\t\tProduct\t\t:\t0x%04lx\n", id.disc_id.product);

        if (verbose)
        {

            printf("\t\t\tUSB Product ID\t\t:\t0x%04lx\n", (id.disc_id.product >> 16) & 0xffff);

            printf("\t\t\tbcdDevice\t\t:\t0x%04lx\n", id.disc_id.product & 0xFFFF);
        }

        printf("\t\tProduct VDO 1\t:\t0x%04lx\n", id.disc_id.product_type_vdo1);

        printf("\t\tProduct VDO 2\t:\t0x%04lx\n", id.disc_id.product_type_vdo2);

        printf("\t\tProduct VDO 3\t:\t0x%04lx\n", id.disc_id.product_type_vdo3);
    }

    if (recepient == AM_SOP_PR)
    {

        printf("\tCable Identity :\n");

        printf("\t\tCertificate\t:\t0x%04lx\n", id.disc_id.cert_stat);

        printf("\t\tID Header\t:\t0x%04lx\n", id.disc_id.id_header);

        if (verbose)
        {
            union id_header id_hdr_val;

            id_hdr_val.id_hdr = id.disc_id.id_header;

            get_vendor_string(vendor_id, sizeof(vendor_id), id_hdr_val.id_hdr_pd3_1_v1_3.usb_vendor_id);

            printf("\t\t\tVendor ID\t\t:\t0x%04x (%s)\n", id_hdr_val.id_hdr_pd3_1_v1_3.usb_vendor_id, vendor_id);

            printf("\t\t\tConnector Type\t\t:\t%d(%s)\n", id_hdr_val.id_hdr_pd3_1_v1_3.connector_type, conn_type[id_hdr_val.id_hdr_pd3_1_v1_3.connector_type]);

            printf("\t\t\tDFP Product Type\t:\t%d(%s)\n", id_hdr_val.id_hdr_pd3_1_v1_3.prd_type_dfp, dfp_type[id_hdr_val.id_hdr_pd3_1_v1_3.prd_type_dfp]);

            printf("\t\t\tModal Operation\t\t:\t%s\n", id_hdr_val.id_hdr_pd3_1_v1_3.modal_operation ? "Yes" : "No");

            printf("\t\t\tUFP Product Type\t:\t%d(%s)\n", id_hdr_val.id_hdr_pd3_1_v1_3.product_type, cbl_type[id_hdr_val.id_hdr_pd3_1_v1_3.product_type]);

            printf("\t\t\tUSB Device Capability\t:\t%s\n", id_hdr_val.id_hdr_pd3_1_v1_3.usb_capable_device ? "Yes" : "No");

            printf("\t\t\tUSB Host Capability\t:\t%s\n", id_hdr_val.id_hdr_pd3_1_v1_3.usb_capable_host ? "Yes" : "No");
        }

        printf("\t\tProduct\t\t:\t0x%04lx\n", id.disc_id.product);

        if (verbose)
        {

            printf("\t\t\tUSB Product ID\t\t:\t0x%04lx\n", (id.disc_id.product >> 16) & 0xffff);

            printf("\t\t\tbcdDevice\t\t:\t0x%04lx\n", id.disc_id.product & 0xFFFF);
        }

        printf("\t\tProduct VDO 1\t:\t0x%04lx\n", id.disc_id.product_type_vdo1);

        if (verbose)
        {
            union passive_vdo_1 psv_vdo1;

            psv_vdo1.psv_vdo_1 = id.disc_id.product_type_vdo1;

            if (conn_data.plug_rev == 2)
            {
                printf("\t\t\tHW Version\t:\t%d\n", psv_vdo1.psv_vdo1_pd2_v1_3.hw_ver);

                printf("\t\t\tFW Version\t:\t%d\n", psv_vdo1.psv_vdo1_pd2_v1_3.fw_ver);

                printf("\t\t\tPlug Type\t:\t%d(%s)\n", psv_vdo1.psv_vdo1_pd2_v1_3.plug_type, plug_type_v2[psv_vdo1.psv_vdo1_pd2_v1_3.plug_type]);

                printf("\t\t\tCable Latency\t:\t%d(%s)\n", psv_vdo1.psv_vdo1_pd2_v1_3.latency, latency[psv_vdo1.psv_vdo1_pd2_v1_3.latency]);

                printf("\t\t\tCable Termination\t:\t%d\n", psv_vdo1.psv_vdo1_pd2_v1_3.termination_type);

                printf("\t\t\tDirection Support\t:\t%d\n", psv_vdo1.psv_vdo1_pd2_v1_3.dir_support);

                printf("\t\t\tVBus Current Capacity\t:\t%d(%s)\n", psv_vdo1.psv_vdo1_pd2_v1_3.vbus_handling, vbus_cur_handling[psv_vdo1.psv_vdo1_pd2_v1_3.vbus_handling]);

                printf("\t\t\tVBus Through Cable\t:\t%d\n", psv_vdo1.psv_vdo1_pd2_v1_3.vbus_thru_cbl);

                printf("\t\t\tUSB Signalling Support\t:\t%d(%s)\n", psv_vdo1.psv_vdo1_pd2_v1_3.usb_signalling, usb_speed_v2[psv_vdo1.psv_vdo1_pd2_v1_3.usb_signalling]);
            }
            else
            {
                printf("\t\t\tHW Version\t:\t%d\n", psv_vdo1.psv_vdo1_pd3_1_v1_3.hw_ver);

                printf("\t\t\tFW Version\t:\t%d\n", psv_vdo1.psv_vdo1_pd3_1_v1_3.fw_ver);

                printf("\t\t\tVDO Version\t:\t%d\n", psv_vdo1.psv_vdo1_pd3_1_v1_3.vdo_version);

                printf("\t\t\tPlug Type\t:\t%d(%s)\n", psv_vdo1.psv_vdo1_pd3_1_v1_3.plug_type, psv_vdo1.psv_vdo1_pd3_1_v1_3.plug_type == 2 ? "USB Type C" : "Captive");

                printf("\t\t\tEPR Mode\t:\t%d\n", psv_vdo1.psv_vdo1_pd3_1_v1_3.epr_mode);

                printf("\t\t\tCable Latency\t:\t%d(%s)\n", psv_vdo1.psv_vdo1_pd3_1_v1_3.latency, latency[psv_vdo1.psv_vdo1_pd3_1_v1_3.latency]);

                printf("\t\t\tCable Termination\t:\t%d(%s)\n", psv_vdo1.psv_vdo1_pd3_1_v1_3.termination_type, psv_vdo1.psv_vdo1_pd3_1_v1_3.termination_type ? "VCONN not required" : "VCONN required");

                printf("\t\t\tVBus Voltage Max\t:\t%d(%s)\n", psv_vdo1.psv_vdo1_pd3_1_v1_3.vbus_max_volt, vbus_max_volt[psv_vdo1.psv_vdo1_pd3_1_v1_3.vbus_max_volt]);

                printf("\t\t\tVBus Current Capacity\t:\t%d(%s)\n", psv_vdo1.psv_vdo1_pd3_1_v1_3.vbus_current_cap, vbus_cur_handling[psv_vdo1.psv_vdo1_pd3_1_v1_3.vbus_current_cap]);

                printf("\t\t\tUSB Highest Speed\t:\t%d(%s)\n", psv_vdo1.psv_vdo1_pd3_1_v1_3.usb_signalling, usb_speed_v3[psv_vdo1.psv_vdo1_pd3_1_v1_3.usb_signalling]);
            }
        }

        printf("\t\tProduct VDO 2\t:\t0x%04lx\n", id.disc_id.product_type_vdo2);

        printf("\t\tProduct VDO 3\t:\t0x%04lx\n", id.disc_id.product_type_vdo3);
    }
}

void lstypec_print(char *val, int type)
{
    if (type == LSTYPEC_ERROR)
    {
        printf("lstypec - ERROR - %s\n", val);
        exit(1);
    }
    else
        printf("lstypec - INFO - %s\n", val);
}

int main(void)
{

    int ret;

    names_init();

    ret = libtypec_init(session_info);

    if (ret < 0)
        lstypec_print("Failed in Initializing libtypec", LSTYPEC_ERROR);

    print_session_info();

    ret = libtypec_get_capability(&get_cap_data);

    if (ret < 0)
        lstypec_print("Failed in Get Capability", LSTYPEC_ERROR);

    print_ppm_capability(get_cap_data);

    for (int i = 0; i < get_cap_data.bNumConnectors; i++)
    {
        /* Resetting port properties */
        cable_prop.cable_type = CABLE_TYPE_UNKNOWN;
        cable_prop.plug_end_type = PLUG_TYPE_OTH;

        printf("\nConnector %d Capablity/Status\n", i);

        libtypec_get_conn_capability(i, &conn_data);

        print_conn_capability(conn_data);

        ret = libtypec_get_cable_properties(i, &cable_prop);

        if (ret >= 0)
            print_cable_prop(cable_prop, i);

        printf("\tAlternate Modes Supported:\n");

        ret = libtypec_get_alternate_modes(AM_CONNECTOR, i, am_data);

        if (ret > 0)
            print_alternate_mode_data(AM_CONNECTOR, ret, am_data);
        else
            printf("\t\tNo Local Modes listed with typec class\n");

        ret = libtypec_get_connector_status(i, &conn_sts);

        if ((ret == 0) && conn_sts.connect_sts)
        {
            ret = libtypec_get_alternate_modes(AM_SOP_PR, i, am_data);

            print_alternate_mode_data(AM_SOP_PR, ret, am_data);

            ret = libtypec_get_pd_message(AM_SOP_PR, i, 24, DISCOVER_ID_REQ, id.buf_disc_id);

            if (ret >= 0)
                print_identity_data(AM_SOP_PR, id);

            ret = libtypec_get_alternate_modes(AM_SOP, i, am_data);

            print_alternate_mode_data(AM_SOP, ret, am_data);

            ret = libtypec_get_pd_message(AM_SOP, i, 24, DISCOVER_ID_REQ, id.buf_disc_id);

            if (ret >= 0)
                print_identity_data(AM_SOP, id);
        }
    }

    printf("\n");

    names_exit();
}
