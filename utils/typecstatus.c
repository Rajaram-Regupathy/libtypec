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
 * @file typecstatus.c
 * @author Rajaram Regupathy <rajaram.regupathy@gmail.com>
 * @brief  Initial prototype. Performs status check of typec ports.
 */

#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include "../libtypec.h"
#include <stdlib.h>
#include "names.h"

struct libtypec_connector_status conn_sts;

struct bb_bos_header
{;
    unsigned char cap_desc_blength;
    unsigned char cap_desc_desc_type;
    unsigned char cap_desc_cap_type;
};

struct bb_bos_descritor
{
    unsigned char cap_desc_blength;
    unsigned char cap_desc_desc_type;
    unsigned char cap_desc_cap_type;
    unsigned char cap_desc_url_info;
    unsigned char cap_desc_num_aum;
    unsigned char cap_desc_pref_aum;
    unsigned char cap_desc_vconn_pwr[2];
    unsigned char cap_desc_bmconfig[32];
    unsigned char cap_desc_bcd_ver[2];
    unsigned char cap_desc_addln_fail_info;
    unsigned char cap_desc_rsvd;
    unsigned char cap_desc_aum_array_start;

};

int find_bb_bos_index(char *bb_data, int len)
{
    struct bb_bos_header *level;
    int index=5;

    while (index < len)
    {
        level = (struct bb_bos_header *) &bb_data[index];

        if(level->cap_desc_cap_type == 0x0D)
            return index;

        index = index + level->cap_desc_blength;

    }

    return -1;

}

static unsigned long get_dword_from_path(char *path)
{
	char buf[64];
	unsigned long dword;

	FILE *fp = fopen(path, "r");
	
	if (fp == NULL)
		return -1;

	if(fgets(buf, 64, fp) == NULL) {
		fclose(fp);
		return -1;
	}

	dword = strtoul(buf, NULL, 10);

	fclose(fp);

	return dword;
}

int typec_status_billboard()
{
        int ret,index=0;;
        unsigned char bb_data[512];

        ret = libtypec_get_bb_status(&index);

        printf("Detected %d BB device(s)\n=======================\n",index);

        if(index > 0)
        {
            for(int i=1;i<=index;i++)
            {
                int bb_loc = 0;

                ret = libtypec_get_bb_data(i,bb_data);

                if(ret < 0)
                {
                    printf("\tUnable to read Billboard device. Try running with higher privileges. Returned error code %d\n", ret);

                    return -1;
                }

                bb_loc = find_bb_bos_index(bb_data,ret);

                struct bb_bos_descritor *bb_bos_desc = (struct bb_bos_descritor *)&bb_data[bb_loc];

                printf("\tBillboard Device Version :  %x.%x\n",bb_bos_desc->cap_desc_bcd_ver[1],bb_bos_desc->cap_desc_bcd_ver[0]);

                printf("\tNumber of Alternate Mode :  %d\n",bb_bos_desc->cap_desc_num_aum);

                for(int x=0;x<bb_bos_desc->cap_desc_num_aum;x++)
                {
                    int idx = 0, k=0, j=0;
                    #define bmCONF_STR_ARR_MAX 4

                    char *bmconf_str_array[]= {"Unspecified Error","AUM not attempted","AUM attempt unsuccessful","AUM configuration successful","Undefined Configuration"};

                    if( (x !=0) && ((x % 4) == 0))
                    {
                        j++;
                        k=0;
                    }

                    idx = bb_bos_desc->cap_desc_bmconfig[j];

                    idx = (idx >> k) & 0x3;

                    k++;

                    idx =  idx < bmCONF_STR_ARR_MAX ? idx : bmCONF_STR_ARR_MAX;

                    char *aum = &bb_bos_desc->cap_desc_aum_array_start;

                    aum = aum + (x*4);

                    printf("\tAlternate Mode 0x%02X%02X in state :  %s\n",aum[1]&0xFF,aum[0]&0xFF,bmconf_str_array[idx]);
                }

            }
        }
	return 0;
}

int typecstatus_power_contract()
{
       unsigned long tdp, bst_pwr;
       int ret;
        struct libtypec_capability_data get_cap_data;

        // PPM Capabilities
        ret = libtypec_get_capability(&get_cap_data);

        if (ret < 0)
            printf("Unable to read typec capabilities\n==================\n");
        else {


            printf("USB-C Power Status\n==================\n");
            printf("Number of USB-C port(s): %d\n======================\n",get_cap_data.bNumConnectors);

            for(int i=0;i<get_cap_data.bNumConnectors;i++)
            {
                ret = libtypec_get_connector_status(i, &conn_sts);

                if(conn_sts.rdo)
                {

                    printf("\tUSB-C power contract Operating Power %ld W, with Max Power %ld W\n",(((conn_sts.rdo>>10)&0x3ff) * 250)/1000,((conn_sts.rdo&0x3ff) * 250)/1000);

                    tdp = get_dword_from_path("/sys/class/powercap/intel-rapl:0/constraint_0_power_limit_uw")/1000000;

                    bst_pwr = get_dword_from_path("/sys/class/powercap/intel-rapl:0/constraint_1_power_limit_uw")/1000000;

                    printf("\tCharging System with TDP %ld W, with boost power requirement of %ld W\n",tdp,bst_pwr);
                }
                else
                    printf("\tNo Power Contract on port %d\n",i);        
            }
        }
	return 0;
}

/* Check all typec ports */
static int ro_flag;
char *session_info[LIBTYPEC_SESSION_MAX_INDEX];

int main (int argc, char **argv)
{
    int ret,index=0;

    if(argc == 1)
    {
	    printf("typecstatus - Check status of typec ports\n Usage:\t typecstatus --ro | --rb \n\
        --ro\t Run once to gather typec port status \n\t--rb\t Run as background and notify\n");
        exit(0);
    }

    static struct option options[] =
    {
        /* These options set a flag. */
        {"ro", no_argument,&ro_flag, 1},
    };

    ret = getopt_long (argc, argv, "", options, &index);

    if(ret != 0)    
        printf("typecstatus - Check status of typec ports\n Usage:\t typecstatus --all \n");

    names_init();

    ret = libtypec_init(session_info);

    if (ret < 0)
    {
        printf("Failed in Initializing libtypec\n");
        return -1;
    }

    if(ro_flag)
    {
        typecstatus_power_contract();

        typec_status_billboard();

    }
    names_exit();

}
