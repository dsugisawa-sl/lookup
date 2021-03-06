//
// Created by dsugisawa on 2017/07/13.
//


#include "gtest/gtest.h"

#include "lookup.hpp"
#include "test.h"

using namespace LOOKUP;

#define LINKSIZE_17BIT  (131072)
#define BIT17(x)        ((__be32)(((uint32_t)x)%(uint32_t)(LINKSIZE_17BIT-2)))

static Lookup<link_t,LINKSIZE_17BIT,__be32> *s_tbl_17_ = NULL;
static boost::thread_group             s_threads_17_;
static boost::thread_group             s_threads_17_wr_;

TEST(MultiThread_17Bit, Init){
    SetAffinity(15);

    s_tbl_17_ = Lookup<link_t,LINKSIZE_17BIT,__be32>::Create();
    EXPECT_NE((void*)s_tbl_17_, (void*)NULL);


    for(uint32_t n = 0;n < LINKSIZE_17BIT;n++){
        EXPECT_EQ(BIT17(n+200)<(LINKSIZE_17BIT-2),true);
    }
}
//
TEST(MultiThread_17Bit, Write){
    uint64_t counter = 0;
    srand(time(NULL));
    uint32_t    randidx[LINKSIZE_17BIT] = {0,};
    for(uint64_t n = 0;n<LINKSIZE_17BIT;n++){
        randidx[n] = BIT17(rand()%LINKSIZE_17BIT);
    }
    //
    auto st = msec();
    s_threads_17_.create_thread([&]{
        SetAffinity(22);

        for(int c = 0;c < 1000;c++){
            for(uint64_t n = 0;n<LINKSIZE_17BIT;n++){
                link_t itm;
                bzero(&itm, sizeof(itm));
                if (c == 0){
                    itm.counter_00 = BIT17(n+1);
                    itm.counter_01 = BIT17(n+2);
                    itm.counter_02 = BIT17(n+3);
                    itm.counter_03 = BIT17(n+4);
                    EXPECT_EQ(s_tbl_17_->Add(BIT17(0+n),&itm, 0), RETOK);
                }else{
                    itm.counter_00 = BIT17(randidx[n]+1);
                    itm.counter_01 = BIT17(randidx[n]+2);
                    itm.counter_02 = BIT17(randidx[n]+3);
                    itm.counter_03 = BIT17(randidx[n]+4);
                    EXPECT_EQ(s_tbl_17_->Add(BIT17(0+randidx[n]),&itm, 0), RETOK);
                }
                //
                counter++;
            }
        }
    });
    s_threads_17_.join_all();
    auto et = msec();
    char bf[128] = {0};
    //
    fprintf(stderr, "### write ###\n %s pps\n#############\n", Norm(bf, (double)counter*1000000.0e0/(double)(et - st)));
    fprintf(stderr, "### write ###\n %s count\n%u us\n#############\n", Norm(bf, (double)counter), (et - st));
}
//
TEST(MultiThread_17Bit, Read){
    uint64_t counter = 0,err_counter = 0;
    srand(time(NULL));
    uint32_t    randidx[LINKSIZE_17BIT] = {0,};
    for(uint64_t n = 0;n<LINKSIZE_17BIT;n++){
        randidx[n] = BIT17(rand()%(LINKSIZE_17BIT-2));
    }

    SetAffinity(15);
    //
    auto st = msec();
    // at main thread
    for(int c = 0;c < 100;c++){
        for(uint64_t n = 0;n<LINKSIZE_17BIT;n++){
            auto it = s_tbl_17_->Find(BIT17(0+randidx[n]),0);
            EXPECT_EQ(it==s_tbl_17_->End(), false);
            if (it != s_tbl_17_->End()){
                EXPECT_EQ(it->counter_00, BIT17(randidx[n]+1));
                EXPECT_EQ(it->counter_01, BIT17(randidx[n]+2));
                EXPECT_EQ(it->counter_02, BIT17(randidx[n]+3));
                EXPECT_EQ(it->counter_03, BIT17(randidx[n]+4));
                counter++;
            }else{
                fprintf(stderr, "not found?(%u)\n", BIT17(0+randidx[n]));
                err_counter++;
            }
        }
    }
    auto et = msec();
    char bf[128] = {0};
    //
    fprintf(stderr, "###  read ###\n %s pps\n#############\n", Norm(bf, (double)counter*1000000.0e0/(double)(et - st)));
    fprintf(stderr, "###  err  ###\n %s pps\n#############\n", Norm(bf, (double)err_counter*1000000.0e0/(double)(et - st)));
}

