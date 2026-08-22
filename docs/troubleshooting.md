# トラブルシューティング

LED はいずれも ATOM Matrix 本体の 5x5 LED マトリクスを指します(色の意味は[構成とメッセージ仕様](architecture.md)を参照)。

- **ATOM Matrix の LED が赤のまま**: Wi-Fi に接続できていません。SSID/パスワードを確認してください(2.4GHz 帯のみ対応)。
- **ATOM Matrix の LED が黄のまま**: agent に接続できていません。`AGENT_IP` が PC の IP と一致しているか、agent が起動しているか、ファイアウォールで UDP 8888 が塞がれていないか確認してください。
- **`VL53L0X not found` がシリアルに出る**: ATOM Mate for toio の装着(ピン接触)を確認してください。
