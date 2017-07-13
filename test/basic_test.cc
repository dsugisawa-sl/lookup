//
// Created by dsugisawa on 2017/07/13.
//
#include "gtest/gtest.h"

#include "lookup.hpp"
#include "test.h"

using namespace LOOKUP;
Lookup<link_t,65536,__be16> *tbl = NULL;

TEST(Basic, Test00){
    tbl = Lookup<link_t,65536,__be16>::Create();
    EXPECT_NE((void*)tbl, (void*)NULL);

    // 変更通知bit ON
    tbl->NotifyChange(12345);

    uint64_t    bmp64 = 0;
    // 変更通知bit 確認
    for(int n=0;n < (LOOKUPTBL_MAX_COUNT>>6);n++){
        if ((bmp64 = tbl->FindNoticeBmp64((__be16)n<<6)) != 0){
            fprintf(stderr, "bmp64 = (%llu)\n", bmp64);
            // 変更を検知したので、64bit boundary 領域の変更状態をクリア
            tbl->ClearNoticeBmp64(n<<6);
            //
            for(int m = 0;m < 64;m++){
                if (bmp64&(((uint64_t)1)<<m)){
                    // 変更通知のあるteid
                    auto cnv= (__be16)((n<<6)+m);
                    //
                    EXPECT_EQ((__be32)cnv,(__be32)12345);
                }
            }
            break;
        }
    }
    EXPECT_NE(bmp64,0);
    // 変更通知bit がクリアされていることを確認
    bmp64 = 0;
    for(int n=0;n < (LOOKUPTBL_MAX_COUNT>>6);n++){
        if ((bmp64 = tbl->FindNoticeBmp64((__be16)n<<6)) != 0){
            FAIL() << "why exist??";
        }
    }
    EXPECT_EQ(bmp64,0);
}
//
