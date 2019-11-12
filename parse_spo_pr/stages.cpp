//#include <math.h>
#include <iostream> 
//#include <stdio>
#include "stages.h"
#include "dwt.h"

using namespace std;
using namespace splab;

SAO2_InPara InPara;
SAO2_OutPara OutPara;
Proc_Para ProcPara;

AlgData_t AccMSum[RESULT_SAVED_MINUTES];
AlgData_t Spo2MSum[RESULT_SAVED_MINUTES];
AlgData_t HrMSum[RESULT_SAVED_MINUTES];
AlgData_t HrMSumTh[RESULT_SAVED_MINUTES];
AlgData_t Spo2Bck[24 * 60 * 60];
uint8_t HeartRateBck[24 * 60 * 60];


AlgData_t WaveletProcbuf[300];
AlgData_t WaveletPara[RESULT_SAVED_MINUTES * 2];

SAO2_Event_RESULT Spo2EventVect[RESULT_SAVED_MINUTES];

uint8_t Sataus[RESULT_SAVED_MINUTES];

int HandOffVect[20];

AlgData_t get_Spo2_mean(AlgData_t *Spo2, int inlen, AlgData_t threshold)
{
	AlgData_t datasum = 0;
	int vldcnt = 0;

	for (int i = 0; i < inlen; i++)
	{
		if (Spo2[i] > threshold)
		{
			datasum += Spo2[i];
			vldcnt++;
		}
	}
	if (vldcnt > 0)
	{
		return datasum / vldcnt;
	}
	else
	{
		return 0;
	}
}

void averagepoint(AlgData_t *Spo2, int inlen, int step)
{
	for (int i = 0; i < inlen - step; i++)
	{
		if (Spo2[i] <= 37)
		{
			continue;
		}

		AlgData_t sum = 0;
		int vldcnt = 0;
		for (int j = i; j < i + step; j++)
		{
			if (Spo2[j] > 37)
			{
				sum += Spo2[j];
				vldcnt++;
			}
		}
		if (vldcnt > 0)
		{
			Spo2[i] = sum / vldcnt;
		}
	}
}

AlgData_t get_Hr_mean(AlgData_t *input, int len)
{
	if (len <= 0)
	{
		return 0;
	}

	AlgData_t sum = 0;
	AlgData_t *intmp = input;
	int cnt = 0;
	for (int i = 0; i < len; i++)
	{
		if (input[i] > 0)
		{
			sum += *intmp++;
			cnt++;
		}
	}
	if (cnt > 0)
	{
		return sum / cnt;
	}
	else
	{
		return 0;
	}
}


void trim_rem(uint8_t* status, int inlenM)
{
	int firstsleepid = 0;
	for (int i = 0; i < inlenM; i++)
	{
		if (status[i] != STATUS_WAKE)
		{
			firstsleepid = i;
			break;
		}
	}

	for (int i = firstsleepid; i < MIN(firstsleepid + 50,inlenM); i++)
	{
		if (status[i] == STATUS_REM)
		{
			status[i] = 1;
		}
	}

	int flg = 0;
	int tcnt = 0;
	for (int i = 0; i < inlenM; i++)
	{
		if (status[i] == STATUS_REM)
		{
			tcnt++;
			flg = 1;
		}
		else
		{
			if (flg == 1)
			{
				if (tcnt < 6)
				{
					for (int j = i-1; j >= i - tcnt; j--)
					{
						status[j] = status[i];
					}
				}
				flg = 0;
				tcnt = 0;
			}
		}
	}

	int spvect[10] = { 0,0,0,0,0,0,0,0,0,0 };
	int lenvect[10] = { 0,0,0,0,0,0,0,0,0,0 };
	int rflg = 0;
	int rcnt = 0;
	for (int i = 0; i < inlenM; i++)
	{
		if (status[i] == STATUS_REM)
		{
			lenvect[rcnt]++;
			if (rflg == 0)
			{
				spvect[rcnt] = i;
			}
			rflg = 1;
		}
		else
		{
			if (rflg == 1)
			{
				rcnt++;
				if (rcnt > 10)
				{
					printf("error!, remcnt = %d\n",rcnt);
					break;
				}
				rflg = 0;
			}
		}
	}

	int Revcnt = 5;
	if (rcnt > Revcnt)
	{
		int thlen = 0;
		for (int i = 0; i < rcnt; i++)
		{
			int tcnt = 0;
			for (int j = 0; j < rcnt; j++)
			{
				if (lenvect[j] > lenvect[i])
				{
					tcnt++;
				}
			}
			if (tcnt == Revcnt-1)
			{
				thlen = lenvect[i];
				break;
			}
		}

		for (int i = 0; i < rcnt; i++)
		{
			if (lenvect[i] < thlen)
			{
				for (int j = spvect[i]; j <= spvect[i] + lenvect[i]; j++)
				{
					status[j] = status[spvect[i] - 1];
				}
			}
		}
	}
}



void proc_Acc(uint8_t *Acc, int inlen)
{
	int L20cnt = 0;
	int L5cnt = 0;
	int baseval = 5;
	int threshold = 20;

	for (int i = 0; i < inlen; i++)
	{
		if (Acc[i] > baseval)
		{
			L5cnt++;
		}
		if (Acc[i] > threshold)
		{
			L20cnt++;
		}
	}

	if (L5cnt < inlen * 0.03)
	{
		baseval -= 1;
	}

	for (int i = 0; i < inlen; i++)
	{
		if (Acc[i] > baseval)
		{
			L5cnt++;
		}
	}

	if (L20cnt > L5cnt * 0.15)
	{
		threshold = 25;
	}
	else
	{
		if (L20cnt < L5cnt * 0.06)
		{
			threshold = 15;
		}
	}
	for (int i = 0; i < inlen; i++)
	{
		if (Acc[i] > threshold)
		{
			if (Acc[i] > 2 * threshold)
			{
				Acc[i] = 3;
			}
			else
			{
				Acc[i] = 2;
			}
		}
		else
		{
			if (Acc[i] < baseval)
			{
				Acc[i] = 0;
			}
			else
			{
				Acc[i] = 1;
			}
		}
	}
}

void get_Mfeature(AlgData_t* Spo2, int inlen, AlgData_t *Mmax, AlgData_t *MDec)
{
	*Mmax = 0;
	*MDec = 0;
	int mcnt = 0;
	int dcnt = 0;
	for (int i = 0; i < inlen; i += 60)
	{
		float tmpmin = 100;
		float tmpmax = 0;
		for (int j = i; j < i + 60; j++)
		{
			if (Spo2[j] > tmpmax)
			{
				tmpmax = Spo2[j];
			}
			if (Spo2[j] < tmpmin)
			{
				tmpmin = tmpmax;
			}
		}
		if (tmpmax > 90)
		{
			*Mmax += tmpmax;
			mcnt++;
		}
		if (tmpmax - tmpmin > 3 && tmpmax > 90)
		{
			*MDec += (tmpmax - tmpmin);
			dcnt++;
		}
	}
	if (mcnt > 0)
	{
		*Mmax = *Mmax / mcnt;
	}
	else
	{
		*Mmax = 88;
	}

	if (dcnt > 0)
	{
		*MDec = *MDec / dcnt;
	}
	else
	{
		*MDec = 0;
	}
}

void get_minaround(AlgData_t *Spo2, unsigned char *Acc, int lid, int rid, int proclen, int len, AlgData_t &minval)
{
	minval = 100;

	//proc left
	int i = lid - 1;
	while (i > 0)
	{
		if (Spo2[i] <= 37)
		{
			i--;
			continue;
		}

		AlgData_t tmax = Spo2[i];
		int tmaxid = i;
		for (int j = i - 1; j > MAX(0, i - 20); j--)
		{
			if (Spo2[j] > Spo2[i])
			{
				i = j;
				break;
			}
		}

		if (tmaxid == i)
		{
			for (int j = MAX(0, tmaxid - proclen); j < tmaxid; j++)
			{
				if (Spo2[j] < minval && Spo2[j] > 37 && Acc[j] == 0 && Acc[j + 1] == 0 && Acc[j + 2] == 0)
				{
					minval = Spo2[j];
				}
			}
			break;
		}
	}


	//proc right
	i = rid + 1;
	while (i < len)
	{
		if (Spo2[i] <= 37)
		{
			i++;
			continue;
		}
		AlgData_t tmax = Spo2[i];
		int tmaxid = i;
		for (int j = i + 1; j < MIN(len, i + 20); j++)
		{
			if (Spo2[j] > Spo2[i])
			{
				i = j;
				break;
			}
		}

		if (tmaxid == i)
		{
			for (int j = tmaxid; j < MIN(tmaxid + proclen, len - 3); j++)
			{
				if (Spo2[j] < minval && Spo2[j] > 37 && Acc[j] == 0 && Acc[j - 1] == 0 && Acc[j - 2] == 0)
				{
					minval = Spo2[j];
				}
			}
			break;
		}
	}
}

void get_minaround(AlgData_t *Spo2, int lid, int rid, int proclen, int len, AlgData_t &minval)
{
	minval = 100;

	//proc left
	int i = lid - 1;
	while (i > 0)
	{
		if (Spo2[i] <= 37)
		{
			i--;
			continue;
		}

		AlgData_t tmax = Spo2[i];
		int tmaxid = i;
		for (int j = i - 1; j > MAX(0, i - 30); j--)
		{
			if (Spo2[j] > Spo2[i])
			{
				i = j;
				break;
			}
		}

		if (tmaxid == i)
		{
			for (int j = MAX(0, tmaxid - proclen); j < tmaxid; j++)
			{
				if (Spo2[j] < minval && Spo2[j] > 37)
				{
					minval = Spo2[j];
				}
			}
			break;
		}
	}

	//proc right
	i = rid + 1;
	while (i < len)
	{
		if (Spo2[i] <= 37)
		{
			i++;
			continue;
		}
		AlgData_t tmax = Spo2[i];
		int tmaxid = i;
		for (int j = i + 1; j < MIN(len, i + 30); j++)
		{
			if (Spo2[j] > Spo2[i])
			{
				i = j;
				break;
			}
		}

		if (tmaxid == i)
		{
			for (int j = tmaxid; j < MIN(tmaxid + proclen, len - 1); j++)
			{
				if (Spo2[j] < minval && Spo2[j] > 37)
				{
					minval = Spo2[j];
				}
			}
			break;
		}
	}
}

