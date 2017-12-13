|        Linux        |       Windows       |       Coverage       |
|:-------------------:|:-------------------:|:--------------------:|
| [![travisCI][1]][2] | [![appveyor][3]][4] | [![coveralls][5]][6] |

[1]: https://travis-ci.org/Udopia/candy-kingdom.svg?branch=master
[2]: https://travis-ci.org/Udopia/candy-kingdom
[3]: https://ci.appveyor.com/api/projects/status/f82s06j62stkseuyksb0
[4]: https://ci.appveyor.com/project/Udopia/candy-kingdom/branch/master
[5]: https://coveralls.io/repos/github/Udopia/candy-kingdom/badge.svg?branch=master
[6]: https://coveralls.io/github/Udopia/candy-kingdom?branch=master

# Candy Kingdom

**Candy Kingdom** is a growing collection of SAT solvers and tools for structure analysis in SAT problems. The original **Candy** solver is a branch of the famous SAT solver **[Glucose](http://www.labri.fr/perso/lsimon/glucose/)**. Several new approaches (e.g. rsar and rsil-variants) in Candy focus on explicit exploitation of structure analysis in SAT solving. Furthermore the core of Glucose has been completely reworked in order to increase *readability* and *maintainability* of the code itself. Much work was done for increasing the *independence* and *exchangeability* of components in the core solver. The allocation model of clauses was revisited with a focus on cache efficient *memory management*. A new *sonification* module provides *[Ear Candy](https://www.youtube.com/watch?v=iupgZGlzMCQ)*, you can now also listen to solver runs. 

## Build

Candy uses the [googletest](https://github.com/google/googletest) API, which is why, in order to build Candy you need to initialize and build the googletest submodule like this:
```bash
git submodule init
git submodule update
cd lib/googletest
cmake .
make
```

Then you can build Candy like this:
```bash
mkdir release
cd release
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Release ..
make candy
```

In order to build and execute unit-tests execute:
```bash
make
ctest
```

## Run

In order to run candy and solve a problem `example.cnf` invoke:
```bash
candy example.cnf
```

Candy offers a multitude of options, like paramaters to tune heuristics and thresholds, or parameters to select an alternative solving strategy (e.g. *rsar* or *rsil*). If you have any questions feel free to contact [me](mailto:2.markus.iser@gmail.com).

## Support

If you like Candy and you want you to support me working on the Candy Kingdom, you can choose to send me 

| Currency 		| Address |
|:-------------------:	|:-------------------:|
| Ethereum 		| `0x118b032be2c45f9a74301e0432d67dbbcdf7b6f0` |
| ZCash 			| `t1PWs26aA9BMLWnPHVrQkur4ecwCt5uDXiK` |
| Bitcoin			| `37kV4SYeBoEQ2f2CWgHfsJs2qV7F58pYZw` |
| Monero			| `44MaszZqqRaFcQKjQbpXUB1CjGovuyi7Qf1Cmv714s4rZQHNtapeXH7WbwaVe4vUMveKAzAiA4j8xgUi29TpKXpm41qZuEo` |
| Dash			| `XuFDFWra2e4rTpGmudDV6nC8g8S3d4ihZs` |

or just plain old [cash](https://www.paypal.me/markusiser/13) 