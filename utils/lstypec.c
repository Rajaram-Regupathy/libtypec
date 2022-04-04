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
#include "lstypec.h"
#include "names.h"
#include <stdlib.h>
#include <stdint.h>

struct libtypec_capabiliy_data get_cap_data;
struct libtypec_connector_cap_data conn_data;
struct libtypec_connector_status conn_sts;
struct libtypec_cable_property cable_prop;
union libtypec_discovered_identity id;

struct altmode_data am_data[64];
char *session_info[LIBTYPEC_SESSION_MAX_INDEX];
int verbose = 1;

enum product_type get_cable_product_type(short rev, uint32_t id)
{
  // Decode cable product type
  if (rev == 0x20)
  {
    // USB PD 2.0 cable
    if ((id & pd_ufp_product_type_mask) == pd2p0_passive_cable)
      return product_type_pd2p0_passive_cable;
    else if ((id & pd_ufp_product_type_mask) == pd2p0_active_cable)
      return product_type_pd2p0_active_cable;
    else
      return product_type_other;
  }
  else if (rev == 0x30)
  {
    // USB PD 3.0 cable
    if ((id & pd_ufp_product_type_mask) == pd3p0_passive_cable)
      return product_type_pd3p0_passive_cable;
    else if ((id & pd_ufp_product_type_mask) == pd3p0_active_cable)
      return product_type_pd3p0_active_cable;
    else
      return product_type_other;
  }
  else if (rev == 0x31)
  {
    // USB PD 3.1 cable
    if ((id & pd_ufp_product_type_mask) == pd3p1_passive_cable)
      return product_type_pd3p1_passive_cable;
    else if ((id & pd_ufp_product_type_mask) == pd3p1_active_cable)
      return product_type_pd3p1_active_cable;
    else if ((id & pd_ufp_product_type_mask) == pd3p1_vpd)
      return product_type_pd3p1_vpd;
    else
      return product_type_other;
  }
  return product_type_other;
}

enum product_type get_partner_product_type(short rev, uint32_t id)
{
  // Decode partner product type
  if (rev == 0x20)
  {
    // USB PD 2.0 partner
    if ((id & pd_ufp_product_type_mask) == pd2p0_ama)
      return product_type_pd2p0_ama;
    else
      return product_type_other;
  }
  else if (rev == 0x30)
  {
    // USB PD 3.0 partner
    char ufp_supported = 0;
    if ((id & pd_ufp_product_type_mask) == pd3p0_hub)
      ufp_supported = 1;
    else if ((id & pd_ufp_product_type_mask) == pd3p0_peripheral)
      ufp_supported = 1;
    else if ((id & pd_ufp_product_type_mask) == pd3p0_ama)
      return product_type_pd3p0_ama;
    else if ((id & pd_ufp_product_type_mask) == pd3p0_vpd)
      return product_type_pd3p0_vpd;

    char dfp_supported = 0;
    if ((id & pd_dfp_product_type_mask) == pd3p0_dfp_hub)
      ufp_supported = 1;
    else if ((id & pd_dfp_product_type_mask) == pd3p0_dfp_host)
      ufp_supported = 1;
    else if ((id & pd_dfp_product_type_mask) == pd3p0_power_brick)
      ufp_supported = 1;

    if (ufp_supported && dfp_supported)
      return product_type_pd3p0_drd;
    else if (ufp_supported)
      return product_type_pd3p0_ufp;
    else if (dfp_supported)
      return product_type_pd3p0_dfp;
    else
      return product_type_other;
  }
  else if (rev == 0x31)
  {
    // USB PD 3.1 partner
    char ufp_supported = 0;
    if ((id & pd_ufp_product_type_mask) == pd3p1_hub)
      ufp_supported = 1;
    else if ((id & pd_ufp_product_type_mask) == pd3p1_peripheral)
      ufp_supported = 1;

    char dfp_supported = 0;
    if ((id & pd_dfp_product_type_mask) == pd3p1_dfp_hub)
      ufp_supported = 1;
    else if ((id & pd_dfp_product_type_mask) == pd3p1_dfp_host)
      ufp_supported = 1;
    else if ((id & pd_dfp_product_type_mask) == pd3p1_power_brick)
      ufp_supported = 1;

    if (ufp_supported && dfp_supported)
      return product_type_pd3p1_drd;
    else if (ufp_supported)
      return product_type_pd3p1_ufp;
    else if (dfp_supported)
      return product_type_pd3p1_dfp;
    else
      return product_type_other;
  }
  return product_type_other;
}

