/* Copyright (c) 2020, Samsung Electronics Co., Ltd.
   All Rights Reserved. */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:
   
   - Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
   
   - Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.
   
   - Neither the name of the copyright owner, nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.
   
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef _EVEY_ARGS_H_
#define _EVEY_ARGS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EVEY_ARGS_VAL_TYPE_MANDATORY       (1<<0) /* mandatory or not */
#define EVEY_ARGS_VAL_TYPE_NONE            (0<<1) /* no value */
#define EVEY_ARGS_VAL_TYPE_INTEGER         (10<<1) /* integer type value */
#define EVEY_ARGS_VAL_TYPE_STRING          (20<<1) /* string type value */

#define EVEY_ARGS_GET_CMD_OPT_VAL_TYPE(x)  ((x) & ~EVEY_ARGS_VAL_TYPE_MANDATORY)

#define EVEY_ARGS_NO_KEY                   (127)
#define EVEY_ARGS_KEY_LONG_CONFIG          "config"

#define MAX_NUM_CONF_FILES 16


typedef struct _EVEY_ARGS_OPTION
{
    char   key; /* option keyword. ex) -f */
    char   key_long[32]; /* option long keyword, ex) --file */
    int    val_type; /* value type */
    int  * flag; /* flag to setting or not */
    void * val; /* actual value */
    char   desc[512]; /* description of option */
} EVEY_ARGS_OPTION;


static int evey_args_search_long_arg(EVEY_ARGS_OPTION * opts, const char * argv)
{
    int oidx = 0;
    EVEY_ARGS_OPTION * o;

    o = opts;

    while(o->key != 0)
    {
        if(!strcmp(argv, o->key_long))
        {
            return oidx;
        }
        oidx++;
        o++;
    }
    return -1;
}


static int evey_args_search_short_arg(EVEY_ARGS_OPTION * ops, const char argv)
{
    int oidx = 0;
    EVEY_ARGS_OPTION * o;

    o = ops;

    while(o->key != 0)
    {
        if(o->key != EVEY_ARGS_NO_KEY && o->key == argv)
        {
            return oidx;
        }
        oidx++;
        o++;
    }
    return -1;
}

static int evey_args_read_value(EVEY_ARGS_OPTION * ops, const char * argv)
{
    if(argv == NULL) return -1;
    if(argv[0] == '-' && (argv[1] < '0' || argv[1] > '9')) return -1;

    switch(EVEY_ARGS_GET_CMD_OPT_VAL_TYPE(ops->val_type))
    {
        case EVEY_ARGS_VAL_TYPE_INTEGER:
            *((int*)ops->val) = atoi(argv);
            break;

        case EVEY_ARGS_VAL_TYPE_STRING:
            strcpy((char*)ops->val, argv);
            break;

        default:
            return -1;
    }
    return 0;
}

static int evey_args_get_help(EVEY_ARGS_OPTION * ops, int idx, char * help)
{
    int optional = 0;
    char vtype[32];
    EVEY_ARGS_OPTION * o = ops + idx;

    switch(EVEY_ARGS_GET_CMD_OPT_VAL_TYPE(o->val_type))
    {
        case EVEY_ARGS_VAL_TYPE_INTEGER:
            strcpy(vtype, "INTEGER");
            break;
        case EVEY_ARGS_VAL_TYPE_STRING:
            strcpy(vtype, "STRING");
            break;
        case EVEY_ARGS_VAL_TYPE_NONE:
        default:
            strcpy(vtype, "FLAG");
            break;
    }
    optional = !(o->val_type & EVEY_ARGS_VAL_TYPE_MANDATORY);

    if(o->key != EVEY_ARGS_NO_KEY)
    {
        sprintf(help, "  -%c, --%s [%s]%s\n    : %s", o->key, o->key_long,
                vtype, (optional) ? " (optional)" : "", o->desc);
    }
    else
    {
        sprintf(help, "  --%s [%s]%s\n    : %s", o->key_long,
                vtype, (optional) ? " (optional)" : "", o->desc);
    }
    return 0;
}

static int evey_args_get_arg(EVEY_ARGS_OPTION * ops, int idx, char * result)
{
    char vtype[32];
    char value[512];
    EVEY_ARGS_OPTION * o = ops + idx;

    switch(EVEY_ARGS_GET_CMD_OPT_VAL_TYPE(o->val_type))
    {
        case EVEY_ARGS_VAL_TYPE_INTEGER:
            strcpy(vtype, "INTEGER");
            sprintf(value, "%d", *((int*)o->val));
            break;

        case EVEY_ARGS_VAL_TYPE_STRING:
            strcpy(vtype, "STRING");
            sprintf(value, "%s", (char*)o->val);
            break;

        case EVEY_ARGS_VAL_TYPE_NONE:
        default:
            strcpy(vtype, "FLAG");
            sprintf(value, "%d", *((int*)o->val));
            break;
    }

    if(o->flag != NULL && (*o->flag))
    {
        strcat(value, " (SET)");
    }
    else
    {
        strcat(value, " (DEFAULT)");
    }

    sprintf(result, "  -%c(--%s) = %s\n    : %s", o->key, o->key_long,
            value, o->desc);

    return 0;

}

