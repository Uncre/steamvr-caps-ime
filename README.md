# SteamVR Caps IME

SteamVRのデスクトップ用キーボードで日本語を入力しやすくする、Windows x64向けのアドオンです。

## なぜ作ったか

SteamVR標準のデスクトップ用キーボードは英語入力向けです。私の環境では、Microsoft IMEを「あ」にしても半角英数字が入力され、日本語を打てませんでした。日本語入力と半角英数入力の切り替えには、タスクバーの「あ」または「A」をクリックする必要がありました。

物理キーボードと同じ感覚で日本語を入力できるように、このアドオンを作りました。

## 動作条件

- Windows x64、SteamVR、Microsoft IME（日本語）が必要です。
- 入力先で日本語の入力言語を選んでください。IMEの半角／全角キーに別の機能を割り当てている場合は、その設定に従います。
- 管理者権限で起動したアプリでは、Windowsの制限によりキー入力が届かない場合があります。

## インストールと更新

1. SteamとSteamVRを終了します。SteamVRをまだ起動したことがない場合は、一度起動してから終了します。
2. 配布ZIPを展開します。
3. 展開先の `Install.bat` をダブルクリックします。

インストール先は `%LOCALAPPDATA%\SteamVRCapsIME\capsime` です。インストール後は展開フォルダを削除できます。更新も同じ手順です。

## 使い方

1. SteamVRダッシュボードの「デスクトップ」を開きます。
2. 入力欄を選び、デスクトップ用キーボードを開きます。
3. CapsでMicrosoft IMEの「あ」（ひらがな）と「A」（半角英数）を切り替えます。「あ」の状態でローマ字を入力し、Spaceで変換します。Tabや矢印キーで候補を選び、Enterで確定します。

インストールや更新の直後に反映されない場合は、SteamVRの起動中に次を実行します。Steamの画面が一時的に閉じるため、入力途中の内容は先に保存してください。

```powershell
& "$env:LOCALAPPDATA\SteamVRCapsIME\capsime\ReloadSteamUi.ps1"
```

## アンインストール

SteamVRを終了し、エクスプローラーで `%LOCALAPPDATA%\SteamVRCapsIME\capsime` を開いて `Uninstall.bat` をダブルクリックします。

診断ログとバックアップはインストール先に残ります。

## 仕組み

OpenVRドライバーはSteamVRの起動時にSteamの `steamui/virtualkeyboard.js` に処理を追加し、正常終了時にその処理を取り除きます。

デスクトップ用キーボードから操作を受け取るため、ドライバーは `127.0.0.1:27391` で待ち受けます。ドライバーが入力先のアプリにWindowsのキー入力を送ると、Microsoft IMEがローマ字入力や変換を処理します。

Steamの更新でキーボードの実装が変わると、動作しなくなる場合があります。診断ログはインストール先の `bin/win64/caps-ime.log` にあります。

## ソースからビルド

ビルドにはMinGW-w64の `g++` が必要です。テストにはPython 3とNode.jsも使います。

```powershell
.\tools\build.ps1
.\tools\test.ps1
```

配布用ファイルは `outputs/SteamVRCapsIME-Setup` に生成されます。

## ライセンス

プロジェクト本体はMITライセンスです。OpenVR SDKにはValveのBSD 3-Clauseライセンスが適用されます。配布ZIPには `LICENSE` と `OpenVR-LICENSE.txt` を同梱しています。

## 注記

このプロジェクトはCodexを利用して作成しています。
