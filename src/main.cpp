//// filepath:
////home/iwasakim/Documents/PlatformIO/Projects/interaction-design-1/src/Imu.cpp
#include "imu.hpp"
#include <M5Unified.h>

// 描画コードやサンプルのための定数や変数はほぼそのまま利用
static auto &dsp = (M5.Display);
static constexpr uint8_t calib_value = 64;

struct rect_t {
  int32_t x;
  int32_t y;
  int32_t w;
  int32_t h;
};

static constexpr uint32_t color_tbl[18] = {
    0xFF0000u, 0xCCCC00u, 0xCC00FFu, 0xFFCC00u, 0x00FF00u, 0x0088FFu,
    0xFF00CCu, 0x00FFCCu, 0x0000FFu, 0xFF0000u, 0xCCCC00u, 0xCC00FFu,
    0xFFCC00u, 0x00FF00u, 0x0088FFu, 0xFF00CCu, 0x00FFCCu, 0x0000FFu,
};
static constexpr float coefficient_tbl[3] = {0.5f, (1.0f / 256.0f),
                                             (1.0f / 1024.0f)};

static rect_t rect_graph_area;
static rect_t rect_text_area;

static uint8_t calib_countdown = 0;
static int prev_xpos[18];

//--------------------------------
// IMUクラスのインスタンスを用意
//--------------------------------
static IMU myIMU;

// グラフ描画用のバーを動かす
void drawBar(int32_t ox, int32_t oy, int32_t nx, int32_t px, int32_t h,
             uint32_t color) {
  uint32_t bgcolor = (color >> 3) & 0x1F1F1Fu;
  if (px && ((nx < 0) != (px < 0))) {
    dsp.fillRect(ox, oy, px, h, bgcolor);
    px = 0;
  }
  if (px != nx) {
    if ((nx > px) != (nx < 0)) {
      bgcolor = color;
    }
    dsp.setColor(bgcolor);
    dsp.fillRect(nx + ox, oy, px - nx, h);
  }
}

// IMUデータとオフセット値をグラフ表示
void drawGraph(const rect_t &r, const m5::imu_data_t &data) {
  int ox = (r.x + r.w) >> 1;
  int oy = r.y;
  int h = (r.h / 18) * (calib_countdown ? 1 : 2);
  int bar_count = 9 * (calib_countdown ? 2 : 1);

  dsp.startWrite();
  for (int index = 0; index < bar_count; ++index) {
    float xval;
    if (index < 9) {
      auto coe = coefficient_tbl[index / 3] * r.w;
      xval = data.value[index] * coe;
    } else {
      // IMUクラス経由でオフセット値を取得するよう変更
      xval = myIMU.getOffsetData(index - 9) * (1.0f / (1 << 19));
    }
    int nx = (int)xval;
    int px = prev_xpos[index];
    if (nx != px) {
      prev_xpos[index] = nx;
    }
    drawBar(ox, oy + h * index, nx, px, h - 1, color_tbl[index]);
  }
  dsp.endWrite();
}

// カウントダウンや背景などを更新
void updateCalibration(uint32_t c, bool clear = false) {
  calib_countdown = c;
  if (c == 0) {
    clear = true;
  }

  if (clear) {
    memset(prev_xpos, 0, sizeof(prev_xpos));
    dsp.fillScreen(TFT_BLACK);

    if (c) {
      // キャリブレーション開始
      // IMUクラスのsetCalibrationAllを呼び出すように変更
      myIMU.setCalibrationAll(calib_value);
    } else {
      // キャリブレーション終了。ただし地磁気のみ継続
      myIMU.setCalibrationKeepMag(calib_value);

      // 必要に応じてすべてのキャリブレーションを止める場合は
      // myIMU.setCalibrationAll(0);

      // キャリブレーション値保存
      myIMU.saveOffsetToNVS();
    }
  }

  auto backcolor = (c == 0) ? TFT_BLACK : TFT_BLUE;
  dsp.fillRect(rect_text_area.x, rect_text_area.y, rect_text_area.w,
               rect_text_area.h, backcolor);

  if (c) {
    dsp.setCursor(rect_text_area.x + 2, rect_text_area.y + 1);
    dsp.setTextColor(TFT_WHITE, TFT_BLUE);
    dsp.printf("Countdown:%d ", c);
  }
}

// 外部から呼び出すと一定時間キャリブレーションする
void startCalibration(void) { updateCalibration(10, true); }

void setup(void) {
  // IMU初期化をクラスでまとめて行う
  myIMU.initIMU();

  // 使用されるIMU名を取得し、表示
  auto imu_type = myIMU.getIMUTypeName();
  M5_LOGI("imu:%s", imu_type);
  dsp.printf("imu:%s", imu_type);

  // 画面レイアウト設定
  int32_t w = dsp.width();
  int32_t h = dsp.height();
  if (w < h) {
    dsp.setRotation(dsp.getRotation() ^ 1);
    w = dsp.width();
    h = dsp.height();
  }
  int32_t graph_area_h = ((h - 8) / 18) * 18;
  int32_t text_area_h = h - graph_area_h;
  float fontsize = text_area_h / 8;
  dsp.setTextSize(fontsize);

  rect_graph_area = {0, 0, w, graph_area_h};
  rect_text_area = {0, graph_area_h, w, text_area_h};

  // IMUクラスの初期化時にオフセットを読み込めていない場合はキャリブレーション
  if (!myIMU.isOffsetLoaded()) {
    startCalibration();
  }
}

void loop(void) {
  static uint32_t frame_count = 0;
  static uint32_t prev_sec = 0;

  // IMUを更新する
  bool imu_update = myIMU.updateIMU();
  if (imu_update) {
    // 新しい値を取得
    auto data = myIMU.getIMUData();
    drawGraph(rect_graph_area, data);
    ++frame_count;
  } else {
    // IMU更新が無いときはUI等を更新
    M5.update();

    // ボタン・タッチでキャリブレーション開始
    if (M5.BtnA.wasClicked() || M5.BtnPWR.wasClicked() ||
        M5.Touch.getDetail().wasClicked()) {
      startCalibration();
    }
  }

  int32_t sec = millis() / 1000;
  if (prev_sec != sec) {
    prev_sec = sec;
    M5_LOGI("sec:%d  frame:%d", sec, frame_count);
    frame_count = 0;

    // キャリブレーションカウントダウン減少
    if (calib_countdown) {
      updateCalibration(calib_countdown - 1);
    }

    // WDT回避用に定期的に待機
    if ((sec & 7) == 0) {
      vTaskDelay(1);
    }
  }
}