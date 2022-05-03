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
 * @coauthor Jameson Thies <jthies@google.com>
 * @brief Implements listing of typec port and port partner details
 *
 */

#include <stdio.h>
#include "../libtypec.h"
#include "names.h"
#include <stdlib.h>
#include <stdint.h>

#define LSTYPEC_MAJOR_VERSION 0
#define LSTYPEC_MINOR_VERSION 2

#define LSTYPEC_ERROR 1
#define LSTYPEC_INFO 2

#define MAX_FIELDS 16

enum product_type {
  product_type_other = 0,
  product_type_pd2p0_passive_cable = 1,
  product_type_pd2p0_active_cable = 2,
  product_type_pd2p0_ama = 3,
  product_type_pd3p0_passive_cable = 4,
  product_type_pd3p0_active_cable = 5,
  product_type_pd3p0_ama = 6,
  product_type_pd3p0_vpd = 7,
  product_type_pd3p0_ufp = 8,
  product_type_pd3p0_dfp = 9,
  product_type_pd3p0_drd = 10,
  product_type_pd3p1_passive_cable = 11,
  product_type_pd3p1_active_cable = 12,
  product_type_pd3p1_vpd = 13,
  product_type_pd3p1_ufp = 14,
  product_type_pd3p1_dfp = 15,
  product_type_pd3p1_drd = 16,
};

struct vdo_field{
  char *name;
  uint8_t print;
  uint8_t index;
  uint32_t mask;
};

union id_header
{
    uint32_t id_hdr;

    struct
    {
        unsigned usb_vendor_id : 16;
        unsigned reserved : 10;
        unsigned modal_operation : 1;
        unsigned product_type : 3;
        unsigned usb_capable_device : 1;
        unsigned usb_capable_host : 1;
    } id_hdr_pd2p0;

    struct
    {
        unsigned usb_vendor_id : 16;
        unsigned reserved : 7;
        unsigned prd_type_dfp : 3;
        unsigned modal_operation : 1;
        unsigned product_type : 3;
        unsigned usb_capable_device : 1;
        unsigned usb_capable_host : 1;
    } id_hdr_pd3p0;

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
    } id_hdr_pd3p1;
};

union passive_vdo_1
{

    uint32_t psv_vdo_1;

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
    } psv_vdo1_pd2p0;

    struct
    {
        unsigned usb_signalling : 3;
        unsigned reserved3 : 2;
        unsigned vbus_current_cap : 2;
        unsigned reserved7 : 2;
        unsigned vbus_max_volt : 2;
        unsigned termination_type : 2;
        unsigned latency : 4;
        unsigned reserved17 : 1;
        unsigned plug_type : 2;
        unsigned reserved20 : 1;
        unsigned vdo_version : 3;
        unsigned fw_ver : 4;
        unsigned hw_ver : 4;
    } psv_vdo1_pd3p0;

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
    } psv_vdo1_pd3p1;
};

//Constants

// ID Header product type masks
const int pd_ufp_product_type_mask = 0x38000000;
const int pd_dfp_product_type_mask = 0x03800000;

const int pd2p0_passive_cable = 0x20000000;
const int pd2p0_active_cable = 0x18000000;
const int pd2p0_ama = 0x28000000;
const int pd3p0_passive_cable = 0x18000000;
const int pd3p0_active_cable = 0x20000000;
const int pd3p0_ama = 0x28000000;
const int pd3p0_vpd = 0x30000000;
const int pd3p0_hub = 0x08000000;
const int pd3p0_peripheral = 0x10000000;
const int pd3p0_dfp_hub = 0x00800000;
const int pd3p0_dfp_host = 0x01000000;
const int pd3p0_power_brick = 0x01800000;
const int pd3p1_passive_cable = 0x18000000;
const int pd3p1_active_cable = 0x20000000;
const int pd3p1_vpd = 0x30000000;
const int pd3p1_hub = 0x08000000;
const int pd3p1_peripheral = 0x10000000;
const int pd3p1_dfp_hub = 0x00800000;
const int pd3p1_dfp_host = 0x01000000;
const int pd3p1_power_brick = 0x01800000;

