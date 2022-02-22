# AviUtl プラグイン - フィルタドラッグ移動

* version 7.0.0 by 蛇色 - 2022/02/22 ターゲットマークを追加
* version 6.0.0 by 蛇色 - 2022/02/18 WideDialog.auf に対応
* version 5.0.5 by 蛇色 - 2022/02/15 ドラッグ中に他の操作をしたときの問題を修正
* version 5.0.4 by 蛇色 - 2022/02/11 特定状況で強制終了する問題を修正
* version 5.0.3 by 蛇色 - 2022/02/06 UNICODE 文字列入力対応機能を削除
* version 5.0.2 by 蛇色 - 2022/02/06 別のオブジェクトを参照してしまう問題などを修正
* version 5.0.1 by 蛇色 - 2022/02/06 複製できないフィルタを除外していなかった問題などを修正
* version 5.0.0 by 蛇色 - 2022/02/05 同じフィルタ効果もしくは完全な複製を作成する機能を追加
* version 4.0.1 by 蛇色 - 2022/02/05 標準描画がないオブジェクトにも対応
* version 4.0.0 by 蛇色 - 2022/02/05 スクロールに対応
* version 3.0.0 by 蛇色 - 2022/02/04 IME からの UNICODE 文字列の入力に対応
* version 2.0.0 by 蛇色 - 2022/02/03 ドラッグ元とドラッグ先をマークするように変更
* version 1.0.0 by 蛇色 - 2022/02/02 初版

オブジェクトダイアログのフィルタをドラッグして移動できるようにします。

![フィルタドラッグ移動](https://user-images.githubusercontent.com/96464759/154659767-e7e5ac3a-c181-4300-872a-1304905210c1.png)

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

## 設定

DragFilter.ini をテキストエディタで編集します。AviUtl を再起動しなくても設定ファイルを保存したときに再読み込みされます。

```ini
[TargetMark]
alpha=192 ; ターゲットマーク全体のアルファ値。
penColor=192,0,0,0 ; ペンの色。
penWidth=4.0f ; ペンの幅。
brushColor=255,255,255,255 ; ブラシの色。
base=16 ; 描画アイテムの基準サイズ。
width=8 ; 描画アイテムの幅。
fontName=Segoe UI ; フォント名。
fontSize=32.0 ; フォントの大きさ。
rotate=7.77 ; 傾ける角度。
beginMoveX=0 ; 表示開始時の移動アニメーション起点。
beginMoveY=100 ; 表示開始時の移動アニメーション起点。
```

## 動作確認

* (必須) AviUtl 1.10 & 拡張編集 0.92 http://spring-fragrance.mints.ne.jp/aviutl/
* (共存確認) patch.aul r11 https://www.nicovideo.jp/watch/sm40027189
* (共存確認) eclipse_fast 1.00 https://www.nicovideo.jp/watch/sm39756003
* (共存確認) WideDialog.auf 1.03 https://www.nicovideo.jp/watch/sm39708120
* (共存確認) 設定ダイアログ画面サイズ固定化プラグイン 2.6 https://github.com/amate/PropertyWindowFixerPlugin
	* ただし、eclipse_fast、bakusoku.auf、「エディットボックス最適化」などのプラグインが別途必要。
