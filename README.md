## rtsp2fmp4

### 程序说明：

本程序是个原型演示。
c++写的 rtsp2fmp4 是一个服务器，通过 RTSP 访问摄像机的 RTSP 服务器，实时把 H264 封装为 FMP4，发送给请求的客户端。
客户端通过 WebSocket+MSE 的技术来支持 HTML5 直播。

### 测试方法:

1.修改 fmp4_server.cpp 的代码

    proxy.insert(std::pair<std::string, std::string>("/101", "rtsp://sam:ibc960014@10.200.2.229/Streaming/Channels/101"));

2.运行#rtsp2fmp4，监控端口 9002，

3.测试依赖 nodejs，在 exmaple 目录下运行#node http.js，网页访问：http://localhost:9080/index.html。
