# ビルド専用コンテナ。書き込みはホスト側で行う(docs/setup.md 参照)
FROM python:3.11-slim

# micro_ros_platformio の libmicroros 生成に必要なツール
RUN apt-get update && apt-get install -y --no-install-recommends \
        git curl cmake build-essential binutils \
    && rm -rf /var/lib/apt/lists/*

# micro_ros_platformio が ~/.platformio/penv を前提にするため、
# pip ではなく公式インストーラで導入する(.github/workflows/build.yml と同じ)
RUN curl -fsSL -o /tmp/get-platformio.py \
        https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py \
    && python3 /tmp/get-platformio.py \
    && rm /tmp/get-platformio.py
ENV PATH="/root/.platformio/penv/bin:${PATH}"

WORKDIR /work
CMD ["pio", "run"]
