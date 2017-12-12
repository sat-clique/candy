|        Linux        |       Windows       |       Coverage       |
|:-------------------:|:-------------------:|:--------------------:|
| [![travisCI][1]][2] | [![appveyor][3]][4] | [![coveralls][5]][6] |

[1]: https://travis-ci.org/Udopia/candy-kingdom.svg?branch=master
[2]: https://travis-ci.org/Udopia/candy-kingdom
[3]: https://ci.appveyor.com/api/projects/status/f82s06j62stkseuyksb0
[4]: https://ci.appveyor.com/project/Udopia/candy-kingdom/branch/master
[5]: https://coveralls.io/repos/github/Udopia/candy-kingdom/badge.svg?branch=master
[6]: https://coveralls.io/github/Udopia/candy-kingdom?branch=master

# candy-kingdom

Candy has a dependency on the "googletest" API. In order to build candy with tests you need to init and build the googletest submodule like this:
```
git submodule init
git submodule update
cd lib/googletest
cmake .
make
```

Then build candy like this:
```
mkdir release
cd release
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Release ..
make candy
```

To build and execute unit-tests (takes a few minutes) type:
```
make
ctest
```

To support my independence as a developer, and thus my work on Candy Solver, send Bitcoin to the following address: 
```
1K4urtiPrFjXwgNodcDYUyLAe8fwyHxbyJ
```