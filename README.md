# OSVC (Open source voice changer)
## 概要
RubberBand (ライブラリ) を用いてリアルタイムに低遅延なボイチェンをしたいソフト  

## 機能
```
--help                      ヘルプを表示します。  
--list-devices              音声の入出力デバイスを表示します。  
--chunklength [len: int]    処理単位をサンプル数で指定します。  
--rblength [len: int]       バッファの大きさを指定します。  
                            大きくすると音声が途切れにくくなりますが、遅延が大きくなります。  
--input-device [idx: int]   入力デバイスを指定します。  
                            (--list-devicesで表示された番号による)  
--output-device [idx: int]  出力デバイスを指定します。  
                            (--list-devicesで表示された番号による)  
--fsample [fs: float]       サンプリング周波数を指定します。  
--init-pitch                ピッチの初期値を指定します。  
--init-formant              フォルマント移動量の初期値を指定します。  
--mono                      出力をモノラル化します。  
                            Rubberbandで処理した際に左右で位相がずれてしまい違和感が生じる場合、力技で直します。  
--ll                        遅延を減らします。  
                            ただし若干品質が下がります。  
--thru                      音声に何も加工せずそのまま横流しします。  
                            (よってボイチェンとしては機能しません。)  
--tr-mix                    音楽的な周波数の範囲外を基準として、波形の位相を波形の立ち上がりに合わせます。  
--tr-smooth                 位相を操作しないようにします。  
--smoothing                 時系列でのスムージングを有効にします。  
--ch-together               左右チャンネル間での位相の差を最小限に抑えます。  
--show-buffer-health             バッファの状態と入出力レベルをバーで表示します。  
--barlength [len: int]           バー表示の長さを指定します。  
--show-interval [interval: int]  表示の更新間隔を指定します。(厳密なものではありません。)  
```

## ビルド・依存ライブラリについて
このソフトはPortAudioとRubberBandに依存しています。  
ビルド前に各ライブラリの開発用ファイルをインストールしてください。  
( 例: `librubberband-dev`, `portaudio19-dev` )
また、ビルドにはCMakeを利用しています。  

## 現状
Linux上で遅延は60msぐらいになっています。  
Windowsでの遅延もWASAPIを使えばそれくらいに収まります。  
ただしリサンプリング機能がないので、WASAPIで使う場合は入出力のサンプリング周波数を設定から合わせてください。  
また、ネットワーク周りの実装は（歴史的に）Linux/macOSで行っていた影響でWindows上だといろいろ不具合があり正常に動作しません。  
よってWindowsではリモコンが動作しない状態になっています。  
また、未実装の機能が割と存在します。  
