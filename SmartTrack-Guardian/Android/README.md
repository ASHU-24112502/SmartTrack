# Android integration notes

The original Android Studio Kotlin source project is not included in this repository package. The previously established application package name is `com.smarttrack.guardian`.

## BLE contract

- Device: `SmartTrack_Wallet`
- Service: `4A5B0001-0000-1000-8000-00805F9B34FB`
- TX (READ + NOTIFY): `4A5B0002-0000-1000-8000-00805F9B34FB`
- RX (WRITE + WRITE WITHOUT RESPONSE): `4A5B0003-0000-1000-8000-00805F9B34FB`

Write `RING_ON` or `RING_OFF` to RX. Subscribe to TX notifications and parse the JSON status payload.

## RSSI anti-loss logic

Use `BluetoothGatt.readRemoteRssi()` on the existing connected GATT instance and handle `BluetoothGattCallback.onReadRemoteRssi()`. Do not create a second GATT connection or overlap RSSI reads.

Suggested initial settings:
- Warning: -75 dBm
- Lost: -90 dBm for 5 continuous seconds
- Recovery: stronger than -80 dBm for 3–5 seconds
- Sample interval: about 1 second

On confirmed loss, send `RING_ON` once. On stable recovery, send `RING_OFF` once. Failed RSSI reads must not trigger a lost event. These values need real-world calibration.

Handle Android 12+ BLE runtime permissions, notification permission and lifecycle. Continuous background monitoring may require an Android foreground service.
