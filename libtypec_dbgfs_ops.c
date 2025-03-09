
/*
MIT License

Copyright (c) 2023 Rajaram Regupathy <rajaram.regupathy@gmail.com>

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
 * @file libtypec_dbgfs_ops.c
 * @author Rajaram Regupathy <rajaram.regupathy@gmail.com>
 * @brief Functions for libtypec debugfs based operations
 */

#include "libtypec_ops.h"
#include <dirent.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <ftw.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>

int fp_command;
int fp_response;
struct pollfd  pfds;

int get_ucsi_response(char *data)
{
	char c[64],i=0,j=0;
	 if(fp_response <=0)
	 	return -1;

	i=poll(&pfds,1,-1);
	if(i<0)
		return -1;
	j = read(fp_response, c,64);
	
	for(i=2;i<j;i++)
	{		
		if(c[i] > '9')
			data[i-2]=c[i]- 87;
		else
			data[i-2]=c[i]-'0';

	}
	lseek(fp_response,0,SEEK_SET);
	return j;
}
static int libtypec_dbgfs_init(char **session_info)
{

   	fp_command = open("/sys/kernel/debug/usb/ucsi/USBC000:00/command", O_WRONLY);
	
	if (fp_command <= 0)
		return -1;

   	fp_response = open("/sys/kernel/debug/usb/ucsi/USBC000:00/response",O_RDONLY);
	
	if (fp_response <= 0)
		return -1;

	pfds.fd = fp_response;
	pfds.events = POLLIN;
	
	return 0;
}

static int libtypec_dbgfs_exit(void)
{
	close(fp_command);
	close(fp_response);
    fp_command = -1;
    fp_response = -1;
	return 0;
}

static int libtypec_dbgfs_get_capability_ops(struct libtypec_capability_data *cap_data)
{
    int ret=-1;
	char buf[64];

    if(fp_command > 0)
    {
        ret = write(fp_command,"6",sizeof("6"));
 
        if(ret)
        {
            ret = get_ucsi_response(buf);

            if(ret<31)
                ret = -1;

			cap_data->bmAttributes = buf[24] << 28 | buf[25] << 24 | buf[26] << 20 | buf[27] << 16 | buf[28] << 12 | buf[29] <<8 | buf[30] << 4 | buf[31];	
			cap_data->bNumConnectors = buf[22] << 4 | buf[23] ;
			cap_data->bmOptionalFeatures = buf[16] << 20 | buf[17] << 16 | buf[18] << 12 | buf[19] << 8 | buf[20] << 4 | buf[21] ;
			cap_data->bNumAltModes = buf[14] << 4 | buf[15] ;
			cap_data->bcdBCVersion = buf[8] << 12 | buf[9] << 8 | buf[10] << 4 | buf[11] ;
			cap_data->bcdPDVersion = buf[4] << 12 | buf[5] << 8 | buf[6] << 4 | buf[7] ;
			cap_data->bcdTypeCVersion = buf[0] << 12 | buf[1] << 8 | buf[2] << 4 | buf[3] ;

        }
    }
	
	return ret;
}

