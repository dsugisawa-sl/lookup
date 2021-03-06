//
// Created by dsugisawa on 2017/07/13.
//

#ifndef LOOKUP_LOOKUP_HH
#define LOOKUP_LOOKUP_HH
#include "lookup.hpp"

#ifndef MIN
# define MIN(a,b) (a<b?a:b)
#endif
#ifndef MAX
# define MAX(a,b) (a>b?a:b)
#endif
#ifndef RETOK
# define RETOK  (0)
#endif
#ifndef RETERR
# define RETERR  (-1)
#endif
#ifndef STAT_ON
# define STAT_ON        (1)
#endif
#ifndef STAT_OFF
# define STAT_OFF       (2)
#endif


namespace LOOKUP{

  static pthread_mutex_t init_mtx_;
  //
  template<typename T,int S,typename K> Lookup<T,S,K>* Lookup<T,S,K>::Create(void){
      return(new Lookup<T,S,K>());
  }
  template<typename T,int S,typename K> Lookup<T,S,K>* Lookup<T,S,K>::Init(void){
      static Lookup<T,S,K>* lookup = NULL;
      pthread_mutex_lock(&init_mtx_);
      if (!lookup){
          lookup = Lookup<T,S,K>::Create();
      }
      pthread_mutex_unlock(&init_mtx_);
      return(lookup);
  }
  template<typename T,int S,typename K> Lookup<T,S,K>::~Lookup(){
      for(int n = 0;n < S;n++){
          free(data_side_a_[n]);
          free(data_side_b_[n]);
      }
      free(end_);
  }
  template<typename T,int S,typename K> Lookup<T,S,K>::Lookup(){
      // instanciate.
      /*
       * [important]
       * initialization process must be called on the main thread.
       */
      if ((end_ = (T*)malloc(sizeof(T))) == NULL){
          throw "malloc for link_ptr(end)";
      }
      for(int n = 0;n < S;n++){
          if ((data_side_a_[n] = (T*)malloc(sizeof(T))) == NULL ||
              (data_side_b_[n] = (T*)malloc(sizeof(T))) == NULL){
              throw "malloc for link_ptr(side_[a/b])";
          }
      }
      Clear();
  }
  template<typename T,int S,typename K> void Lookup<T,S,K>::Clock(void){
      virtual_clock_counter_++;
      // fprintf(stderr, "VCLOCK[%p]..vcc :" FMT_LLU "/vpc :" FMT_LLU "\n",(void*)this, virtual_clock_counter, virtual_clock_counter_prv);
  }
  template<typename T,int S,typename K> void Lookup<T,S,K>::Clear(){
      int n;
      memset(data_side_bmp_,0,sizeof(data_side_bmp_));
      memset(data_notice_bmp_,0,sizeof(data_notice_bmp_));
      for(n = 0;n < S;n++){
          memset(data_side_a_[n], 0, sizeof(*data_side_a_[n]));
          memset(data_side_b_[n], 0, sizeof(*data_side_b_[n]));
      }
      memset(end_,  0, sizeof(*end_));
      virtual_clock_counter_ = 0;
      virtual_clock_counter_prv_ = 0;
  }
  template<typename T,int S,typename K> void Lookup<T,S,K>::ClearNoticeBmp64(const K gcnt){
      if (gcnt <= S){
          data_notice_bmp_[gcnt>>6] = 0;
      }
  }
  template<typename T,int S,typename K> typename Lookup<T,S,K>::Iterator Lookup<T,S,K>::Find(const K gcnt, const int flag){
      /*
       * find process
       * 1. determine side of valid
       * 2. return valid side data
       */
      if (gcnt > (S-1)){
          return(End());
      }
      uint64_t bits = (((uint64_t)1)<<(gcnt - ((gcnt>>6)<<6)));
      if (flag){
          // back side is always return.
          if (!(data_side_bmp_[(gcnt>>6)]&bits)){
              return(Iterator(data_side_a_[gcnt]));
          }else{
              return(Iterator(data_side_b_[gcnt]));
          }
      }else{
          if ((data_side_bmp_[(gcnt>>6)]&bits)){
              if (data_side_a_[gcnt]->stat.valid==STAT_ON){
                  return(Iterator(data_side_a_[gcnt]));
              }
          }else{
              if (data_side_b_[gcnt]->stat.valid==STAT_ON){
                  return(Iterator(data_side_b_[gcnt]));
              }
          }
      }
//    fprintf(stderr, "failed..stat.valid!=ON .find(%u - %u)\n", key , gcnt);
      return(End());
  }
  template<typename T,int S,typename K> uint64_t Lookup<T,S,K>::FindNoticeBmp64(const K gcnt){
      if (gcnt > (S-1)){
          return(0);
      }
      return(data_notice_bmp_[(gcnt>>6)]);
  }

