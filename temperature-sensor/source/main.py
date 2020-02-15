from config import load_config, save_config
import utime


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
    print(config)


if __name__ == '__main__':
    main()
