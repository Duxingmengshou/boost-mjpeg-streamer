#include <opencv2/opencv.hpp>
#include <thread>
#include "http_server.h"

int main() {
    http_server hs;
    hs.run();
//    cv::VideoCapture cap("check.mp4");
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
