# nachalnick
=======
Simple TODO linux application.
Relies on sending notifications via ```notify-send```, but can be easily replaced with another command (see definitions section of source file).

#### Depends on
* C compiler with c89 support (i.e. any);
* Any UNIX-like OS, like macos or linux.

## Installation
```
$ make
# make install
```

## Usage
Starting background notifier process : ```nachalnick -d &```

Adding simple todo entry: ```nachalnick -a 0 "TEXT"```

List entries: ```nachalnick -L```

More info: ```nachalnick -h```

## License
MIT
