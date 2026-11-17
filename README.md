# SteamVR Caps IME

SteamVRダッシュボードのデスクトップから開く内蔵キーボードで、Capsを日本語IMEの「あ ⇔ A」切り替えに使うWindows x64用アドオン。

## 構成

- `driver_capsime.dll`: SteamVRのOpenVRサーバードライバー。専用スレッドで注入された専用のUnicode制御文字（U+E0FE）だけを消費し、日本語入力の前面アプリへ半角/全角キー（`VK_KANJI`）を送る。長押しは1回だけ切り替える。
- Steam UIの小さなアダプター: DLLが起動時に`steamui/virtualkeyboard.js`のCaps処理へ追加し、正常終了時に自身の追加部分だけを除去する。開いているキーボードの対象が`system.desktop`または`system.desktop.N`の場合だけ文字入力と同じ経路で制御文字を送る。Capsの大文字状態は解除する。
- `CapsImeHost.exe`: SteamVRを再起動せず同じDLLを動かす診断用ホスト。通常の利用では不要。

実機の診断でCapsのWindowsキーイベントが発生しないことを確認した。CapsはSteam UI内部の状態であり、DLLのキーフックだけでは捕まえられない。そのためUIアダプターとDLLを組み合わせている。Steamのファイル自体はリポジトリに含めない。

## インストール・削除

`tools/build.ps1`で生成される`outputs/SteamVRCapsIME-v2`を使用する。

```powershell
& .\tools\build.ps1
& .\outputs\SteamVRCapsIME-v2\Install.ps1
```

Steamクライアントが起動済みの状態で、SteamVRだけを起動・再起動する。Steam本体の再起動時にはUIファイルが自動修復されるため、適用はSteamVRのDLL読み込み時に行う。対象アプリで日本語IMEを選んでテキスト欄にフォーカスし、デスクトップ用キーボードのCapsを押す。

```powershell
& .\outputs\SteamVRCapsIME-v2\Uninstall.ps1
```

削除前にSteamVRを終了する。削除時にUIパッチが残っていれば除去し、ドライバー登録を解除する。パッケージフォルダーは登録パスなので、インストール後に移動しないこと。

## 範囲・注意点

- Windowsの入力言語・キーボード配列の切り替えは行わない。前面アプリの入力言語が日本語以外の場合は何もしない。
- Microsoft IMEの通常の半角/全角動作を利用する。IME側で半角/全角キーを別機能に割り当てている場合、その設定に従う。他社IME・カスタムキー設定は未検証。
- 物理キーボード、Shift、通常のUnicode文字、Steamの通常キーボードは変更しない。U+E0FEの注入イベントだけを切り替え指示として扱う。
- 管理者権限のアプリへ入力する場合、Windowsの権限差（UIPI）で送信が失敗することがある。
- Steam起動時の自動修復後でも、SteamVRを起動するとDLLが再適用する。Steam更新で対応する挿入位置や変数が変わった場合は、ファイルを書き換えずログにエラーを残す。
- バックアップはパッケージの`backups/`、ログは`bin/win64/caps-ime.log`。入力された文字は記録しない。削除処理は自身の追加箇所だけを除き、ほかの変更を保つ。SteamVRの正常終了時にも同じ復元処理を行う。
- バイナリはMinGWのランタイムを静的リンク。Windowsのカーネルドライバーではなく、SteamVRが読み込むユーザーモードDLLである。

## ビルド・検証

MinGW-w64の`g++`、Python 3、Node.jsを使用。OpenVRヘッダーはValve公式SDK v2.15.6を固定して同梱。

```powershell
& .\tools\build.ps1
& .\tools\test.ps1
```

## 検証結果

- DLLのロード、OpenVRファクトリーと仮想関数テーブルを検証。
- UIパッチの可逆性、未対応バージョンの拒否、対象ウィンドウの制限、キーのリピート処理のテストに合格。
- ネイティブDLLの起動でUIパッチが適用され、停止後に元ファイルとSHA-256が一致することを実機で確認。
- HMDからの最終動作は実機確認中。

## 参照

- [OpenVR Driver Documentation](https://github.com/ValveSoftware/openvr/wiki/Driver-Documentation)
- [OpenVR SDK v2.15.6](https://github.com/ValveSoftware/openvr/tree/v2.15.6)
- [LowLevelKeyboardProc](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc)
- [SendInput](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-sendinput)
- [Virtual-Key Codes](https://learn.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes)

Steam UIのアダプターはローカルのSteamビルドを調査して作成した非公式の変更。
