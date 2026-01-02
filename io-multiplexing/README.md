# select, epoll study

## Build -> 実行
```
gcc -Wall -Wextra -std=c11 -O2 -o select_http select.c
./select_http
```

## socketプログラミングの流れ
- ソケットを作成
    - TCPのソケットを作成
- ソケットに紐づいたファイルディスクリプタを返す
    - 
- connect 関数で接続
- send, recv関数でデータの送受信
    - 双方向で通信
    - この時の読み込みできる状態になっているかの確認としてselectを利用する