int fix_Spo2(AlgData_t* Spo2, int inlen, int procid,int proclen, AlgData_t Vldth)
{
	int bgpoint = 0;
	AlgData_t bgmax = Vldth+1;
	for (int j = procid - 1; j > MAX(0, procid - proclen); j--)
	{
		if (Spo2[j] <= 37)
		{
			bgpoint = j;
			break;
		}
		if (Spo2[j] > Vldth)
		{
			bgpoint = j;
			bgmax = Spo2[bgpoint];
			break;
		}
	}
	if (bgpoint == 0)
	{
		AlgData_t tmpmax = 0;
		for (int j = procid - 1; j > MAX(0, procid - proclen); j--)
		{
			if (Spo2[j] > tmpmax)
			{
				tmpmax = Spo2[j];
				bgpoint = j;
			}
		}
	}

	int edpoint = inlen;
	AlgData_t edmax = Vldth + 1;
	for (int j = procid; j < MIN(inlen,procid + proclen); j++)
	{
		if (Spo2[j] <= 37)
		{
			edpoint = j;
			break;
		}
		if (Spo2[j] > Vldth)
		{
			edpoint = j;
			edmax = Spo2[edpoint];
			break;
		}
	}

	if (edpoint == inlen)
	{
		AlgData_t tmpmax = 0;
		for (int j = procid + 1; j < MIN(inlen, procid + proclen); j++)
		{
			if (Spo2[j] > tmpmax)
			{
				tmpmax = Spo2[j];
				edpoint = j;
			}
		}
	}

	if (edpoint - bgpoint >= 300)
	{
		for (int j = bgpoint + 1; j < edpoint; j++)
		{
			Spo2[j] = 0;
		}
	}
	else
	{
		AlgData_t stepval = (edmax - bgmax) / (edpoint - bgpoint);
		int tcnt = 1;
		for (int j = bgpoint + 1; j < edpoint; j++)
		{
			if (bgmax + tcnt*stepval > Spo2[j] && Spo2[j] < Vldth + 1)
			{
				Spo2[j] = bgmax + tcnt*stepval;
				tcnt++;
			}
			else
			{
				stepval = (edmax - Spo2[j]) / (edpoint - j);
				bgmax = Spo2[j];
				tcnt = 1;
			}
		}
	}

	return edpoint + 1;
}

void SmoothOffhand(AlgData_t* Spo2, unsigned char *Hr, AlgData_t VldSpo2Max, int inlen)
{
	int conlen = 0;
	int disconlen = 0;
	int conthrehold = 10;
	AlgData_t Offhandval = 37;
	for (int i = 0; i < inlen;)
	{
		if (Spo2[i] <= Offhandval)
		{
			i = fix_Spo2(Spo2, inlen, i, inlen, VldSpo2Max);
		}
		else
		{
			i++;
		}
	}
}

void smooth_Spo2(AlgData_t *Spo2, int inlen, AlgData_t DecSpo2th, AlgData_t VldSpo2Max)
{
	for (int i = 0; i < inlen - 3; i++)
	{
		if (Spo2[i] - Spo2[i + 1] >= DecSpo2th && Spo2[i] >= 97)
		{
			if (Spo2[i + 2] > Spo2[i] - 3 && Spo2[i + 3] > Spo2[i] - 3)
			{
				Spo2[i + 1] = (Spo2[i] + Spo2[i + 2]) / 2;
			}
			else
			{

				i = fix_Spo2(Spo2, inlen, i, 60, VldSpo2Max);
			}
		}
	}

#if 0
	for (int i = 0; i < inlen - 1; i++)
	{
		AlgData_t t1 = Spo2[i + 1] - Spo2[i];
		AlgData_t t2 = Spo2[i] * 0.1 - 12;

		if (t1 < t2 && Spo2[i + 1] > 37)
		{
			int marlen = 1;
			if (t1 < t2 - 1.5)
			{
				marlen++;
			}
			if (Spo2[i + 1] < 90)
			{
				if (marlen > 1)
				{
					marlen--;
				}
			}

			int tpsp = MAX(0, i - 1);
			AlgData_t tpmaxval = 0;
			for (int j = i - 1; j > MAX(0, i - 10); j--)
			{
				if (Spo2[j] <= 37)
				{
					tpmaxval = Spo2[j + 1];
					tpsp = j + 1;
					break;
				}
				if (Spo2[j] > tpmaxval)
				{
					tpmaxval = Spo2[j];
					tpsp = j;
				}
			}

			tpmaxval = 0;
			int tpep = inlen;
			for (int j = i + 1; j < MIN(inlen, i + 10); j++)
			{
				if (Spo2[j] <= 37)
				{
					tpmaxval = Spo2[j - 1];
					tpsp = j - 1;
					break;
				}
				if (Spo2[j] > tpmaxval)
				{
					tpmaxval = Spo2[j];
					tpep = j;
				}
			}

			for (int j = tpsp; j < tpep; j++)
			{
				int cnt = 0;
				AlgData_t tmpmean = 0;
				for (int n = MAX(0, j - marlen); n < MIN(inlen, j + marlen); n++)
				{
					if (Spo2[n] > 37)
					{
						tmpmean += Spo2[n];
						cnt++;
					}
				}
				if (cnt > 0)
				{
					Spo2[j] = tmpmean / cnt;
				}
				else
				{
					Spo2[j] = 0;
				}
			}
			i = tpep;
		}
	}
#endif
}

void SmoothMove(AlgData_t* Spo2, uint8_t* Acc, int inlen, AlgData_t VldSpo2Max)
{
	int flg = 0;
	AlgData_t bgmax = 0;
	int bgpoint = 0;
	AlgData_t edmax = 0;
	int edpoint = 0;
	int count = 0;
	for (int i = 1; i < inlen - 1; i++)
	{
		if (Spo2[i] <= 37)
		{
			continue;
		}
		if (Acc[i] > 0)
		{
			count = 0;
			for (int j = i + 1; j < inlen - 5; j++)
			{
				count++;
				if (Acc[j] + Acc[j + 1] + Acc[j + 2] + Acc[j + 3] + Acc[j + 4] == 0 || Spo2[j + 1] <= 37)
				{
					break;
				}
			}
			flg = 0;
			int tmpep = MIN(i + count, inlen);
			int tmpsp = MAX(i - 20, 0);
			bgmax = 0;
			bgpoint = tmpsp;
			for (int j = tmpsp; j < i + 1; j++)
			{
				if (bgmax <= Spo2[j])
				{
					bgmax = Spo2[j];
					bgpoint = j;
				}
			}

			AlgData_t minaroud = 100;
			get_minaround(Spo2, Acc, tmpsp, tmpep, 120, inlen, minaroud);

			if (minaroud > MIN(bgmax - 3, 95))
			{
				for (int j = bgpoint; j < tmpep; j++)
				{
					if (Spo2[j] <= bgmax - 3)
					{
						flg = 1;
						break;
					}
				}
			}
			else
			{
				for (int j = bgpoint; j < tmpep; j++)
				{
					if (Spo2[j] <= minaroud - MAX((100 - minaroud)*0.4, 3))
					{
						flg = 1;
						break;
					}
				}
			}

			if (flg == 1)
			{
				i = fix_Spo2(Spo2, inlen, i, 20, VldSpo2Max);
			}
			else
			{
				i = tmpep;
			}
		}
		else
		{
			count = 0;
		}
	}
}

