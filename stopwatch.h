/* Subject	: Handles timing operations
 * Author	: Rakesh Malik
 * Date		: 20/04/2011
 */

#include <time.h>

typedef struct StopWatch
{
  unsigned long time;
  int stop;				/* =0(running)/1(stopped) */
}StopWatch;

time_t __timei,__timef;

void reset_timer(StopWatch *w);
void stop_timer(StopWatch *w);
void start_timer(StopWatch *w);
int increment_time(StopWatch *w);	/* returns 1 if time is incremented */

void reset_timer(StopWatch *w)
{
  w->time=0;
  start_timer(w);
}

void stop_timer(StopWatch *w)
{
  w->stop=1;
}

void start_timer(StopWatch *w)
{
  w->stop=0;
  __timei=time(&__timei);
  localtime(&__timei);
}

int increment_time(StopWatch *w)
{
  int diff;
  if(w->stop) return 0;
  __timef=time(&__timef);
  localtime(&__timef);
  if((diff=difftime(__timef,__timei))>=1)
  {
    __timei=time(&__timei);
    localtime(&__timei);
    w->time+=diff;
    return 1;
  }
  return 0;
}