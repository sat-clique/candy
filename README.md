|        Linux        |       Windows       |       Coverage       |     Metrics      |    Sonarcloud     |
|:-------------------:|:-------------------:|:--------------------:|:----------------:|:-----------------:|
| [![travisCI][1]][2] | [![appveyor][3]][4] | [![coveralls][5]][6] | [![tokei][7]][8] | [![sonar][9]][10] |

[1]: https://travis-ci.org/Udopia/candy-kingdom.svg?branch=master
[2]: https://travis-ci.org/Udopia/candy-kingdom
[3]: https://ci.appveyor.com/api/projects/status/s9w7la4p8pdi5cja?svg=true
[4]: https://ci.appveyor.com/project/Udopia/candy-kingdom/branch/master
[5]: https://coveralls.io/repos/github/Udopia/candy-kingdom/badge.svg?branch=master
[6]: https://coveralls.io/github/Udopia/candy-kingdom?branch=master
[7]: https://tokei.rs/b1/github/udopia/candy-kingdom?category=code
[8]: https://github.com/Aaronepower/tokei#badges
[9]: https://sonarcloud.io/api/project_badges/measure?project=candy&metric=alert_status
[10]: https://sonarcloud.io/dashboard?id=candy

# Candy

**Candy** is a modular SAT solver forked from Candy Kingdom and reduced to its core. The original **Candy** solver is a branch of the well-known SAT solver **[Glucose](http://www.labri.fr/perso/lsimon/glucose/)**. It has been refactored w.r.t. *independence* and *exchangeability* of components in the core solver, while increasing the *readability* and *maintainability* of the code. 

## Build

You can build Candy like this:
```bash
mkdir build
cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Release ..
make candy
```

## Run

In order to run candy and solve a problem `example.cnf` invoke:
```bash
./candy example.cnf
```

Candy offers a multitude of options, like paramaters to tune heuristics and thresholds, or parameters to select an alternative solving strategy:
```bash
./candy --help
``` 


## Credits

Credits go to SAT CLIQUE, all the people at ITI and to our students. 


## Build and make on Windows
- mingw installed ([mingw-64 (posix)](https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/installer/mingw-w64-install.exe/download))
- mingw32-pthreads (mingw32-libpthreadgc-dll, mingw32-libpthreadgc-dev, mingw32-pthreads-w32*)
- mingw32-libz-dll and mingw32-libz-dev
- mingw32-libgomp-dll for OpenMP (if needed)
- everything else required to compile c/c++
- mingw64\bin directory added to PATH

```bash
mkdir release
cd release
cmake -G "MinGW Makefiles" -DZLIB_INCLUDE_DIR=<mingw-include-dir> -DZLIB_LIBRARY=<path-to-mingw-libz.a> -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Release ..
cmake --bild . --target candy
```

example path-to-mingw-include-dir: C:/mingw-w64/x86_64-8.1.0-posix-seh-rt_v6-rev0/mingw64/x86_64-w64-mingw32/include

example path-to-mingw-libz: C:/mingw-w64/x86_64-8.1.0-posix-seh-rt_v6-rev0/mingw64/x86_64-w64-mingw32/lib/libz.a
