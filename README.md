# atom_mate_toio_ros2

[![PlatformIO CI](https://github.com/atinfinity/atom_mate_toio_ros2/actions/workflows/build.yml/badge.svg)](https://github.com/atinfinity/atom_mate_toio_ros2/actions/workflows/build.yml)

ATOM Matrix + [ATOM Mate for toio](https://www.switch-science.com/products/8500) の内蔵距離センサ(VL53L0X ToF)の測定値を、Wi-Fi 経由で ROS 2 Jazzy のトピックとして publish するファームウェアです。

<img src="images/atom_mate_toio.svg" width="560" alt="toio コアキューブの上に ATOM Mate for toio を載せ、その天面に ATOM Matrix を取り付けた構成のイメージ図">

※ 公開されている製品写真・寸法をもとに作成したイメージ図です。細部は実物と異なります。

## できること

- VL53L0X ToF 距離センサ(測定範囲 最大2m)の測定値を Wi-Fi(micro-ROS)経由で約 10Hz で配信
- **PC のブラウザ(Chrome / Edge)から書き込み**できる。PlatformIO や Docker は不要
- Wi-Fi と micro-ros-agent の IP は本体の設定モード(SoftAP + 設定ページ)からスマホ/PC で設定。ソースに直書きしない
- 配信モードをビルド時に選択: `sensor_msgs/Range`(デフォルト。配布バイナリはこちら)または `sensor_msgs/LaserScan`
- 接続状態を ATOM Matrix の 5x5 LED で表示(青=設定モード / 赤=Wi-Fi 接続中 / 黄=agent 待ち / 緑=publish 中)
- 追加配線は不要(ATOM Mate for toio を装着するだけ)

<img src="images/led_status.svg" width="760" alt="LED 状態表示の遷移図: 青(回転)=設定モード → 設定保存で赤=Wi-Fi 接続中 → Wi-Fi 接続完了で黄=agent 待ち → agent 接続完了で緑=publish 中">

## クイックスタート

1. ATOM Matrix を USB で PC に接続し、PC 版 Chrome / Edge で **インストールページ** <https://atinfinity.github.io/atom_mate_toio_ros2/> を開いて書き込む(PlatformIO 不要)
2. 初回起動で設定モード(LED が青く回転)になるので、スマホ/PC から Wi-Fi `AtomToio-XXXX` に接続する
3. 開いた設定ページで Wi-Fi SSID / パスワード、micro-ros-agent を動かす PC の IP アドレスを入力して「保存して再起動」
4. PC 側で micro-ros-agent を起動し、トピックを確認する

```bash
docker compose up agent          # micro-ros-agent(macOS / Windows の Docker Desktop でも可)
ros2 topic echo /toio/range      # 手をかざすと range が変化する
```

詳しい手順(設定ページの項目、自分でビルドする方法、Ubuntu / macOS での PlatformIO のインストールなど)は[セットアップガイド](docs/setup.md)を参照してください。

## ドキュメント

| ドキュメント | 内容 |
|---|---|
| [セットアップガイド](docs/setup.md) | 必要なもの、ブラウザからの書き込み、初期設定、自分でビルドする方法、micro-ros-agent の起動、動作確認 |
| [構成とメッセージ仕様](docs/architecture.md) | システム構成、配信モード、トピックとメッセージの詳細 |
| [単体テスト](docs/testing.md) | native 環境での単体テストの実行方法 |
| [トラブルシューティング](docs/troubleshooting.md) | LED が赤/黄のまま、設定をやり直したいなど、よくある問題と対処 |

## ライセンス

[Apache License 2.0](LICENSE)
