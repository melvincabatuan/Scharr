#include "com_cabatuan_scharr_MainActivity.h"
#include <android/log.h>
#include <android/bitmap.h>

#include "opencv2/imgproc/imgproc.hpp"

using namespace cv;

#define  LOG_TAG    "Scharr"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)





// Average 16 fps after 500 frames
void computeScharr(const cv::Mat &image, cv::Mat &result) {
 
  //result.create(image.size(), image.type());
  uchar grad_x, grad_y;

  for (int j= 1; j<image.rows-1; j++) { // for all rows except first and last
  
      const uchar* previous = image.ptr<const uchar>(j-1); // previous row
      const uchar* current = image.ptr<const uchar>(j); // current row
      const uchar* next = image.ptr<const uchar>(j+1); // next row
      uchar* output= result.ptr<uchar>(j); // output row
    
      for (int i=1; i<image.cols-1; i++) {
      
          grad_x = cv::saturate_cast<uchar>(-3 * previous[i-1] + 3 * previous[i+1] - 10 * current[i-1] + 10 * current[i + 1] - 3 * next[i - 1] + 3 * next[i + 1]); 
          
          grad_y = cv::saturate_cast<uchar>(-3 * previous[i-1] - 10 * previous[i] - 3 * previous[i+1] + 3 * next[i-1] + 10 * next[i] + 3 * next[i + 1]  );
          
          *output++ = (grad_x>>1) + (grad_y>>1);
      }
   }
   
}


// Average 10.5 frames after 500 frames
void opencvScharr(const cv::Mat &image, cv::Mat &result) {

  int scale = 1;
  int delta = 0;
  //int ddepth = CV_16S;
  //int ddepth = CV_8U; // worse

/// Generate grad_x and grad_y
  Mat grad_x, grad_y;
  Mat abs_grad_x, abs_grad_y;
  Mat grad;

  /// Gradient X
  Scharr( image, grad_x, ddepth, 1, 0, scale, delta, BORDER_DEFAULT );

  /// Gradient Y
  Scharr( image, grad_y, ddepth, 0, 1, scale, delta, BORDER_DEFAULT );
  
  /// Convert to Absolute value
  convertScaleAbs( grad_x, abs_grad_x );
  convertScaleAbs( grad_y, abs_grad_y );

  /// Total Gradient (approximate)
  addWeighted( abs_grad_x, 0.5, abs_grad_y, 0.5, 0, result );
 /* 
  normalize(grad, result, 0, 255, NORM_MINMAX, CV_8UC1);
*/
}





/* Global variables */
double t;
Mat output;

/*
 * Class:     com_cabatuan_scharr_MainActivity
 * Method:    process
 * Signature: (Landroid/graphics/Bitmap;[B)V
 */
JNIEXPORT void JNICALL Java_com_cabatuan_scharr_MainActivity_process
  (JNIEnv *pEnv, jobject clazz, jobject pTarget, jbyteArray pSource){

   AndroidBitmapInfo bitmapInfo;
   uint32_t* bitmapContent; // Links to Bitmap content

   if(AndroidBitmap_getInfo(pEnv, pTarget, &bitmapInfo) < 0) abort();
   if(bitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) abort();
   if(AndroidBitmap_lockPixels(pEnv, pTarget, (void**)&bitmapContent) < 0) abort();

   /// Access source array data... OK
   jbyte* source = (jbyte*)pEnv->GetPrimitiveArrayCritical(pSource, 0);
   if (source == NULL) abort();

   /// cv::Mat for YUV420sp source and output BGRA 
    Mat srcGray(bitmapInfo.height, bitmapInfo.width, CV_8UC1, (unsigned char *)source);
    Mat mbgra(bitmapInfo.height, bitmapInfo.width, CV_8UC4, (unsigned char *)bitmapContent);

/***********************************************************************************************/
    /// Native Image Processing HERE... 

    if(output.empty())
         output = Mat(srcGray.size(), srcGray.type());
    
    t = (double)getTickCount(); 

    //computeScharr( srcGray, output); 
    
    opencvScharr(srcGray, output);

    t = 1000*((double)getTickCount() - t)/getTickFrequency();
    
    LOGI("Processing took %0.2f ms.", t);

    cvtColor(output, mbgra, CV_GRAY2BGRA);

/************************************************************************************************/ 
   
   /// Release Java byte buffer and unlock backing bitmap
   pEnv-> ReleasePrimitiveArrayCritical(pSource,source,0);
   if (AndroidBitmap_unlockPixels(pEnv, pTarget) < 0) abort();

}
