# lookup

Numa（Umaでも動作する）CPUをターゲットとした、高速非ロックスレッド間Heapメモリ共有実装

## 前提条件

CPUコア間（スレッド間）を常に一方方向にメモリ共有が必要な場合に利用できる

### 例）KVS的な利用

比較的小さなデータ領域を共有、そしてlookupはパケットプロセッシングから実行するので高速でなくてはならない

+ スレッド[A]
  + mysql スレーブサーバとして、binlogを監視
  + binlogイベント発生をトリガとしメモリを書き換え
+ スレッド[B]
  + パケットプロセッシングスレッド
  + 1回のパケットプロセッシングで、1回lookup
  

## 適用例

Key Value Storeの実装設計

+ ネットワークインタフェイス
+ インメモリ
+ 永続化
+ コンシステントハッシュ分散

等の基本機能がKVSには概ね必要であり、これらの基本設計に本テンプレートクラスと、
mysql-binlog、udp 、netmap、NIC@RSSを適用すると、１コア：1Mpps〜をさらに
スケールアウトすることが可能

## 類似実装

### boost::lockfree::spsc_queue

boost::lockfree::spsc_queue 、boost::atomic_uint64_t によるロックフリー実装による実装設計を検討評価した
しかし、スケールはするものの（１モジュールに必要なコア数＝３）CPUコアリソースが多く必要

### facebook lib

boostより若干高速、必要リソースは同様（１モジュールあたり、３コア）

### pthread_mutex 

pthread_mutex + std::map + バッチ処理 の実装設計は、非常に高速であったが
アイテムサイズ、バッチカウントによってパフォーマンスの揺らぎが大きかった

### 類似実装での課題

前述した、pthread_mutexを除く類似実装では、リソース使用量が多くなってしまう
主処理である、パケットプロセッシングのbusy-loopと、本来サブ処理であるconsumerスレッドbusy loopが競合してしまい、結果充分なパフォーマンスを実現するのが難しい
そして、pthread_mutex 実装は、ほとんど良いと言えるが、パラメータによる揺らぎが大きかった


# compile/link
## dependency
+ c++11
+ pthread
+ boost(テストに利用）

## usage(for test)
```
mkdir ./build
cd ./build
cmake ../
make
make test
```


### 補遺
Xeon-E5系実機でのcoreをまたいだ（numanodeをまたいだケースも）メモリコピーの実測値は numatest.c で確認できる
37GB/s の論理値が実現可能なことがわかっている

