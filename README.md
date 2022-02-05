# AviUtl プラグイン - フィルタドラッグ移動

* version 4.0.1 by 蛇色 - 2022/02/05 標準描画がないオブジェクトにも対応
* version 4.0.0 by 蛇色 - 2022/02/05 スクロールに対応
* version 3.0.0 by 蛇色 - 2022/02/04 IME からの UNICODE 文字列の入力に対応
* version 2.0.0 by 蛇色 - 2022/02/03 ドラッグ元とドラッグ先をマークするように変更
* version 1.0.0 by 蛇色 - 2022/02/02 初版

オブジェクトダイアログのフィルタをドラッグして移動できるようにします。

## 導入方法

1. 以下のファイルを AviUtl の Plugins フォルダに配置します。
	* DragFilter.auf

## 使い方

1. オブジェクトダイアログでフィルタ (の何もない所) をドラッグします。
2. マウスカーソルが変化します。
3. 移動させたい場所 (他のフィルタの上) にドロップします。

## 設定

今のところありません。

## 動作確認

* (必須) AviUtl 1.10 & 拡張編集 0.92 http://spring-fragrance.mints.ne.jp/aviutl/
* (共存確認) patch.aul r10 https://www.nicovideo.jp/watch/sm39491708
* (共存確認) eclipse_fast 1.00 https://www.nicovideo.jp/watch/sm39756003