void get_Spo2Vldlen(AlgData_t *Spo2, uint8_t *Acc, uint8_t *Hr, int inlen, AlgData_t VldSpo2Max,int *sp, int *ep)
{
	int len = inlen;
	for (int i = inlen - 1; i > 0; i--)
	{
		if (Spo2[i] > 37)
		{
			len = i + 1;
			break;
		}
	}
	*sp = 0;
	*ep = len;

	int conlen = 0;
	int disconlen = 0;

	float tmpsum = 0;
	float varmean = 0;
	float tmpAccsum = 0;
	int varcnt = 0;
	int tmpcnt = len - 1;
	int tmpHsum = 0;
	float varHmean = 0;
	for (; tmpcnt > len - 31; tmpcnt--)
	{
		tmpsum += abs(Spo2[tmpcnt] - Spo2[tmpcnt - 1]);
		tmpHsum += abs(Hr[tmpcnt] - Hr[tmpcnt - 1]);
		tmpAccsum += Acc[tmpcnt];
	}

	float tmpsumbck = tmpsum;
	int tmpHsumbck = tmpHsum;
	varmean += tmpsum;
	varcnt++;
	for (; tmpcnt > 1; tmpcnt--)
	{
		if (Spo2[tmpcnt] > 37 && Spo2[tmpcnt - 1] > 37)
		{
			tmpsum = tmpsum + abs(Spo2[tmpcnt] - Spo2[tmpcnt - 1]);
			tmpHsum = tmpHsum + abs(Hr[tmpcnt] - Hr[tmpcnt - 1]);
		}
		if (Spo2[tmpcnt + 30] > 37 && Spo2[tmpcnt + 29] > 37)
		{
			tmpsum = tmpsum - abs(Spo2[tmpcnt + 30] - Spo2[tmpcnt + 29]);
			tmpHsum = tmpHsum - abs(Hr[tmpcnt + 30] - Hr[tmpcnt + 29]);
		}

		varmean += tmpsum;
		varHmean += tmpHsum;
		varcnt++;
	}

	varmean = varmean / varcnt;
	varHmean = varHmean / varcnt; 
	float threshold = MIN(MAX(30, varmean * 2), 50);
	float thresholdH = MIN(MAX(40, varHmean * 2), 60);

	printf("threshold = %f, varmean = %f \n", threshold, varmean);

	float tmpmaxSpo2 = 0;
	int tmpmaxSid = 0;
	for (int i = len - 1; i > len - 90; i--)
	{
		if (Spo2[i] > tmpmaxSpo2)
		{
			tmpmaxSpo2 = Spo2[i];
			tmpmaxSid = i;
		}
	}

	tmpcnt = len - 31;
	tmpsum = tmpsumbck;
	tmpHsum = tmpHsumbck;

	// Proc the tail
	for (; tmpcnt > 1; tmpcnt--)
	{
		if (tmpsum < threshold && tmpmaxSpo2 > 90 && tmpHsum < thresholdH)
		{
			conlen++;
			if (conlen > 180)
			{
				*ep = tmpcnt + conlen + 29;
				break;
			}
		}
		else
		{
			if (tmpAccsum < 60)
			{
				disconlen++;
				if (disconlen > 2)
				{
					conlen = 0;
					disconlen = 0;
				}
			}
		}

		tmpsum = tmpsum + abs(Spo2[tmpcnt] - Spo2[tmpcnt - 1]) - abs(Spo2[tmpcnt + 30] - Spo2[tmpcnt + 29]);
		tmpHsum = tmpHsum + abs(Hr[tmpcnt] - Hr[tmpcnt - 1]) - abs(Hr[tmpcnt + 30] - Hr[tmpcnt + 29]);
		tmpAccsum = tmpAccsum + Acc[tmpcnt] - Acc[tmpcnt + 30];

		if (Spo2[tmpcnt] > tmpmaxSpo2)
		{
			tmpmaxSpo2 = Spo2[tmpcnt];
			tmpmaxSid = tmpcnt;
		}
		if (tmpmaxSid == tmpcnt + 90)
		{
			tmpmaxSpo2 = 0;
			for (int j = tmpcnt; j < tmpcnt + 90; j++)
			{
				if (Spo2[j] > tmpmaxSpo2)
				{
					tmpmaxSpo2 = Spo2[j];
					tmpmaxSid = j;
				}
			}
		}
	}

	if (tmpcnt <= 1)
	{
		*ep = 0;
		return;
	}

	for (int i = *ep + 1; i < len; i++)
	{
		Hr[i] = 0;
		Spo2[i] = 0;
	}

	//continue
	float threshold1 = MIN(MAX(35, varmean * 3), 80);
	float thresholdH1 = MIN(MAX(40, varHmean * 3), 120);

	conlen = 0;
	disconlen = 0;

	int Offhandflg = 0;
	int offpos = tmpcnt;

	for (; tmpcnt > 0; tmpcnt--)
	{
		if (tmpsum < threshold1 && tmpmaxSpo2 > MAX(VldSpo2Max - 10, 86) && tmpHsum < thresholdH1)
		{
			conlen++;
			if (Offhandflg == 1)
			{
				if (conlen > 40)
				{
					//on hand
					for (int i = tmpcnt + 1; i < offpos; i++)
					{
						Hr[i] = 0;
					}

					int conoffpos = 0;
					for (int i = offpos; i < MIN(len, offpos + 300); i++)
					{
						if (Hr[i] == 0)
						{
							conoffpos = i;
						}
					}

					if (conoffpos > 0)
					{
						for (int i = offpos; i < conoffpos; i++)
						{
							Hr[i] = 0;
						}
					}

					Offhandflg = 0;
				}
			}
			else
			{
				if (conlen > 10)
				{
					conlen = 0;
					disconlen = 0;
				}
			}

		}
		else
		{
			if ((tmpmaxSpo2 < MAX(VldSpo2Max - 8, 86)) || (tmpmaxSpo2 < MAX(93, VldSpo2Max - 4) && tmpAccsum < 30))// || tmpsum > threshold1 + 30)
			{
				disconlen++;
				if (disconlen >= 60)
				{
					if (Offhandflg == 0)
					{
						Offhandflg = 1;
						offpos = tmpcnt + disconlen + conlen;
						conlen = 0;
					}
				}
			}
		}

		tmpsum = tmpsum + abs(Spo2[tmpcnt] - Spo2[tmpcnt - 1]) - abs(Spo2[tmpcnt + 30] - Spo2[tmpcnt + 29]);
		tmpHsum = tmpHsum + abs(Hr[tmpcnt] - Hr[tmpcnt - 1]) - abs(Hr[tmpcnt + 30] - Hr[tmpcnt + 29]);
		tmpAccsum = tmpAccsum + Acc[tmpcnt] - Acc[tmpcnt + 30];
		if (Spo2[tmpcnt] > tmpmaxSpo2)
		{
			tmpmaxSpo2 = Spo2[tmpcnt];
			tmpmaxSid = tmpcnt;
		}
		if (tmpmaxSid == tmpcnt + 90)
		{
			tmpmaxSpo2 = 0;
			for (int j = tmpcnt; j < tmpcnt + 90; j++)
			{
				if (Spo2[j] > tmpmaxSpo2)
				{
					tmpmaxSpo2 = Spo2[j];
					tmpmaxSid = j;
				}
			}
		}
	}

	if (Offhandflg == 1)
	{
		*sp = offpos + 1;
		for (int i = 0; i < offpos + 1; i++)
		{
			Hr[i] = 0;
		}
	}

	for (int i = 0; i < len; i++)
	{
		if (Hr[i] == 0)
		{
			Spo2[i] = 0;
		}
	}

	int thresholdlen = 10;
	for (int i = 0; i < inlen - 1;)
	{
		if (Spo2[i] < 37 && Spo2[i + 1] > 37)
		{
			conlen = 0;
			disconlen = 0;
			for (int j = i; j >= 0; j--)
			{
				if (Spo2[j] > VldSpo2Max)
				{
					conlen++;
					if (conlen >= thresholdlen)
					{
						for (int n = j + conlen; n < i + 1; n++)
						{
							Spo2[n] = 0;
							Hr[n] = 0;
						}
						break;
					}
				}
				else
				{
					disconlen++;
					if (disconlen >= 3)
					{
						conlen = 0;
						disconlen = 0;
					}
				}
				if (j == 0)
				{
					for (int n = 0; n < i; n++)
					{
						Spo2[n] = 0;
						Hr[n] = 0;
					}
				}
			}

			conlen = 0;
			disconlen = 0;
			for (int j = i; j < *ep; j++)
			{
				if (Spo2[j] > VldSpo2Max)
				{
					conlen++;
					if (conlen >= thresholdlen)
					{
						for (int n = i; n < j - conlen; n++)
						{
							Spo2[n] = 0;
							Hr[n] = 0;
						}
						i = j;
						break;
					}
				}
				else
				{
					disconlen++;
					if (disconlen >= 3)
					{
						conlen = 0;
						disconlen = 0;
					}
				}
				if (j == *ep - 1)
				{
					for (int n = i; n < *ep; n++)
					{
						Spo2[n] = 0;
						Hr[n] = 0;
					}
					i = *ep;
				}
			}

		}
		else
		{
			i++;
		}
	}

	for (int i = 0; i < inlen; i++)
	{
		if (Spo2[i] > 37)
		{
			*sp = i;
			break;
		}
	}
	int concnt = 0;
	for (int i = *ep; i > 0; i--)
	{
		if (Spo2[i] > 37)
		{
			concnt++;
			if (concnt > 300)
			{
				*ep = i + concnt;
				break;

			}
		}
		else
		{
			concnt = 0;
		}
	}

	if (varmean > 20)
	{
		averagepoint(Spo2, *ep, (int)(varmean / 10));
	}
}

void smoothMinSpo(AlgData_t *Spo2, uint8_t *Acc, int inlen, AlgData_t VldSpo2Max,int count)
{
	AlgData_t mx = 10;//×î´ó×îÐ¡ÑªÑõÖµ²îÖµÃÅÏÞ£¬ÔÝÊ±Éè¶¨Îª10
	AlgData_t minSpo = 100;
	int minPoint = 0;
	AlgData_t aroundMinSpo_minSpo = 100;

	for (int i = 30; i < inlen - 30; i++)
	{
		if (Spo2[i] <= 37)
		{
			i += 30;
		}
		if (minSpo > Spo2[i] && Spo2[i] > 37 && Spo2[i + 29] > 37 && Acc[i] == 0)
		{
			minSpo = Spo2[i];
			minPoint = i;
		}
	}

	get_minaround(Spo2, minPoint - 120, minPoint + 120, 1200, inlen, aroundMinSpo_minSpo);

	mx = (100 - minSpo)*0.3;

	mx = MIN(mx, 15);
	mx = MAX(mx, 3);

	if (aroundMinSpo_minSpo - minSpo > mx)
	{
		fix_Spo2(Spo2, inlen, minPoint, 30, VldSpo2Max);

		if (count < 10)
		{
			count++;
			smoothMinSpo(Spo2, Acc, inlen, VldSpo2Max, count);
		}
	}
}

void minSpo2(AlgData_t *Spo2, int inlen, int time, AlgData_t *MinSpo2, int* MinSpo2Pos)
{
	int minindex = 0;
	*MinSpo2 = 100;
	*MinSpo2Pos = 0;
	int bgpoint = time * 60;

	int m = bgpoint;
	while (m < inlen - bgpoint)
	{
		if (Spo2[m] <= 37)
		{
			m++;
			if (Spo2[m] > 37)
			{
				m += 60;
			}
		}
		else
		{
			if (Spo2[m] < *MinSpo2)
			{
				*MinSpo2 = Spo2[m];
				*MinSpo2Pos = m;
			}
			m++;
		}
	}
}

