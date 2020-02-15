from broadcaster import Broadcaster
from config import load_config, save_config
import utime
import machine


def measure(f):
    def wrap(*args, **kwargs):
        start = utime.ticks_ms()
        ret = f(*args, **kwargs)
        end = utime.ticks_ms()
        print("Time:", end - start, "ms")
        return ret

    return wrap


@measure
def main():
    config = load_config()
    broadcaster = Broadcaster(1000)
    broadcaster.broadcast(42.42)
    machine.reset()


if __name__ == '__main__':
    main()
