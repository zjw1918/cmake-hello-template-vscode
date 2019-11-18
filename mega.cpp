#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mega.h"
#include "parse_spo_pr/stages.h"

void parse(char *buf, int buf_len)
{
    spo2_analysis_t *res = (spo2_analysis_t *)malloc(sizeof(spo2_analysis_t));
    if (res != NULL)
    {
        memset(res, 0, sizeof(spo2_analysis_t));
        parse_pr_spo2(buf, buf_len, res);

        printf("duration: %d\n", res->duration);
        printf("timeStart: %d\n", res->time_start);
        printf("bin_start_sec: %d\n", res->bin_start_sec);
        printf("bin_stop_sec: %d\n", res->bin_stop_sec);

        free(res);
    }
}