void print_vdo(uint32_t vdo, int num_fields, struct vdo_field vdo_fields[], char *vdo_field_desc[][MAX_FIELDS])
{
  for (int i = 0; i < num_fields; i++){
    if(!vdo_fields[i].print)
      continue;

    if(vdo_field_desc[i][0] != NULL)
       printf("\t\t\t%s\t:\t%s\n", vdo_fields[i].name, vdo_field_desc[i][((vdo >> vdo_fields[i].index) & vdo_fields[i].mask)]);
    else
       printf("\t\t\t%s\t:\t0x%0x\n", vdo_fields[i].name, ((vdo >> vdo_fields[i].index) & vdo_fields[i].mask));

  }
}

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

void print_alternate_mode_data(int recipient, int num_mode, struct altmode_data *am_data)
{

    if (recipient == AM_CONNECTOR)
    {
        for (int i = 0; i < num_mode; i++)
        {
            printf("\tLocal Modes %d:\n", i);
            printf("\t\tSVID\t:\t0x%04lx\n", am_data[i].svid);
            printf("\t\tVDO\t:\t0x%04lx\n", am_data[i].vdo);
        }
    }

    if (recipient == AM_SOP)
    {
        for (int i = 0; i < num_mode; i++)
        {
            printf("\tPartner Modes %d:\n", i);
            printf("\t\tSVID\t:\t0x%04lx\n", am_data[i].svid);
            printf("\t\tVDO\t:\t0x%04lx\n", am_data[i].vdo);
        }
    }

    if (recipient == AM_SOP_PR)
    {
        for (int i = 0; i < num_mode; i++)
        {
            printf("\tCable Plug Modes %d:\n", i);
            printf("\t\tSVID\t:\t0x%04lx\n", am_data[i].svid);
            printf("\t\tVDO\t:\t0x%04lx\n", am_data[i].vdo);
        }
    }
}

