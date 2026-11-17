# SteamVR Caps IME

SteamVRダッシュボードのデスクトップから開く内蔵キーボードで、Capsを日本語IMEの「あ ⇔ A」切り替えに使うWindows x64用アドオン。

## 構成

- `driver_capsime.dll`: SteamVRのOpenVRサーバードライバー。専用スレッドで注入されたPauseを消費し、日本語入力の前面アプリへ半角/全角キー（`VK_KANJI`）を送る。長押しは1回だけ切り替える。
- Steam UIの小さなアダプター: `steamui/virtualkeyboard.js`のCaps処理と`steamui/sp.js`のキーボード管理クラスに追加。開いているキーボードの対象が`system.desktop`または`system.desktop.N`の場合だけPauseを送る。Capsの大文字状態は解除する。
- `CapsImeHost.exe`: SteamVRを再起動せず同じDLLを動かす診断用ホスト。通常の利用では不要。

実機の診断でCapsのWindowsキーイベントが発生しないことを確認した。CapsはSteam UI内部の状態であり、DLLのキーフックだけでは捕まえられない。そのためUIアダプターとDLLを組み合わせている。Steamのファイル自体はリポジトリに含めない。

## インストール・削除

`tools/build.ps1`で生成される`outputs/SteamVRCapsIME`を使用する。

```powershell
& .\tools\build.ps1
& .\outputs\SteamVRCapsIME\Install.ps1
```

インストール後、Steamクライアントを終了して起動し、その後SteamVRを起動する。対象アプリで日本語IMEを選んでテキスト欄にフォーカスし、デスクトップ用キーボードのCapsを押す。

```powershell
& .\outputs\SteamVRCapsIME\Uninstall.ps1
```

削除後もSteamとSteamVRの再起動が必要。パッケージフォルダーは登録パスなので、インストール後に移動しないこと。

## 範囲・注意点

- Windowsの入力言語・キーボード配列の切り替えは行わない。前面アプリの入力言語が日本語以外の場合は何もしない。
- Microsoft IMEの通常の半角/全角動作を利用する。IME側で半角/全角キーを別機能に割り当てている場合、その設定に従う。他社IME・カスタムキー設定は未検証。
- 物理Caps、物理Pause、Shift、Steamの通常キーボードは変更しない。アドオンの動作中、他のソフトウェアが注入するPauseも予約キーとして消費する。
- 管理者権限のアプリへ入力する場合、Windowsの権限差（UIPI）で送信が失敗することがある。
- Steam更新でUIファイルが置き換わったら`Install.ps1`を再実行する。対応する挿入位置が変わった場合は、ファイルを書き換えずエラーにする。
- バックアップはパッケージの`backups/`、ログは`bin/win64/caps-ime.log`。入力された文字は記録しない。削除処理は自身の追加箇所だけを除き、ほかの変更を保つ。
- バイナリはMinGWのランタイムを静的リンク。Windowsのカーネルドライバーではなく、SteamVRが読み込むユーザーモードDLLである。

## ビルド・検証

MinGW-w64の`g++`、Python 3、Node.jsを使用。OpenVRヘッダーはValve公式SDK v2.15.6を固定して同梱。

```powershell
& .\tools\build.ps1
python -m unittest discover -s tests -p test_patch.py
node tests\adapter.test.js
g++ -std=c++17 -Wall -Wextra -Werror tests\key_policy_test.cpp -o work\key_policy_test.exe
& .\work\key_policy_test.exe
```

## 参照

- [OpenVR Driver Documentation](https://github.com/ValveSoftware/openvr/wiki/Driver-Documentation)
- [OpenVR SDK v2.15.6](https://github.com/ValveSoftware/openvr/tree/v2.15.6)
- [LowLevelKeyboardProc](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc)
- [SendInput](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput)
- [Virtual-Key Codes](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes)

Steam UIのアダプターはローカルのSteamビルドを調査して作成した非公式の変更。
