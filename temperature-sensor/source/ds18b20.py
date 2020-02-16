import ds18x20
import time


class DS18B20(ds18x20.DS18X20):
    def convert_read_temp(self, rom) -> float:
        self.convert_temp()
        time.sleep(0.75)
        temperature = self.read_temp(rom)
        if temperature == 85.0:
            raise RuntimeError("Wrong read")
        return temperature
