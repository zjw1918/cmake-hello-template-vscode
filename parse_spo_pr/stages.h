#ifndef _STAGES_H_
#define _STAGES_H_

typedef float AlgData_t;
typedef unsigned char uint8_t;

#define STATUS_WAKE 0
#define STATUS_REM 2
#define STATUS_N1 3
#define STATUS_N2 4

#define STATUS_NOBODY 6

#define RESULT_SAVED_MINUTES (60*24)

#define MAX_HIST_LEN 600

#define MIN(x1,x2) ((x1) < (x2) ? (x1):(x2))
#define MAX(x1,x2) ((x1) > (x2) ? (x1):(x2))

typedef struct
{
	AlgData_t* AccM;
	AlgData_t* Spo2M;
	AlgData_t* HRvect;
}Proc_Para;

typedef struct
{
	int len;
	int timeStart;
	AlgData_t *Spo2;
	uint8_t *HeartRate;
	uint8_t *AccFlg;
//	uint8_t *MoveFlg;
}SAO2_InPara;

typedef struct
{
	int startpos;
	int len;
}SAO2_Event_RESULT;


typedef struct
{
	int duration; // ?????(s)
	int startpos;
	int endpos;
	int handonTotalTime; // ??????(s)
	int timeStart;// ??'????(s)
	float Spo2Avg; // ???????????(%) 
	float Spo2Min;// ???????????(%)
	int maxSpo2DownTime;// ?????????(s)

	int prAvg;// ???????(bpm)
	int prMax;// ???????
	int prMin;// ???????

	int diffThdLge3Cnts; // ????????
	float diffThdLge3Pr; // ???????

	int spo2Less95Time; // ???????? <95% ?????(s)
	int spo2Less90Time;
	int spo2Less85Time;
	int spo2Less80Time;
	int spo2Less70Time;
	int spo2Less60Time;

	float spo2Less95TimePercent; // ???????? <95% ????????(%)
	float spo2Less90TimePercent;
	float spo2Less85TimePercent;
	float spo2Less80TimePercent;
	float spo2Less70TimePercent;
	float spo2Less60TimePercent;

	int handOffArrlen;//??????????
	float *Spo2Arr; //????:duration
	unsigned char *prArr;//????:duration

	int *handOffArr;//????:handOffArrlen*2
}SAO2_STATIC_RESULT;

typedef struct
{
	SAO2_STATIC_RESULT Static;
	SAO2_Event_RESULT *SEvent;
	int MinuteCnt;
	uint8_t *status;
}SAO2_OutPara;

//extern SAO2_InPara InPara;
//extern SAO2_OutPara OutPara;
//extern Proc_Para ProcPara;

//extern void Proc_Schedule(SAO2_InPara *InPara, Proc_Para *Procpara, SAO2_OutPara *OutPara);
#define MAX_SLEEP_MINUTES (60*24)
#define MAX_SPO2_CNT 43200
#define MAX_SPO2_HANDOFF 1024
typedef struct {
	unsigned int start;
	unsigned int end;
} time_stamp_t;

typedef struct {
	int num;
	time_stamp_t time_stamp[1024];
} handoff_t;

typedef struct {
	unsigned int stage_N1_startime;
	unsigned int stage_endtime;
} app_input_t;

typedef struct {
	app_input_t app_define;

	/* peer device info */
	int version;	//data format version
	int data_type;
	int peer_data_length; //nrf52832's records flash size, multiple of data_block_size
	int data_block_size;		//stable 256
	int data_block_elenum;	//82

	/* parse result */
	int handon_total_time;
	float Spo2f[MAX_SPO2_CNT];	    //full flash is 132K, each tuple element is <spo2, pr, acc>, so spo2 occupied 1/3
	unsigned short spo2[MAX_SPO2_CNT];
	unsigned char pr[MAX_SPO2_CNT];	//full flash is 132K, each tuple element is <spo2, pr, acc>, so pr occupied 1/3
	unsigned char moveflag[MAX_SPO2_CNT];	// moveflag max flags is 132K/3
	unsigned char acc[MAX_SPO2_CNT];	//full flash is 132K, each tuple element is <spo2, pr, acc>, so acc occupied 1/3
	handoff_t handoff;
	unsigned int time[132 * 4];	//each 256 bytes has a time header, so 132K occupied 132*4
	int length_time;
	int length_spo2;
	char error_tips[1024];
	unsigned int time_start;
	unsigned int time_total;
	int pr_max;
	int pr_min;
	int pr_average;
	int spo2_min;
	//int spo2_min_hold_time;
	int spo2_average;
	int spo2_less95_time;
	int spo2_less90_time;
	int spo2_less85_time;
	int spo2_less80_time;
	int spo2_less70_time;
	int spo2_less60_time;
	float spo2_less95_time_percent;
	float spo2_less90_time_percent;
	float spo2_less85_time_percent;
	float spo2_less80_time_percent;
	float spo2_less70_time_percent;
	float spo2_less60_time_percent;
	//int diff_thd_lge4_cnts;	//Threshold difference is greater than or equal to 4% 
	int diff_thd_lge3_cnts;	//Threshold difference is greater than or equal to 3%
	//int diff_thd_lge2_cnts;	//Threshold difference is greater than or equal to 2%
	//int diff_thd_lge4_pr;
	float diff_thd_lge3_pr;
	//int diff_thd_lge2_pr;
	int max_spo2_down_time;
	int duration;
	unsigned char status[60 * 24];
	int length_status;
	unsigned int bin_start_sec;
	unsigned int bin_stop_sec;
	//int SAO2sp;
	//int SAO2ep;
	//int SAO2EventCnt;
}spo2_analysis_t;

void parse_pr_spo2(char* data_in, int data_len, void* pr_spo2_result);

#endif
