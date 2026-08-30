# セットアップガイド

ビルドから ROS 2 トピックの確認までの手順です。

## 必要なもの

- ATOM Matrix + [ATOM Mate for toio](https://www.switch-science.com/products/8500)
- ビルド環境: 次のどちらか
  - **A. Docker**(おすすめ。cmake や binutils を自分で入れなくてよい)
  - **B. ホストに PlatformIO をインストール**
- ROS 2 Jazzy 環境の PC(micro-ros-agent を実行。Docker 利用可)

### A. Docker でビルドする場合

Docker Desktop(macOS / Windows)または Docker Engine(Linux)を入れておけば、ほかに必要なものはありません。

- 初回は libmicroros の生成に数分かかります。Docker Desktop ではメモリの割り当てを 4GB 以上にしておくと安心です(Settings → Resources)
- **macOS / Windows の Docker Desktop はコンテナから USB シリアルを扱えない**ため、書き込みはホスト側の esptool で行います(後述)

```bash
pip install esptool    # 書き込み用(ホスト側)
```

### B. ホストに PlatformIO をインストールする場合

#### Ubuntu

```bash
# PlatformIO 本体(公式インストーラ。~/.platformio/penv が作られる)
sudo apt install -y python3 python3-venv curl git
curl -fsSL -o get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 get-platformio.py
echo 'export PATH="$HOME/.platformio/penv/bin:$PATH"' >> ~/.bashrc && source ~/.bashrc

# micro_ros_platformio のライブラリ生成(libmicroros)に必要
sudo apt install -y cmake build-essential binutils

# シリアルポートへのアクセス権(再ログインで反映)
sudo usermod -aG dialout "$USER"
```

#### macOS

```bash
# PlatformIO 本体(VSCode 拡張でも可)
brew install platformio

# micro_ros_platformio のライブラリ生成(libmicroros)に必要。無いとビルドが失敗する
~/.platformio/penv/bin/pip install cmake   # PlatformIO の Python 環境内に入れると確実
brew install binutils                       # micro_ros_platformio が /opt/homebrew/opt/binutils/ を参照する
```

※ 本リポジトリ作成時(2026-08-12)に上記2点のインストールで初回ビルド成功を確認済み。

## 1. 設定ファイルの作成

```bash
cp include/config.h.example include/config.h
```

`include/config.h` を編集して Wi-Fi SSID/パスワードと、micro-ros-agent を動かす PC の IP アドレスを設定してください。

- `AGENT_IP` には **PC 自身の LAN 上の IP**(Wi-Fi の IP)を書きます。agent を Docker で動かす場合もコンテナの IP ではなく PC の IP です

配信モード(`sensor_msgs/Range` / `sensor_msgs/LaserScan`)もこのファイルの `PUBLISH_MODE` で選択します。詳しくは[構成とメッセージ仕様](architecture.md)を参照してください。

## 2. ビルド & 書き込み

### A. Docker の場合

```bash
docker compose run --rm build     # .pio/build/m5stack-atom/firmware.bin ができる
```

書き込みはホスト側で行います。ATOM Matrix を USB で接続してから:

```bash
# ポート名は macOS: /dev/cu.usbserial-XXXX、Linux: /dev/ttyUSB0 など
esptool.py --chip esp32 --port /dev/cu.usbserial-XXXX --baud 1500000 write_flash \
    0x1000  .pio/build/m5stack-atom/bootloader.bin \
    0x8000  .pio/build/m5stack-atom/partitions.bin \
    0xe000  ~/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin \
    0x10000 .pio/build/m5stack-atom/firmware.bin
```

※ `boot_app0.bin` はホストに PlatformIO が無い場合は [Arduino-ESP32 のリポジトリ](https://github.com/espressif/arduino-esp32/blob/master/tools/partitions/boot_app0.bin)から取得してください。

Linux ホストなら `compose.yml` の `devices:` のコメントを外すと、コンテナから直接書き込めます:

```bash
docker compose run --rm build pio run -t upload
```

ログ確認(115200bps)は `pio device monitor` の代わりに `screen` などでも構いません:

```bash
screen /dev/cu.usbserial-XXXX 115200    # 終了は Ctrl-A, K
```

### B. ホストの PlatformIO の場合

```bash
pio run -t upload
pio device monitor   # ログ確認(115200bps)
```

初回ビルドは libmicroros の生成に数分かかります(Python3 が必要)。

## 3. micro-ros-agent の起動(PC 側)

Docker を使う場合(macOS / Windows / Linux 共通):

```bash
docker compose up agent
```

`--net=host` ではなく UDP 8888 のポート公開で動かしているので、macOS / Windows の Docker Desktop でもそのまま動きます。

ROS 2 環境にソースビルド済みの場合:

```bash
ros2 run micro_ros_agent micro_ros_agent udp4 --port 8888
```

agent に接続されると ATOM Matrix の LED が緑になります。

## 動作確認

Docker で agent を動かしている場合は、agent のコンテナ内で `ros2` コマンドを実行します(別ターミナルで):

```bash
docker compose exec agent bash -lc "source /opt/ros/jazzy/setup.bash && ros2 topic echo /toio/range"
```

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
