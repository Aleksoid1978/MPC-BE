### Introduction

Compact Encoding Detection(CED for short) is a library written in C++ that
scans given raw bytes and detect the most likely text encoding.

Basic usage:

```
#include "compact_enc_det/compact_enc_det.h"

const char* text = "Input text";
bool is_reliable;
int bytes_consumed;

Encoding encoding = CompactEncDet::DetectEncoding(
        text, strlen(text),
        nullptr, nullptr, nullptr,
        UNKNOWN_ENCODING,
        UNKNOWN_LANGUAGE,
        CompactEncDet::WEB_CORPUS,
        false,
        &bytes_consumed,
        &is_reliable);

```

### How to build

You need [CMake](https://cmake.org/) to build the package. After unzipping
the source code , run `autogen.sh` to build everything automatically.
The script also downloads [Google Test](https://github.com/google/googletest)
framework needed to build the unittest.

```
$ cd compact_enc_det
$ ./autogen.sh
...
$ bin/ced_unittest
```

On Windows, run `cmake .` to download the test framework, and generate
project files for Visual Studio.

```
D:\packages\compact_enc_det> cmake .
```

### 3rd party bindings for other languages

- **[sonicdoe/ced](https://github.com/sonicdoe/ced)** Node.js module
- **[cloudaper/compact_enc_det](https://github.com/cloudaper/compact_enc_det)** Ruby gem

Have you created bindings for another language? Open a PR and add it to the list!
