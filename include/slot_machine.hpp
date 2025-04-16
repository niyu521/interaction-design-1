//// filepath:
////home/iwasakim/Documents/PlatformIO/Projects/interaction-design-1/include/SlotMachine.hpp
#pragma once
#include "imu.hpp"
#include <M5Unified.h>

class SlotMachine {
private:
  static constexpr int COLS = 3;       // スロット列数
  static constexpr int ROWS = 3;       // 描画対象を 3x3 として扱う
  static constexpr int SYMBOL_SET = 6; // 絵柄の種類数

  IMU &imuRef;                  // 既存のIMUクラス参照
  bool stopped[COLS];           // スロット停止フラグ
  bool finished;                // 判定済みフラグ
  uint16_t symbols[COLS][ROWS]; // 絵柄

public:
  explicit SlotMachine(IMU &imu) : imuRef(imu), finished(false) {
    // コンストラクタで配列初期化など必要なら実施
    for (int i = 0; i < COLS; ++i) {
      stopped[i] = false;
      for (int j = 0; j < ROWS; ++j) {
        symbols[i][j] = 0;
      }
    }
  }

  void begin() {
    // IMUなどの初期化（IMUは外部で実行済みでもOK）
    imuRef.initIMU();
    M5.Display.setTextSize(2);
    initSlots();
  }

  // 毎フレーム呼び出す
  void update() {
    M5.update();
    // 回転中スロットをランダム色でアニメ
    animateSlots();

    // 加速度チェックでスロット停止 or リセット
    if (imuRef.updateIMU()) {
      auto data = imuRef.getIMUData();
      float ax = fabsf(data.accel.x);
      float ay = fabsf(data.accel.y);
      float az = fabsf(data.accel.z);
      if ((ax > 2.0f || ay > 2.0f || az > 2.0f)) {
        if (finished) {
          initSlots();
        } else {
          stopNextSlot();
        }
        M5.delay(500);
      }
    }
    M5.delay(50);
  }

private:
  // スロットを初期化・リセット
  void initSlots() {
    finished = false;
    for (int i = 0; i < COLS; ++i) {
      stopped[i] = false;
      for (int j = 0; j < ROWS; ++j) {
        symbols[i][j] = rand() % 0xFFFF;
        drawSlot(i, j, symbols[i][j]);
      }
    }
  }

  // スロットが止まっていない列をランダム描画
  void animateSlots() {
    for (int i = 0; i < COLS; ++i) {
      if (!stopped[i]) {
        for (int j = 0; j < ROWS; ++j) {
          symbols[i][j] = rand() % 0xFFFF;
          drawSlot(i, j, symbols[i][j]);
        }
      }
    }
  }

  // スロットを1列ずつ停止。最後の列停止で判定
  void stopNextSlot() {
    for (int i = 0; i < COLS; ++i) {
      if (!stopped[i]) {
        stopped[i] = true;
        if (i == COLS - 1) {
          // 全停止 => 勝敗判定
          if (checkWin()) {
            M5.Display.setTextSize(3);
            M5.Display.setCursor(10, 10);
            M5.Display.printf("YOU WIN!!");
          } else {
            M5.Display.setTextSize(3);
            M5.Display.setCursor(10, 10);
            M5.Display.printf("TRY AGAIN");
          }
          finished = true;
        }
        break;
      }
    }
  }

  // 揃い判定 (横・縦・斜め)
  bool checkWin() {
    // 横
    for (int r = 0; r < ROWS; ++r) {
      if (symbols[0][r] == symbols[1][r] && symbols[1][r] == symbols[2][r]) {
        return true;
      }
    }
    // 縦
    for (int c = 0; c < COLS; ++c) {
      if (symbols[c][0] == symbols[c][1] && symbols[c][1] == symbols[c][2]) {
        return true;
      }
    }
    // 斜め2通り
    if (symbols[0][0] == symbols[1][1] && symbols[1][1] == symbols[2][2]) {
      return true;
    }
    if (symbols[0][2] == symbols[1][1] && symbols[1][1] == symbols[2][0]) {
      return true;
    }
    return false;
  }

  // 単純な長方形の描画
  void drawSlot(int col, int row, uint16_t color) {
    int slotW = M5.Display.width() / COLS;
    int slotH = M5.Display.height() / ROWS;
    M5.Display.fillRect(col * slotW, row * slotH, slotW, slotH, color);
  }
};