AlgData_t ODI_peak[6 * 1024];
int get_Spo2event(AlgData_t *Spo2, int inlen, AlgData_t threshold, SAO2_Event_RESULT* Spo2Event, AlgData_t *odi_peak, int calnum)
{
	int i = 1;
	int lowvalue = 0;
	int trylen = 30;
	int flg = 0;
	int tmpminid = 0;
	AlgData_t tmpminval = 0;
	int evcnt = 0;
	while (i < inlen - trylen - 120)
	{
		if (i > 8290)
		{
			//printf("%d\n",i);
		}
		if (Spo2[i] <= Spo2[i - 1] && Spo2[i] > 37)
		{
			for (int j = i + 1; j < i + trylen; j++)
			{
				if (Spo2[j] <= 37)
				{
					i = j + 1;
					break;
				}
				if (Spo2[j] <= Spo2[i] - threshold && Spo2[j] > 37)
				{
					flg = 1;
					tmpminid = j;
					tmpminval = Spo2[j];
					break;
				}
			}

			if (0 == flg)
			{
				i = i + 1;
			}
			else
			{
				flg = 0;
				//Spo2event start
				float maxval = Spo2[i];
				int maxid = i;
				for (int j = i + 1; j < tmpminid; j++)
				{
					if (Spo2[j] >= maxval)
					{
						maxval = Spo2[j];
						maxid = j;
					}
				}

				//Spo2event end
				float minval = Spo2[tmpminid];
				int minid = tmpminid;
				float endval = 0;
				int endid = 0;
				int tmpflg = 1;
				int addlen = 0;
				for (int j = tmpminid + 1; j < MIN(tmpminid + 50 + addlen, inlen - 3); j++)
				{
					if (Spo2[j] <= 37)
					{
						tmpflg = 0;
						endid = j;
						break;
					}

					if (Spo2[j] < minval)
					{
						minval = Spo2[j];
						minid = j;
						addlen++;
					}
					else
					{
						float tmpth = minval + MIN(5, (100 - minval) / 3);

						if (Spo2[j] > tmpth && Spo2[j] > Spo2[j + 1])
						{
							for (int t = j; t < MIN(j + 30, inlen - 2); t++)
							{
								if (Spo2[t + 1] <= minval)
								{
									minval = Spo2[t + 1];
									minid = t + 1;
									addlen++;
									break;
								}
								if (Spo2[t + 2] <= minval)
								{
									minval = Spo2[t + 2];
									minid = t + 2;
									addlen++;
									break;
								}
								if (Spo2[t] >= Spo2[t + 1] && Spo2[t] >= Spo2[t + 2])
								{
									endval = Spo2[t - 1];
									endid = t - 1;
									break;
								}
							}
						}
						if (endid > 0)
						{
							break;
						}
					}
				}
				if (endid == 0)
				{
					minval = Spo2[tmpminid];
					minid = tmpminid;
					for (int j = tmpminid + 1; j < MIN(tmpminid + 30, inlen - 2); j++)
					{
						if (Spo2[j] <= 37)
						{
							tmpflg = 0;
							endid = j;
							break;
						}

						if (Spo2[j] < minval)
						{
							minval = Spo2[j];
							minid = j;
						}
						else
						{
							if (Spo2[j] >= Spo2[j + 1])
							{
								endval = Spo2[j];
								endid = j;
								break;
							}
						}
					}
				}

				if (endid == 0)
				{
					endval = Spo2[tmpminid + 1];
					endid = tmpminid + 1;
				}

				if (tmpflg == 0)
				{
					i = endid + 1;
					continue;
				}

				//if the spo2event is vld
				float minaround = 100;
				get_minaround(Spo2, maxid, endid, 300, inlen, minaround);

				if (minval < minaround - MAX((100 - minval)*0.4, 3))
				{
					//invld spo2event
					AlgData_t stepval = (endval - maxval) / (endid - maxid);
					int tcnt = 1;
					for (int j = maxid + 1; j < endid; j++)
					{
						if (maxval + tcnt*stepval > Spo2[j] && Spo2[j] < 97)
						{
							Spo2[j] = maxval + tcnt*stepval;
							tcnt++;
						}
						else
						{
							stepval = (endval - Spo2[j]) / (endid - j);
							maxval = Spo2[j];
							tcnt = 1;
						}
					}

					i = endid + 1;
				}
				else
				{
					//vld spo2event
					if (endid - maxid >= 3)
					{
						int fsp = maxid;
						float tmpth1 = 2;
						if (minval > 96)
						{
							tmpth1 = 1;
						}
						for (int t = maxid + 1; t < tmpminid; t++)
						{
							if (Spo2[t] < Spo2[maxid] - tmpth1)
							{
								fsp = t;
							}
						}
						Spo2Event[evcnt].startpos = fsp;
						Spo2Event[evcnt].len = endid - fsp - 2;

						odi_peak[evcnt] = maxval - minval;
						if (maxval - minval < 3.29)
						{
							lowvalue++;
						}
						evcnt++;
					}
					else
					{
						printf("debug, spo2event too short\n");
					}
					i = endid + 1;
				}
			}

		}
		else
		{
			i = i + 1;
		}
	}

	if (evcnt > 0)
	{
		AlgData_t odipeaksum = 0;
		AlgData_t odipeakmean = 0;
		AlgData_t lowvaluepct = 0;
		for (int i = 0; i < evcnt; i++)
		{
			odipeaksum += odi_peak[i];
		}
		odipeakmean = odipeaksum / evcnt;
		lowvaluepct = (float)(lowvalue - 1) / evcnt;
		if (odipeakmean < 5 && lowvaluepct >= 0.2 && evcnt > 60 && calnum > 1)
		{
			calnum--;
			threshold = 3;
			averagepoint(Spo2, inlen, 3);
			evcnt = get_Spo2event(Spo2, inlen, threshold, Spo2Event, odi_peak, calnum);
		}
	}
	return evcnt;
}

int get_InSleep(uint8_t *Acc, uint8_t *Spo2, int Proccnt)
{
	if (Proccnt < MAX_HIST_LEN)
	{
		return 0;
	}

	int Newpos = Proccnt % MAX_HIST_LEN;

	int Accsum = 0;
	int sumcnt = 0;
	int tmpvldlen = 0;
	int tmpvldlenbck = 0;
	int convldflg = 0;
	for (int i = Newpos; i > 0; i--)
	{
		Accsum += Acc[i];
		if (Acc[i] == 0 && convldflg == 0)
		{
			tmpvldlen++;
		}
		else
		{
			convldflg = 1;
		}

		sumcnt++;
		if (sumcnt > 300)
		{
			break;
		}
	}

	if (sumcnt < 300)
	{
		for (int i = MAX_HIST_LEN - 1; i > Newpos; i--)
		{
			Accsum += Acc[i];

			if (Acc[i] == 0 && convldflg == 0)
			{
				tmpvldlen++;
			}
			else
			{
				convldflg = 1;
			}

			sumcnt++;
			if (sumcnt > 300)
			{
				break;
			}
		}
	}

	if ((Accsum < 60 && tmpvldlen > 60) || Accsum < 50 || tmpvldlen > 150)
	{
		return 1;
	}

	uint8_t maxSpo2 = 0;
	uint8_t minSpo2 = 200;
	for (int i = 0; i < MAX_HIST_LEN; i++)
	{
		if (Spo2[i] > maxSpo2)
		{
			maxSpo2 = Spo2[i];
		}
		if (Spo2[i] < minSpo2 && Acc[i] == 0)
		{
			minSpo2 = Spo2[i];
		}
	}

	maxSpo2 = MIN(maxSpo2, 96);
	if (minSpo2 < maxSpo2 - 5)
	{
		uint8_t threshold = maxSpo2 - 3;

		int eventcnt = 0;
		int eventflg = 0;
		int proclen = 0;
		for (int i = Newpos; i > 0; i--)
		{
			if (Spo2[i] < threshold && Acc[i] == 0)
			{
				eventflg++;
			}
			else
			{
				if (eventflg > 10)
				{
					eventflg = 0;
					eventcnt++;
				}
			}
			proclen++;
		}

		for (int i = MAX_HIST_LEN - 1; i > Newpos; i--)
		{
			if (Spo2[i] < threshold && Acc[i] == 0)
			{
				eventflg++;
			}
			else
			{
				if (eventflg > 10)
				{
					eventflg = 0;
					eventcnt++;
				}
			}
		}

		if (eventcnt > 8 || (Accsum < 100 && eventcnt > 4))
		{
			return 1;
		}
	}
	return 0;
}

void get_MinuteAcc(uint8_t *Acc, int inlen, AlgData_t *AccM, int &outlen)
{
	outlen = 0;
	for (int i = 0; i < inlen - 60; i += 60)
	{
		AccM[outlen] = 0;
		for (int j = i; j < i + 60; j++)
		{
			AccM[outlen] += Acc[j];
		}
		outlen++;
	}
}

void get_MinuteSpo2(AlgData_t* Spo2, int inlen, AlgData_t *Spo2M, int &Outlen)
{
	Outlen = 0;
	for (int i = 0; i < inlen - 60; i += 60)
	{
		AlgData_t tmpsum = 0;
		AlgData_t tmpmean = get_Spo2_mean(Spo2 + i, 60, 37);
		for (int j = i; j < i + 60; j++)
		{
			tmpsum += fabs(Spo2[j] - tmpmean);
		}
		Spo2MSum[Outlen++] = tmpsum;
	}
}

AlgData_t get_abs_sum(AlgData_t *in, int inlen)
{
	AlgData_t sum = 0;
	for (int i = 0; i < inlen; i++)
	{
		sum += abs(in[i]);
	}
	return sum;
}

void waveletProc(AlgData_t *in, int inlen, int level, AlgData_t *Pwrvect)
{
	//return pwr of each level and the sum of pwr of details coefs
	Vector<AlgData_t> sig(inlen, in);

	DWT<AlgData_t> discreteWT("db4");
	Vector<AlgData_t> coefs = discreteWT.dwt(sig, level);

	Vector<AlgData_t>coefstmp;

	coefstmp = discreteWT.getApprox(coefs);
	Pwrvect[0] = get_abs_sum(coefstmp.begin(), coefstmp.dim());

	Pwrvect[level+1] = 0;
	for (int i = 1; i <= level; i++)
	{
		coefstmp = discreteWT.getDetial(coefs, i);
		Pwrvect[i] = get_abs_sum(coefstmp.begin(), coefstmp.dim());
		Pwrvect[level + 1] += Pwrvect[i];
	}
}

void get_Hrwavelet(uint8_t *Hr, int inlen, int Wavelevel, AlgData_t *HrWavePwr, int MinuteCnt)
{
	int proclen = 300;
	int step = 60;
	int PwrCnt = Wavelevel + 1;
	int FrameCnt = MinuteCnt;
	int FrameId = 0;
	for (int i = 0; i < inlen - proclen; i += step)
	{
		for (int j = i, m = 0; j < i + proclen; j++, m++)
		{
			WaveletProcbuf[m] = Hr[j];
		}
		AlgData_t Pwrvect[10];
		waveletProc(WaveletProcbuf, proclen, Wavelevel, Pwrvect);
#if 0
		for (int j = 0; j < 2; j++)
		{
			HrWavePwr[FrameId + j*FrameCnt] = Pwrvect[j];
		}
#endif
		HrWavePwr[FrameId] = Pwrvect[Wavelevel +1];
		FrameId++;
	}
}

void get_offhand(AlgData_t* Spo2, uint8_t* Hr, int inlen, uint8_t *status)
{
	int Mincnt = 0;
	int flg = 0;
	for (int i = 0; i < inlen; i += 60)
	{
		flg = 0;
		for (int j = i; j < MIN(i + 60, inlen); j++)
		{
			if (Hr[j] == 0 || Spo2[j] < 37)
			{
				flg = 1;
				break;
			}
		}
		if (flg > 0)
		{
			status[Mincnt] = STATUS_NOBODY;
		}
		Mincnt++;
	}
}

int get_Largecnt(AlgData_t *input, int inlen, AlgData_t threshold)
{
	int largcnt = 0;
	for (int i = 0; i < inlen; i++)
	{
		if (input[i] > threshold)
		{
			largcnt++;
		}
	}
	return largcnt;
}

AlgData_t get_threshold(AlgData_t* in, int inlen, AlgData_t baseval, int Vldlen, AlgData_t highth, AlgData_t lowth, AlgData_t step)
{
	if (step == 0 || baseval == 0)
	{
		return baseval;
	}
	AlgData_t Baserate = 1;
	int cnt = get_Largecnt(in, inlen, Baserate*baseval);
	while (cnt > highth * Vldlen)
	{
		Baserate += step;
		cnt = get_Largecnt(in, inlen, Baserate*baseval);
	}

	while (cnt < lowth*Vldlen)
	{
		Baserate -= step;
		cnt = get_Largecnt(in, inlen, Baserate*baseval);
		if (Baserate <= 0)
		{
			Baserate = step;
		}
	}
	printf("%f\n, ", Baserate);
	return Baserate * baseval;
}

