## PNG

This is a C++20 library which allows reading png images from a binary stream.

I decided to create this because I wanted to learn how png images are stored.
This project was made for educational purposes and may be discontinued at any time.

### Building

This project uses `premake5` to create build files. All build files created by premake are put into the `build` directory.
Solutions and Make files are not pushed to this repo, you should run `premake5` when you download this project.

**NOTE:** Remember to have a compiler which fully supports C++20!

Build output can be found inside either `build/Debug` or `build/Release` depending on the configuration. If building `png` (the static
library's project) the library file will be found in the same directories, and include files can be found inside the `include` folder.

If you want to see an example of linking with `png` check out the `png-dev` project inside [`premake5.lua`](https://github.com/Marco4413/png/blob/master/premake5.lua#L13).

### TODO

1. ~~Splitting the project into multiple files~~
2. ~~CRC implementation~~
3. ~~Writing images to streams~~
4. ~~PLTE Chunk support~~
5. Custom ZLib implementation (maybe)

### Resources

- PNG 1.2 spec: [http://www.libpng.org/pub/png/spec/1.2](http://www.libpng.org/pub/png/spec/1.2)
- Test Image: [https://upload.wikimedia.org/wikipedia/commons/4/47/PNG_transparency_demonstration_1.png](https://upload.wikimedia.org/wikipedia/commons/4/47/PNG_transparency_demonstration_1.png)
  - Adam7 Interlace version was created using GIMP.