void print_identity_data(int recipient, union libtypec_discovered_identity id)
{
  char vendor_id[128];
  union id_header id_hdr_val;
  id_hdr_val.id_hdr = id.disc_id.id_header;

  if (recipient == AM_SOP)
  {
    printf("\tPartner Identity :\n");

    // Partner ID
    if (verbose)
    {
      // ID Header/Cert Stat/Product are base on revision
      switch (conn_data.partner_rev)
      {
        case 0x20:
          printf("\t\tID Header\t:\t0x%08lx\n", id.disc_id.id_header);
          get_vendor_string(vendor_id, sizeof(vendor_id), id_hdr_val.id_hdr_pd2p0.usb_vendor_id);
          print_vdo(((uint32_t) id.disc_id.id_header), 6, pd2p0_partner_id_header_fields, pd2p0_partner_id_header_field_desc);
          printf("\t\t\tDecoded VID\t:\t0x%04x (%s)\n", id_hdr_val.id_hdr_pd2p0.usb_vendor_id, (vendor_id == NULL ? "unknown" : vendor_id));
          printf("\t\tCert Stat\t:\t0x%08lx\n", id.disc_id.cert_stat);
          print_vdo(((uint32_t) id.disc_id.cert_stat), 1, pd2p0_cert_stat_fields, pd2p0_cert_stat_field_desc);
          printf("\t\tProduct\t\t:\t0x%08lx\n", id.disc_id.product);
          print_vdo(((uint32_t) id.disc_id.product), 2, pd2p0_product_fields, pd2p0_product_field_desc);
          break;
        case 0x30:
          printf("\t\tID Header\t:\t0x%08lx\n", id.disc_id.id_header);
          get_vendor_string(vendor_id, sizeof(vendor_id), id_hdr_val.id_hdr_pd3p0.usb_vendor_id);
          print_vdo(((uint32_t) id.disc_id.id_header), 7, pd3p0_partner_id_header_fields, pd3p0_partner_id_header_field_desc);
          printf("\t\t\tDecoded VID\t:\t0x%04x (%s)\n", id_hdr_val.id_hdr_pd3p0.usb_vendor_id, (vendor_id == NULL ? "unknown" : vendor_id));
          printf("\t\tCert Stat\t:\t0x%08lx\n", id.disc_id.cert_stat);
          print_vdo(((uint32_t) id.disc_id.cert_stat), 1, pd3p0_cert_stat_fields, pd3p0_cert_stat_field_desc);
          printf("\t\tProduct\t\t:\t0x%08lx\n", id.disc_id.product);
          print_vdo(((uint32_t) id.disc_id.product), 2, pd3p0_product_fields, pd3p0_product_field_desc);
          break;
        case 0x31:
          printf("\t\tID Header\t:\t0x%08lx\n", id.disc_id.id_header);
          get_vendor_string(vendor_id, sizeof(vendor_id), id_hdr_val.id_hdr_pd3p1.usb_vendor_id);
          print_vdo(((uint32_t) id.disc_id.id_header), 8, pd3p1_partner_id_header_fields, pd3p1_partner_id_header_field_desc);
          printf("\t\t\tDecoded VID\t:\t0x%04x (%s)\n", id_hdr_val.id_hdr_pd3p1.usb_vendor_id, (vendor_id == NULL ? "unknown" : vendor_id));
          printf("\t\tCert Stat\t:\t0x%08lx\n", id.disc_id.cert_stat);
          print_vdo(((uint32_t) id.disc_id.cert_stat), 1, pd3p1_cert_stat_fields, pd3p1_cert_stat_field_desc);
          printf("\t\tProduct\t\t:\t0x%08lx\n", id.disc_id.product);
          print_vdo(((uint32_t) id.disc_id.product), 2, pd3p1_product_fields, pd3p1_product_field_desc);
          break;
        default:
          printf("\t\tID Header\t:\t0x%08lx\n", id.disc_id.id_header);
          printf("\t\tCert Stat\t:\t0x%08lx\n", id.disc_id.cert_stat);
          printf("\t\tProduct\t\t:\t0x%08lx\n", id.disc_id.product);
          break;
      }

      //Product Type VDOs based on product type
      enum product_type partner_product_type = get_partner_product_type(conn_data.partner_rev, ((uint32_t) id.disc_id.id_header));
      switch (partner_product_type)
      {
        case product_type_pd2p0_ama:
          printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 11, pd2p0_ama_fields, pd2p0_ama_field_desc);
          printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
          printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p0_ama:
          printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 8, pd3p0_ama_fields, pd3p0_ama_field_desc);
          printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
          printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p0_vpd:
          printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 10, pd3p0_vpd_fields, pd3p0_vpd_field_desc);
          printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
          printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p0_ufp:
          printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 6, pd3p0_ufp_vdo1_fields, pd3p0_ufp_vdo1_field_desc);
          printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo2), 6, pd3p0_ufp_vdo2_fields, pd3p0_ufp_vdo2_field_desc);
          printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p0_dfp:
          printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 5, pd3p0_dfp_fields, pd3p0_dfp_field_desc);
          printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
          printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p0_drd:
          printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 6, pd3p0_ufp_vdo1_fields, pd3p0_ufp_vdo1_field_desc);
          printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo2), 6, pd3p0_ufp_vdo2_fields, pd3p0_ufp_vdo2_field_desc);
          printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo3), 5, pd3p0_dfp_fields, pd3p0_dfp_field_desc);
          break;
        case product_type_pd3p1_ufp:
          printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 10, pd3p1_ufp_fields, pd3p1_ufp_field_desc);
          printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
          printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p1_dfp:
          printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 6, pd3p1_dfp_fields, pd3p1_dfp_field_desc);
          printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
          printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p1_drd:
          printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 10, pd3p1_ufp_fields, pd3p1_ufp_field_desc);
          printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
          printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo3), 6, pd3p1_dfp_fields, pd3p1_dfp_field_desc);
        default:
          printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
          printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
          printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
          break;
      }
    }
    else
    {
      printf("\t\tID Header\t:\t0x%08lx\n", id.disc_id.id_header);
      printf("\t\tCert Stat\t:\t0x%08lx\n", id.disc_id.cert_stat);
      printf("\t\tProduct\t\t:\t0x%08lx\n", id.disc_id.product);
      printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
      printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
      printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
    }
  }
  else if (recipient == AM_SOP_PR)
  {

    printf("\tCable Identity :\n");

    // Partner ID
    if (verbose)
    {
      // ID Header/Cert Stat/Product are base on revision
      switch (conn_data.cable_rev)
      {
        case 0x20:
          printf("\t\tID Header\t:\t0x%08lx\n", id.disc_id.id_header);
          get_vendor_string(vendor_id, sizeof(vendor_id), id_hdr_val.id_hdr_pd2p0.usb_vendor_id);
          print_vdo(((uint32_t) id.disc_id.id_header), 6, pd2p0_cable_id_header_fields, pd2p0_cable_id_header_field_desc);
          printf("\t\t\tDecoded VID\t:\t0x%04x (%s)\n", id_hdr_val.id_hdr_pd2p0.usb_vendor_id, (vendor_id == NULL ? "unknown" : vendor_id));
          printf("\t\tCert Stat\t:\t0x%08lx\n", id.disc_id.cert_stat);
          print_vdo(((uint32_t) id.disc_id.cert_stat), 1, pd2p0_cert_stat_fields, pd2p0_cert_stat_field_desc);
          printf("\t\tProduct\t\t:\t0x%08lx\n", id.disc_id.product);
          print_vdo(((uint32_t) id.disc_id.product), 2, pd2p0_product_fields, pd2p0_product_field_desc);
          break;
        case 0x30:
          printf("\t\tID Header\t:\t0x%08lx\n", id.disc_id.id_header);
          get_vendor_string(vendor_id, sizeof(vendor_id), id_hdr_val.id_hdr_pd3p0.usb_vendor_id);
          print_vdo(((uint32_t) id.disc_id.id_header), 7, pd3p0_cable_id_header_fields, pd3p0_cable_id_header_field_desc);
          printf("\t\t\tDecoded VID\t:\t0x%04x (%s)\n", id_hdr_val.id_hdr_pd3p0.usb_vendor_id, (vendor_id == NULL ? "unknown" : vendor_id));
          printf("\t\tCert Stat\t:\t0x%08lx\n", id.disc_id.cert_stat);
          print_vdo(((uint32_t) id.disc_id.cert_stat), 1, pd3p0_cert_stat_fields, pd3p0_cert_stat_field_desc);
          printf("\t\tProduct\t\t:\t0x%08lx\n", id.disc_id.product);
          print_vdo(((uint32_t) id.disc_id.product), 2, pd3p0_product_fields, pd3p0_product_field_desc);
          break;
        case 0x31:
          printf("\t\tID Header\t:\t0x%08lx\n", id.disc_id.id_header);
          get_vendor_string(vendor_id, sizeof(vendor_id), id_hdr_val.id_hdr_pd3p1.usb_vendor_id);
          print_vdo(((uint32_t) id.disc_id.id_header), 8, pd3p1_cable_id_header_fields, pd3p1_cable_id_header_field_desc);
          printf("\t\t\tDecoded VID\t:\t0x%04x (%s)\n", id_hdr_val.id_hdr_pd3p1.usb_vendor_id, (vendor_id == NULL ? "unknown" : vendor_id));
          printf("\t\tCert Stat\t:\t0x%08lx\n", id.disc_id.cert_stat);
          print_vdo(((uint32_t) id.disc_id.cert_stat), 1, pd3p1_cert_stat_fields, pd3p1_cert_stat_field_desc);
          printf("\t\tProduct\t\t:\t0x%08lx\n", id.disc_id.product);
          print_vdo(((uint32_t) id.disc_id.product), 2, pd3p1_product_fields, pd3p1_product_field_desc);
          break;
        default:
          printf("\t\tID Header\t:\t0x%08lx\n", id.disc_id.id_header);
          printf("\t\tCert Stat\t:\t0x%08lx\n", id.disc_id.cert_stat);
          printf("\t\tProduct\t\t:\t0x%08lx\n", id.disc_id.product);
          break;
      }

      //Product Type VDOs based on product type
      enum product_type cable_product_type = get_cable_product_type(conn_data.cable_rev, ((uint32_t) id.disc_id.id_header));
      switch (cable_product_type)
      {
        case product_type_pd2p0_passive_cable:
          printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 15, pd2p0_passive_cable_fields, pd2p0_passive_cable_field_desc);
          printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
          printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd2p0_active_cable:
          printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 15, pd2p0_active_cable_fields, pd2p0_active_cable_field_desc);
          printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
          printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p0_passive_cable:
          printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 13, pd3p0_passive_cable_fields, pd3p0_passive_cable_field_desc);
          printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
          printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p0_active_cable:
          printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 15, pd3p0_active_cable_vdo1_fields, pd3p0_active_cable_vdo1_field_desc);
          printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo2), 15, pd3p0_active_cable_vdo2_fields, pd3p0_active_cable_vdo2_field_desc);
          printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p1_passive_cable:
          printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 13, pd3p1_passive_cable_fields, pd3p1_passive_cable_field_desc);
          printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
          printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p1_active_cable:
          printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 15, pd3p1_active_cable_vdo1_fields, pd3p1_active_cable_vdo1_field_desc);
          printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo2), 15, pd3p1_active_cable_vdo2_fields, pd3p1_active_cable_vdo2_field_desc);
          printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
          break;
        case product_type_pd3p1_vpd:
          printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
          print_vdo(((uint32_t) id.disc_id.product_type_vdo1), 10, pd3p1_vpd_fields, pd3p0_vpd_field_desc);
          printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
          printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
          break;
        default:
          printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
          printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
          printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
          break;
      }
    }
    else
    {
      printf("\t\tID Header\t:\t0x%08lx\n", id.disc_id.id_header);
      printf("\t\tCert Stat\t:\t0x%08lx\n", id.disc_id.cert_stat);
      printf("\t\tProduct\t\t:\t0x%08lx\n", id.disc_id.product);
      printf("\t\tProduct VDO 1\t:\t0x%08lx\n", id.disc_id.product_type_vdo1);
      printf("\t\tProduct VDO 2\t:\t0x%08lx\n", id.disc_id.product_type_vdo2);
      printf("\t\tProduct VDO 3\t:\t0x%08lx\n", id.disc_id.product_type_vdo3);
    }
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