TEST(MultiThread_17Bit, ReadWrite){
    uint64_t counter = 0,rcounter = 0,rcounter_e = 0;
    unsigned long long r_st = 0,r_et = 0;
    int done = 0;


    delete s_tbl_17_;
    s_tbl_17_ = NULL;
    //
    s_tbl_17_ = Lookup<link_t,LINKSIZE_17BIT,__be32>::Create();
    EXPECT_NE((void*)s_tbl_17_, (void*)NULL);

    srand(time(NULL));
    //
    s_threads_17_wr_.create_thread([&]{
        // 標準ではstack over flow....(bus error 10..)
        uint32_t    *randidx = (uint32_t*)malloc(LINKSIZE_17BIT*sizeof(uint32_t));
        bzero(randidx, LINKSIZE_17BIT*sizeof(uint32_t));
        //
        for(uint64_t n = 0;n<LINKSIZE_17BIT;n++){
            randidx[n] = BIT17(rand()%(LINKSIZE_17BIT-2));
        }

        SetAffinity(22);
        for(int c = 0;c < 10;c++){
            for(uint64_t n = 0;n<LINKSIZE_17BIT;n+=(c>0?17:1)){
                link_t itm;
                bzero(&itm, sizeof(itm));
                itm.counter_00 = BIT17(n+2);
                itm.counter_01 = BIT17(n+3);
                itm.counter_02 = BIT17(n+4);
                itm.counter_03 = BIT17(n+5);
                //
                EXPECT_EQ(s_tbl_17_->Add(BIT17(0+n),&itm, 0), RETOK);
                counter++;
                if (c > 0){ usleep(10); }
            }
            done = 1;
        }
        done=2;
        free(randidx);
    });

    s_threads_17_wr_.create_thread([&]{

        SetAffinity(15);
        while(1){
            if (done == 0){ usleep(100); continue; }
            break;
        }
        uint32_t    *randidx = (uint32_t*)malloc(LINKSIZE_17BIT*sizeof(uint32_t));
        bzero(randidx, LINKSIZE_17BIT*sizeof(uint32_t));

        for(uint64_t n = 0;n<LINKSIZE_17BIT;n++){
            randidx[n] = BIT17(rand()%(LINKSIZE_17BIT-2));
        }
        r_st = msec();
        // at main thread
        for(int c = 0;c < 100;c++){
            for(uint64_t n = 0;n<LINKSIZE_17BIT;n++){
                auto it = s_tbl_17_->Find(BIT17(0+randidx[n]),0);
                EXPECT_EQ(it == s_tbl_17_->End(), false);
                if (it != s_tbl_17_->End()){
                    if (it->counter_00 != BIT17(randidx[n]+2)){
//                      fprintf(stderr, ">>>>\ndiff[%u, %u, %u, %u]\n<<<<\n", it->counter_00,   BIT17(randidx[n]+2), n, randidx[n]);
                        rcounter_e++;
                        continue;
                    }
                    if (it->counter_01 != BIT17(randidx[n]+3)){
//                      fprintf(stderr, ">>>>\ndiff[%u, %u, %u, %u]\n<<<<\n", it->counter_01,   BIT17(randidx[n]+3), n, randidx[n]);
                        rcounter_e++;
                        continue;
                    }
                    if (it->counter_02 != BIT17(randidx[n]+4)){
//                      fprintf(stderr, ">>>>\ndiff[%u, %u, %u, %u]\n<<<<\n", it->counter_02,   BIT17(randidx[n]+4), n, randidx[n]);
                        rcounter_e++;
                        continue;
                    }
                    if (it->counter_03 != BIT17(randidx[n]+5)){
//                      fprintf(stderr, ">>>>\ndiff[%u, %u, %u, %u]\n<<<<\n", it->counter_03,   BIT17(randidx[n]+5), n, randidx[n]);
                        rcounter_e++;
                        continue;
                    }
                    EXPECT_EQ(it->counter_00,   BIT17(randidx[n]+2));
                    EXPECT_EQ(it->counter_01,   BIT17(randidx[n]+3));
                    EXPECT_EQ(it->counter_02,   BIT17(randidx[n]+4));
                    EXPECT_EQ(it->counter_03,   BIT17(randidx[n]+5));
                    rcounter++;
                }
                if (done == 2){ break; }
            }
        }
        r_et = msec();
    });

    s_threads_17_wr_.join_all();
    auto et = msec();
    char bf[128] = {0};
    //
    fprintf(stderr, "### read/write ###\n %s pps\n#############\n", Norm(bf, (double)rcounter*1000000.0e0/(double)(r_et - r_st)));
    fprintf(stderr, "### read/write error ###\n %s \n#############\n", Norm(bf, (double)rcounter_e));
    rcounter_e++;

}


TEST(MultiThread_17Bit, Finish){
    if (s_tbl_17_ != NULL){ delete s_tbl_17_; }
    s_tbl_17_ = NULL;
}
