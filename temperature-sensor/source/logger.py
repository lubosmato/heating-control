import utime as time


class Logger:
    class Level:
        ERROR = 4
        WARNING = 3
        INFO = 2
        DEBUG = 1
        NOTSET = 0

    def debug(self, message):
        pass

    def info(self, message):
        pass

    def warning(self, message):
        pass

    def error(self, message):
        pass


class SimpleLogger(Logger):
    def __init__(self, level=Logger.Level.NOTSET):
        self._level = level

    def write(self, message: str):
        pass

    def debug(self, message):
        if self._level <= Logger.Level.DEBUG:
            self.write("{} | DEBUG | {}".format(self._get_date_time_message(), message))

    def info(self, message):
        if self._level <= Logger.Level.INFO:
            self.write("{} | INFO | {}".format(self._get_date_time_message(), message))

    def warning(self, message):
        if self._level <= Logger.Level.WARNING:
            self.write("{} | WARNING | {}".format(self._get_date_time_message(), message))

    def error(self, message):
        if self._level <= Logger.Level.ERROR:
            self.write("{} | ERROR | {}".format(self._get_date_time_message(), message))

    @staticmethod
    def _get_date_time_message():
        return "{0:04d}-{1:02d}-{2:02d} {3:02d}:{4:02d}:{5:02d}".format(*time.localtime(time.time()))


# do not use this logger - terribly slow
class FileLogger(SimpleLogger):
    def __init__(self, file_stream, level=Logger.Level.NOTSET):
        super().__init__(level)
        self._stream = file_stream

    def write(self, message: str):
        self._stream.write(message + "\n")
        self._stream.flush()


# do not use this logger - terribly slow
class FileRotatingLogger(FileLogger):
    def __init__(self, file_path: str, max_size: int, level=Logger.Level.NOTSET):
        """
        :param max_size: maximum log file size in bytes
        """
        self._max_size = max_size
        self._file_path = file_path
        stream = self._open_log_file()
        super().__init__(stream, level)
        self._truncate_file()

    def _open_log_file(self):
        # seems like opening a missing file with "r+" mode on micropython causes error
        try:
            f = open(self._file_path, "r")
            f.close()
        except FileNotFoundError:
            f = open(self._file_path, "w")
            f.close()
        stream = open(self._file_path, "r+")
        stream.seek(0, 2)  # seek to the end

        return stream

    def _truncate_file(self):
        position = self._stream.tell()
        if position >= self._max_size:
            self._stream.seek(0)
            self._stream.truncate(self._max_size - 1)
            self._stream.seek(0)

    def write(self, message: str):
        new_end = self._stream.tell() + len(message)
        if new_end >= self._max_size:
            self._stream.seek(0)
        super().write(message)

    def __del__(self):
        self._stream.close()


class ConsoleLogger(SimpleLogger):
    def __init__(self, level=Logger.Level.NOTSET):
        super().__init__(level)

    def write(self, message: str):
        print(message)

    def debug(self, message):
        if self._level <= Logger.Level.DEBUG:
            self.write("\033[1;30m{} | DEBUG | {}\033[0m".format(self._get_date_time_message(), message))

    def info(self, message):
        if self._level <= Logger.Level.INFO:
            self.write("{} | INFO | {}".format(self._get_date_time_message(), message))

    def warning(self, message):
        if self._level <= Logger.Level.WARNING:
            self.write("\033[1;33m{} | WARNING | {}\033[0m".format(self._get_date_time_message(), message))

    def error(self, message):
        if self._level <= Logger.Level.ERROR:
            self.write("\033[1;31m{} | ERROR | {}\033[0m".format(self._get_date_time_message(), message))


class CombinedLogger(Logger):
    def __init__(self, loggers):
        self._loggers = loggers

    def debug(self, message):
        for logger in self._loggers:
            logger.debug(message)

    def info(self, message):
        for logger in self._loggers:
            logger.info(message)

    def warning(self, message):
        for logger in self._loggers:
            logger.warning(message)

    def error(self, message):
        for logger in self._loggers:
            logger.error(message)