static int evey_parse_cfg(FILE * fp, EVEY_ARGS_OPTION * ops)
{
    char * parser;
    char line[256] = "", tag[50] = "", val[256] = "";
    int oidx;

    while(fgets(line, sizeof(line), fp))
    {
        parser = strchr(line, '#');
        if(parser != NULL) *parser = '\0';

        parser = strtok(line, "= \t");
        if(parser == NULL) continue;
        strcpy(tag, parser);

        parser = strtok(NULL, "=\n");
        if(parser == NULL) continue;
        strcpy(val, parser);

        oidx = evey_args_search_long_arg(ops, tag);
        if(oidx < 0) continue;

        if(EVEY_ARGS_GET_CMD_OPT_VAL_TYPE(ops[oidx].val_type) !=
           EVEY_ARGS_VAL_TYPE_NONE)
        {
            if(evey_args_read_value(ops + oidx, val)) continue;
        }
        else
        {
            *((int*)ops[oidx].val) = 1;
        }
        *ops[oidx].flag = 1;
    }
    return 0;
}


static int evey_parse_cmd(int argc, const char * argv[], EVEY_ARGS_OPTION * ops,
                          int * idx)
{
    int    aidx; /* arg index */
    int    oidx; /* option index */

    aidx = *idx + 1;

    if(aidx >= argc || argv[aidx] == NULL) goto NO_MORE;
    if(argv[aidx][0] != '-') goto ERR;

    if(argv[aidx][1] == '-')
    {
        /* long option */
        oidx = evey_args_search_long_arg(ops, argv[aidx] + 2);
        if(oidx < 0) goto ERR;
    }
    else if(strlen(argv[aidx]) == 2)
    {
        /* short option */
        oidx = evey_args_search_short_arg(ops, argv[aidx][1]);
        if(oidx < 0) goto ERR;
    }
    else
    {
        goto ERR;
    }

    if(EVEY_ARGS_GET_CMD_OPT_VAL_TYPE(ops[oidx].val_type) !=
       EVEY_ARGS_VAL_TYPE_NONE)
    {
        if(aidx + 1 >= argc) goto ERR;
        if(evey_args_read_value(ops + oidx, argv[aidx + 1])) goto ERR;
        *idx = *idx + 1;
    }
    else
    {
        *((int*)ops[oidx].val) = 1;
    }
    *ops[oidx].flag = 1;
    *idx = *idx + 1;

    return ops[oidx].key;


NO_MORE:
    return 0;

ERR:
    return -1;
}

static int evey_args_parse_all(int argc, const char * argv[],
                               EVEY_ARGS_OPTION * ops)
{
    /*const char* n_argv[] = { "eveya_encoder", "--config", "./cfg/encoder_randomaccess.cfg", "-i", "../../data/mobile_cif.yuv", "-w", "352", "-h", "288", "-d", "8","-z", "30", "-q", "37", "-f", "300", "-o", "mobile_37.bin", "--nn_base_port", "8032" };
    argv = n_argv;
    argc = 21;*/

    int i, ret = 0, idx = 0;
    EVEY_ARGS_OPTION *o;
    const char *fname_cfg = NULL;
    FILE *fp;
    int num_configs = 0;
    int pos_conf_files[MAX_NUM_CONF_FILES];
    memset(&pos_conf_files, -1, sizeof(int) * MAX_NUM_CONF_FILES);


    /* config file parsing */
    for(i = 1; i < argc; i++)
    {
        if(!strcmp(argv[i], "--"EVEY_ARGS_KEY_LONG_CONFIG))
        {
            if(i + 1 < argc)
            {
                pos_conf_files[num_configs++] = i + 1;
            }
        }
    }

    for(int i = 0; i < num_configs; i++)
    {
        fname_cfg = argv[pos_conf_files[i]];
        if(fname_cfg)
        {
            fp = fopen(fname_cfg, "r");
            if(fp == NULL)
            {
                return -1; /* config file error */
            }

            if(evey_parse_cfg(fp, ops))
            {
                fclose(fp);
                return -1; /* config file error */
            }
            fclose(fp);
        }
    }

    /* command line parsing */
    while(1)
    {
        ret = evey_parse_cmd(argc, argv, ops, &idx);
        if(ret == 0)
        {
            break;
        }
        else if(ret < 0)
        {
            printf("unknown argument: %s\n", argv[idx + 1]);
            return ret;
        }
    }

    /* check mandatory argument */
    o = ops;

    while(o->key != 0)
    {
        if(o->val_type & EVEY_ARGS_VAL_TYPE_MANDATORY)
        {
            if(*o->flag == 0)
            {
                /* not filled all mandatory argument */
                return o->key;
            }
        }
        o++;
    }
    return ret;
}

static int evey_args_parse_int_x_int(char * str, int * num0, int * num1)
{
    char str0_t[64];
    int i, cnt0, cnt1;
    char * str0, *str1 = NULL;

    str0 = str;
    cnt1 = (int)strlen(str);

    /* find 'x' */
    for(i = 0; i < (int)strlen(str); i++)
    {
        if(str[i] == 'x' || str[i] == 'X')
        {
            str1 = str + i + 1;
            cnt0 = i;
            cnt1 = cnt1 - cnt0 - 1;
            break;
        }
    }

    /* check malformed data */
    if(str1 == NULL || cnt0 == 0 || cnt1 == 0) return -1;

    for(i = 0; i < cnt0; i++)
    {
        if(str0[i] < 0x30 || str0[i] > 0x39) return -1; /* not a number */
    }
    for(i = 0; i < cnt1; i++)
    {
        if(str1[i] < 0x30 || str1[i] > 0x39) return -1; /* not a number */
    }


    strncpy(str0_t, str0, cnt0);
    str0_t[cnt0] = '\0';

    *num0 = atoi(str0_t);
    *num1 = atoi(str1);

    return 0;
}

#ifdef __cplusplus
}
#endif

#endif /*_EVEY_ARGS_H_ */
