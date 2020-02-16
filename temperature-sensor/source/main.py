from broadcaster import Broadcaster
from ds18b20 import DS18B20
from config import load_config, save_config
import utime as time
import machine
import onewire
import esp

start = time.ticks_ms()
end = 0


def measure(label):
    global start, end
    end = time.ticks_ms()
    print("[{}] Time:".format(label), end - start, "ms")


def main():
    esp.osdebug(None)
    config = load_config()
    broadcaster = Broadcaster(100)
    one_wire = onewire.OneWire(machine.Pin(22, machine.Pin.PULL_UP))
    temp_sensor = DS18B20(one_wire)

    sensor_ids = temp_sensor.scan()
    if not sensor_ids:
        machine.reset()
        return
    sensor_id = sensor_ids[0]

    try:
        temperature = temp_sensor.convert_read_temp(sensor_id)
        broadcaster.broadcast(temperature)
    except Exception:
        broadcaster.broadcast(Broadcaster.ERROR_TEMPERATURE)

    measure("end")
    machine.reset()


if __name__ == '__main__':
    main()
