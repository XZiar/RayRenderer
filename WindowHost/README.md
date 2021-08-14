# WindowHost

Utility for native window access.

# Library Dependency

## Termux

Install `x11-repo` first.

Install following library: `libxkbcommon`, `libx11`, `xorgproto`.

## Ubuntu

Install following library: `libx11-dev`, `libx11-xcb-dev`, `libxkbcommon-dev`, `libxkbcommon-x11-dev`.

## iOS

Install `x11`.

You may see missing dependency of `xextproto`, which cna be found on [`Cydia/Telesphoreo`](http://apt.saurik.com/). It may not be avaliable on new Cydia, but you can still manually download and install from [here](http://apt.saurik.com/debs/xextproto_7.0.2-2_iphoneos-arm.deb).

`xkbcommon` is not provided on Cydia, maybe try build yourself.


## License

WindowHost (including its component) is licensed under the [MIT license](./License.txt).
