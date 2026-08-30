# セットアップガイド

書き込みから ROS 2 トピックの確認までの手順です。

## 必要なもの

- ATOM Matrix + [ATOM Mate for toio](https://www.switch-science.com/products/8500)
- データ通信できる USB-C ケーブル
- 書き込み用の PC: 次のどちらか
  - **A. ブラウザから書き込む**(おすすめ。PC 版 Chrome / Edge だけあればよい)
  - **B. 自分でビルドする**(Docker、またはホストに PlatformIO をインストール)
- ROS 2 Jazzy 環境の PC(micro-ros-agent を実行。Docker 利用可)
- Wi-Fi(2.4GHz 帯)。ATOM Matrix と agent を動かす PC が同じネットワークにつながること

## 1. 書き込み

### A. ブラウザから書き込む(おすすめ)

1. ATOM Matrix を USB で PC に接続する
2. PC 版 Chrome / Edge で **インストールページ** <https://atinfinity.github.io/atom_mate_toio_ros2/> を開く
3. 「CONNECT」を押し、表示されたシリアルポートを選んで「接続」する
4. 「INSTALL」を選ぶ。保存済みの Wi-Fi / agent 設定を消したい場合だけ「Erase device」にチェックを入れる

ATOM が認識されない場合は、[FTDI VCP ドライバ](https://docs.m5stack.com/en/download)をインストールしてください。Safari / Firefox / スマホは Web Serial に対応していないため使えません。

ブラウザから書き込まれるのは range モード(`sensor_msgs/Range`)のバイナリです。scan モードを使う場合は、[GitHub Release](https://github.com/atinfinity/atom_mate_toio_ros2/releases) の `-scan.app.bin` を `esptool.py write_flash 0x10000` で書き込むか、B の手順で自分でビルドしてください。

### B. 自分でビルドする

コードを変更したい場合や scan モードを使いたい場合の手順です。ビルド環境は Docker か、ホストへの PlatformIO インストールのどちらかを選びます。

#### B-1. Docker でビルドする

Docker Desktop(macOS / Windows)または Docker Engine(Linux)を入れておけば、ほかに必要なものはありません。

- 初回は libmicroros の生成に数分かかります。Docker Desktop ではメモリの割り当てを 4GB 以上にしておくと安心です(Settings → Resources)
- **macOS / Windows の Docker Desktop はコンテナから USB シリアルを扱えない**ため、書き込みはホスト側の esptool で行います

```bash
docker compose run --rm build                            # range モード
docker compose run --rm build pio run -e m5stack-atom-scan  # scan モード
```

`.pio/build/m5stack-atom/`(scan モードは `m5stack-atom-scan/`)に `firmware.bin` ができます。書き込みはホスト側で:

```bash
pip install esptool    # 初回のみ

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

#### B-2. ホストに PlatformIO をインストールする

##### Ubuntu

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

##### macOS

```bash
# PlatformIO 本体(VSCode 拡張でも可)
brew install platformio

# micro_ros_platformio のライブラリ生成(libmicroros)に必要。無いとビルドが失敗する
~/.platformio/penv/bin/pip install cmake   # PlatformIO の Python 環境内に入れると確実
brew install binutils                       # micro_ros_platformio が /opt/homebrew/opt/binutils/ を参照する
```

※ 本リポジトリ作成時(2026-08-12)に上記2点のインストールで初回ビルド成功を確認済み。

##### ビルド & 書き込み

```bash
pio run -t upload                        # range モード(デフォルト)
pio run -e m5stack-atom-scan -t upload   # scan モード
pio device monitor                       # ログ確認(115200bps)
```

初回ビルドは libmicroros の生成に数分かかります(Python3 が必要)。

Wi-Fi や agent の IP はソースに書きません。書き込み後に次の「初期設定」で設定します。

## 2. 初期設定(Wi-Fi と agent の IP)

書き込み後の初回起動では、設定が無いので自動で **設定モード**(LED が青く回転)になります。

1. スマホ/PC の Wi-Fi 一覧から `AtomToio-XXXX`(XXXX は機体ごとに異なる)に接続する(パスワードなし)
2. 設定ページが自動で開く。開かない場合はブラウザで `http://192.168.4.1/` を開く
3. 次の項目を入力して「保存して再起動」

| 項目 | 内容 |
|---|---|
| Wi-Fi SSID / パスワード | ATOM Matrix を接続する Wi-Fi(2.4GHz 帯のみ) |
| micro-ros-agent の IP アドレス | agent を動かす **PC 自身の LAN 上の IP**(Wi-Fi の IP)。agent を Docker で動かす場合もコンテナの IP ではなく PC の IP |
| micro-ros-agent のポート | 通常は `8888` |
| ROS 2 namespace | 通常は空欄。複数台を同時に使うときに `robot1` などを指定すると、トピックが `/robot1/toio/range` になる |

保存すると ATOM Matrix が再起動し、LED が 赤(Wi-Fi 接続中)→ 黄(agent 待ち)と変わります。設定は本体に保存されるので、次回以降は入力不要です。

**設定をやり直す**には、ATOM Matrix のボタンを押しながら USB を挿し直す(または押しながらリセット)と、再び設定モードに入ります。パスワード以外は前回の値が初期表示されます。

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

scan モード(`m5stack-atom-scan` でビルドした場合):

```bash
ros2 topic echo /toio/scan      # ranges[0] に距離が入る
ros2 topic hz /toio/scan        # 約 10Hz
```

うまく動かない場合は[トラブルシューティング](troubleshooting.md)を参照してください。

## リリース手順(メンテナ向け)

`v*` タグを push すると CI がファームウェアをビルドし、GitHub Release(`merged.bin` / `app.bin` / `-scan.app.bin`)の作成と、インストールページ(GitHub Pages)の更新を行います。

```bash
git tag v0.1.0 && git push origin v0.1.0
```

タグを打つ前に main の最新をインストールページで試したい場合は、Actions の「PlatformIO CI」を **Run workflow** で手動実行すると、Release は作らずに Pages だけ更新されます(バージョン表示は `dev-<コミット>`)。
