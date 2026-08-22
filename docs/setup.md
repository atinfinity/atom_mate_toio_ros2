# セットアップガイド

ビルドから ROS 2 トピックの確認までの手順です。

## 必要なもの

- ATOM Matrix + [ATOM Mate for toio](https://www.switch-science.com/products/8500)
- [PlatformIO](https://platformio.org/)(VSCode 拡張 or CLI)
- ROS 2 Jazzy 環境の PC(micro-ros-agent を実行。Docker 利用可)

### ビルドに必要な追加ツール(macOS)

micro_ros_platformio のライブラリ生成(libmicroros)に以下が必要です。無いとビルドが失敗します。

```bash
# cmake(PlatformIO の Python 環境内に入れると確実)
~/.platformio/penv/bin/pip install cmake

# GNU binutils(micro_ros_platformio が /opt/homebrew/opt/binutils/ を参照する)
brew install binutils
```

※ 本リポジトリ作成時(2026-08-12)に上記2点のインストールで初回ビルド成功を確認済み。

## 1. 設定ファイルの作成

```bash
cp include/config.h.example include/config.h
```

`include/config.h` を編集して Wi-Fi SSID/パスワードと、micro-ros-agent を動かす PC の IP アドレスを設定してください。

配信モード(`sensor_msgs/Range` / `sensor_msgs/LaserScan`)もこのファイルの `PUBLISH_MODE` で選択します。詳しくは[構成とメッセージ仕様](architecture.md)を参照してください。

## 2. ビルド & 書き込み

```bash
pio run -t upload
pio device monitor   # ログ確認(115200bps)
```

初回ビルドは libmicroros の生成に数分かかります(Python3 が必要)。

## 3. micro-ros-agent の起動(PC 側)

Docker を使う場合:

```bash
docker run -it --rm --net=host microros/micro-ros-agent:jazzy udp4 --port 8888
```

ROS 2 環境にソースビルド済みの場合:

```bash
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888
```

agent に接続されると ATOM Matrix の LED が緑になります。

## 動作確認

range モード(デフォルト):

```bash
ros2 topic list                 # /toio/range が表示される
ros2 topic echo /toio/range     # 手をかざすと range が変化する
ros2 topic hz /toio/range       # 約 10Hz
```

scan モード(`PUBLISH_MODE_SCAN` でビルドした場合):

```bash
ros2 topic echo /toio/scan      # ranges[0] に距離が入る
ros2 topic hz /toio/scan        # 約 10Hz
```

うまく動かない場合は[トラブルシューティング](troubleshooting.md)を参照してください。
