import bluetooth
import utime as time

_ADV_TYPE_FLAGS = b"\x01"
_ADV_TYPE_NAME = b"\x09"
_ADV_TYPE_SERVICE_DATA = b"\x16"
_SERVICE_UUID_ENV_SENSING = b"\x18\x1a"
_CHARACTERISTIC_UUID_TEMPERATURE = b"\x2a\x6e"


class Broadcaster:
    ERROR_TEMPERATURE = -273.15

    def __init__(self, advertising_ms: int):
        self._ble = bluetooth.BLE()
        self._advertising_ms = advertising_ms

    def broadcast(self, temperature: float):
        """
        BUG: When called repeatedly (~32 times) device will cause PANIC RESET.
        Probably due to a bug in Micropython (or nimble BLE implementation).
        Error message:
            assertion "rc == 0" failed: file "/home/micropython/esp-idf-v4/components/bt/host/nimble/nimble/nimble/host/src/ble_hs.c", line 576, function: ble_hs_event_start_stage2
            abort() was called at PC 0x4016a043 on core 1

        :param temperature: temperature to broadcast
        """
        self._ble.active(True)
        payload = self.make_advertising_payload(temperature)
        self._ble.gap_advertise(100000, adv_data=payload, connectable=False)
        time.sleep_ms(self._advertising_ms)
        self._ble.gap_advertise(None)

    def make_advertising_payload(self, temperature: float):
        raw_temperature = self._convert_to_raw_temperature(temperature)
        payload = bytearray()
        payload += self._make_adv_element(_ADV_TYPE_FLAGS, b"\x05")  # LE limited discoverable mode (0x04) | BR/EDR not supported (0x01)
        payload += self._make_adv_element(_ADV_TYPE_NAME, "Temperature Sensor")
        payload += self._make_adv_element(_ADV_TYPE_SERVICE_DATA, _SERVICE_UUID_ENV_SENSING + _CHARACTERISTIC_UUID_TEMPERATURE + raw_temperature)
        return payload

    @classmethod
    def _convert_to_raw_temperature(cls, temperature: float) -> bytes:
        try:
            return int(temperature * 100).to_bytes(2, "little", True)
        except OverflowError:
            return int(cls.ERROR_TEMPERATURE * 100).to_bytes(2, "little", True)

    @staticmethod
    def _make_adv_element(adv_type: bytes, data) -> bytearray:
        element = adv_type + data
        length_bytes = len(element).to_bytes(1, "little")
        return bytearray(length_bytes) + element