int get_Vldmean(AlgData_t *Spo2M, AlgData_t *HrM, AlgData_t *AccM, uint8_t *status, int inlen,
	AlgData_t &Spo2mean, AlgData_t &Hrmean, AlgData_t &Accmean, int &vldcnt)
{
	vldcnt = 0;
	Accmean = 0;
	Hrmean = 0;
	Spo2mean = 0;
	for (int i = 0; i < inlen; i++)
	{
		if (status[i] != STATUS_NOBODY)
		{
			Accmean += AccM[i];
			Hrmean += HrM[i];
			Spo2mean += Spo2M[i];
			vldcnt++;
		}
	}

	if (vldcnt == 0)
	{
		return -1;
	}

	Accmean = Accmean / vldcnt;
	Hrmean = Hrmean / vldcnt;
	Spo2mean = Spo2mean / vldcnt;
	if (Accmean == 0)
	{
		Accmean = 5;
	}
	return 0;
}

void get_deepsleep(AlgData_t* AccM, AlgData_t* HrM, int inlen, AlgData_t Accmean, AlgData_t Hrmean, int VldMintCnt, uint8_t* status)
{
	AlgData_t AccMdeepthreshold = 5;

	AlgData_t HrMdeepthreshold = get_threshold(HrM,inlen,Hrmean, VldMintCnt,0.85,0.75,0.02);

	int i = 0;
	int vldlengh = 4;
	int invldlenth = 4;
	int flg = 0;
	int vldcnt = 0;
	int invldcnt = 0;

	for (; i < inlen; i++)
	{
		if (status[i] == 1)
		{
			break;
		}
	}
	i = i + 10;
	for (; i < inlen; i++)
	{
		if (status[i] != 1)
		{
			continue;
		}
		if (AccM[i] < AccMdeepthreshold && HrM[i] < HrMdeepthreshold)
		{
			flg = 1;
			vldcnt++;
			if (vldcnt > vldlengh)
			{
				//deepsleep
				int startpos = i - vldcnt - invldcnt;

				int tmpcnt1 = 0;
				int tmpcnt2 = 0;
				int endpos = i;
				for (int j = i; j < inlen; j++)
				{
					if (AccM[j] > AccMdeepthreshold || HrM[j] > HrMdeepthreshold)
					{
						tmpcnt1++;
						if (tmpcnt1 > invldlenth)
						{
							endpos = j - tmpcnt1;
							break;
						}
					}
					else
					{
						tmpcnt2++;
						if (tmpcnt2 > invldlenth / 2)
						{
							tmpcnt1 = 0;
							tmpcnt2 = 0;
						}
					}
				}

				for (int j = startpos; j < endpos; j++)
				{
					if (status[j] == 1)
					{
						status[j] = STATUS_N2;
					}
					
				}
				i = endpos - 1;
				flg = 0;
				vldcnt = 0;
				invldcnt = 0;
			}
		}
		else
		{
			if (flg == 1)
			{
				invldcnt++;
				if (invldcnt > invldlenth)
				{
					invldcnt = 0; 
					vldcnt = 0;
					flg = 0;
				}
			}
		}

		HrMdeepthreshold -= HrMdeepthreshold * 0.001;
	}
}

AlgData_t get_HrVldmean(AlgData_t *HrM, int inlen)
{
	AlgData_t Hrmean = get_Hr_mean(HrM, inlen);
	AlgData_t Hrsum = 0;
	int HrVldCnt = 0;
	for (int i = 0; i < inlen; i++)
	{
		if (HrM[i] - Hrmean < 10 && HrM[i] - Hrmean > -10)
		{
			Hrsum += HrM[i];
			HrVldCnt++;
		}
	}
	if (HrVldCnt > 0)
	{
		return Hrsum / HrVldCnt;
	}
	else
	{
		return Hrmean;
	}
}

void get_HrMSmooth(AlgData_t *HrM, int inlen, int proclen, AlgData_t *HrMF)
{
	int i = 0;
	for (; i < proclen; i++)
	{
		HrMF[i] = get_HrVldmean(HrM,i+proclen);
	}
	for (; i < inlen - proclen; i++)
	{
		HrMF[i] = get_HrVldmean(HrM + i-proclen, 2*proclen);
	}
	for (; i < inlen; i++)
	{
		HrMF[i] = get_HrVldmean(HrM + i-proclen, inlen - i);
	}
}

int get_largerCnt(AlgData_t *in, AlgData_t *inth, AlgData_t Addval,int inlen)
{
	int cnt = 0;
	for (int i = 0; i < inlen; i++)
	{
		if (in[i] > inth[i] + Addval)
		{
			cnt++;
		}
	}
	return cnt;
}
void get_REM(AlgData_t* HrM, AlgData_t *Spo2, int SEvent, int inlen, AlgData_t Hrmean, AlgData_t Spo2mean,int VldMintCnt, int Thflg,uint8_t* status)
{
	AlgData_t ODI3 = SEvent * 60.0 / VldMintCnt;

#if 1
	AlgData_t ODI3_lowth = 10;
	AlgData_t Spo2REMthreshold;
	AlgData_t Addval = Hrmean * 0.01;
	AlgData_t highrate = 0.25;
	AlgData_t lowrate = 0.2;

	if (Thflg > 0)
	{
		highrate += 0.05;
		lowrate += 0.05;
	}
	if (Thflg < 0)
	{
		highrate -= 0.03;
		lowrate -= 0.03;
	}

	if (ODI3 > ODI3_lowth && ODI3 < 30)
	{
		Spo2REMthreshold = get_threshold(Spo2, inlen, Spo2mean, VldMintCnt, highrate, lowrate, 0.01);
	}
	else
	{
		get_HrMSmooth(HrM, inlen, 6, HrMSum);
		get_HrMSmooth(HrM, inlen, 60, HrMSumTh);

		int cnt = get_largerCnt(HrMSum, HrMSumTh, Addval, inlen);
		while (cnt > inlen *highrate)
		{
			Addval += Hrmean * 0.002;
			cnt = get_largerCnt(HrMSum, HrMSumTh, Addval, inlen);
		}

		while (cnt < inlen * lowrate)
		{
			Addval -= Hrmean *0.002;
			cnt = get_largerCnt(HrMSum, HrMSumTh, Addval, inlen);
		}
	}

	int i = 0;
	int vldlengh = 6;
	int invldlenth = 8;
	int flg = 0;
	int vldcnt = 0;
	int invldcnt = 0;

	for (; i < inlen; i++)
	{
		if (status[i] == 1)
		{
			break;
		}
	}
	i = i + 40;
	for (; i < inlen; i++)
	{
		if (status[i] != 1)
		{
			continue;
		}
		AlgData_t tmpval;
		AlgData_t tmpth;
		if (ODI3 > ODI3_lowth && ODI3 < 30)
		{
			tmpval = Spo2[i];
			tmpth = Spo2REMthreshold;
		}
		else
		{
			tmpval = HrMSum[i];
			tmpth = HrMSumTh[i] + Addval;
		}
		if (tmpval > tmpth)
		{
			flg = 1;
			vldcnt++;
			if (vldcnt > vldlengh)
			{
				//REM
				int startpos = i - vldcnt - invldcnt;

				int tmpcnt1 = 0;
				int tmpcnt2 = 0;
				int endpos = i;
				for (int j = i; j < inlen; j++)
				{
					if (ODI3 > ODI3_lowth && ODI3 < 30)
					{
						tmpval = Spo2[j];
						tmpth = Spo2REMthreshold * 1.01;
					}
					else
					{
						tmpval = HrMSum[j];
						tmpth = HrMSumTh[j] + Addval * 1.01;
					}
					if (tmpval < tmpth)
					{
						tmpcnt1++;
						if (tmpcnt1 > invldlenth)
						{
							endpos = j - tmpcnt1;
							break;
						}
					}
					else
					{
						tmpcnt2++;
						if (tmpcnt2 > invldlenth / 2)
						{
							tmpcnt1 = 0;
							tmpcnt2 = 0;
						}
					}
				}

				for (int j = startpos; j < endpos; j++)
				{
					if (status[j] == 1)
					{
						status[j] = STATUS_REM;
					}
				}
				i = endpos - 1;
				flg = 0;
				vldcnt = 0;
				invldcnt = 0;
			}
		}
		else
		{
			if (flg == 1)
			{
				invldcnt++;
				if (invldcnt > invldlenth)
				{
					invldcnt = 0;
					vldcnt = 0;
					flg = 0;
				}
			}
		}
	}

#else
    AlgData_t Spo2REMthreshold1 , Spo2REMthreshold2;

    AlgData_t HrMREMthreshold1, HrMREMthreshold2;
    if (ODI3 > 5 && ODI3 < 30)
    {
	    Spo2REMthreshold1 = get_threshold(Spo2, inlen, Spo2mean, VldMintCnt, 0.4, 0.2, 0.02);
		Spo2REMthreshold2 = Spo2REMthreshold1;

		HrMREMthreshold1 = get_threshold(HrM, inlen, Hrmean, VldMintCnt, 0.9, 0.8, 0.02);
		HrMREMthreshold2 = get_threshold(HrM, inlen, Hrmean, VldMintCnt, 0.3, 0.2, 0.02);
    }
    else
    {
		Spo2REMthreshold1 = 0;
		Spo2REMthreshold2 = get_threshold(Spo2, inlen, Spo2mean, VldMintCnt, 0.9, 0.8, 0.02);
		HrMREMthreshold1 = get_threshold(HrM, inlen, Hrmean, VldMintCnt, 0.3, 0.15, 0.02);
		HrMREMthreshold2 = HrMREMthreshold1;
    }

	
	int i = 0;
	int vldlengh = 6;
	int invldlenth = 6;
	int flg = 0;
	int vldcnt = 0;
	int invldcnt = 0;

	for (; i < inlen; i++)
	{
		if (status[i] == 1)
		{
			break;
		}
	}
	i = i + 40;
	for (; i < inlen; i++)
	{
		if (status[i] != 1)
		{
			continue;
		}
		if (HrM[i] > HrMREMthreshold1 && Spo2[i] > Spo2REMthreshold1)
		{
			flg = 1;
			vldcnt++;
			if (vldcnt > vldlengh)
			{
				//REM
				int startpos = i - vldcnt - invldcnt;

				int tmpcnt1 = 0;
				int tmpcnt2 = 0;
				int endpos = i;
				for (int j = i; j < inlen; j++)
				{
					if (HrM[j] < HrMREMthreshold2 && Spo2[i] < Spo2REMthreshold2)
					{
						tmpcnt1++;
						if (tmpcnt1 > invldlenth)
						{
							endpos = j - tmpcnt1;
							break;
						}
					}
					else
					{
						tmpcnt2++;
						if (tmpcnt2 > invldlenth /2)
						{
							tmpcnt1 = 0;
							tmpcnt2 = 0;
						}
					}
				}

				for (int j = startpos; j < endpos; j++)
				{
					if (status[j] == 1)
					{
						status[j] = STATUS_REM;
					}
				}
				i = endpos - 1;
				flg = 0;
				vldcnt = 0;
				invldcnt = 0;
			}
		}
		else
		{
			if (flg == 1)
			{
				invldcnt++;
				if (invldcnt > invldlenth)
				{
					invldcnt = 0;
					vldcnt = 0;
					flg = 0;
				}
			}
		}
	}
#endif

}


