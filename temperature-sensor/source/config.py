import ujson as json


class DotDict(dict):
    """
    a dictionary that supports dot notation
    as well as dictionary access notation
    usage: d = DotDict() or d = DotDict({'val1':'first'})
    set attributes: d.val2 = 'second' or d['val2'] = 'second'
    get attributes: d.val2 or d['val2']
    """
    __getattr__ = dict.__getitem__
    __setattr__ = dict.__setitem__
    __delattr__ = dict.__delitem__

    def __init__(self, dct):
        super().__init__()
        for key, value in dct.items():
            if hasattr(value, 'keys'):
                value = DotDict(value)
            self[key] = value


def load_config() -> DotDict:
    config = {
        "wifi": {  # wifi setting (wifi is active only when charging)
            "ssid": "Temperature Sensor",
            "password": "12345678"
        },
        "period": 120,  # how long the device will be sleeping [s]
    }
    try:
        with open("./config.json", "r") as f:
            custom_config = json.load(f)
            config.update(custom_config)
            return DotDict(config)
    except Exception as e:
        save_config(config)
        return DotDict(config)


def save_config(config: dict) -> None:
    with open("./config.json", "w") as f:
        json.dump(config, f)