  template<typename T,int S,typename K> typename Lookup<T,S,K>::Iterator Lookup<T,S,K>::End(){ return(Lookup<T,S,K>::Iterator(end_)); }
  template<typename T,int S,typename K> T& Lookup<T,S,K>::operator[](const K gcnt){
      if (gcnt > (S-1)){
          fprintf(stderr, "capability over ,link operator[] %u - %u\n", gcnt, S);
          throw "invalid index";
      }
      Iterator itr = Find(gcnt);
      if (itr != End()){
          itr->stat.valid = STAT_ON;
          return((*itr));
      }
      itr = Find(gcnt, 1);
      if (itr == End()){
          throw "exception operator[]";
      }
      itr->stat.valid = STAT_ON;
      SwapSide(gcnt);
      return((*itr));
  }
  template<typename T,int S,typename K> void Lookup<T,S,K>::SwapSide(const K gcnt){
      uint64_t bits = (((uint64_t)1)<<(gcnt - ((gcnt>>6)<<6)));
      data_side_bmp_[(gcnt>>6)]^=bits;
  }
  template<typename T,int S,typename K> void Lookup<T,S,K>::NotifyChange(const K gcnt){
      uint64_t bits = (((uint64_t)1)<<(gcnt - ((gcnt>>6)<<6)));
      data_notice_bmp_[(gcnt>>6)]^=bits;
  }
  template <typename T,int S,typename K> void Lookup<T,S,K>::EnumerateForUpdate(std::function<void(T*,void*)> func, void* udata){
      K gcnt;
      //
      for(gcnt = 1; gcnt < S;gcnt++){
          uint64_t bits = (((uint64_t)1)<<(gcnt - ((gcnt>>6)<<6)));
          // return at back side.
          if (data_side_bmp_[(gcnt>>6)]&bits){
              *(data_side_b_[gcnt]) = *(data_side_a_[gcnt]);
              func(data_side_b_[gcnt],udata);
              data_side_b_[gcnt]->stat.valid  = STAT_ON;
          }else{
              *(data_side_a_[gcnt]) = *(data_side_b_[gcnt]);
              func(data_side_a_[gcnt],udata);
              data_side_a_[gcnt]->stat.valid  = STAT_ON;
          }
      }
      // 最後にまとめて、ビット変更、ひっくり返す
      for(gcnt = 0; gcnt < (S>>6);gcnt++){
          data_side_bmp_[gcnt]^=((uint64_t)-1);
      }
  }
  template<typename T,int S,typename K>int Lookup<T,S,K>::Add(K gcnt, T* itm,int nochk_counter){
      Lookup<T,S,K>::Iterator itr = NULL;
      // read phase
      if (nochk_counter){
          if (virtual_clock_counter_ <= virtual_clock_counter_prv_){
              fprintf(stderr, "failed[%p]..add(%u) vcc :%llu/vpc :%llu\n",(void*)this, gcnt, virtual_clock_counter_, virtual_clock_counter_prv_);
              return(RETERR);
          }
          virtual_clock_counter_prv_ = virtual_clock_counter_;
      }
      itm->stat.valid = STAT_ON;

      // access back side.
      if ((itr = Find(gcnt, 1)) == End()){
          fprintf(stderr, "failed[%p]..add(%u) vcc :%llu/vpc :%llu\n",(void*)this, gcnt, virtual_clock_counter_, virtual_clock_counter_prv_);
          throw "exception not found(add_link).";
      }
      // update and swap.
      memcpy(&(*itr), itm, sizeof(*itm));
      SwapSide(gcnt);
      NotifyChange(gcnt);
      //
      return(RETOK);
  }
  template<typename T,int S,typename K>int Lookup<T,S,K>::Del(K gcnt, int nochk_counter){
      Lookup<T,S,K>::Iterator itr = NULL;
      if (nochk_counter){
          // read phase
          if (virtual_clock_counter_ <= virtual_clock_counter_prv_){
              fprintf(stderr, "failed[%p]..delete(%u) vcc :%llu/vpc :%llu\n",(void*)this, gcnt, virtual_clock_counter_, virtual_clock_counter_prv_);
              return(RETERR);
          }
          virtual_clock_counter_prv_ = virtual_clock_counter_;
      }
      // access back side.
      if ((itr = Find(gcnt, 1)) == End()){
          throw "exception not found(add_link).";
      }
      // update and swap.
      itr->stat.valid = STAT_OFF;
      SwapSide(gcnt);
      NotifyChange(gcnt);
      //
      return(RETOK);
  }
}; // namespace LOOKUP

#endif //LOOKUP_LOOKUP_HH
