# lookup

Numa（Umaでも動作する）CPUをターゲットとした、高速非ロックスレッド間Heapメモリ共有実装

# keyword

+ 1 Core 1 thread/1 Nic Ring
  + Nic割り込みをsmp_affinity設定
+ ランダムアクセスで高速
+ Mutex Lock しない -> context switch
+ protocol stackを使わない
+ コアスケール

## 前提条件

CPUコア間（スレッド間）を常に一方方向にメモリ共有が必要な場合に利用できる


## 基本設計

32bit整数値から、関連データ（以降、リンクデータと記述）をlookupする。
例えば、ユーザidからそのユーザの属性データを走査し
ユーザ属性データによって分岐した処理を実施する機能である。

リンクデータは、[A]スレッド(CPU[A])でのみ挿入・更新・削除処理が実施し
[B]スレッド(CPU[B])でのみ参照処理を実施する

lookupテンプレートクラスは、CPU[A]とCPU[B]間でデータ共有する機能である

この場合、CPU[A]と、CPU[B]はそれぞれに動作しているので
同じメモリ領域を同時に、更新、参照することになる。
そうすると参照処理の途中に、更新処理が行われると参照処理で読み出したデータは
不定な値となることはマルチスレッドプログラミングにおいて自明である

スレッド間のデータ領域の同期処理は、セマフォ、ミューテクス等のOperation Systemの機能が
提供するsystem callによって同期、保護するのが一般的である

## 要求仕様

10Gbps - 40Gbps - 100Gbps で受信したネットワークパケット処理の
lookup機能に充分な高速lookup機能を実装する


|packet size|rate|Mpps|cost(ns)|
|---:|---|---:|---:|
|64|10Gbps|14.88|67.2|
|128|10Gbps|8.45|118.4|
|64|40Gbps|59.52|16.8|
|128|40Gbps|33.78|29.6|
|64|100Gbps|148.81|6.72|
|128|100Gbps|84.86|11.84|

![numa](https://github.com/dsugisawa-sl/lookup/blob/master/doc/0000.png)



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

