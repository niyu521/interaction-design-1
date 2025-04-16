#pragma once
#include <M5Unified.h>

// IMUクラスを拡張して、
//   - setCalibrationAll(...)
//   - setCalibrationKeepMag(...)
//   - saveOffsetToNVS()
//   - getOffsetData(...)
//   - getIMUTypeName()
//   - isOffsetLoaded()
// などを用意
class IMU {
public:
  // 初期化とオフセット読み込み
  void initIMU() {
    auto cfg = M5.config();
    // 必要に応じて外部IMUを有効化
    // cfg.external_imu = true;
    M5.begin(cfg);

    offsetLoaded = M5.Imu.loadOffsetFromNVS();
  }

  // 毎フレームIMU更新
  bool updateIMU() {
    return M5.Imu.update(); // 取得できればtrueを返す
  }

  // IMUの値を参照する
  m5::imu_data_t getIMUData() { return M5.Imu.getImuData(); }

  // すべての軸をキャリブレーション
  void setCalibrationAll(uint8_t value) {
    M5.Imu.setCalibration(value, value, value);
  }

  // 地磁気だけ継続してキャリブレーションする場合
  void setCalibrationKeepMag(uint8_t value) {
    M5.Imu.setCalibration(0, 0, value);
  }

  // キャリブレーション情報をNVSに書き込む
  void saveOffsetToNVS() { M5.Imu.saveOffsetToNVS(); }

  // オフセット値を取得
  float getOffsetData(int index) { return M5.Imu.getOffsetData(index); }

  // 読み込んだオフセットがあるかどうか
  bool isOffsetLoaded() const { return offsetLoaded; }

  // IMU種類を文字列で返す
  const char *getIMUTypeName() {
    auto type = M5.Imu.getType();
    switch (type) {
    case m5::imu_none:
      return "not found";
    case m5::imu_sh200q:
      return "sh200q";
    case m5::imu_mpu6050:
      return "mpu6050";
    case m5::imu_mpu6886:
      return "mpu6886";
    case m5::imu_mpu9250:
      return "mpu9250";
    case m5::imu_bmi270:
      return "bmi270";
    default:
      return "unknown";
    }
  }

private:
  bool offsetLoaded = false;
};