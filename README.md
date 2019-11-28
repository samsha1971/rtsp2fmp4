## rtsp2fmp4

### Description

Rtsp2fmp4 is a proxy server written in C++, whose main function is to access the RTSP server of the camera through live555, package H264 as FMP4 in real time, and send it to the requesting client. The client supports the live broadcast of delayed HTML5 in milliseconds through WebSocket+MSE technology.

Rtsp2fmp4 is a prototype demo that currently supports CMake compilation for Windows/Linux.

### Dependencies 

- Visual Studio 15 2017 Win64 /  [GCC](http://ftp.tsukuba.wide.ad.jp/software/gcc/releases/ "gcc") 4.8.5|8.3.0 ( Test using CentOS 7.7.1908 )， reference ：[Centos7 manually compile upgrade GCC](https://blog.csdn.net/z960339491/article/details/98882711 "Centos7 manually compile upgrade GCC")

- [live555 lastest](http://live555.com/liveMedia/ "live555")

- [boost 1.69.0]( https://dl.bintray.com/boostorg/release/1.69.0 "boost 1.69.0")，reference： [Linux：Compile boost 1.69](https://blog.csdn.net/weixin_34309435/article/details/92393006  "Linux：Compile boost 1.69")

- [rapidjson 1.1.0](https://github.com/Tencent/rapidjson/tree/v1.1.0 "rapidjson 1.1.0")

- [websocketpp 0.8.1](https://github.com/zaphoyd/websocketpp "websocketpp 0.8.1") 

- [cmake 3.15.3]( https://cmake.org/download/ "cmake 3.15.3")

### Test

1. Compile live555, boost 1.69.0, modify  CMakeLists.txt in the source directory, confirm include and lib's directories.

2. Configure  Visual Studio 2017/2019 Cross Platform.

3. Modify config.json:

```
[
    {
        "source" : "/101",
        "target" : "rtsp://sam:ibc960014@10.200.2.229/Streaming/Channels/101"
    }
]
```
4. Run #rtsp2fmp4, monitor port is 9002.

5. The test relies on nodejs, in exmaple directory run #node http.js, web access: http://localhost:9080/index.html.
