# 単体テスト

ハードウェア非依存のロジック(`lib/toio_range_logic/`)は、native 環境(ホスト上)で Unity による単体テストを実行できます。実機は不要です。対象は距離値の変換・out-of-range 判定・タイムスタンプ変換・scan モードの LaserScan 構成パラメータです。テストスイートは `test/test_range_logic/`(range 系)と `test/test_scan_logic/`(scan 系)の2つです。

```bash
pio test -e native
```

CI(GitHub Actions)でも push / Pull Request ごとに実行されます。
