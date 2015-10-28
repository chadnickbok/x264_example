// OpenCV Grab Capture

#include <iostream>
#include <string>
#include <cstdio>

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

    // Output file
    FILE *out_file = fopen(argv[1], "wb");

    // OpenCV Data Source
    Mat frame_bgr;
    Mat frame_yuv;

    VideoCapture cap(0);

    cap >> frame_bgr;
    imshow("webcam", frame_bgr);

    double freq = getTickFrequency();
    cout << "freq: " << freq << endl;

    width = frame_bgr.cols;
    height = frame_bgr.rows;

    // x264 Config
    x264_param_t encoder_config;

    // Get default params
    x264_param_default_preset( &encoder_config, "fast", NULL );

    // Non-default params
    encoder_config.i_csp = X264_CSP_I420;
    encoder_config.i_width = width;
    encoder_config.i_height = height;
    encoder_config.b_vfr_input = 0;
    encoder_config.b_repeat_headers = 1;

    encoder_config.i_fps_num = 10;
    encoder_config.i_fps_den = 1;
    
    encoder_config.b_annexb = 1;

    // Apply a profile
    x264_param_apply_profile( &encoder_config, "baseline" );


    // Initialize the encoder
    x264_t *encoder = x264_encoder_open(&encoder_config);

    // Setup the input picture
    x264_picture_t pic_in;
    x264_picture_alloc( &pic_in, encoder_config.i_csp, encoder_config.i_width, encoder_config.i_height);

    // Outputs
    x264_picture_t pic_out;
    x264_nal_t *nalus;

    int frame_size;
    int num_nalus;

    // Raw data size
    int luma_size = encoder_config.i_width * encoder_config.i_height;
    int chroma_size = luma_size / 4;

    // Display our webcam while capturing
    Mat edges;
    namedWindow("edges", 1);

    // Encode frames (max 1000)
    for (int i = 0; i < 10000; i++)
    {
        // Capture a frame 
        cap >> frame_bgr;
        imshow("webcam", frame_bgr);

        // Convert to the right colorspace
        cvtColor(frame_bgr, frame_yuv, CV_BGR2YUV_I420);

        // Copy to our pic_in
        memcpy( pic_in.img.plane[0], frame_yuv.data, luma_size );
        memcpy( pic_in.img.plane[1], frame_yuv.data + luma_size, chroma_size );
        memcpy( pic_in.img.plane[2], frame_yuv.data + luma_size + chroma_size, chroma_size );

        // Set the timestamp
        pic_in.i_pts = i;
        cout << "Cur timestamp: " << pic_in.i_pts << endl;

        // Encode the frame
        frame_size = x264_encoder_encode( encoder, &nalus, &num_nalus, &pic_in, &pic_out );
        
        // Check if we received a frame
        if ( frame_size > 0 )
        {
            fwrite( nalus->p_payload, 1, frame_size, out_file);
        }

        // Stop on keypress
        if ( waitKey(30) >= 0 )
        {
            break;
        }
    }

    // Flush delayed frames
    while ( x264_encoder_delayed_frames( encoder ))
    {
        frame_size = x264_encoder_encode( encoder, &nalus, &num_nalus, NULL, &pic_out );
        if ( frame_size > 0 )
        {
            fwrite( nalus->p_payload, frame_size, 1, out_file );
        }
    }

    x264_encoder_close( encoder );
    x264_picture_clean( &pic_in );
    fclose( out_file );

    return 0;
}

