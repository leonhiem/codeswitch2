/*
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>


typedef struct {
        int8_t year, month, day, hour, minute;
} Time;

// generate random number:
int8_t get_number(int min,int max)
{
    float rf=(float)rand() / (float)RAND_MAX * (float)(max+1);
    int8_t r = (int8_t)rf;

    if(r<min) r=min;
    
    //printf("r=%d\n",r);
    return (int8_t)r;
}

int8_t get_days_in_month(Time *d)
{
	if((d->year&0x3) == 0) { // leap year
		if(d->month==2) return 29; // february
	} else {
		if(d->month==2) return 28; // february
	}
	switch(d->month) {
		case 1: // january
		case 3: // march
		case 5: // may
		case 7: // july
		case 8: // august
		case 10: // october
		case 12: // december
		    return 31;
	}
	return 30;
}
int16_t get_days_in_year(Time *d)
{
	if((d->year&0x3) == 0) { // leap year
		return 366;
	} else {
		return 365;
	}
}

int8_t check_Time(Time *t)
{
    //printf("check:%02d-%02d-%02d %02d:%02d\n",t->year,t->month,t->day,t->hour,t->minute);
    
    if(t->year <= 0)  goto err;
    if(t->month <= 0) goto err;
    if(t->day <= 0)   goto err;
    if(t->hour < 0)   goto err;
    if(t->minute < 0) goto err;

    if(t->month > 12)  goto err;
    if(t->day > 31)    goto err;
    if(t->hour > 23)   goto err;
    if(t->minute > 59) goto err;

    int8_t days_in_month = get_days_in_month(t);
    //printf("max days_in_month=%d\n",days_in_month);
    if(t->day > days_in_month) {
        printf("nr days in month error\n");
        goto err;
    }

    return 0;
  err:
    printf("ERROR!\n");
    exit(0);
}

int8_t compare_Time(Time *a, Time *b) 
{
    if(a->year   != b->year)   return -1;
    if(a->month  != b->month)  return -1;
    if(a->day    != b->day)    return -1;
    if(a->hour   != b->hour)   return -1;
    if(a->minute != b->minute) return -1;
 
    return 0;
}

/*
// decrease time by 1 minute:
void Time_decr(Time *t)
{
    t->minute--;
    if(t->minute < 0) { t->hour--; t->minute+=60; }

    t->hour--;
    if(t->hour < 0) { t->day--; t->hour+=24; }

    t->day--;
    if(t->day < 0) { t->month--; t->day+=get_days_in_month(t); }

    t->month--;
    if(t->month < 0) { t->year--; t->month+=12; }

    t->year--;
}
*/

int8_t Days_left(Time *a, Time *b)
{
	int8_t i,sign=0;
        int32_t d,minutes_a=0;
        int32_t minutes_b=0;
        int32_t days_year_a=0;
        int32_t days_year_b=0;
        //float days_delta;
        int32_t minutes_delta;
        Time tmp;

        tmp.year = a->year;
        for(i=2;i <= a->month;i++) {
            tmp.month = i-1;
            d = get_days_in_month(&tmp);
            //printf("1tmp.month=%d days_in_month=%d\n",tmp.month,d);
            minutes_a += (d * 1440);
        }
        minutes_a += (a->day * 1440);
        minutes_a += (a->hour * 60);
        minutes_a += a->minute;

        tmp.year  = b->year;
        for(i=2;i <= b->month;i++) {
            tmp.month = i-1;
            d = get_days_in_month(&tmp);
            //printf("2tmp.month=%d days_in_month=%d\n",tmp.month,d);
            minutes_b += (d * 1440);
        }
        minutes_b += (b->day * 1440);
        minutes_b += (b->hour * 60);
        minutes_b += b->minute;


        minutes_delta = minutes_a - minutes_b;
        //printf("minutes_a=%d minutes_b=%d minutes_delta=%d\n",minutes_a,minutes_b,minutes_delta);

        for(i=2;i <= a->year; i++) {
            tmp.year = i-1;
            days_year_a += get_days_in_year(&tmp);
        }
        for(i=2;i <= b->year; i++) {
            tmp.year = i-1;
            days_year_b += get_days_in_year(&tmp);
        }
        //printf("days_year_a=%d days_year_b=%d\n",days_year_a,days_year_b);
        minutes_delta += (days_year_a-days_year_b)*1440;
        //printf("minutes_delta=%d\n",minutes_delta);
        if(minutes_delta < 0) return -1;

        //days_delta=(float)minutes_delta/1440.;
        //printf("delta=%f\n",days_delta);

        return (int8_t)(minutes_delta/1440);
}