static int libtypec_dbgfs_get_conn_capability_ops(int conn_num, struct libtypec_connector_cap_data *conn_cap_data)
{
    int ret=-1;
	unsigned char buf[64];


    if(fp_command > 0)
    {
		snprintf(buf, sizeof(buf), "%d", (conn_num+1)<<16|7);
		
        ret = write(fp_command,buf,sizeof(buf));
	
        if(ret)
        {
            ret = get_ucsi_response(buf);
            if(ret<31)
                ret = -1;
			conn_cap_data->opr_mode = buf[28] << 12 | buf[29] <<8 | buf[30] << 4 | buf[31];	
	}
    }
    
    return ret;
}
static int libtypec_dbgfs_get_alternate_modes(int recipient, int conn_num, struct altmode_data *alt_mode_data)
{
	union get_am_cmd
	{
		unsigned long long cmd_val;
		struct{
			char cmd;
			char len;
			char rcp;
			char con;
			char offset;
			char num_am;
		}s;
	}am_cmd;

	int ret=-1,i=0;
	unsigned char buf[64];

	if(fp_command > 0)
	{
		do
		{
			am_cmd.s.cmd = 0xc;
			am_cmd.s.len = 0;
			am_cmd.s.rcp = recipient;
			am_cmd.s.con = conn_num+1;
			am_cmd.s.offset = i;
			am_cmd.s.num_am = 0;

			snprintf(buf, sizeof(buf), "%lld", am_cmd.cmd_val);
			
			ret = write(fp_command,buf,sizeof(buf));

			if(ret)
			{
				ret = get_ucsi_response(buf);
				if(ret<31)
					return -1;
				
				alt_mode_data[i].svid 	 = buf[28] << 12 | buf[29] <<8 | buf[30] << 4 | buf[31];
				alt_mode_data[i].vdo 	 = buf[24] << 12 | buf[25] <<8 | buf[26] << 4 | buf[27];	
				
				if(alt_mode_data[i].svid == 0
				|| (i > 0 && alt_mode_data[i].svid == alt_mode_data[i-1].svid && alt_mode_data[i].vdo == alt_mode_data[i-1].vdo))
					break;
			}
			i++;
		}while(1);

	}
	
	return i;
}

static int libtypec_dbgfs_get_pdos_ops(int conn_num, int partner, int offset, int *num_pdo, int src_snk, int type, unsigned int *pdo_data)
{
	union get_pdo_cmd
	{
		unsigned long long cmd_val;
		struct{
			unsigned int cmd	: 8;
			unsigned int len	: 8;
			unsigned int con	: 7;
			unsigned int ptnr	: 1;
			unsigned int offset	: 8;
			unsigned int num	: 2;
			unsigned int src_snk	: 1;
			unsigned int type	: 2;
		}s;
	}pdo_cmd;

	int ret=-1,i=0;
	unsigned char buf[64];

	if(fp_command > 0)
	{
		do
		{
			pdo_cmd.s.cmd = 0x10;
			pdo_cmd.s.len = 0;
			pdo_cmd.s.con = conn_num+1;
			pdo_cmd.s.ptnr = partner;
			pdo_cmd.s.offset = i;
			pdo_cmd.s.num = 0;
			pdo_cmd.s.src_snk = src_snk;
			pdo_cmd.s.type = type;
			

			snprintf(buf, sizeof(buf), "%lld", pdo_cmd.cmd_val);
			//printf("cmd %lld \n",pdo_cmd.cmd_val);
			
			ret = write(fp_command,buf,sizeof(buf));

			if(ret)
			{
				ret = get_ucsi_response(buf);
				if(ret<31)
					return -1;
				pdo_data[i] = buf[24] << 28 | buf[25] << 24 | buf[26] << 20 | buf[27] << 16 | buf[28] << 12 | buf[29] <<8 | buf[30] << 4 | buf[31];					
				if(pdo_data[i] == 0)
					break;
			}
			i++;
		}while(1);

	}
	
	*num_pdo = i;
	return i;


}

const struct libtypec_os_backend libtypec_lnx_dbgfs_backend = {
	.init = libtypec_dbgfs_init,
	.exit = libtypec_dbgfs_exit,
	.get_capability_ops = libtypec_dbgfs_get_capability_ops,
	.get_conn_capability_ops = libtypec_dbgfs_get_conn_capability_ops,
	.get_alternate_modes = libtypec_dbgfs_get_alternate_modes,
	.get_cam_supported_ops = NULL,
	.get_current_cam_ops = NULL,
	.get_pdos_ops = libtypec_dbgfs_get_pdos_ops,
	.get_cable_properties_ops = NULL,
	.get_connector_status_ops = NULL,
	.get_pd_message_ops = NULL,
	.get_bb_status = NULL,
	.get_bb_data = NULL,
};
