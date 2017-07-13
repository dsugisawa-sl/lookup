//
// Created by dsugisawa on 2017/07/13.
//

#ifndef LOOKUP_TEST_H
#define LOOKUP_TEST_H

#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>

#include "lookup.hpp"


#ifndef __u8
typedef unsigned char __u8;
#endif
#ifndef __u32
typedef uint32_t __u32;
#endif
#ifndef __be16
typedef uint16_t __be16;
#endif
#ifndef __be32
typedef uint32_t __be32;
#endif

#define LOOKUPTBL_MAX_COUNT     (65536)


#define  POLICER_COLOR_GREEN    (0)
#define  POLICER_COLOR_YELLOW   (1)
#define  POLICER_COLOR_RED      (2)
//
typedef struct policer{
  struct _stat{
      uint32_t    valid:1;
      uint32_t    epoch_w:16;
      uint32_t    padd00:15;
      uint32_t    padd01:32;
  }stat;
  //
  uint32_t    commit_rate;
  uint32_t    commit_burst_size;
  uint32_t    commit_information_rate;
  uint32_t    excess_rate;
  uint32_t    excess_burst_size;
  uint32_t    excess_information_rate;
}policer_t, *policer_ptr;
//
static_assert(LOOKUPTBL_MAX_COUNT%sizeof(policer_t)==0,   "need 32byte alligned(policer)");


//
typedef struct link {
    struct _stat{
        __be32 valid:1;
        __be32 type: 8;
        __be32 padd:23;
    }stat;
    //
    __be32      counter_00;
    __be32      counter_01;
    __be32      counter_02;
    __be32      counter_03;
    struct _group{
        __be32 gcnt:32;
    }group;
    //
    union _u {
        struct _bitrate{
            __be16      rate_00;
            __be16      rate_01;
            __be16      rate_02;
            __be16      rate_03;
        }bitrate;
    }u;
}__attribute__ ((packed)) link_t,*link_ptr;
//
static_assert(LOOKUPTBL_MAX_COUNT%sizeof(uint64_t)==0, "need 64bit  alligned");
static_assert(LOOKUPTBL_MAX_COUNT%64==0,               "need 64byte alligned");
static_assert(LOOKUPTBL_MAX_COUNT%sizeof(link_t)==0,   "need 32byte alligned");


static inline unsigned long long msec(void){
    struct timeval	tv;
    gettimeofday(&tv,NULL);
    return((((uint64_t)tv.tv_sec * 1000000) + ((uint64_t)tv.tv_usec)));
}

static inline const char *Norm2(char *buf, double val, const char *fmt){
    const char *units[] = { "", "K", "M", "G", "T" };
    u_int i;
    for (i = 0; val >=1000 && i < sizeof(units)/sizeof(char *) - 1; i++)
        val /= 1000;
    sprintf(buf, fmt, val, units[i]);
    return(buf);
}
static inline const char *Norm(char *buf, double val){
    return Norm2(buf, val, "%.3f %s");
}

static inline void SetAffinity(int id){
#ifndef __APPLE__
    cpu_set_t cpumask;
      CPU_ZERO(&cpumask);
      CPU_SET(cpuid, &cpumask);
      //
      if (cpuid >= 0){
          if (pthread_setaffinity_np(pthread_self(), sizeof(cpumask), &cpumask) != 0) {
              fprintf(stderr, "Unable to set affinity: %s\n", strerror(errno));
          }
      }
#else
    fprintf(stderr, "not implmented(uma): %d\n", id);
#endif
}

#endif //LOOKUP_TEST_H
