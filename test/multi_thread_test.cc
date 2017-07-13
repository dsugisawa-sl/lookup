//
// Created by dsugisawa on 2017/07/13.
//

#include "gtest/gtest.h"

#include "lookup.hpp"
#include "test.h"

using namespace LOOKUP;



static Lookup<link_t,65536,__be16> *s_tbl_ = NULL;
static boost::mutex                    s_mutex_;
static boost::thread_group             s_threads_;

TEST(MultiThread, Init){
    SetAffinity(15);

    s_tbl_ = Lookup<link_t,65536,__be16>::Create();
    EXPECT_NE((void*)s_tbl_, (void*)NULL);
}
//
TEST(MultiThread, Write){
    uint64_t counter = 0;
    srand(time(NULL));
    uint32_t    randidx[65536] = {0,};
    for(uint64_t n = 0;n<65536;n++){
        randidx[n] = rand()%65536;
    }
    //
    auto st = msec();
    s_threads_.create_thread([&]{
        SetAffinity(22);

        for(int c = 0;c < 1000;c++){
            for(uint64_t n = 0;n<65536;n++){
                link_t itm;
                bzero(&itm, sizeof(itm));
                if (c == 0){
                    itm.counter_00 = n+1;
                    itm.counter_01 = n+2;
                    itm.counter_02 = n+3;
                    itm.counter_03 = n+4;
                    EXPECT_EQ(s_tbl_->Add((__be16)(200+n),&itm, 0), RETOK);
                }else{
                    itm.counter_00 = randidx[n]+1;
                    itm.counter_01 = randidx[n]+2;
                    itm.counter_02 = randidx[n]+3;
                    itm.counter_03 = randidx[n]+4;
                    EXPECT_EQ(s_tbl_->Add((__be16)(200+randidx[n]),&itm, 0), RETOK);
                }
                //
                counter++;
            }
        }
    });
    s_threads_.join_all();
    auto et = msec();
    char bf[128] = {0};
    //
    fprintf(stderr, "### write ###\n %s pps\n#############\n", Norm(bf, (double)counter*1000000.0e0/(double)(et - st)));
    fprintf(stderr, "### write ###\n %s count\n%u us\n#############\n", Norm(bf, (double)counter), (et - st));
}
//
TEST(MultiThread, Read){
    uint64_t counter = 0,err_counter = 0;
    srand(time(NULL));
    uint32_t    randidx[65536] = {0,};
    for(uint64_t n = 0;n<65536;n++){
        randidx[n] = rand()%65536;
    }

    SetAffinity(15);
    //
    auto st = msec();
    // at main thread
    for(int c = 0;c < 100;c++){
        for(uint64_t n = 0;n<65536;n++){
            auto it = s_tbl_->Find(200+randidx[n],0);
            EXPECT_EQ(it==s_tbl_->End(), false);
            if (it != s_tbl_->End()){
                EXPECT_EQ(it->counter_00, (randidx[n]+1));
                EXPECT_EQ(it->counter_01, (randidx[n]+2));
                EXPECT_EQ(it->counter_02, (randidx[n]+3));
                EXPECT_EQ(it->counter_03, (randidx[n]+4));
                counter++;
            }else{
                fprintf(stderr, "not found?(%u)\n", 200+randidx[n]);
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

TEST(MultiThread, ReadWrite){
    uint64_t counter = 0,rcounter = 0;
    unsigned long long r_st = 0,r_et = 0;
    int done = 0;
    //
    s_threads_.create_thread([&]{
        srand(time(NULL));
        uint32_t    randidx[65536] = {0,};
        for(uint64_t n = 0;n<65536;n++){
            randidx[n] = rand()%65536;
        }

        SetAffinity(22);
        for(int c = 0;c < 10;c++){
            for(uint64_t n = 0;n<65536;n+=(c>0?17:1)){
                link_t itm;
                bzero(&itm, sizeof(itm));
                itm.counter_00 = randidx[n]+2;
                itm.counter_01 = randidx[n]+3;
                itm.counter_02 = randidx[n]+4;
                itm.counter_03 = randidx[n]+5;
                //
                EXPECT_EQ(s_tbl_->Add((__be16)(200+randidx[n]),&itm, 0), RETOK);
                counter++;
                if (c > 0){ usleep(10); }
            }
            done = 1;
        }
        done=2;
    });

    s_threads_.create_thread([&]{

        SetAffinity(15);
        while(1){
            if (done == 0){ usleep(100); continue; }
            break;
        }
        srand(time(NULL));
        uint32_t    randidx[65536] = {0,};
        for(uint64_t n = 0;n<65536;n++){
            randidx[n] = rand()%65536;
        }

        r_st = msec();
        // at main thread
        for(int c = 0;c < 100;c++){
            for(uint64_t n = 0;n<65536;n++){
                auto it = s_tbl_->Find(200+randidx[n],0);
                EXPECT_EQ(it == s_tbl_->End(), false);
                if (it != s_tbl_->End()){
                    EXPECT_EQ(it->counter_00, (randidx[n]+2));
                    EXPECT_EQ(it->counter_01, (randidx[n]+3));
                    EXPECT_EQ(it->counter_02,   (randidx[n]+4));
                    EXPECT_EQ(it->counter_03,   (randidx[n]+5));
                    rcounter++;
                }
                if (done == 2){ break; }
            }
        }
        r_et = msec();
    });

    s_threads_.join_all();
    auto et = msec();
    char bf[128] = {0};
    //
    fprintf(stderr, "### read/write ###\n %s pps\n#############\n", Norm(bf, (double)rcounter*1000000.0e0/(double)(r_et - r_st)));
}


TEST(MultiThread, Finish){
    if (s_tbl_ != NULL){ delete s_tbl_; }
    s_tbl_ = NULL;
}