// USB PD 2.0 ID Header VDO (Section 6.4.4.3.1.1)
const struct vdo_field pd2p0_partner_id_header_fields[] = {
  {"USB Vendor ID", 1, 0, 0xffff},
  {"Reserved", 0, 16, 0x3ff},
  {"Modal Operation Supported", 1, 26, 0x1},
  {"Produt Type (UFP)", 1, 27, 0x7},
  {"USB Capable as a Device", 1, 30, 0x1},
  {"USB Capable as a Host", 1, 31, 0x1},
};
const char *pd2p0_partner_id_header_field_desc[][MAX_FIELDS] = {
  {NULL},
  {NULL},
  {"No", "Yes"},
  {"Undefined", "PDUSB Hub", "PDUSB Peripheral", "Reserved", "Reserved", "Alternate Mode Adapter", "Reserved", "Reserved"},
  {"No", "Yes"},
  {"No", "Yes"},
};
const struct vdo_field pd2p0_cable_id_header_fields[] = {
  {"USB Vendor ID", 1, 0, 0xffff},
  {"Reserved", 0, 16, 0x3ff},
  {"Modal Operation Supported", 1, 26, 0x1},
  {"Produt Type (UFP)", 1, 27, 0x7},
  {"USB Capable as a Device", 1, 30, 0x1},
  {"USB Capable as a Host", 1, 31, 0x1},
};
const char *pd2p0_cable_id_header_field_desc[][MAX_FIELDS] = {
  {NULL},
  {NULL},
  {"No", "Yes"},
  {"Undefined", "Reserved", "Reserved", "Passive Cable", "Active Cable", "Reserved", "Reserved", "Reserved"},
  {"No", "Yes"},
  {"No", "Yes"},
};

// USB PD 2.0 Cert Stat VDO (Section 6.4.4.3.1.2)
const struct vdo_field pd2p0_cert_stat_fields[] = {
  {"XID", 1, 0, 0xffffffff},
};
const char *pd2p0_cert_stat_field_desc[][MAX_FIELDS] = {
  {NULL},
};

// USB PD 2.0 Product VDO (Section 6.4.4.3.1.3)
const struct vdo_field pd2p0_product_fields[] = {
  {"bcdDevice", 1, 0, 0xffff},
  {"USB Product ID", 1, 16, 0xffff},
};
const char *pd2p0_product_field_desc[][MAX_FIELDS] = {
  {NULL},
  {NULL},
};