void get_wakestatus(AlgData_t *AccM, AlgData_t *Spo2, int inlen, AlgData_t Accmean, int VldMintCnt, uint8_t *status)
{
	AlgData_t AccMwakethreshold = get_threshold(AccM, inlen, Accmean, VldMintCnt, 0.06, 0.02, 0.1);

	printf("Acc Th = %f\n", AccMwakethreshold);
	AccMwakethreshold = MIN(AccMwakethreshold, 25);

	AlgData_t Spo2Maxmean = get_Spo2_mean(Spo2, inlen * 60, 90);
	//´¦ÀíÈëË¯½×¶Î
	int i = 0;
	int tc0 = 0;
	for (; i < inlen - 5; i++)
	{
		if (AccM[i] > AccMwakethreshold / 2 || status[i] == STATUS_NOBODY)
		{
			tc0 = 0;
		}
		else
		{
			tc0++;
			int fallinSleep = 0;
			int endpos = 0;
			if (tc0 > 10)
			{
				fallinSleep = 1;
				endpos = i - tc0;
			}
			if (AccM[i] == 0)
			{
				for (int j = i * 60; j < i * 60 + 60; j++)
				{
					if (Spo2[j] < Spo2Maxmean - 3)
					{
						fallinSleep = 1;
						endpos = i - 1;
						break;
					}
				}
			}
			if (fallinSleep == 1)
			{
				for (int j = 0; j < endpos; j++)
				{
					if (status[j] == 1)
					{
						status[j] = STATUS_WAKE;
					}
				}
				break;
			}
		}
	}

	//continue proc
	int vldlengh = 2;
	int invldlenth = 4;

	for (; i < inlen - 10; i++)
	{
		if (status[i] != 1)
		{
			continue;
		}
		if (AccM[i] > AccMwakethreshold)
		{
			int vldcnt = 0;
			int startpos = i;
			for (int j = i + 8; j > i - 8; j--)
			{
				if (AccM[j] > AccMwakethreshold / 2)
				{
					vldcnt++;
					startpos = j;
				}
				else
				{
					for (int t = j * 60; t < j * 60 + 60; t++)
					{
						if (Spo2[t] < Spo2Maxmean - 3)
						{
							vldcnt--;
							break;
						}
					}
				}
			}
			if (vldcnt > 2)
			{
				//into wake status
				int invldcnt = 0;
				int endpos = 0;
				for (int j = i + 2; j < inlen; j++)
				{
					if (AccM[j] < AccMwakethreshold / 2)
					{
						int spo2decflg = 0;
						for (int t = j * 60; t < j * 60 + 60; t++)
						{
							if (Spo2[t] < Spo2Maxmean - 3)
							{
								endpos = j - 1;
								break;
							}
						}
						invldcnt++;
						if (invldcnt > 6)
						{
							//sleep
							endpos = j - invldcnt;
							break;
						}
					}
					else
					{
						invldcnt = 0;
					}
					
				}
				if (endpos == 0)
				{
					endpos = inlen;
				}

				for (int j = startpos; j < endpos; j++)
				{
					status[j] = STATUS_WAKE;
				}
				i = endpos;
			}
		}
	}
	int tmpcnt = 0;
	int j = inlen - 1;
	for (; j > 1; j--)
	{
		if (AccM[j] > AccMwakethreshold / 2 || status[j] == STATUS_NOBODY)
		{
			tmpcnt = 0;
		}
		else
		{
			tmpcnt++;
			if (tmpcnt > 5)
			{
				break;
			}
		}
	}
	for (int k = j + tmpcnt; k < inlen; k++)
	{
		status[k] = STATUS_WAKE;
	}

}

void get_sleepstatus(SAO2_InPara *Inpara, Proc_Para *Procpara, SAO2_OutPara *Outpara)
{
	int startpos = Outpara->Static.startpos;
	int endpos = Outpara->Static.endpos;
	int len = endpos - startpos;

	AlgData_t *AccM = Procpara->AccM;
	AlgData_t *Spo2M = Procpara->Spo2M;
	AlgData_t *HrM = Procpara->HRvect;

	uint8_t *Acc = Inpara->AccFlg + startpos;
	uint8_t *Hr = Inpara->HeartRate + startpos;
	AlgData_t *Spo2 = Inpara->Spo2 + startpos;

	int MinuteCnt = 0;
	get_MinuteAcc(Acc, len, AccM, MinuteCnt);
	get_MinuteSpo2(Spo2, len, Spo2M, MinuteCnt);

	get_Hrwavelet(Hr, len, 8, HrM, MinuteCnt);

	uint8_t *status = Outpara->status;
	for (int i = 0; i < MinuteCnt; i++)
	{
		status[i] = 1;
	}

	get_offhand(Spo2, Hr, len, status);

	AlgData_t AccMmean;
	AlgData_t HrMmean;
	AlgData_t Spo2Mmean;
	int vldcnt;
	int ret;
	ret = get_Vldmean(Spo2M,HrM,AccM,status,MinuteCnt,Spo2Mmean,HrMmean,AccMmean,vldcnt);

	if (ret != 0)
	{
		memset(status, 0, MinuteCnt);
		return;
	}

	//¸ù¾ÝAccÅÐ¶ÏWake
	get_wakestatus(AccM, Spo2, MinuteCnt, AccMmean, vldcnt, status);

	vldcnt = 0;
	for (int i = 0; i < MinuteCnt; i++)
	{
		if (status[i] == 1)
		{
			vldcnt++;
		}
	}

	int tmpSEcnt = 0;
	SAO2_Event_RESULT *SEvect = Outpara->SEvent;
	for (int i = 0; i < Outpara->Static.diffThdLge3Cnts; i++)
	{
		int mid = SEvect[i].startpos / 60;
		if (status[i] == 1)
		{
			tmpSEcnt++;
		}
	}


	//¸ù¾ÝHRÅÐ¶ÏREM
	int flg = 0;
	//get_REM(HrM, Spo2M, tmpSEcnt, MinuteCnt, HrMmean, Spo2Mmean, vldcnt, flg, status);
	get_REM(HrM, Spo2M, tmpSEcnt, MinuteCnt, HrMmean, Spo2Mmean, vldcnt, flg, status);

	int REMCnt = 0;
	for (int i = 0; i < MinuteCnt; i++)
	{
		if (status[i] == STATUS_REM)
		{
			REMCnt++;
		}
	}

	if (REMCnt > vldcnt * 0.3)
	{
		for (int i = 0; i < MinuteCnt; i++)
		{
			if (status[i] == STATUS_REM)
			{
				status[i] = 1;
			}
		}
		flg = -1;
		//get_REM(HrM, Spo2M, tmpSEcnt, MinuteCnt, HrMmean, Spo2Mmean, vldcnt, flg, status);
		get_REM(HrM, Spo2M, tmpSEcnt, MinuteCnt, HrMmean, Spo2Mmean, vldcnt, flg, status);

	}
	if (REMCnt < vldcnt * 0.15)
	{
		for (int i = 0; i < MinuteCnt; i++)
		{
			if (status[i] == STATUS_REM)
			{
				status[i] = 1;
			}
		}
		flg = 1;
		//get_REM(HrM, Spo2M, tmpSEcnt, MinuteCnt, HrMmean, Spo2Mmean, vldcnt, flg, status);
		get_REM(HrM, Spo2M, tmpSEcnt, MinuteCnt, HrMmean, Spo2Mmean, vldcnt, flg, status);

	}

	//¸ù¾ÝHRºÍACCÅÐ¶ÏN1 N2
	get_deepsleep(AccM, HrM, MinuteCnt, AccMmean, HrMmean, vldcnt, status);

	for (int i = 1; i < MinuteCnt; i++)
	{
		if (status[i] == 1)
		{
			status[i] = STATUS_N1;
		}
	}
	Outpara->MinuteCnt = MinuteCnt;
}

