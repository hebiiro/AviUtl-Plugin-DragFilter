# AviUtl プラグイン - フィルタドラッグ移動

* version 5.0.1 by 蛇色 - 2022/02/06 複製できないフィルタを除外していなかった問題などを修正
* version 5.0.0 by 蛇色 - 2022/02/05 同じフィルタ効果もしくは完全な複製を作成する機能を追加
* version 4.0.1 by 蛇色 - 2022/02/05 標準描画がないオブジェクトにも対応
* version 4.0.0 by 蛇色 - 2022/02/05 スクロールに対応
* version 3.0.0 by 蛇色 - 2022/02/04 IME からの UNICODE 文字列の入力に対応
* version 2.0.0 by 蛇色 - 2022/02/03 ドラッグ元とドラッグ先をマークするように変更
* version 1.0.0 by 蛇色 - 2022/02/02 初版

オブジェクトダイアログのフィルタをドラッグして移動できるようにします。

## 導入方法

1. 以下のファイルを AviUtl の Plugins フォルダに配置します。
	* DragFilter.auf

## ドラッグ移動の仕方

1. オブジェクトダイアログでフィルタ (の何もない所) をドラッグします。
2. マウスカーソルが変化します。
3. 移動させたい場所 (他のフィルタの上) にドロップします。

## 複製の作り方

1. フィルタ効果を右クリックします。
	1. コンテキストメニューから「完全な複製を隣に作成」を選択すると、数値まで複製されたフィルタ効果が作成されます。
	2. コンテキストメニューから「同じフィルタ効果を上に作成」を選択すると、同じフィルタ効果がすぐ上に作成されます。
	3. コンテキストメニューから「同じフィルタ効果を下に作成」を選択すると、同じフィルタ効果がすぐ下に作成されます。

## 設定

今のところありません。

## 動作確認

* (必須) AviUtl 1.10 & 拡張編集 0.92 http://spring-fragrance.mints.ne.jp/aviutl/
* (共存確認) patch.aul r10 https://www.nicovideo.jp/watch/sm39491708
* (共存確認) eclipse_fast 1.00 https://www.nicovideo.jp/watch/sm39756003
