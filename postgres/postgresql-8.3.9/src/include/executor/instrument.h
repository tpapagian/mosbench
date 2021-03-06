/*-------------------------------------------------------------------------
 *
 * instrument.h
 *	  definitions for run-time statistics collection
 *
 *
 * Copyright (c) 2001-2008, PostgreSQL Global Development Group
 *
 * $PostgreSQL: pgsql/src/include/executor/instrument.h,v 1.18 2008/01/01 19:45:57 momjian Exp $
 *
 *-------------------------------------------------------------------------
 */
#ifndef INSTRUMENT_H
#define INSTRUMENT_H

#include <sys/time.h>


/*
 * gettimeofday() does not have sufficient resolution on Windows,
 * so we must use QueryPerformanceCounter() instead.  These macros
 * also give some breathing room to use other high-precision-timing APIs
 * on yet other platforms.	(The macro-ization is not complete, however;
 * see subtraction code in instrument.c and explain.c.)
 */
#ifndef WIN32

typedef struct timeval instr_time;

#define INSTR_TIME_IS_ZERO(t)	((t).tv_sec == 0 && (t).tv_usec == 0)
#define INSTR_TIME_SET_ZERO(t)	((t).tv_sec = 0, (t).tv_usec = 0)
#define INSTR_TIME_SET_CURRENT(t)	gettimeofday(&(t), NULL)
#define INSTR_TIME_GET_DOUBLE(t) \
	(((double) (t).tv_sec) + ((double) (t).tv_usec) / 1000000.0)

#define INSTR_TIME_SUBTRACT(x,y) \
 do { \
  (x).tv_sec -= (y).tv_sec; \
  (x).tv_usec -= (y).tv_usec; \
  /* Normalize */ \
  while ((x).tv_usec < 0) \
  { \
   (x).tv_usec += 1000000; \
   (x).tv_sec--; \
  } \
 } while (0)

#define INSTR_TIME_ACCUM_DIFF(x,y,z) \
 do { \
  (x).tv_sec += (y).tv_sec - (z).tv_sec; \
  (x).tv_usec += (y).tv_usec - (z).tv_usec; \
  /* Normalize after each add to avoid overflow/underflow of tv_usec */ \
  while ((x).tv_usec < 0) \
  { \
   (x).tv_usec += 1000000; \
   (x).tv_sec--; \
  } \
  while ((x).tv_usec >= 1000000) \
  { \
   (x).tv_usec -= 1000000; \
   (x).tv_sec++; \
  } \
 } while (0)

#else							/* WIN32 */

typedef LARGE_INTEGER instr_time;

#define INSTR_TIME_IS_ZERO(t)	((t).QuadPart == 0)
#define INSTR_TIME_SET_ZERO(t)	((t).QuadPart = 0)
#define INSTR_TIME_SET_CURRENT(t)	QueryPerformanceCounter(&(t))
#define INSTR_TIME_GET_DOUBLE(t) \
	(((double) (t).QuadPart) / GetTimerFrequency())

static __inline__ double
GetTimerFrequency(void)
{
	LARGE_INTEGER f;

	QueryPerformanceFrequency(&f);
	return (double) f.QuadPart;
}
#endif   /* WIN32 */


typedef struct Instrumentation
{
	/* Info about current plan cycle: */
	bool		running;		/* TRUE if we've completed first tuple */
	instr_time	starttime;		/* Start time of current iteration of node */
	instr_time	counter;		/* Accumulated runtime for this node */
	double		firsttuple;		/* Time for first tuple of this cycle */
	double		tuplecount;		/* Tuples emitted so far this cycle */
	/* Accumulated statistics across all completed cycles: */
	double		startup;		/* Total startup time (in seconds) */
	double		total;			/* Total total time (in seconds) */
	double		ntuples;		/* Total tuples produced */
	double		nloops;			/* # of run cycles for this node */
} Instrumentation;

extern Instrumentation *InstrAlloc(int n);
extern void InstrStartNode(Instrumentation *instr);
extern void InstrStopNode(Instrumentation *instr, double nTuples);
extern void InstrEndLoop(Instrumentation *instr);

#endif   /* INSTRUMENT_H */