void Time_add(Time *sum, Time *now, Time *credit)
{
	int8_t days_in_month;
	sum->minute = 0;
	sum->hour = 0;
	sum->day = 0;

	sum->year  = now->year  + credit->year;
	sum->month = now->month + credit->month;
	while(sum->month > 12) { 
            sum->year++; sum->month-=12; 
            //printf("year=%d month=%d\n",sum->year, sum->month);
        }

	sum->minute += now->minute + credit->minute;
	while(sum->minute > 59) { 
            sum->hour++; sum->minute-=60; 
            //printf("hour=%d minute=%d\n",sum->hour,sum->minute);
        }

	sum->hour   += now->hour   + credit->hour;
	while(sum->hour > 23) { 
            sum->day++; sum->hour-=24; 
            //printf("day=%d hour=%d\n",sum->day,sum->hour);
        }

	days_in_month = get_days_in_month(sum);
        //printf("init days_in_month(%d)=%d\n",now->month,days_in_month);

	sum->day    += now->day    + credit->day;
        //printf("before while: sum->day=%d\n",sum->day);

	while(sum->day > days_in_month) { 

            sum->day-=days_in_month; 

            sum->month++; if(sum->month > 12) { sum->year++; sum->month-=12; }
            //printf("in while: sum->month=%d\n",sum->month);

	    days_in_month = get_days_in_month(sum);
            //printf("days_in_month(%d)=%d  day=%d\n",sum->month,days_in_month,sum->day);
        }

}

int main(void)
{
  Time now,now_incr,sum,credit,delta;
  int8_t givendays=30;
  int8_t days_left;
  uint16_t many_days=999;
  uint16_t many_days_i;;
  uint16_t cnt_days;

  now.year=1;
  now.month=1;
  now.day=1;
  now.hour=0;
  now.minute=0;


  int i;
  for(i=0;i<1000;i++) {
    printf("\nrun %d\n",i);

    many_days_i=many_days;
    cnt_days=0;
    while(many_days_i > 0) {
        int8_t d;
        if(many_days_i > 96) d=96; else d=(int8_t)many_days_i;
        many_days_i -= d;
        cnt_days+= d;
        printf("d=%d many_days_i=%d cnt_days=%d\n",d,many_days_i,cnt_days);

        credit.year  =0;//get_number(1,10);
        credit.month =0;//get_number(1,12);
    
        //            day goes 1-31 max number for increment days is 127-31=96 to prevent overflow
        credit.day   =d; //get_number(30,96);//1;//get_number(1,63);//get_number(1,31);//givendays;
        credit.hour  =0;//get_number(0,23);
        credit.minute=0;//get_number(0,59);
    
        Time_add(&sum, &now, &credit);
        printf(" time:%02d-%02d-%02d %02d:%02d\n",now.year,now.month,now.day,now.hour,now.minute);
        printf(" cred:%02d-%02d-%02d %02d:%02d\n",credit.year,credit.month,credit.day,credit.hour,credit.minute);
        printf("  sum:%02d-%02d-%02d %02d:%02d\n",sum.year,sum.month,sum.day,sum.hour,sum.minute);
    }


    while(1) {
        printf(" time:%02d-%02d-%02d %02d:%02d\t",now.year,now.month,now.day,now.hour,now.minute);
        days_left = Days_left(&sum,&now);
        printf("days left=%d\n",days_left);
        if(days_left<0) break;
        /*
        if(days_left != credit.day) {
            printf("ERROR days_left != credit.day\n");
            exit(0);
        }
        */
        credit.year  =0;
        credit.month =0;
        credit.day   =0;
        credit.hour  =0;
        credit.minute=1;
        Time_add(&now_incr, &now, &credit);
        now.year  =now_incr.year;
        now.month =now_incr.month;
        now.day   =now_incr.day;
        now.hour  =now_incr.hour;
        now.minute=now_incr.minute;
    }

    now.year  =sum.year;
    now.month =sum.month;
    now.day   =sum.day;
    now.hour  =sum.hour;
    now.minute=sum.minute;
    check_Time(&now);

    if(now.year > 100) now.year=1; // restart
  }
}
