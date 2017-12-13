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

**Candy Kingdom** is a growing collection of SAT solvers and tools for structure analysis in SAT problems. The original **Candy** solver is a branch of the famous **Glucose** solver. Several new approaches (e.g. rsar and rsil-variants) in Candy focus on explicit exploitation of structure analysis in SAT solving. Furthermore the core of Glucose has been completely reworked in order to increase *readability* and *maintainability* of the code itself. Much work was done for increasing the independence and exchangeability of components in the core solver. The allocation model of clauses was revisited with a focus on cache efficient memory management. A new sonification module provides *Ear Candy*, you can now also listen to solver runs. 

## Build

Candy depends on the "googletest" API. In order to build Candy with tests you need to init and build the googletest submodule like this:
```bash
git submodule init
git submodule update
cd lib/googletest
cmake .
make
```

Then build Candy like this:
```bash
mkdir release
cd release
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DCMAKE_BUILD_TYPE=Release ..
make candy
```

To build and execute unit-tests (takes a few minutes) type:
```bash
make
ctest
```

## Run

In order to run candy and solve a problem `example.cnf` just type
```bash
candy example.cnf
```

There are may options, like paramaters that enable you to tune the solver, or to pick an alternative solving strategy (e.g. **rsar** or **rsil**). If you have any questions feel free to contact [me](mailto:2.markus.iser@gmail.com).

## Support

If you like candy and want to support my work on candy kingdom, you can choose to send me 

| Currency 		| Address |
|:-------------------:	|:-------------------:|
| Ethereum 		| `0x118b032be2c45f9a74301e0432d67dbbcdf7b6f0` |
| ZCash 			| `t1PWs26aA9BMLWnPHVrQkur4ecwCt5uDXiK` |
| Bitcoin			| `37kV4SYeBoEQ2f2CWgHfsJs2qV7F58pYZw` |
| Monero			| `44MaszZqqRaFcQKjQbpXUB1CjGovuyi7Qf1Cmv714s4rZQHNtapeXH7WbwaVe4vUMveKAzAiA4j8xgUi29TpKXpm41qZuEo` |
| Dash			| `XuFDFWra2e4rTpGmudDV6nC8g8S3d4ihZs` |

Or just plain old [cash](https://www.paypal.me/markusiser/10) 