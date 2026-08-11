# atom_mate_toio_ros2

ATOM Matrix + [ATOM Mate for toio](https://www.switch-science.com/products/8500) の内蔵距離センサ(VL53L0X ToF)の測定値を、Wi-Fi 経由で ROS 2 Jazzy のトピック(`sensor_msgs/Range`)として publish するファームウェアです。

※ ATOM Mate for toio の内蔵距離センサは超音波ではなく **ToF(レーザー)方式の VL53L0X** です(測定範囲 最大2m)。

## 構成

```
ATOM Matrix (+ ATOM Mate for toio)
   │  Wi-Fi (UDP, micro-ROS)
   ▼
micro-ros-agent (PC)
   │
   ▼
ROS 2 Jazzy トピック /toio/range (sensor_msgs/Range, 10Hz)
```

- センサ接続: ATOM Mate for toio 装着のみ(I2C: SDA=G25, SCL=G21)。追加配線不要。
- QoS: best-effort(センサストリームの標準)
- LED ステータス表示: 赤=Wi-Fi 接続中 / 黄=agent 待ち / 緑=publish 中

## 必要なもの

- ATOM Matrix + ATOM Mate for toio
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

## セットアップ

### 1. 設定ファイルの作成

```bash
cp include/config.h.example include/config.h
```

`include/config.h` を編集して Wi-Fi SSID/パスワードと、micro-ros-agent を動かす PC の IP アドレスを設定してください。

### 2. ビルド & 書き込み

```bash
pio run -t upload
pio device monitor   # ログ確認(115200bps)
```

初回ビルドは libmicroros の生成に数分かかります(Python3 が必要)。

### 3. micro-ros-agent の起動(PC 側)

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

```bash
ros2 topic list                 # /toio/range が表示される
ros2 topic echo /toio/range     # 手をかざすと range が変化する
ros2 topic hz /toio/range       # 約 10Hz
```

## 実機確認チェックリスト(未実施)

ビルド成功(`pio run`、2026-08-12)までは確認済み。実機での動作確認は未実施のため、以下を順に確認する:

- [ ] `include/config.h` に Wi-Fi SSID/パスワードと agent PC の IP を設定
- [ ] `pio run -t upload` で書き込み成功
- [ ] `pio device monitor`(115200bps)で `VL53L0X initialized` が出る(出ない場合は Mate の装着を確認)
- [ ] LED が赤(Wi-Fi 接続中)→ 黄(agent 待ち)に遷移する
- [ ] PC 側で micro-ros-agent(UDP, ポート8888)を起動すると LED が緑になり、シリアルに `micro-ROS agent connected` が出る
- [ ] `ros2 topic list` に `/toio/range` が表示される
- [ ] `ros2 topic echo /toio/range` で手をかざすと `range` が変化する(単位 m)
- [ ] 何も無い方向に向けると `range: inf`(out of range)になる
- [ ] `ros2 topic hz /toio/range` が約 10Hz
- [ ] agent を Ctrl+C で止める → LED が黄に戻る → agent 再起動で緑に復帰する(再接続確認)

## メッセージ仕様

| フィールド | 値 |
|---|---|
| `header.frame_id` | `toio_range_link` |
| `header.stamp` | agent と時刻同期した epoch 時刻 |
| `radiation_type` | `INFRARED (1)` |
| `field_of_view` | 0.44 rad(約25°) |
| `min_range` / `max_range` | 0.03 m / 2.0 m |
| `range` | 測定距離 [m]。検出なし(out of range)時は `+Inf` |

## トラブルシューティング

- **LED が赤のまま**: Wi-Fi に接続できていません。SSID/パスワードを確認してください(2.4GHz 帯のみ対応)。
- **LED が黄のまま**: agent に接続できていません。`AGENT_IP` が PC の IP と一致しているか、agent が起動しているか、ファイアウォールで UDP 8888 が塞がれていないか確認してください。
- **`VL53L0X not found` がシリアルに出る**: ATOM Mate for toio の装着(ピン接触)を確認してください。
