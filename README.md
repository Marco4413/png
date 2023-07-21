## PNG

This is a C++20 library which enables to read png images from a binary stream.

I decided to create this because I wanted to learn how png images are stored.
This project was made for educational purposes and may be discontinued at any time.

### Building

**Building is only supported on Linux.** (you can use WSL if you are on Windows)

To download libraries you should run the script file `setup.sh` which is located in the `libs` directory
(THE WORKING DIR MUST BE THE `libs` FOLDER).

Running `setup.sh` requires `git` to be installed.

```sh
$ cd libs
$ ./setup.sh
$ cd ..
```

To actually build the project you should run the `build.sh` script, have the `zlib1g-dev` and `zlib1g` apt packages and the `g++` compiler
installed.

The executable will be found at `out/main`.

```sh
$ ./build.sh
```

### TODO

1. Splitting the project into multiple files
2. CRC implementation
3. Writing images to streams
4. Custom ZLib implementation

### Resources

- PNG 1.2 spec: [http://www.libpng.org/pub/png/spec/1.2](http://www.libpng.org/pub/png/spec/1.2)
- Test Image: [https://upload.wikimedia.org/wikipedia/commons/4/47/PNG_transparency_demonstration_1.png](https://upload.wikimedia.org/wikipedia/commons/4/47/PNG_transparency_demonstration_1.png)
  - Adam7 Interlace version was created using GIMP.
