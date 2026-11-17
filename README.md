# SteamVR Caps IME

SteamVRダッシュボードのデスクトップから開く内蔵キーボードで、Capsを日本語IMEの「あ ⇔ A」切り替えに使うWindows x64用アドオン。

## 使い方

インストール先は `outputs/SteamVRCapsIME-v2`。登録後はこのフォルダーを移動しないこと。

```powershell
& .\tools\build.ps1
& .\outputs\SteamVRCapsIME-v2\Install.ps1
```

Steamクライアントを起動した状態でSteamVRを起動・再起動する。対象アプリで日本語IMEを選び、テキスト欄にフォーカスして、デスクトップ用キーボードのCapsを押す。Windowsの入力言語やキーボード配列の変更は行わない。

Steam起動中にアドオンを新規インストール・更新すると、画面部が古いキーボードコードを保持している場合がある。SteamVRを起動した状態で `ReloadSteamUi.ps1` を実行すると画面部を読み込み直せる。この操作はSteamの画面部を強制終了するため、未送信のSteamチャットなどは事前に保存すること。通常起動時に毎回行う必要はない。

```powershell
& .\outputs\SteamVRCapsIME-v2\ReloadSteamUi.ps1
```

削除時はSteamVRを終了してから実行する。

```powershell
& .\outputs\SteamVRCapsIME-v2\Uninstall.ps1
```

## 仕組み

- `driver_capsime.dll` はSteamVRが読み込むOpenVRサーバードライバー。Windowsのカーネルドライバーではない。
- DLLが起動時に `steamui/virtualkeyboard.js` のCaps処理へ小さなアダプターを追加し、正常終了時には自身の追加部分だけを除去する。
- 対象が `system.desktop` または `system.desktop.N` のキーボードの場合だけ、CapsからDLLへローカル通信で指示する。Capsの大文字ロックは解除する。
- DLLは `127.0.0.1:27391` にだけ待ち受ける。切り替えにはSteam UIのOriginと専用ヘッダーを付けたPOSTが必要。外部PCから接続できるアドレスにはバインドしない。
- 受信後、日本語入力の前面アプリへ通常の半角/全角キー（`VK_KANJI`）を送る。物理キーボードのキーを横取りする処理はない。

Steam再起動時にはUIファイルが自動修復されるため、アダプターはSteamVRのDLL読み込み時に適用する。Steamやゲームの通常キーボードは変更しない。ValveのUIファイル自体はGitリポジトリに含めない。

## 動作条件

- 日本語Microsoft IMEの通常の半角/全角動作を使用。IME側で半角/全角キーを別機能に割り当てている場合は、その設定に従う。他社IME・カスタム設定は未検証。
- 前面アプリの入力言語が日本語以外の場合は切り替えない。
- 管理者権限のアプリはWindowsの権限差（UIPI）により入力送信が失敗する場合がある。
- Steam更新で対応するコードの位置や変数が変わった場合、UIファイルを変更せずログにエラーを残す。
- バックアップはパッケージの `backups/`、ログは `bin/win64/caps-ime.log`。入力した文字は記録しない。
- 異常終了でUIへの追加が残った場合、`Uninstall.ps1` で除去できる。削除処理はほかの変更を保つ。
- `CapsImeHost.exe` は同じDLLを一時起動する診断用。通常の利用では不要。

## ビルド・検証

MinGW-w64の `g++`、Python 3、Node.jsを使用。OpenVRヘッダーはValve公式SDK v2.15.6を固定して同梱。MinGWランタイムは静的リンクしている。

```powershell
& .\tools\build.ps1
& .\tools\test.ps1
```

UIアダプターの対象制限、パッチの可逆性、未対応ファイルの拒否、HTTP要求のOrigin・メソッド制限、DLLのOpenVRファクトリーを自動テストする。実機でローカル接続、CORS事前確認、別Originの拒否、DLL停止後の元ファイルとのSHA-256一致を確認済み。

2026年11月18日、ユーザーのHMDからCapsの「あ ⇔ A」両方向の切り替えと、`a`キーで「あ」／「a」が入力されることを確認済み。DLLのログでもCaps要求の受信とIMEキーの送信を確認した。

## 参照

- [OpenVR Driver Documentation](https://github.com/ValveSoftware/openvr/wiki/Driver-Documentation)
- [OpenVR SDK v2.15.6](https://github.com/ValveSoftware/openvr/tree/v2.15.6)
- [SendInput](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput)
- [Virtual-Key Codes](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes)

Steam UIのアダプターはローカルのSteamビルドを調査して作成した非公式の変更。
