# API Documentation

This document keeps a list of all available options you can specify in your cbuild.toml file.

## Global Options

| Option   | Type | Possible Values | Mandatory | Description            |
|:--------:|:----:|:---------------:|:---------:|:----------------------:|
| language | str  | c, c++          | x         | project language       |
| primary  | str  | any             | x         | name of primary module |
| compiler | str  | any             |           | what compiler to use   |
| linker   | str  | any             |           | what linker to use     |
| archiver | str  | any             |           | what archiver to use   |

## Per-module Options

| Option  | Type  | Possible Values                   | Mandatory | Description                         |
|:-------:|:-----:|:---------------------------------:|:---------:|:-----------------------------------:|
| type    | str   | executable, dynamiclib, staticlib | x         | what kind of module                 |
| sources | array | any                               | x         | list of source files to compile     |
| cflags  | array | any                               |           | list of compiler flags to use       |
| output  | str   | any                               |           | custom output name over module name |
