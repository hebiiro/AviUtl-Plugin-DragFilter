# AviUtl プラグイン - フィルタドラッグ移動

オブジェクトダイアログのフィルタをドラッグして移動できるようにします。
[最新バージョンをダウンロード](../../releases/latest/)

![フィルタドラッグ移動 7 0 0 トリム後](https://user-images.githubusercontent.com/96464759/155107551-2556fb10-3f99-4000-ac7b-199a8476bebf.png)

## 導入方法

1. 以下のファイルを AviUtl の Plugins フォルダに配置します。
	* DragFilter.auf
	* DragFilter.ini

## ドラッグ移動の仕方

1. オブジェクトダイアログでフィルタ (の何もない所) をドラッグします。
2. マウスカーソルが変化します。
3. 移動させたい場所 (他のフィルタの上) にドロップします。

## 複製の作り方

1. フィルタ効果を右クリックします。
	1. コンテキストメニューから「完全な複製を隣に作成」を選択すると、数値まで複製されたフィルタ効果が作成されます。
	2. コンテキストメニューから「同じフィルタ効果を上に作成」を選択すると、同じフィルタ効果がすぐ上に作成されます。
	3. コンテキストメニューから「同じフィルタ効果を下に作成」を選択すると、同じフィルタ効果がすぐ下に作成されます。

## 設定方法

DragFilter.ini をテキストエディタで編集します。AviUtl を再起動しなくても設定ファイルを保存したときに再読み込みされます。

```ini
[TargetMark]
alpha=192 ; ターゲットマーク全体のアルファ値。
penColor=192,0,0,0 ; ペンの色。#argb(16進数)、#aarrggbb(16進数)、a,r,g,b(10進数) の形式で指定する。
penWidth=4.0 ; ペンの幅。
brushColor=255,255,255,255 ; ブラシの色。
base=16 ; 描画アイテムの基準サイズ。
width=8 ; 描画アイテムの幅。
fontName=Segoe UI ; フォント名。
fontSize=32.0 ; フォントの大きさ。
rotate=7.77 ; 傾ける角度。
beginMoveX=0 ; 表示開始時の移動アニメーション起点。
beginMoveY=100 ; 表示開始時の移動アニメーション起点。
[Settings]
dragSrcColor=0,0,255 ; ドラッグ元の色。
dragDstColor=255,0,0 ; ドラッグ先の色。
```

## 更新履歴

* 9.2.0 - 2022/12/15 ドラッグ元とドラッグ先の色を変更できるように修正
* 9.1.0 - 2022/07/14 リファクタリング
* 9.0.0 - 2022/04/13 リファクタリング
* 8.0.0 - 2022/03/01 設定ダイアログ画面サイズ固定化プラグイン使用時の不具合を減少
* 7.0.1 - 2022/02/25 アニメーション効果の名前が取得できていなかった問題を修正
* 7.0.0 - 2022/02/22 ターゲットマークを追加
* 6.0.0 - 2022/02/18 WideDialog.auf に対応
* 5.0.5 - 2022/02/15 ドラッグ中に他の操作をしたときの問題を修正
* 5.0.4 - 2022/02/11 特定状況で強制終了する問題を修正
* 5.0.3 - 2022/02/06 UNICODE 文字列入力対応機能を削除
* 5.0.2 - 2022/02/06 別のオブジェクトを参照してしまう問題などを修正
* 5.0.1 - 2022/02/06 複製できないフィルタを除外していなかった問題などを修正
* 5.0.0 - 2022/02/05 同じフィルタ効果もしくは完全な複製を作成する機能を追加
* 4.0.1 - 2022/02/05 標準描画がないオブジェクトにも対応
* 4.0.0 - 2022/02/05 スクロールに対応
* 3.0.0 - 2022/02/04 IME からの UNICODE 文字列の入力に対応
* 2.0.0 - 2022/02/03 ドラッグ元とドラッグ先をマークするように変更
* 1.0.0 - 2022/02/02 初版

## 動作確認

* (必須) AviUtl 1.10 & 拡張編集 0.92 http://spring-fragrance.mints.ne.jp/aviutl/
* (共存確認) patch.aul r42 https://scrapbox.io/ePi5131/patch.aul

## クレジット

* Microsoft Research Detours Package https://github.com/microsoft/Detours
* aviutl_exedit_sdk https://github.com/ePi5131/aviutl_exedit_sdk
* Common Library https://github.com/hebiiro/Common-Library

## 作成者情報
 
* 作成者 - 蛇色 (へびいろ)
* GitHub - https://github.com/hebiiro
* Twitter - https://twitter.com/io_hebiiro

## 免責事項

このプラグインおよび同梱物を使用したことによって生じたすべての障害・損害・不具合等に関しては、私と私の関係者および私の所属するいかなる団体・組織とも、一切の責任を負いません。各自の責任においてご使用ください。
