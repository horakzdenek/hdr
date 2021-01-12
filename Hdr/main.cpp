#include "opencv2/opencv.hpp"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>


using namespace cv;
using namespace std;
using namespace std::chrono;



Mat overlay(Mat a, Mat b)
{
        Mat tmp_hsv;
        cvtColor(a,tmp_hsv,COLOR_BGR2HSV);

        Mat img1 = tmp_hsv.clone();
        Mat img2 = b.clone();
        Mat result = a.clone();


        for(int i = 0; i < img1.size().height; ++i)
        {
            for(int j = 0; j < img1.size().width; ++j)
            {
                //float target = float(img1.at<uchar>(i, j)) / 255;
                Vec3b target_intensity = img1.at<Vec3b>(i, j);
                float target_b = float(target_intensity.val[0])/255;
                float target_g = float(target_intensity.val[1])/255;
                float target_r = float(target_intensity.val[2])/255;


                //float blend = float(img2.at<uchar>(i, j)) / 255;
                Vec3b blend_intensity = img2.at<Vec3b>(i, j);
                float blend_r = float(blend_intensity.val[2])/255;

                // overlay
                if(target_r > 0.5)
                {

                    //result.at<float>(i, j) = (1 - (1-2*(target-0.5)) * (1-blend));
                    Vec3b color;
                    color.val[0] = target_b*255;
                    color.val[1] = target_g*255;
                    color.val[2] = (1 - (1-2*(target_r-0.5)) * (1-blend_r))*255;
                    result.at<Vec3b>(i, j) = color;

                }
                else
                {
                    //result.at<float>(i, j) = ((2*target) * blend);
                    Vec3b color;
                    color.val[0] = target_b*255;
                    color.val[1] = target_g*255;
                    color.val[2] = ((2*target_r) * blend_r)*255;
                    result.at<Vec3b>(i, j) = color;
                }
            }
        }

        // back to RGB
        cvtColor(result,result,COLOR_HSV2BGR);

        return result;

}

void hdr(const Mat &image, Mat &result)
{
        Mat frame = image.clone();
        Mat gray = frame.clone();
        cvtColor(frame, gray, COLOR_BGR2GRAY);
        cvtColor(gray, gray, COLOR_GRAY2BGR);
        bitwise_not(gray,gray);
        GaussianBlur(gray,gray,Size(175,175),0);
        result = overlay(frame,gray);
}


// globalni deklarace kvuli multithreadu
Mat img_1;
Mat img_2;
Mat img_3;
Mat img_4;

Mat gray_1;
Mat gray_2;
Mat gray_3;
Mat gray_4;

void h1()
{
    img_1 = overlay(img_1,gray_1);
}

void h2()
{
     img_2 = overlay(img_2,gray_2);
}

void h3()
{
     img_3 = overlay(img_3,gray_3);
}

void h4()
{
     img_4 = overlay(img_4,gray_4);
}



int main( int argc, char *argv[] )
{
    string filename = "";
    string fileOut = "";
    int START = 0;
    int LEN = 1;
    if (argc > 4)
    {
        string arg1(argv[1]);
        string arg2(argv[2]);
        string arg3(argv[3]);
        string arg4(argv[4]);
        filename = arg1;
        fileOut = arg2;
        START = stoi(arg3);
        LEN = stoi(arg4);

    }
    else
    {
        cout << "arg1:input arg2:output arg3:start frame arg4:frame_count" << endl;
        return 0;
    }

    VideoCapture cap(filename);
    if(!cap.isOpened())
        return -1;

     // set frame interval
    if(START > 0)
    {
        cap.set(CAP_PROP_POS_FRAMES,START);
    }else
    {
        cap.set(CAP_PROP_POS_FRAMES,0);
    }
    if(LEN == 0)
    {
        LEN = cap.get(CAP_PROP_FRAME_COUNT);
    }

    Mat frame;
    cap >> frame; // new frame
    int W = frame.cols;
    int H = frame.rows;
    int framerate = cap.get(CAP_PROP_FPS);
    int frame_count = LEN;
    int iterace = 0;



    // Save video
    VideoWriter video(fileOut,VideoWriter::fourcc('p','n','g',' '),framerate, Size(W,H));




    while(1)
    {


        cap >> frame; // new frame
        if(frame.empty())
            {
                cout << "end of video or stream" << endl;
                return 0;
            }


        // gray prepare
        Mat gray = frame.clone();
        cvtColor(frame, gray, COLOR_BGR2GRAY);
        cvtColor(gray, gray, COLOR_GRAY2BGR);
        bitwise_not(gray,gray);
        int sirka = frame.cols;
        int vyska = frame.rows;
        resize(gray,gray,Size(200,200));
        GaussianBlur(gray,gray,Size(5,5),0);
        resize(gray,gray,Size(sirka,vyska));




        // multithread

        // #1 where cut
        int a = round((double)frame.cols/2.0);
        int b = frame.cols - a;
        int c = round((double)frame.rows/2.0);
        int d = frame.rows - c;
        Rect r(0,0,a,c);
        Rect s(a,0,b,c);
        Rect t(0,c,a,d);
        Rect u(a,c,b,d);

        // #2 cut
        img_1 = frame(r);
        img_2 = frame(s);
        img_3 = frame(t);
        img_4 = frame(u);

        gray_1 = gray(r);
        gray_2 = gray(s);
        gray_3 = gray(t);
        gray_4 = gray(u);



         // #3 hdr multithread
        thread th1(h1);
        thread th2(h2);
        thread th3(h3);
        thread th4(h4);
        th1.join();
        th2.join();
        th3.join();
        th4.join();




        // #4 glue
        Mat slepenec = frame.clone();
        img_1.copyTo(slepenec(Rect(0,0,a,c)));
        img_2.copyTo(slepenec(Rect(a,0,b,c)));
        img_3.copyTo(slepenec(Rect(0,c,a,d)));
        img_4.copyTo(slepenec(Rect(a,c,b,d)));


        // pomer stran a zobrazeni
        double pomer = (double)frame.cols/(double)frame.rows;
        Mat nahled = slepenec.clone();
        resize(nahled,nahled, Size((int)round(300*pomer),300));

        double pct = (double)iterace/(double)frame_count*100;
        int p = (int)round(pct);

        // draw info
        Point pt(0,12);
        Point pt1(0,0);
        Point pt2((int)round(0.01*p*nahled.cols),13);
        rectangle(nahled, pt2, pt1, Scalar(0, 0, 255), -1, 8, 0); /// Scalar definuje barvu -1 je výplň/tloušťka
        String text = "Toto je testovaci text";
        text = to_string(p)+"%";
        int fontFace = FONT_HERSHEY_SIMPLEX;
        double fontScale = 0.4;
        int thickness = 1;
        putText(nahled, text, pt, fontFace, fontScale, Scalar(0, 255, 0), thickness, 8);


        imshow("nahled",nahled);
        // write HDR video file as motion PNG (quality)
        video.write(slepenec);


        if (iterace == LEN)
        {
            break;
        }
        char key = (char)waitKeyEx(20);
        if( key == 27 || key == 'q' || key == 'Q' ) // 'ESC'
            break;

         iterace++;

    }
    return 0;
}

