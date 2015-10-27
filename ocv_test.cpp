// OpenCV Grab Capture

#include <iostream>
#include <string>

extern "C" {
    #include <x264.h>
}

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"

using namespace std;
using namespace cv;

Mat src; Mat dst;
char window_name1[] = "Unprocessed Image";
char window_name2[] = "Processed Image";

int main(int argc, char *argv[])
{
    // Frame width and height
    int width, height;

    // OpenCV
    Mat frame_bgr;
    Mat frame_yuv;

    VideoCapture cap(0);
    if (!cap.isOpened()) {
        return -1;
    }

    cap >> frame_bgr;
    width = frame_bgr.cols;
    height = frame_bgr.rows;

    // x264
    x264_param_t param;
    x264_picture_t pic;
    x264_picture_t pic_out;
    x264_t *enc;

    int frame_size = 0;
    x264_nal_t *nal;
    int i_nal;

    // Get default params
    if (x264_param_default_preset( &param, "medium", NULL) < 0) {
        return -1;
    }

    // Non-default params
    param.i_csp = X264_CSP_I420;
    param.i_width = width;
    param.i_height = height;
    param.b_vfr_input = 0;
    param.b_repeat_headers = 1;
    param.b_annexb = 1;

    if (x264_param_apply_profile(&param, "baseline") < 0) {
        return -1;
    }

    if (x264_picture_alloc(&pic, param.i_csp, param.i_width, param.i_height) < 0) {
        return -1;
    }

    enc = x264_encoder_open(&param);
    if (!enc) {
        return -1;
    }

    int luma_size = width * height;
    int chroma_size = luma_size / 4;

    Mat edges;
    namedWindow("edges", 1);

    for (int i = 0; i < 100; i++) {
        cap >> frame_bgr;
        imshow("webcam", frame_bgr);

        cvtColor(frame_bgr, frame_yuv, CV_BGR2YUV_I420);

        cout << "cols: " << frame_bgr.cols << endl;
        cout << "rows: " << frame_bgr.rows << endl;
        cout << "channels: " << frame_bgr.channels() << endl;

        cout << "y_cols: " << frame_yuv.cols << endl;
        cout << "y_rows: " << frame_yuv.rows << endl;
        cout << "y_chans: " << frame_yuv.channels() << endl;

        memcpy(pic.img.plane[0], frame_yuv.data, luma_size);
        memcpy(pic.img.plane[1], frame_yuv.data + luma_size, chroma_size);
        memcpy(pic.img.plane[2], frame_yuv.data + luma_size + chroma_size, chroma_size);

        pic.i_pts = i;
        frame_size = x264_encoder_encode(enc, &nal, &i_nal, &pic, &pic_out);
        if (frame_size < 0) {
            return -1;
        } else if (frame_size > 0) {
            cout << "Frame size: " << frame_size << endl;
        }

/*
        if (waitKey(30) >= 0) {
            break;
        }
*/
    }

    while (x264_encoder_delayed_frames(enc)) {
        frame_size = x264_encoder_encode(enc, &nal, &i_nal, NULL, &pic_out);
        if (frame_size < 0) {
            return -1;
        } else if (frame_size > 0) {
            cout << "Frame size: " << frame_size << endl;
        }
    }

    x264_encoder_close(enc);
    x264_picture_clean(&pic);

    return 0;
}

