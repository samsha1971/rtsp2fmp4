## rtsp2fmp4

### 程序说明

rtsp2fmp4 是一个用C++写的代理服务器，主要功能是通过live555访问摄像机的 RTSP 服务器，实时把 H264 封装为 FMP4，发送给请求的客户端，客户端通过 WebSocket+MSE 的技术来支持毫秒级别的延迟HTML5 直播。

rtsp2fmp4是个原型演示，目前支持Windows/Linux的CMake编译。

### 依赖

- Visual Studio 15 2017 Win64 或者  [gcc](http://ftp.tsukuba.wide.ad.jp/software/gcc/releases/ "gcc") 4.8.5 (测试使用的是CentOS 7.7.1908)，参考资料：[Centos7 手动编译升级 gcc](https://blog.csdn.net/z960339491/article/details/98882711 "Centos7 手动编译升级 gcc")

- [live555 lastest](http://live555.com/liveMedia/ "live555")

- [boost 1.69.0]( https://dl.bintray.com/boostorg/release/1.69.0 "boost 1.69.0")，参考资料： [Linux：编译安装boost 1.69库](https://blog.csdn.net/weixin_34309435/article/details/92393006  "Linux：编译安装boost 1.69库")

- [rapidjson 1.1.0](https://github.com/Tencent/rapidjson/tree/v1.1.0 "rapidjson 1.1.0")

- [websocketpp 0.8.1](https://github.com/zaphoyd/websocketpp "websocketpp 0.8.1") 

- [cmake 3.15.3]( https://cmake.org/download/ "cmake 3.15.3")

### 测试方法

1. 编译live555、boost 1.69.0，修改源码目录下的CMakeLists.txt，确认include、lib目录位置。

2. 配置Visual Studio 2017/2019的跨平台功能。

3. 修改 config.json：
    [
        {
            "source": "/101",
            "target": "rtsp://user:password@ipaddr/Streaming/Channels/101"
        }
    ]
增加redirect的参数支持，比如：
```
ws://10.200.2.46:9002/?redirect=rtsp://user:password@ipaddr/Streaming/Channels/101
```
4. 运行#rtsp2fmp4，启动服务，服务端口为 9002，退出服务运行#rtsp2fmp4 -q。

5. 测试依赖 nodejs，在 exmaple 目录下运行#node http.js，网页访问：http://localhost:9080/index.html。