// USB PD 2.0 Passive Cable VDO (Section 6.4.4.3.1.4.1)
const struct vdo_field pd2p0_passive_cable_fields[] = {
  {"USB SuperSpeed Support", 1, 0, 0x7},
  {"Reserved", 0, 3, 0x1},
  {"Vbus Through Cable", 1, 4, 0x1},
  {"Vbus Current Handling", 1, 5, 0x3},
  {"SSRX2 Support", 1, 7, 0x1},
  {"SSRX1 Support", 1, 8, 0x1},
  {"SSTX2 Support", 1, 9, 0x1},
  {"SSTX1 Support", 1, 10, 0x1},
  {"Cable Termination Type", 1, 11, 0x3},
  {"Cable Latency", 1, 13, 0xf},
  {"Reserved", 0, 17, 0x1},
  {"USB Type-C plug to", 1, 18, 0x3},
  {"Reserved", 0, 20, 0xf},
  {"Firmware Version", 1, 24, 0xf},
  {"HW Version", 1, 28, 0xf},
};
const char *pd2p0_passive_cable_field_desc[][MAX_FIELDS] = {
  {"USB 2.0 Only", "USB 3.1 Gen1", "USB 3.1 Gen1 and Gen2", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"},
  {NULL},
  {"No", "Yes"},
  {"Reserved", "3A", "5A", "Reserved"},
  {"Fixed", "Configurable"},
  {"Fixed", "Configurable"},
  {"Fixed", "Configurable"},
  {"Fixed", "Configurable"},
  {"Vconn Not Required", "Vconn Required", "Reserved", "Reserved"},
  {"Reserved", "<10ns (~1m)", "10ns to 20ns (~2m)", "20ns to 30ns (~3m)", "30ns to 40ns (~4m)", "40ns to 50ns (~5m)", "50ns to 60ns (~6m)", "60ns to 70ns (~7m)", " 70ns (>~7m)", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"},
  {NULL},
  {"USB Type-A", "USB Type-B", "USB Type-C", "Captive"},
  {NULL},
  {NULL},
  {NULL},
};

// USB PD 2.0 Active Cable VDO (Section 6.4.4.3.1.4.2)
const struct vdo_field pd2p0_active_cable_fields[] = {
  {"USB SuperSpeed Support", 1, 0, 0x7},
  {"SOP'' Controller Present", 1, 3, 0x1},
  {"Vbus Through Cable", 1, 4, 0x1},
  {"Vbus Current Handling", 1, 5, 0x3},
  {"SSRX2 Support", 1, 7, 0x1},
  {"SSRX1 Support", 1, 8, 0x1},
  {"SSTX2 Support", 1, 9, 0x1},
  {"SSTX1 Support", 1, 10, 0x1},
  {"Cable Termination Type", 1, 11, 0x3},
  {"Cable Latency", 1, 13, 0xf},
  {"Reserved", 0, 17, 0x1},
  {"USB Type-C plug to", 1, 18, 0x3},
  {"Reserved", 0, 20, 0xf},
  {"Firmware Version", 1, 24, 0xf},
  {"HW Version", 1, 28, 0xf},
};
const char *pd2p0_active_cable_field_desc[][MAX_FIELDS] = {
  {"USB 2.0 Only", "USB 3.1 Gen1", "USB 3.1 Gen1 and Gen2", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"},
  {NULL},
  {"No", "Yes"},
  {"Reserved", "3A", "5A", "Reserved"},
  {"Fixed", "Configurable"},
  {"Fixed", "Configurable"},
  {"Fixed", "Configurable"},
  {"Fixed", "Configurable"},
  {"Vconn Not Required", "Vconn Required", "Reserved", "Reserved"},
  {"Reserved", "<10ns (~1m)", "10ns to 20ns (~2m)", "20ns to 30ns (~3m)", "30ns to 40ns (~4m)", "40ns to 50ns (~5m)", "50ns to 60ns (~6m)", "60ns to 70ns (~7m)", " 70ns (>~7m)", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"},
  {NULL},
  {"USB Type-A", "USB Type-B", "USB Type-C", "Captive"},
  {NULL},
  {NULL},
  {NULL},
};

// USB PD 2.0 AMA VDO (Section 6.4.4.3.1.5)
const struct vdo_field pd2p0_ama_fields[] = {
  {"USB SuperSpeed Support", 1, 0, 0x7},
  {"Vbus required", 1, 3, 0x1},
  {"Vconn required", 1, 4, 0x1},
  {"Vconn power", 1, 5, 0x7},
  {"SSRX2 Support", 1, 8, 0x1},
  {"SSRX1 Support", 1, 9, 0x1},
  {"SSTX2 Support", 1, 10, 0x1},
  {"SSTX1 Support", 1, 11, 0x1},
  {"Reserved", 0, 12, 0xfff},
  {"Firmware Version", 1, 24, 0xf},
  {"HW Version", 1, 28, 0xf},
};
const char *pd2p0_ama_field_desc[][MAX_FIELDS] = {
  {"USB 2.0 Only", "USB 3.1 Gen1", "USB 3.1 Gen1 and Gen2", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"},
  {"No", "Yes"},
  {"No", "Yes"},
  {"1W", "1.5W", "2W", "3W", "4W", "5W", "6W", "Reserved"},
  {"Fixed", "Configurable"},
  {"Fixed", "Configurable"},
  {"Fixed", "Configurable"},
  {"Fixed", "Configurable"},
  {NULL},
  {NULL},
  {NULL},
};


// USB PD 3.0 ID Header VDO (Section 6.4.4.3.1.1)
const struct vdo_field pd3p0_partner_id_header_fields[] = {
  {"USB Vendor ID", 1, 0, 0xffff},
  {"Reserved", 0, 16, 0x3f},
  {"Product Type (DFP)", 1, 23, 0x7},
  {"Modal Operation Supported", 1, 26, 0x1},
  {"Product Type (UFP)", 1, 27, 0x7},
  {"USB Capable as a Device", 1, 30, 0x1},
  {"USB Capable as a Host", 1, 31, 0x1},
};
const char *pd3p0_partner_id_header_field_desc[][MAX_FIELDS] = {
  {NULL},
  {NULL},
  {"Undefined", "PDUSB Hub", "PDUSB Host", "Power Brick", "Alternate Mode Controller", "Reserved", "Reserved", "Reserved"},
  {"No", "Yes"},
  {"Undefined", "PDUSB Hub", "PDUSB Peripheral", "PSD", "Reserved", "Alternate Mode Adapter", "Vconn Powered USB Device", "Reserved"},
  {"No", "Yes"},
  {"No", "Yes"},
};
const struct vdo_field pd3p0_cable_id_header_fields[] = {
  {"USB Vendor ID", 1, 0, 0xffff},
  {"Reserved", 0, 16, 0x3f},
  {"Product Type (DFP)", 0, 23, 0x7},
  {"Modal Operation Supported", 1, 26, 0x1},
  {"Product Type (Cable Plug)", 1, 27, 0x7},
  {"USB Capable as a Device", 1, 30, 0x1},
  {"USB Capable as a Host", 1, 31, 0x1},
};
const char *pd3p0_cable_id_header_field_desc[][MAX_FIELDS] = {
  {NULL},
  {NULL},
  {NULL},
  {"No", "Yes"},
  {"Undefined", "Reserved", "Reserved", "Passive Cable", "Active Cable", "Reserved", "Reserved", "Reserved"},
  {"No", "Yes"},
  {"No", "Yes"},
};

// USB PD 3.0 Cert Stat VDO (Section 6.4.4.3.1.2)
const struct vdo_field pd3p0_cert_stat_fields[] = {
  {"XID", 1, 0, 0xffffffff},
};
const char *pd3p0_cert_stat_field_desc[][MAX_FIELDS] = {
  {NULL},
};

// USB PD 3.0 Product VDO (Section 6.4.4.3.1.3)
const struct vdo_field pd3p0_product_fields[] = {
  {"bcdDevice", 1, 0, 0xffff},
  {"USB Product ID", 1, 16, 0xffff},
};
const char *pd3p0_product_field_desc[][MAX_FIELDS] = {
  {NULL},
  {NULL},
};

// USB PD 3.0 Passive Cable VDO (Section 6.4.4.3.1.6)
const struct vdo_field pd3p0_passive_cable_fields[] = {
  {"USB Highest Speed", 1, 0, 0x7},
  {"Reserved", 0, 3, 0x3},
  {"Vbus Current Handling", 1, 5, 0x3},
  {"Reserved", 0, 7, 0x3},
  {"Maximum Vbus Voltage", 1, 9, 0x3},
  {"Cable Termination Type", 1, 11, 0x3},
  {"Cable Latency", 1, 13, 0xf},
  {"Reserved", 0, 17, 0x1},
  {"Connector Type", 1, 18, 0x3},
  {"Reserved", 0, 20, 0x1},
  {"VDO version", 1, 21, 0x7},
  {"Firmware Version", 1, 24, 0xf},
  {"HW Version", 1, 28, 0xf},
};
const char *pd3p0_passive_cable_field_desc[][MAX_FIELDS] = {
  {"USB 2.0 Only", "USB 3.2 Gen1", "USB 3.2/USB4 Gen2", "USB4 Gen3", "Reserved", "Reserved", "Reserved", "Reserved"},
  {NULL},
  {"Reserved", "3A", "5A", "Reserved"},
  {NULL},
  {"20V", "30V", "40V", "50V"},
  {"Vconn Not Required", "Vconn Required", "Reserved", "Reserved"},
  {"Reserved", "<10ns (~1m)", "10ns to 20ns (~2m)", "20ns to 30ns (~3m)", "30ns to 40ns (~4m)", "40ns to 50ns (~5m)", "50ns to 60ns (~6m)", "60ns to 70ns (~7m)", " 70ns (>~7m)", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"},
  {NULL},
  {"Reserved", "Reserved", "USB Type-C", "Captive"},
  {NULL},
  {"Version 1.0", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"},
  {NULL},
  {NULL},
};

// USB PD 3.0 Active Cable VDO1/VDO2 (Section 6.4.4.3.1.7)
const struct vdo_field pd3p0_active_cable_vdo1_fields[] = {
  {"USB Highest Speed", 1, 0, 0x7},
  {"SOP'' Controller Present", 1, 3, 0x1},
  {"Vbus Through Cable", 1, 4, 0x1},
  {"Vbus Current Handling", 1, 5, 0x3},
  {"SBU Type", 1, 7, 0x1},
  {"SBU Supported", 1, 8, 0x1},
  {"Maximum Vbus Voltage", 1, 9, 0x3},
  {"Cable Termination Type", 1, 11, 0x3},
  {"Cable Latency", 1, 13, 0xf},
  {"Reserved", 0, 17, 0x1},
  {"Conector Type", 1, 18, 0x3},
  {"Reserved", 0, 20, 0x1},
  {"VDO version", 1, 21, 0x7},
  {"Firmware Version", 1, 24, 0xf},
  {"HW Version", 1, 28, 0xf},
};
const char *pd3p0_active_cable_vdo1_field_desc[][MAX_FIELDS] = {
  {"USB 2.0 Only", "USB 3.2 Gen1", "USB 3.2/USB4 Gen2", "USB4 Gen3", "Reserved", "Reserved", "Reserved", "Reserved"},
  {"No", "Yes"},
  {"No", "Yes"},
  {"USB Type-C Default Current", "3A", "5A", "Reserved"},
  {"SBU is passive", "SBU is active"},
  {"SBU connections supported", "SBU connections are not supported"},
  {"20V", "30V", "40V", "50V"},
  {"Reserved", "Reserved", "One end active, one end passive, Vconn required", "Both ends active, Vconn required"},
  {"Reserved", "<10ns (~1m)", "10ns to 20ns (~2m)", "20ns to 30ns (~3m)", "30ns to 40ns (~4m)", "40ns to 50ns (~5m)", "50ns to 60ns (~6m)", "60ns to 70ns (~7m)", "1000b –1000ns (~100m)", "1001b –2000ns (~200m)", "1010b – 3000ns (~300m)", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"},
  {NULL},
  {"Reserved", "Reserved", "USB Type-C", "Captive"},
  {NULL},
  {"Reserved", "Reserved", "Reserved", "Version 1.3", "Reserved", "Reserved", "Reserved", "Reserved"},
  {NULL},
  {NULL},
};
const struct vdo_field pd3p0_active_cable_vdo2_fields[] = {
  {"USB Gen", 1, 0, 0x1},
  {"Reserved", 0, 1, 0x1},
  {"Optically Isolated Active Cable", 1, 2, 0x1},
  {"USB Lanes Supported", 1, 3, 0x1},
  {"USB 3.2 Supported", 1, 4, 0x1},
  {"USB 2.0 Supported", 1, 5, 0x1},
  {"USB 2.0 Hub Hops Consumed", 1, 6, 0x3},
  {"USB4 Supported", 1, 8, 0x1},
  {"Active element", 1, 9, 0x1},
  {"Physical connection", 1, 10, 0x1},
  {"U3 to U0 transition mode", 1, 11, 0x1},
  {"U3/Cld Power", 1, 12, 0x7},
  {"Reserved", 0, 15, 0x1},
  {"Shutdown Tempurature", 1, 16, 0xff},
  {"Maximum Operating Tempurature", 1, 24, 0xff},
};
const char *pd3p0_active_cable_vdo2_field_desc[][MAX_FIELDS] = {
  {"Gen 1", "Gen 2 or higher"},
  {NULL},
  {"No", "Yes"},
  {"One Lane", "Two Lanes"},
  {"USB 3.2 SuperSpeed supported", "USB 3.2 SuperSpeed not supported"},
  {"USB 2.0 supported", "USB 2.0 not supported"},
  {NULL},
  {"USB4 Supported", "USB4 Not Supported"},
  {"Active Redriver", "Active Retimer"},
  {"Copper", "Optical"},
  {"U3 to U0 direct", "U3 to U0 through U35"},
  {">10mW", "5-10mW", "1-5mW", "0.5-1mW", "0.2-0.5mW", "50-200uW", "<50uW", "Reserved"},
  {NULL},
  {NULL},
  {NULL},
};

// USB PD 3.0 AMA VDO (Section 6.4.4.3.1.8)
const struct vdo_field pd3p0_ama_fields[] = {
  {"USB Highest Speed", 1, 0, 0x7},
  {"Vbus required", 1, 3, 0x1},
  {"Vconn required", 1, 4, 0x1},
  {"Vconn power", 1, 5, 0x7},
  {"Reserved", 0, 8, 0x1fff},
  {"VDO Version", 1, 21, 0x7},
  {"Firmware Version", 1, 24, 0xf},
  {"HW Version", 1, 28, 0xf},
};
const char *pd3p0_ama_field_desc[][MAX_FIELDS] = {
  {"USB 2.0 only", "USB 3.2 Gen1 and USB 2.0", "USB 3.2 Gen1, Gen2 and USB 2.0", "billboard only", "Reserved", "Reserved", "Reserved", "Reserved"},
  {"No", "Yes"},
  {"No", "Yes"},
  {"1W", "1.5W", "2W", "3W", "4W", "5W", "6W", "Reserved"},
  {NULL},
  {"Version 1.0", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"},
  {NULL},
  {NULL},
};

// USB PD 3.0 VPD VDO (Section 6.4.4.3.1.9)
const struct vdo_field pd3p0_vpd_fields[] = {
  {"Charge Through Support", 1, 0, 0x1},
  {"Ground Impedance", 1, 1, 0x3f},
  {"Vbus Impedance", 1, 7, 0x3f},
  {"Reserved", 0, 13, 0x3},
  {"Charge Through Current Support", 1, 14, 0x1},
  {"Maximum Vbus Voltage", 1, 15, 0x3},
  {"Reserved", 0, 17, 0xf},
  {"VDO Version", 1, 21, 0x7},
  {"Firmware Version", 1, 24, 0xf},
  {"HW Version", 1, 28, 0xf},
};
const char *pd3p0_vpd_field_desc[][MAX_FIELDS] = {
  {"No","Yes"},
  {NULL},
  {NULL},
  {NULL},
  {"3A capable", "5A capable"},
  {"20V", "30V", "40V", "50V"},
  {NULL},
  {"Version 1.0", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"},
  {NULL},
  {NULL},
};

// USB PD 3.0 UFP VDO1/VDO2 (Section 6.4.4.3.1.4)
const struct vdo_field pd3p0_ufp_vdo1_fields[] = {
  {"USB Highest Speed", 1, 0, 0x7},
  {"Alternate Modes", 1, 3, 0x7},
  {"Reserved", 0, 6, 0x3ffff},
  {"Device Capability", 1, 24, 0xf},
  {"Reserved", 0, 28, 0x1},
  {"UFP VDO Version", 1, 29, 0x7},
};
const char *pd3p0_ufp_vdo1_field_desc[][MAX_FIELDS] = {
  {"USB 2.0 only", "USB 3.2 Gen1", "USB 3.2/USB4 Gen2", "USB4 Gen3", "Reserved", "Reserved", "Reserved", "Reserved"},
  {NULL},
  {NULL},
  {NULL},
  {NULL},
  {"Version 1.0", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"},
};
const struct vdo_field pd3p0_ufp_vdo2_fields[] = {
  {"USB3 Max Power", 1, 0, 0x7f},
  {"USB3 Min Power", 1, 7, 0x7f},
  {"Reserved", 0, 14, 0x3},
  {"USB4 Max Power", 1, 16, 0x7f},
  {"USB4 Min Power", 1, 23, 0x7f},
  {"Reserved", 0, 30, 0x3},
};
const char *pd3p0_ufp_vdo2_field_desc[][MAX_FIELDS] = {
  {NULL},
  {NULL},
  {NULL},
  {NULL},
  {NULL},
  {NULL},
};

// USB PD 3.0 DFP VDO (Section 6.4.4.3.1.5)
const struct vdo_field pd3p0_dfp_fields[] = {
  {"Port Number", 1, 0, 0x1f},
  {"Reserved", 0, 5, 0x7ffff},
  {"Host Capability", 1, 24, 0x7},
  {"Reserved", 0, 27, 0x3},
  {"DFP VDO Version", 1, 29, 0x7},
};
const char *pd3p0_dfp_field_desc[][MAX_FIELDS] = {
  {NULL},
  {NULL},
  {NULL},
  {NULL},
  {"Version 1.0", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"},
};


// USB PD 3.1 ID Header VDO (Section 6.4.4.3.1.1)
const struct vdo_field pd3p1_partner_id_header_fields[] = {
  {"USB Vendor ID", 1, 0, 0xffff},
  {"Reserved", 0, 16, 0x1f},
  {"Connector Type", 1, 21, 0x3},
  {"Product Type (DFP)", 1, 23, 0x7},
  {"Modal Operation Supported", 1, 26, 0x1},
  {"Product Type (UFP)", 1, 27, 0x7},
  {"USB Capable as a Device", 1, 30, 0x1},
  {"USB Capable as a Host", 1, 31, 0x1},
};
const char *pd3p1_partner_id_header_field_desc[][MAX_FIELDS] = {
  {NULL},
  {NULL},
  {"Reserved", "Reserved", "USB Type-C Receptacle", "USB Type-C Plug"},
  {"Undefined", "PDUSB Hub", "PDUSB Host", "Power Brick", "Alternate Mode Controller", "Reserved", "Reserved", "Reserved"},
  {"No", "Yes"},
  {"Undefined", "PDUSB Hub", "PDUSB Peripheral", "PSD", "Reserved", "Alternate Mode Adapter", "Vconn Powered USB Device", "Reserved"},
  {"No", "Yes"},
  {"No", "Yes"},
};
const struct vdo_field pd3p1_cable_id_header_fields[] = {
  {"USB Vendor ID", 1, 0, 0xffff},
  {"Reserved", 0, 16, 0x1f},
  {"Connector Type", 1, 21, 0x3},
  {"Product Type (DFP)", 0, 23, 0x7},
  {"Modal Operation Supported", 1, 26, 0x1},
  {"Product Type (Cable Plug)", 1, 27, 0x7},
  {"USB Capable as a Device", 1, 30, 0x1},
  {"USB Capable as a Host", 1, 31, 0x1},
};
const char *pd3p1_cable_id_header_field_desc[][MAX_FIELDS] = {
  {NULL},
  {NULL},
  {"Reserved", "Reserved", "USB Type-C Receptacle", "USB Type-C Plug"},
  {NULL},
  {"No", "Yes"},
  {"Undefined", "Reserved", "Reserved", "Passive Cable", "Active Cable", "Reserved", "Reserved", "Reserved"},
  {"No", "Yes"},
  {"No", "Yes"},
};

// USB PD 3.1 Cert Stat VDO (Section 6.4.4.3.1.2)
const struct vdo_field pd3p1_cert_stat_fields[] = {
  {"XID", 1, 0, 0xffffffff},
};
const char *pd3p1_cert_stat_field_desc[][MAX_FIELDS] = {
  {NULL},
};

// USB PD 3.1 Product VDO (Section 6.4.4.3.1.3)
const struct vdo_field pd3p1_product_fields[] = {
  {"bcdDevice", 1, 0, 0xffff},
  {"USB Product ID", 1, 16, 0xffff},
};
const char *pd3p1_product_field_desc[][MAX_FIELDS] = {
  {NULL},
  {NULL},
};

// USB PD 3.1 Passive Cable VDO (Section 6.4.4.3.1.6)
const struct vdo_field pd3p1_passive_cable_fields[] = {
  {"USB Highest Speed", 1, 0, 0x7},
  {"Reserved", 0, 3, 0x3},
  {"Vbus Current Handling", 1, 5, 0x3},
  {"Reserved", 0, 7, 0x3},
  {"Maximum Vbus Voltage", 1, 9, 0x3},
  {"Cable Termination Type", 1, 11, 0x3},
  {"Cable Latency", 1, 13, 0xf},
  {"EPR Mode Capable", 1, 17, 0x1},
  {"Type-C Connector to", 1, 18, 0x3},
  {"Reserved", 0, 20, 0x1},
  {"VDO version", 1, 21, 0x7},
  {"Firmware Version", 1, 24, 0xf},
  {"HW Version", 1, 28, 0xf},
};
const char *pd3p1_passive_cable_field_desc[][MAX_FIELDS] = {
  {"USB 2.0 Only", "USB 3.2 Gen1", "USB 3.2/USB4 Gen2", "USB4 Gen3", "Reserved", "Reserved", "Reserved", "Reserved"},
  {NULL},
  {"Reserved", "3A", "5A", "Reserved"},
  {NULL},
  {"20V", "30V (Deprecated)", "40V (Deprecated)", "50V"},
  {"Vconn Not Required", "Vconn Required", "Reserved", "Reserved"},
  {"Reserved", "<10ns (~1m)", "10ns to 20ns (~2m)", "20ns to 30ns (~3m)", "30ns to 40ns (~4m)", "40ns to 50ns (~5m)", "50ns to 60ns (~6m)", "60ns to 70ns (~7m)", " 70ns (>~7m)", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"},
  {"No", "Yes"},
  {"Reserved", "Reserved", "USB Type-C", "Captive"},
  {NULL},
  {"Version 1.0", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"},
  {NULL},
  {NULL},
};

// USB PD 3.1 Active Cable VDO1/VDO2 (Section 6.4.4.3.1.7)
const struct vdo_field pd3p1_active_cable_vdo1_fields[] = {
  {"USB Highest Speed", 1, 0, 0x7},
  {"SOP'' Controller Present", 1, 3, 0x1},
  {"Vbus Through Cable", 1, 4, 0x1},
  {"Vbus Current Handling", 1, 5, 0x3},
  {"SBU Type", 1, 7, 0x1},
  {"SBU Supported", 1, 8, 0x1},
  {"Maximum Vbus Voltage", 1, 9, 0x3},
  {"Cable Termination Type", 1, 11, 0x3},
  {"Cable Latency", 1, 13, 0xf},
  {"EPR Mode Capable", 1, 17, 0x1},
  {"USB Type-C to", 1, 18, 0x3},
  {"Reserved", 0, 20, 0x1},
  {"VDO version", 1, 21, 0x7},
  {"Firmware Version", 1, 24, 0xf},
  {"HW Version", 1, 28, 0xf},
};
const char *pd3p1_active_cable_vdo1_field_desc[][MAX_FIELDS] = {
  {"USB 2.0 Only", "USB 3.2 Gen1", "USB 3.2/USB4 Gen2", "USB4 Gen3", "Reserved", "Reserved", "Reserved", "Reserved"},
  {"No", "Yes"},
  {"No", "Yes"},
  {"USB Type-C Default Current", "3A", "5A", "Reserved"},
  {"SBU is passive", "SBU is active"},
  {"SBU connections supported", "SBU connections are not supported"},
  {"20V", "30V", "40V", "50V"},
  {"Reserved", "Reserved", "One end active, one end passive, Vconn required", "Both ends active, Vconn required"},
  {"Reserved", "<10ns (~1m)", "10ns to 20ns (~2m)", "20ns to 30ns (~3m)", "30ns to 40ns (~4m)", "40ns to 50ns (~5m)", "50ns to 60ns (~6m)", "60ns to 70ns (~7m)", "1000b –1000ns (~100m)", "1001b –2000ns (~200m)", "1010b – 3000ns (~300m)", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"},
  {"No", "Yes"},
  {"Reserved", "Reserved", "USB Type-C", "Captive"},
  {NULL},
  {"Reserved", "Reserved", "Reserved", "Version 1.3", "Reserved", "Reserved", "Reserved", "Reserved"},
  {NULL},
  {NULL},
};
const struct vdo_field pd3p1_active_cable_vdo2_fields[] = {
  {"USB Gen", 1, 0, 0x1},
  {"Reserved", 0, 1, 0x1},
  {"Optically Isolated Active Cable", 1, 2, 0x1},
  {"USB Lanes Supported", 1, 3, 0x1},
  {"USB 3.2 Supported", 1, 4, 0x1},
  {"USB 2.0 Supported", 1, 5, 0x1},
  {"USB 2.0 Hub Hops Consumed", 1, 6, 0x3},
  {"USB4 Supported", 1, 8, 0x1},
  {"Active element", 1, 9, 0x1},
  {"Physical connection", 1, 10, 0x1},
  {"U3 to U0 transition mode", 1, 11, 0x1},
  {"U3/Cld Power", 1, 12, 0x7},
  {"Reserved", 0, 15, 0x1},
  {"Shutdown Tempurature", 1, 16, 0xff},
  {"Maximum Operating Tempurature", 1, 24, 0xff},
};
const char *pd3p1_active_cable_vdo2_field_desc[][MAX_FIELDS] = {
  {"Gen 1", "Gen 2 or higher"},
  {NULL},
  {"No", "Yes"},
  {"One Lane", "Two Lanes"},
  {"USB 3.2 SuperSpeed supported", "USB 3.2 SuperSpeed not supported"},
  {"USB 2.0 supported", "USB 2.0 not supported"},
  {NULL},
  {"USB4 Supported", "USB4 Not Supported"},
  {"Active Redriver", "Active Retimer"},
  {"Copper", "Optical"},
  {"U3 to U0 direct", "U3 to U0 through U35"},
  {">10mW", "5-10mW", "1-5mW", "0.5-1mW", "0.2-0.5mW", "50-200uW", "<50uW", "Reserved"},
  {NULL},
  {NULL},
  {NULL},
};

// USB PD 3.0 VPD VDO (Section 6.4.4.3.1.9)
const struct vdo_field pd3p1_vpd_fields[] = {
  {"Charge Through Support", 1, 0, 0x1},
  {"Ground Impedance", 1, 1, 0x3f},
  {"Vbus Impedance", 1, 7, 0x3f},
  {"Reserved", 0, 13, 0x3},
  {"Charge Through Current Support", 1, 14, 0x1},
  {"Maximum Vbus Voltage", 1, 15, 0x3},
  {"Reserved", 0, 17, 0xf},
  {"VDO Version", 1, 21, 0x7},
  {"Firmware Version", 1, 24, 0xf},
  {"HW Version", 1, 28, 0xf},
};
const char *pd3p1_vpd_field_desc[][MAX_FIELDS] = {
  {"No","Yes"},
  {NULL},
  {NULL},
  {NULL},
  {"3A capable", "5A capable"},
  {"20V", "30V (Deprecated)", "40V (Deprecated)", "50V (Deprecated)"},
  {NULL},
  {"Version 1.0", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"},
  {NULL},
  {NULL},
};

// USB PD 3.1 UFP VDO (Section 6.4.4.3.1.4)
const struct vdo_field pd3p1_ufp_fields[] = {
  {"USB Highest Speed", 1, 0, 0x7},
  {"Alternate Modes", 1, 3, 0x7},
  {"Vbus Required", 1, 6, 0x1},
  {"Vconn Required", 1, 7, 0x1},
  {"Vconn Power", 1, 8, 0x7},
  {"Reserved", 0, 11, 0x7ff},
  {"Connector Type", 1, 22, 0x3},
  {"Device Capability", 1, 24, 0xf},
  {"Reserved", 0, 28, 0x1},
  {"UFP VDO Version", 1, 29, 0x7},
};
const char *pd3p1_ufp_field_desc[][MAX_FIELDS] = {
  {"USB 2.0 only", "USB 3.2 Gen1", "USB 3.2/USB4 Gen2", "USB4 Gen3", "Reserved", "Reserved", "Reserved", "Reserved"},
  {NULL},
  {"Yes", "No"},
  {"No", "Yes"},
  {"1W", "1.5W", "2W", "3W", "4W", "5W", "6W", "Reserved"},
  {NULL},
  {NULL},
  {NULL},
  {NULL},
  {"Reserved", "Reserved", "Reserved", "Version 1.3", "Reserved", "Reserved", "Reserved", "Reserved"},
};

// USB PD 3.1 DFP VDO (Section 6.4.4.3.1.5)
const struct vdo_field pd3p1_dfp_fields[] = {
  {"Port Number", 1, 0, 0x1f},
  {"Reserved", 0, 5, 0x1ffff},
  {"Connector Type", 1, 22, 0x3},
  {"Host Capability", 1, 24, 0x7},
  {"Reserved", 0, 27, 0x3},
  {"DFP VDO Version", 1, 29, 0x7},
};
const char *pd3p1_dfp_field_desc[][MAX_FIELDS] = {
  {NULL},
  {NULL},
  {NULL},
  {NULL},
  {NULL},
  {"Version 1.0", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved", "Reserved"},
};


struct libtypec_capabiliy_data get_cap_data;
struct libtypec_connector_cap_data conn_data;
struct libtypec_connector_status conn_sts;
struct libtypec_cable_property cable_prop;
union libtypec_discovered_identity id;

struct altmode_data am_data[64];
char *session_info[LIBTYPEC_SESSION_MAX_INDEX];

// TODO: Add comments describing each function
// Used to determine a cable plug's product type
enum product_type get_cable_product_type(short rev, uint32_t id);

// Used to determine a partner's product type
enum product_type get_partner_product_type(short rev, uint32_t id);

//print_vdo will expand and print a VDO
void print_vdo(uint32_t vdo, int num_fields, struct vdo_field vdo_fields[], char *vdo_field_desc[][MAX_FIELDS]);

void print_session_info();

void print_ppm_capability(struct libtypec_capabiliy_data ppm_data);

void print_conn_capability(struct libtypec_connector_cap_data conn_data);

void print_cable_prop(struct libtypec_cable_property cable_prop, int conn_num);

void print_alternate_mode_data(int recipient, int num_mode, struct altmode_data *am_data);

void print_identity_data(int recipient, union libtypec_discovered_identity id);

void lstypec_print(char *val, int type);

