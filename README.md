# PDF一括印刷

Windowsで、フォルダー内のPDFを既定のプリンターへまとめて送る小さなツールです。

**[ダウンロード（Windows 10 / 11・64ビット）](https://github.com/tosh-m12/pdf-batch-print/releases/latest/download/PDF-Batch-Print-v1.0.0-win64.zip)**

## 使い方

1. ZIPをダウンロードして「すべて展開」。
2. `PDF一括印刷.exe` を開く。
3. PDFが入ったフォルダーを選ぶ。
4. プリンターと対象を確認して「はい」。

インストール、PowerShell、追加ソフトのダウンロードは不要です。印刷処理に使うSumatraPDFを同梱しています。最初はPDF1件で確認してください。

## 動作

- 選んだフォルダー直下のPDFをファイル名順に送信。サブフォルダーは対象外。
- Windowsの既定のプリンターを使用。部数・色・両面などをプログラムから指定しません。
- 実際の印刷設定はプリンタードライバーとSumatraPDFの既定動作に従います。
- 複数PDFを1回の起動で処理。パスの合計がWindowsの上限を超える場合だけ分割します。
- 完了画面に送信時間と起動回数を表示。

送信完了は紙への印刷完了を意味しません。プリンター側の待ち時間やPDFごとの印刷ジョブ処理は残ります。エラー時には一部が送信済みのことがあるため、再実行前に印刷待ち一覧を確認してください。

## 確認状況

旧EXE版は利用者のWindows PCで印刷できたことを確認済みです。この版はビルドと配布内容を確認済みですが、速度改善効果は実機で未確認です。

会社PCでは、その会社のソフト利用ルールに従ってください。EXE自体の実行が制限されている場合は社内IT担当へ確認してください。

## ビルド

MinGW-w64のGCCでビルドできます。

```sh
x86_64-w64-mingw32-gcc -std=c11 -Wall -Wextra -Werror -Os -static -municode -mwindows src/print_pdfs.c -o PDF-Batch-Print.exe -lshell32 -lole32 -lwinspool
```

実行ファイルと同じフォルダーに `SumatraPDF.exe` を置いてください。

## ライセンス

このツールのソースコードはMITライセンスです。同梱のSumatraPDFは別のライセンスに従います。著作権表示・ライセンスは `third_party/` に、SumatraPDF 3.6.1のソース一式は同じ[リリースページ](https://github.com/tosh-m12/pdf-batch-print/releases/tag/v1.0.0)に掲載しています。
