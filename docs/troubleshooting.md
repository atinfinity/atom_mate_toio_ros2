# トラブルシューティング

LED はいずれも ATOM Matrix 本体の 5x5 LED マトリクスを指します(色の意味は[構成とメッセージ仕様](architecture.md)を参照)。

- **ATOM Matrix の LED が青く回っている**: 設定モードです。スマホ/PC から Wi-Fi `AtomToio-XXXX` に接続し、開いた設定ページ(開かなければ `http://192.168.4.1/`)で Wi-Fi と agent の IP を入力して「保存して再起動」してください。
- **設定をやり直したい(SSID を打ち間違えた等)**: ATOM Matrix のボタンを押しながら USB を挿し直す(または押しながらリセット)と設定モードに入ります。保存済みの値は SSID / IP / ポート / namespace が初期表示され、パスワードだけ再入力が必要です。
- **ATOM Matrix の LED が赤のまま**: Wi-Fi に接続できていません。SSID/パスワードを確認してください(2.4GHz 帯のみ対応)。修正するには上記の手順で設定モードに入ります。
- **ATOM Matrix の LED が黄のまま**: agent に接続できていません。設定した agent の IP が PC の IP と一致しているか、agent が起動しているか、ファイアウォールで UDP 8888 が塞がれていないか確認してください。
- **`VL53L0X not found` がシリアルに出る**: ATOM Mate for toio の装着(ピン接触)を確認してください。
- **インストールページで ATOM が見つからない**: PC 版 Chrome / Edge を使っているか、USB ケーブルがデータ通信対応か、[FTDI VCP ドライバ](https://docs.m5stack.com/en/download)が入っているかを確認してください。
- **設定ページが自動で開かない**: スマホ/PC の機種によっては captive portal の自動表示が出ません。ブラウザで `http://192.168.4.1/` を直接開いてください。