int get_StaticResult(SAO2_InPara *Inpara, Proc_Para *Procpara,SAO2_OutPara *Outpara)
{
	/*tongji*/
	SAO2_STATIC_RESULT *Static = &(Outpara->Static);
	SAO2_Event_RESULT *SEvect = Outpara->SEvent;

	int startpos = Static->startpos;
	int endpos = Static->endpos;
	int len = endpos - startpos;

	AlgData_t *AccM = Procpara->AccM;
	AlgData_t *Spo2M = Procpara->Spo2M;
	AlgData_t *HrM = Procpara->HRvect;

	uint8_t *Acc = Inpara->AccFlg + startpos;
	uint8_t *Hr = Inpara->HeartRate + startpos;
	AlgData_t *Spo2 = Inpara->Spo2 + startpos;

	int MinuteCnt = Outpara->MinuteCnt;
	uint8_t *status = Outpara->status;

	int vldSecond = 0;

	Static->Spo2Avg = 0;

	Static->maxSpo2DownTime = 0;
	Static->Spo2Avg = 0;
	Static->Spo2Min = 100;
	
	Static->prMax = 0;
	Static->prMin = 300;
	Static->prAvg = 0;

	Static->spo2Less60Time = 0;
	Static->spo2Less70Time = 0;
	Static->spo2Less80Time = 0;
	Static->spo2Less85Time = 0;
	Static->spo2Less90Time = 0;
	Static->spo2Less95Time = 0;
	
	int vldSEcnt = 0;
	int SEcnt = 0;
	for (int mid = 0; mid < MinuteCnt; mid++)
	{
		if (status[mid] > STATUS_WAKE && status[mid] < STATUS_NOBODY)
		{
			int tmpid = mid * 60;
			vldSecond += 60;
			for (int i = tmpid; i < tmpid + 60; i++)
			{
				Static->Spo2Avg += Spo2[i];
				Static->prAvg += Hr[i];
				if (Spo2[i] < Static->Spo2Min)
				{
					Static->Spo2Min = Spo2[i];
				}
				if (Hr[i] > Static->prMax)
				{
					Static->prMax = Hr[i];
				}
				if (Hr[i] < Static->prMin)
				{
					Static->prMin = Hr[i];
				}

				if (Spo2[i] < 95)
				{
					Static->spo2Less95Time++;
				}
				if (Spo2[i] < 90)
				{
					Static->spo2Less90Time++;
				}
				if (Spo2[i] < 85)
				{
					Static->spo2Less85Time++;
				}
				if (Spo2[i] < 80)
				{
					Static->spo2Less80Time++;
				}
				if (Spo2[i] < 70)
				{
					Static->spo2Less70Time++;
				}
				if (Spo2[i] < 60)
				{
					Static->spo2Less60Time++;
				}
			}
			while (SEvect[SEcnt].startpos < tmpid - 30)
			{
				SEcnt++;
			}

			for (int loop = 0; loop < 3; loop++)
			{
				if (SEvect[SEcnt].startpos < tmpid + 60 && SEcnt < Static->diffThdLge3Cnts)
				{
					SEvect[vldSEcnt].startpos = SEvect[SEcnt].startpos;
					SEvect[vldSEcnt].len = SEvect[SEcnt].len;
					if (Static->maxSpo2DownTime < SEvect[vldSEcnt].len)
					{
						Static->maxSpo2DownTime = SEvect[vldSEcnt].len;
					}
					vldSEcnt++;
					SEcnt++;
				}
				else
				{
					break;
				}
			}

		}
	}

	if (vldSecond == 0)
	{
		return 1;
	}

	Static->Spo2Avg = Static->Spo2Avg / vldSecond;
	Static->prAvg = Static->prAvg / vldSecond;

	Static->spo2Less60TimePercent = Static->spo2Less60Time * 100.0 / vldSecond;
	Static->spo2Less70TimePercent = Static->spo2Less70Time * 100.0 / vldSecond;
	Static->spo2Less80TimePercent = Static->spo2Less80Time * 100.0 / vldSecond;
	Static->spo2Less85TimePercent = Static->spo2Less85Time * 100.0 / vldSecond;
	Static->spo2Less90TimePercent = Static->spo2Less90Time * 100.0 / vldSecond;
	Static->spo2Less95TimePercent = Static->spo2Less95Time * 100.0 / vldSecond;
	Static->diffThdLge3Cnts = vldSEcnt;
	Static->diffThdLge3Pr = vldSEcnt * 3600.0 / vldSecond;


	printf("\n/***************Spo2EventCnt = %d, ODI3 = %f, minSpo2 = %f\n", Static->diffThdLge3Cnts, Static->diffThdLge3Pr, Static->Spo2Min);
	Static->handOffArrlen = 0;

	int offhandflg = 0;
	if (Spo2[0] <= 37 && Hr[0] == 0)
	{
		offhandflg = 1;
		Static->handOffArr[Static->handOffArrlen] = 0;
		Static->handOffArrlen++;
	}
	for (int i = 1; i < len; i++)
	{
		if (Spo2[i] <= 37 && Hr[i] == 0 && offhandflg == 0)
		{
			offhandflg = 1;
			Static->handOffArr[Static->handOffArrlen] = i;
			Static->handOffArrlen++;
		}

		if (Spo2[i] > 37 && Hr[i] > 0 && offhandflg == 1)
		{
			offhandflg = 0;
			Static->handOffArr[Static->handOffArrlen] = i;
			Static->handOffArrlen++;
		}
	}
	return 0;
}


//Spo2 ºó´¦Àí

void Init_Para(SAO2_InPara *InPara, Proc_Para *Procpara, SAO2_OutPara *Outpara)
{
	Procpara->AccM = AccMSum;
	Procpara->Spo2M = Spo2MSum;
	Procpara->HRvect = WaveletPara;

	Outpara->status = Sataus;
	Outpara->SEvent = Spo2EventVect;
	Outpara->Static.Spo2Arr = InPara->Spo2;
	Outpara->Static.prArr = InPara->HeartRate;
	Outpara->Static.handOffArr = HandOffVect;
}

void rebckSpo2(AlgData_t* Spo2new, unsigned char* Hrnew, AlgData_t* Spo2old, unsigned char* Hrold, int len)
{
	for (int i = 0; i < len;)
	{
		if (Spo2new[i] > 0)
		{
			i++;
		}
		else
		{
			if (Spo2old[i] <= 37)
			{
				i++;
			}
			else
			{
				int ozflg = 0;
				for (int j = i; j < MIN(len, i + 300); j++)
				{
					if (Spo2old[j] <= 37)
					{
						ozflg = 1;
						break;
					}
					if (j == len - 1)
					{
						ozflg = 1;
						i = len - 1;
					}
				}
				if (ozflg == 1)
				{
					i++;
				}
				else
				{
					for (int j = i; j < len; j++)
					{
						if (Spo2new[j] > 0)
						{
							i = j;
							break;
						}
						Spo2new[j] = Spo2old[j];
						Hrnew[j] = Hrold[j];
						i = j;
					}
				}
			}
		}
	}
}


void Proc_Schedule(SAO2_InPara *InPara, Proc_Para *Procpara, SAO2_OutPara *OutPara)
{
	int inlen = InPara->len;
	AlgData_t *Spo2 = InPara->Spo2;
	unsigned char* Acc = InPara->AccFlg;
	unsigned char* Hr = InPara->HeartRate;

	if (inlen < 1800)
	{
		memset(OutPara, 0, sizeof(SAO2_STATIC_RESULT));
		OutPara->Static.timeStart = InPara->timeStart;
		return;
	}
	Init_Para(InPara, Procpara, OutPara);

	memcpy(Spo2Bck, Spo2, 4*inlen);
	memcpy(HeartRateBck, Hr, inlen);

	proc_Acc(Acc, inlen);

	AlgData_t MinuteMaxSpo2;
	AlgData_t MinuteMeanDec;
	get_Mfeature(Spo2, inlen, &MinuteMaxSpo2, &MinuteMeanDec);

	AlgData_t VldSpo2Maxth = MIN(MinuteMaxSpo2 - 2, 96);
	SmoothOffhand(Spo2, Hr, VldSpo2Maxth,inlen);
	smooth_Spo2(Spo2, inlen, MAX(MinuteMeanDec/2, 3), VldSpo2Maxth);

	int startp = 0;
	int endp = inlen;

	get_Spo2Vldlen(Spo2,Acc,Hr,inlen,VldSpo2Maxth,&startp,&endp);

	int len = endp - startp;
	if (len < 1800)
	{
		memset(OutPara, 0, sizeof(SAO2_STATIC_RESULT));
		OutPara->Static.timeStart = InPara->timeStart;
		return;
	}

	SmoothMove(Spo2 + startp, Acc + startp, len, VldSpo2Maxth);

	smoothMinSpo(Spo2 + startp, Acc + startp, len, VldSpo2Maxth, 5);

	averagepoint(Spo2 + startp, len, 4);

	for (int i = 1; i < inlen - 1; i++)
	{
		Hr[i] = (Hr[i - 1] + Hr[i] + Hr[i + 1]) / 3;
	}

	int dips3 = get_Spo2event(Spo2 + startp, len, 3, OutPara->SEvent, ODI_peak, 2);
	printf(" Orign SAO2 event cnt = %d ", dips3);

	SAO2_STATIC_RESULT *Static = &(OutPara->Static);
	Static->diffThdLge3Cnts = dips3;
	Static->startpos = startp;
	Static->endpos = endp;
	Static->timeStart = InPara->timeStart;
	Static->duration = InPara->len;
	
	get_sleepstatus(InPara, Procpara, OutPara);

	int sleepcnt = 0;
	for (int i = 0; i < OutPara->MinuteCnt; i++)
	{
		if (OutPara->status[i] > 0)
		{
			sleepcnt++;
		}
	}

	if (sleepcnt > 0)
	{
		get_StaticResult(InPara, Procpara, OutPara);
	}
}

void Unify_Result(spo2_analysis_t* analysis_t, SAO2_OutPara* OutPara)
{
	for (int i = 0; i < analysis_t->length_spo2; i++)
	{
		unsigned short tmpspo2 = (unsigned short)(analysis_t->Spo2f[i] * 100);
		analysis_t->spo2[i] = tmpspo2;
	}
	for (int i = 0; i < analysis_t->length_spo2; i++) {
		if (analysis_t->spo2[i] == 0) {
			analysis_t->pr[i] = 0;
		}
	}
	analysis_t->handoff.num = 0;
	int hanoffnum = 0;
	while (hanoffnum < OutPara->Static.handOffArrlen)
	{
		analysis_t->handoff.time_stamp[analysis_t->handoff.num].start = OutPara->Static.handOffArr[hanoffnum];
		analysis_t->handoff.time_stamp[analysis_t->handoff.num].end = OutPara->Static.handOffArr[hanoffnum + 1];
		analysis_t->handoff.num++;
		hanoffnum += 2;
	}
	analysis_t->startpos = OutPara->Static.startpos;
	analysis_t->endpos = OutPara->Static.endpos;
	analysis_t->duration = OutPara->Static.endpos;
	analysis_t->time_start = OutPara->Static.timeStart;
	analysis_t->spo2_average = (int)(OutPara->Static.Spo2Avg * 100);
	analysis_t->spo2_min = (int)(OutPara->Static.Spo2Min * 100);
	analysis_t->diff_thd_lge3_cnts = OutPara->Static.diffThdLge3Cnts;
	analysis_t->diff_thd_lge3_pr = OutPara->Static.diffThdLge3Pr;
	analysis_t->pr_average = OutPara->Static.prAvg;
	analysis_t->pr_max = OutPara->Static.prMax;
	analysis_t->pr_min = OutPara->Static.prMin;
	//analysis_t->handon_total_time = OutPara->Static.handonTotalTime;
	analysis_t->handon_total_time = analysis_t->length_spo2;
	analysis_t->spo2_less60_time = OutPara->Static.spo2Less60Time;
	analysis_t->spo2_less70_time = OutPara->Static.spo2Less70Time;
	analysis_t->spo2_less80_time = OutPara->Static.spo2Less80Time;
	analysis_t->spo2_less85_time = OutPara->Static.spo2Less85Time;
	analysis_t->spo2_less90_time = OutPara->Static.spo2Less90Time;
	analysis_t->spo2_less95_time = OutPara->Static.spo2Less95Time;
	analysis_t->spo2_less60_time_percent = OutPara->Static.spo2Less60TimePercent;
	analysis_t->spo2_less70_time_percent = OutPara->Static.spo2Less70TimePercent;
	analysis_t->spo2_less80_time_percent = OutPara->Static.spo2Less80TimePercent;
	analysis_t->spo2_less85_time_percent = OutPara->Static.spo2Less85TimePercent;
	analysis_t->spo2_less90_time_percent = OutPara->Static.spo2Less90TimePercent;
	analysis_t->spo2_less95_time_percent = OutPara->Static.spo2Less95TimePercent;
	for (int i = 0; i < OutPara->MinuteCnt; i++)
	{
		analysis_t->status[i] = OutPara->status[i];
	}
	analysis_t->length_status = OutPara->MinuteCnt;
}

