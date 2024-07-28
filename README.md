# boost-mjpeg-streamer

A simple imitation of the https://github.com/nadjieb/cpp-mjpeg-streamer project using the boost library.

### Usage Examples

```c++
#include <opencv2/opencv.hpp>
#include <thread>
#include "http_server.h"

int main() {
    http_server hs;
    hs.run();
    cv::VideoCapture cap(0);
    if (!cap.isOpened()) {
        std::cerr << "VideoCapture not opened\n";
        exit(EXIT_FAILURE);
    }
    while (true) {
        cv::Mat frame;
        cap >> frame;
        if (frame.empty()) {
            std::cerr << "frame not grabbed\n";
            exit(EXIT_FAILURE);
        }
        std::vector<uchar> buff_bgr;
        cv::imencode(".jpg", frame, buff_bgr);
        std::shared_ptr<std::vector<char>> buf = std::make_shared<std::vector<char>>(buff_bgr.begin(), buff_bgr.end());
        hs.publish("check", buf);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    return 0;
}
```

Another project of mine builds a video stream information obtained from TCP/UDP, and then forwards it as HTTP MJPEG video stream in the middle layer. Its address is: https://github.com/Duxingmengshou/PySocketMjpegStreamer, but I think its performance is limited, so I want to use C++ and Boost library to reconstruct it, and this small HTTP service is one of the components responsible for HTTP service.