static void init_spo2result(spo2_analysis_t* analysis)
{
	memset((char*)& analysis->version, 0, sizeof(spo2_analysis_t) - sizeof(app_input_t));
}

void parse_pr_spo2(char* data_in, int data_len, void* pr_spo2_result)
{
	spo2_analysis_t* spo2_analysis = (spo2_analysis_t*)pr_spo2_result;
	init_spo2result(spo2_analysis);

	//int skip = 16;
	int offlinetime = 0;
	bool offhandEnd = true;
	bool offhandBegin = false;
	int acc_variance_mx = 15;
	int grouplength = 0;
	int startsp = 0;
	int flagforb = 0;
	int flagstagetime = 0;
	unsigned int data_in_startime = 0;
	int detanum_end = 0;
	//AlgData_t* Spo2Matrixbck = NULL;
	//unsigned char* HRMatrixbck = NULL;
	int accthrenum = 0;

	spo2_analysis->version = data_in[0];
	spo2_analysis->data_type = data_in[1];
	spo2_analysis->peer_data_length = data_len - 0x04;
	spo2_analysis->data_block_size = 256;
	spo2_analysis->data_block_elenum = 82;

	//Spo2Matrixbck = (AlgData_t*)malloc(MAX_SPO2_CNT * sizeof(AlgData_t));
	//HRMatrixbck = (unsigned char*)malloc(MAX_SPO2_CNT * sizeof(unsigned char));
	//SAO2Static = (SAO2_STATIC_RESULT*)malloc(sizeof(SAO2_STATIC_RESULT));
	//SAO2_STATIC_RESULT* SAO2Static = (SAO2_STATIC_RESULT*)malloc(sizeof(SAO2_STATIC_RESULT));
	//memset((char*)SAO2Static, 0, sizeof(SAO2_STATIC_RESULT));

	//SAO2_InPara* InPara = (SAO2_InPara*)malloc(sizeof(SAO2_InPara));
	//SAO2_OutPara* OutPara = (SAO2_OutPara*)malloc(sizeof(SAO2_OutPara));
	//Proc_Para* ProcPara = (Proc_Para*)malloc(sizeof(Proc_Para));

	startsp = 4;
	data_in_startime = *((unsigned int*)data_in + startsp / sizeof(unsigned int)); // start time of data_in

	if (data_len < spo2_analysis->data_block_size)
	{
		spo2_analysis->bin_stop_sec = 0;
		spo2_analysis->bin_start_sec = 0;
	}
	else
	{
		spo2_analysis->bin_start_sec = data_in_startime;
		spo2_analysis->bin_stop_sec = *((unsigned int*)(data_in + data_len - spo2_analysis->data_block_size)) + spo2_analysis->data_block_elenum;
	}

	int exceptgroup = 0;
	int detanum_start = 0;
	detanum_end = 0;
	if (spo2_analysis->app_define.stage_N1_startime > data_in_startime)
	{
		exceptgroup = (spo2_analysis->app_define.stage_N1_startime - data_in_startime) / spo2_analysis->data_block_elenum;
		detanum_start = (spo2_analysis->app_define.stage_N1_startime - data_in_startime) % spo2_analysis->data_block_elenum;
	}

	int start_group = MAX(0, exceptgroup); //start time for use

	grouplength = (data_len - 4) / spo2_analysis->data_block_size;
	if (grouplength > start_group) // ignore the front 6 groups or use the stage's start time
	{
		startsp += spo2_analysis->data_block_size * start_group;
		grouplength = grouplength - start_group;
	}

	spo2_analysis->length_spo2 = 0;
	for (int i = startsp; i < data_len; i += spo2_analysis->data_block_size) // A group with 256 byte 
	{
		if (detanum_start > 0)
		{
			spo2_analysis->time[0] = spo2_analysis->app_define.stage_N1_startime;
		}
		else
		{
			spo2_analysis->time[spo2_analysis->length_time] = *((unsigned int*)data_in + i / sizeof(unsigned int));
		}

		if (spo2_analysis->app_define.stage_endtime >= spo2_analysis->time[spo2_analysis->length_time] && spo2_analysis->app_define.stage_endtime < spo2_analysis->time[spo2_analysis->length_time] + spo2_analysis->data_block_elenum)
		{
			detanum_end = spo2_analysis->data_block_elenum - (spo2_analysis->app_define.stage_endtime - spo2_analysis->time[spo2_analysis->length_time]) % spo2_analysis->data_block_elenum;
			flagstagetime = 1;
		}
		spo2_analysis->length_time++;

		for (int j = i + 8 + detanum_start * 3; j < i + spo2_analysis->data_block_size - 2 - detanum_end * 3; j += 3)
		{
			if ((uint8_t)data_in[i + 4] != 0xcd || (uint8_t)data_in[i + 5] != 0xcd || (uint8_t)data_in[i + 6] != 0xcd || (uint8_t)data_in[i + 7] != 0xcd)
			{
				spo2_analysis->error_tips[0] = 1;
				flagforb = 1;
				break;
			}

			int ringspo2 = ((uint8_t)data_in[j] >> 2) + 37;
			float tmpspo2 = ringspo2 + ((uint8_t)data_in[j] % 4) * 0.25f;
			spo2_analysis->Spo2f[spo2_analysis->length_spo2] = tmpspo2;
			spo2_analysis->pr[spo2_analysis->length_spo2] = data_in[j + 1];
			spo2_analysis->acc[spo2_analysis->length_spo2] = data_in[j + 2] & 0x7F;
			spo2_analysis->moveflag[spo2_analysis->length_spo2] = (uint8_t)data_in[j + 2] >> 7;
			spo2_analysis->length_spo2++;

		}
		if (detanum_start > 0)
		{
			detanum_start = 0;
		}

		if (flagforb == 1 || flagstagetime == 1)
		{
			break;
		}
	}
	spo2_analysis->time_start = spo2_analysis->time[0];
	//spo2_analysis->duration = spo2_analysis->length_spo2;
	InPara.len = spo2_analysis->length_spo2;
	InPara.timeStart = spo2_analysis->time_start;
	InPara.Spo2 = spo2_analysis->Spo2f;
	InPara.HeartRate = spo2_analysis->pr;
	InPara.AccFlg = spo2_analysis->acc;

	Proc_Schedule(&InPara, &ProcPara, &OutPara);
	Unify_Result(spo2_analysis, &OutPara);

	//smooth proc
	//Sao2Event_Detect(spo2_analysis, ODI_peak, SAO2Static, Spo2Matrixbck, HRMatrixbck);
	//Unify_Result(spo2_analysis, SAO2Static);
	//stages proc
	//get_sleepstatus(spo2_analysis, AccSum_minit, pr_para);

	//if (spo2_analysis->length_spo2 > 60 * 10 && spo2_analysis->handon_total_time > 60)
	//{
	//	//1¡¢Screening SAO2EventCnt base sleep status
	//	Screening_Event(spo2_analysis);
	//	SAO2Static->diffThdLge3Cnts = spo2_analysis->SAO2EventCnt;
	//	SAO2Static->diffThdLge3Pr = (float)SAO2Static->diffThdLge3Cnts * 3600.0 / spo2_analysis->handon_total_time;
	//	spo2_analysis->diff_thd_lge3_cnts = SAO2Static->diffThdLge3Cnts;
	//	spo2_analysis->diff_thd_lge3_pr = SAO2Static->diffThdLge3Pr;
	//	//2¡¢get spo2min base sleep status
	//	int len = spo2_analysis->SAO2ep - spo2_analysis->SAO2sp;
	//	AlgData_t spo2minval;
	//	int spo2minid;
	//	minSpo2(spo2_analysis->Spo2f + spo2_analysis->SAO2sp, spo2_analysis->status, len, 3, 2, &spo2minval, &spo2minid);
	//	SAO2Static->Spo2Min = spo2minval;
	//	spo2_analysis->spo2_min = (int)(spo2minval * 100);
	//}
	//reback spo2 & hr offhan data
	//rebckSpo2(spo2_analysis->Spo2f + spo2_analysis->SAO2sp, Spo2Matrixbck + spo2_analysis->SAO2sp,
	//	spo2_analysis->pr + spo2_analysis->SAO2sp, HRMatrixbck + spo2_analysis->SAO2sp,
	//	spo2_analysis->SAO2ep - spo2_analysis->SAO2sp);
	//Unify_Result(spo2_analysis, SAO2Static);

#if 0
	int d = 0;
	FILE* fpwrite = fopen("data.txt", "w");
	if (fpwrite == NULL)
	{
		return;
	}
	//fprintf(fpwrite, "spo2:\n", d);
	for (int i = 0; i < spo2_analysis->length_spo2; i++)
	{
		d = (int)spo2_analysis->spo2[i];
		fprintf(fpwrite, "%d,", d);
	}

	fprintf(fpwrite, "\n\npr:\n", d);
	for (int i = 0; i < spo2_analysis->length_spo2; i++)
	{
		d = spo2_analysis->pr[i];
		fprintf(fpwrite, "%d,", d);
	}
	fprintf(fpwrite, "\n\nmoveflag:\n", d);
	for (int i = 0; i < spo2_analysis->length_spo2; i++)
	{
		d = spo2_analysis->moveflag[i];
		fprintf(fpwrite, "%d,", d);
	}
	fprintf(fpwrite, "\n\nacc:\n", d);
	for (int i = 0; i < spo2_analysis->length_spo2; i++)
	{
		d = spo2_analysis->acc[i];
		fprintf(fpwrite, "%d,", d);
	}
	fprintf(fpwrite, "\n\nhandoff:\n", d);
	for (int i = 0; i < spo2_analysis->length_spo2; i++)
	{
		d = spo2_analysis->handoff[i];
		fprintf(fpwrite, "%d,", d);
	}
	fclose(fpwrite);
#endif

	//if (SAO2Static != NULL)          free(SAO2Static);
	//if (Spo2Matrixbck != NULL)       free(Spo2Matrixbck);
	//if (HRMatrixbck != NULL)         free(HRMatrixbck);

	//if (InPara != NULL)         free(InPara);
	//if (OutPara != NULL)        free(OutPara);
	//if (ProcPara != NULL)       free(ProcPara);
}
